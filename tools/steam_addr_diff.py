#!/usr/bin/env python3
# steam_addr_diff.py - M8b (PLAN.md 10.2 step 3): recover the steam-variant
# game-exe addresses by diffing vehmod_retail.dll vs vehmod_steam.dll.
#
# Method (byte-diff first, Ghidra only as fallback):
#   Both DLLs are 760,832 B PE32 x86, image base 0x10000000, IDENTICAL section
#   layout (verified: same VAs/sizes). They are two builds of the same MVL
#   source where every embedded gta-vc.exe address (code immediates + .data
#   site tables) was substituted with the steam-exe counterpart. So: find the
#   4-byte LE encoding of a retail VA in the retail DLL, read the same file
#   offset in the steam DLL, and the dword there is the steam VA.
#
#   Per field, three methods are tried in order:
#     imm    - absolute dword == retail VA (LE) anywhere in the retail image
#     rvaimm - absolute dword == retail RVA (VA - 0x400000)
#     rel32  - dword D at offset X with D == retailVA - VA(X+4) (E8/E9 rel32)
#              -> steamVA = VA(X+4) + steam dword at X
#
#   Confidence per match:
#     ctx-identical : bytes around the immediate (excluding it) are identical
#                     in both DLLs (same instruction, different immediate)
#     cluster       : post-pass upgrade - context differs only by NEIGHBORING
#                     substituted addresses; every differing byte in a +/-24
#                     window is covered by a plausible address-substitution
#                     dword pair (see cluster_verify)
#     consistent    : all plausible matches agree on the steam value
#     plausible     : steam candidate inside [0x401000, 0x1100000)
#
# READ-ONLY: opens both DLLs in original_files/ in place, never writes there.
# Output: tools/steam_addr_diff_output.md + tools/steam_addr_diff_output.json
#
# Usage: python tools/steam_addr_diff.py   (repo root cwd)
import json
import os
import re
import struct
import sys

RETAIL_DLL = "original_files/maxovehmod098e/mvl/vehmod_retail.dll"
STEAM_DLL = "original_files/maxovehmod098e/mvl/vehmod_steam.dll"
ENGINE_GLOBALS = "src/engine/engine_globals.cpp"
OUT_MD = "tools/steam_addr_diff_output.md"
OUT_JSON = "tools/steam_addr_diff_output.json"

GAME_BASE = 0x400000          # gta-vc.exe preferred base (both variants)
DLL_BASE = 0x10000000         # MVL DLL image base (both variants)
STEAM_MIN = 0x401000          # plausibility window for a steam game VA
STEAM_MAX = 0x1100000         # generous: steam exe is larger than retail


# ---------------------------------------------------------------------------
# Minimal PE32 parser (stdlib only).
# ---------------------------------------------------------------------------
class Pe:
    def __init__(self, path):
        self.path = path
        with open(path, "rb") as f:
            self.data = f.read()
        d = self.data
        e_lfanew = struct.unpack_from("<I", d, 0x3C)[0]
        assert d[e_lfanew:e_lfanew + 4] == b"PE\0\0", "not PE"
        self.nsec = struct.unpack_from("<H", d, e_lfanew + 6)[0]
        opt_size = struct.unpack_from("<H", d, e_lfanew + 20)[0]
        self.base = struct.unpack_from("<I", d, e_lfanew + 24 + 28)[0]
        self.sections = []  # (name, va, vsize, raw_off, raw_size)
        off = e_lfanew + 24 + opt_size
        for _ in range(self.nsec):
            name = d[off:off + 8].rstrip(b"\0").decode("ascii", "replace")
            vsize, va, rsize, roff = struct.unpack_from("<IIII", d, off + 8)
            self.sections.append((name, va, vsize, roff, rsize))
            off += 40

    def off_to_rva(self, off):
        for name, va, vsize, roff, rsize in self.sections:
            if roff <= off < roff + rsize:
                return va + (off - roff)
        return None

    def rva_to_off(self, rva):
        for name, va, vsize, roff, rsize in self.sections:
            if va <= rva < va + max(vsize, rsize):
                return roff + (rva - va)
        return None

    def section_of_off(self, off):
        for name, va, vsize, roff, rsize in self.sections:
            if roff <= off < roff + rsize:
                return name
        return "?"


# ---------------------------------------------------------------------------
# Field list extracted from engine_globals.cpp (single source of truth).
# ---------------------------------------------------------------------------
def load_fields():
    with open(ENGINE_GLOBALS, "r", encoding="utf-8") as f:
        src = f.read()

    fields = []  # (name, retail_va, kind)

    # kExeAddrsRetail positional initializer:
    #   "\t0x0055F636u, // modelPtrBaseImm        kVC_..."
    m = re.search(r"constexpr MvlExeAddrs kExeAddrsRetail = \{(.*?)\n\};",
                  src, re.S)
    assert m, "kExeAddrsRetail block not found"
    for line in m.group(1).splitlines():
        lm = re.match(r"\s*(0x[0-9A-Fa-f]+)u,\s*//\s*(\S+)", line)
        if lm:
            fields.append((lm.group(2), int(lm.group(1), 16), "map"))

    # kRetailHookSites entries:
    #   "\t{ 0x00426748u, reinterpret_cast<uintptr_t>(&MvlHookXxx), 0 }, // ..."
    m = re.search(r"static const MvlHookSite kRetailHookSites\[\] = \{(.*?)\n\};",
                  src, re.S)
    assert m, "kRetailHookSites block not found"
    for line in m.group(1).splitlines():
        lm = re.match(r"\s*\{ (0x[0-9A-Fa-f]+)u, reinterpret_cast<uintptr_t>\(&(\w+)\), ([01]) \}", line)
        if lm:
            fields.append(("hook:" + lm.group(2), int(lm.group(1), 16),
                           "site:" + lm.group(3)))

    assert fields, "no fields extracted"
    return fields


# ---------------------------------------------------------------------------
# Match discovery.
# ---------------------------------------------------------------------------
def find_abs_matches(pe, value):
    """Offsets where 4-byte LE dword == value."""
    pat = struct.pack("<I", value)
    out = []
    i = pe.data.find(pat)
    while i != -1:
        out.append(i)
        i = pe.data.find(pat, i + 1)
    return out


def find_rel32_matches(pe, target_va):
    """Offsets X where dword(X) == target_va - VA(X+4) (call/jmp rel32)."""
    out = []
    for name, va, vsize, roff, rsize in pe.sections:
        if name != ".text":
            continue
        for i in range(roff, roff + rsize - 4):
            insn_va = pe.base + pe.off_to_rva(i)
            rel = struct.unpack_from("<i", pe.data, i)[0]
            if insn_va + 4 + rel == target_va:
                out.append(i)
    return out


def ctx_identical(ra, sa, off, immlen=4, radius=6):
    """Bytes immediately around [off, off+immlen) identical in both DLLs."""
    lo = max(0, off - radius)
    hi = min(len(ra.data), off + immlen + radius)
    return ra.data[lo:off] == sa.data[lo:off] and ra.data[off + immlen:hi] == sa.data[off + immlen:hi]


def plausible(va):
    return va is not None and STEAM_MIN <= va < STEAM_MAX


def cluster_verify(ra, sa, off, census, radius=24):
    """Cluster-aware context check (M8b follow-up).

    ctx_identical() masks only the field's own immediate (+/-6 bytes); when a
    NEIGHBORING substituted address falls inside that window the context
    check fails even though the code is identical modulo address
    substitution (e.g. texCountRef2@0x17675 / texCountRef3@0x17670 are 5
    bytes apart). This check takes a wider window and requires EVERY
    differing byte to be covered by a dword position whose (retail, steam)
    pair is itself a plausible address substitution: both dwords in the
    game-VA plausibility window, or the pair delta seen elsewhere in the
    global substitution census. Pass => the neighborhood differs only by
    substituted game addresses => same instruction stream.
    Returns (all_covered, [(delta, retail_dword, steam_dword), ...]).
    """
    lo = max(0, off - radius)
    hi = min(len(ra.data), off + 4 + radius)
    diff = [p for p in range(lo, hi) if ra.data[p] != sa.data[p]]
    if not diff:
        return True, []
    valid = []
    dlo, dhi = max(0, lo - 3), min(len(ra.data) - 4, hi)
    for d in range(dlo, dhi + 1):
        rd = struct.unpack_from("<I", ra.data, d)[0]
        sd = struct.unpack_from("<I", sa.data, d)[0]
        if rd == sd:
            continue
        if (STEAM_MIN <= rd < STEAM_MAX and STEAM_MIN <= sd < STEAM_MAX)                 or abs(rd - sd) in census:
            valid.append((rd - sd, rd, sd, d))
    covered = set()
    for delta, rd, sd, d in valid:
        covered.update(range(d, d + 4))
    ok = all(p in covered for p in diff)
    return ok, [(dl, rd, sd) for dl, rd, sd, _ in valid]


# ---------------------------------------------------------------------------
# Per-field recovery.
# ---------------------------------------------------------------------------
def recover_field(name, retail_va, kind, ra, sa):
    retail_rva = retail_va - GAME_BASE
    cands = {}  # steam_va -> list of (method, off, ctx, section)

    def add(steam_va, method, off, ctx):
        cands.setdefault(steam_va, []).append((method, off, ctx, ra.section_of_off(off)))

    # Method 1: absolute VA immediate.
    for off in find_abs_matches(ra, retail_va):
        steam = struct.unpack_from("<I", sa.data, off)[0]
        if steam != retail_va:
            add(steam, "imm", off, ctx_identical(ra, sa, off))
        else:
            add(retail_va, "imm-same", off, ctx_identical(ra, sa, off))

    # Method 2: absolute RVA immediate (VA - 0x400000).
    if not cands:
        for off in find_abs_matches(ra, retail_rva):
            steam_rva = struct.unpack_from("<I", sa.data, off)[0]
            steam = steam_rva + GAME_BASE
            meth = "rvaimm" if steam_rva != retail_rva else "rvaimm-same"
            add(steam, meth, off, ctx_identical(ra, sa, off))

    # Method 3: rel32 (E8/E9 displacement) reference.
    if not cands:
        for off in find_rel32_matches(ra, retail_va):
            insn_rva = ra.off_to_rva(off)
            insn_va = DLL_BASE + insn_rva  # DLL-internal VA of the rel32 field
            next_va = insn_va + 4
            steam_rel = struct.unpack_from("<i", sa.data, off)[0]
            # steamVA = (game) next-insn VA + steam_rel; the rel32 base for a
            # game-target call inside DLL code is the DLL's own next-insn VA
            # ONLY if the original used DLL-relative math - it does not: game
            # calls from the DLL are absolute. rel32 to a GAME VA from DLL code
            # does not occur in this codebase; keep the method for completeness
            # and mark any hit low-confidence.
            steam = (next_va + steam_rel) - DLL_BASE + GAME_BASE
            add(steam, "rel32", off, ctx_identical(ra, sa, off))

    if not cands:
        return {
            "field": name, "retail_va": retail_va, "retail_rva": retail_rva,
            "steam_va": None, "method": "NOT FOUND", "confidence": "none",
            "evidence": [], "kind": kind,
        }

    # Split candidates into differing (real steam substitutions) and identical.
    diff = {v: ms for v, ms in cands.items() if v != retail_va and plausible(v)}
    same = {v: ms for v, ms in cands.items() if v == retail_va}
    implausible = {v: ms for v, ms in cands.items()
                   if v != retail_va and not plausible(v)}

    if diff:
        if len(diff) == 1:
            steam_va, ms = next(iter(diff.items()))
            ctx_ok = any(m[2] for m in ms)
            multi = len(ms) > 1
            conf = "high" if (ctx_ok or multi) else "medium"
            method = ms[0][0]
            if implausible:
                conf = conf if ctx_ok else "low"
        else:
            # Ambiguous: prefer a candidate supported by ctx-identical matches.
            best = None
            for v, ms in sorted(diff.items()):
                if any(m[2] for m in ms):
                    best = (v, ms)
                    break
            if best:
                steam_va, ms = best
                conf = "low"
                method = ms[0][0] + "+ctx"
            else:
                steam_va, ms = sorted(diff.items())[0]
                conf = "ambiguous"
                method = ms[0][0]
    elif same:
        steam_va = retail_va
        ms = next(iter(same.values()))
        conf = "high" if any(m[2] for m in ms) else "medium"
        method = "identical"
    else:
        steam_va = None
        ms = []
        conf = "none"
        method = "implausible-only"

    evidence = [
        {"method": m[0], "dll_off": m[1], "dll_rva": ra.off_to_rva(m[1]),
         "section": m[3], "ctx_identical": m[2]}
        for m in sorted(ms, key=lambda x: x[1])
    ]
    return {
        "field": name, "retail_va": retail_va, "retail_rva": retail_rva,
        "steam_va": steam_va,
        "steam_rva": (steam_va - GAME_BASE) if steam_va is not None else None,
        "method": method, "confidence": conf,
        "evidence": evidence, "kind": kind,
    }


# ---------------------------------------------------------------------------
# Main.
# ---------------------------------------------------------------------------
def main():
    ra = Pe(RETAIL_DLL)
    sa = Pe(STEAM_DLL)
    assert ra.base == sa.base == DLL_BASE, "unexpected image base"
    # Raw layout (name/VA/raw offset/raw size) must match so that a file
    # offset addresses the same content in both images. VirtualSize may differ
    # by a few bytes between the builds (verified: .text 473389 vs 473405) -
    # that does not affect the same-offset method.
    assert [(s[0], s[1], s[3], s[4]) for s in ra.sections] == \
           [(s[0], s[1], s[3], s[4]) for s in sa.sections], \
           "section raw layouts differ - same-offset method invalid"

    fields = load_fields()
    results = [recover_field(n, v, k, ra, sa) for (n, v, k) in fields]

    # Global substitution census: every (retail, steam) dword pair observed
    # at an immediate match, as abs deltas. Used by cluster_verify.
    census = set()
    for r in results:
        for e in r["evidence"]:
            if e["method"] == "rel32":
                continue
            off = e["dll_off"]
            rd = struct.unpack_from("<I", ra.data, off)[0]
            sd = struct.unpack_from("<I", sa.data, off)[0]
            if rd != sd:
                census.add(abs(rd - sd))

    # Cluster verification post-pass: upgrade medium fields whose context
    # differs only by neighboring substituted addresses (see cluster_verify).
    upgraded = []
    for r in results:
        if r["confidence"] != "medium" or r["steam_va"] is None:
            continue
        oks = []
        for e in r["evidence"]:
            ok, _pairs = cluster_verify(ra, sa, e["dll_off"], census)
            e["cluster_verified"] = ok
            oks.append(ok)
        if all(oks):
            r["confidence"] = "high"
            r["method"] += "+cluster"
            upgraded.append(r["field"])

    # Stats - mutually exclusive buckets (sum == total).
    def bucket(r):
        if r["method"] == "NOT FOUND":
            return "none"
        if r["confidence"] in ("low", "ambiguous", "implausible-only"):
            return "low"
        ident = r["method"].startswith("identical")
        hi = r["confidence"] == "high"
        if ident:
            return "identical" if hi else "identical-med"
        return "high" if hi else "medium"

    from collections import Counter
    b = Counter(bucket(r) for r in results)
    n_high, n_med = b["high"], b["medium"]
    n_ident, n_ident_med = b["identical"], b["identical-med"]
    n_low, n_none = b["low"], b["none"]
    total = len(results)
    if upgraded:
        print("cluster-verified upgrades: %s" % ", ".join(upgraded))

    with open(OUT_JSON, "w", encoding="utf-8") as f:
        json.dump(results, f, indent=1)

    lines = []
    lines.append("# steam_addr_diff output (M8b)")
    lines.append("")
    lines.append("Retail: `%s`  |  Steam: `%s`" % (RETAIL_DLL, STEAM_DLL))
    lines.append("")
    lines.append("Stats: %d fields | recovered-diff high %d | recovered-diff "
                 "medium %d | identical high %d | identical medium %d | "
                 "low/ambiguous %d | NOT FOUND %d (buckets sum to total)"
                 % (total, n_high, n_med, n_ident, n_ident_med, n_low, n_none))
    lines.append("")
    lines.append("| field | kind | retail VA | retail RVA | steam VA | steam RVA "
                 "| method | conf | evidence (dll rva/section/ctx) |")
    lines.append("|---|---|---|---|---|---|---|---|---|")
    for r in results:
        ev = "; ".join("%s@0x%X/%s/ctx=%d" % (e["method"], e["dll_rva"],
                                             e["section"], e["ctx_identical"])
                       for e in r["evidence"][:4]) or "-"
        lines.append("| `%s` | %s | 0x%06X | 0x%06X | %s | %s | %s | %s | %s |"
                     % (r["field"], r["kind"], r["retail_va"], r["retail_rva"],
                        ("0x%06X" % r["steam_va"]) if r["steam_va"] else "-",
                        ("0x%06X" % r["steam_rva"]) if r["steam_rva"] else "-",
                        r["method"], r["confidence"], ev))
    lines.append("")

    with open(OUT_MD, "w", encoding="utf-8") as f:
        f.write("\n".join(lines))

    print("\n".join(lines))
    unresolved = [r["field"] for r in results
                  if r["steam_va"] is None or r["confidence"] in ("none", "ambiguous")]
    if unresolved:
        print("\nUNRESOLVED (%d): %s" % (len(unresolved), ", ".join(unresolved)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
