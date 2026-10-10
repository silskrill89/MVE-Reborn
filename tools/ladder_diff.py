#!/usr/bin/env python3
"""
ladder_diff.py — MVL version-ladder static PE analyzer.

Stdlib-only (struct-based PE parsing, no pefile). Reads binaries under
original_files\maxovehmod*\ (READ-ONLY), extracts per-binary:
  - machine / link timestamp
  - sections (name, raw size, entropy)
  - imports (DLL -> imported function names)
  - ASCII strings (min len 5)
  - UTF-16LE strings (min len 5)

Usage:
  python tools/ladder_diff.py scan [paths...]     # dump JSON info per binary
  python tools/ladder_diff.py strings <bin>       # print strings of one binary
  python tools/ladder_diff.py ladder              # default ladder scan -> JSON
  python tools/ladder_diff.py diff <scan.json>    # per-version deltas (imports,
                                                  #   notable new strings)
  python tools/ladder_diff.py fullnew <scan.json> <version>   # ALL new strings
                                                  #   vs previous main binary
"""

import struct
import sys
import os
import json
import math
import re

MACHINE_NAMES = {
    0x014C: "i386",
    0x8664: "x64",
}

SECTION_FLAGS = [
    (0x00000020, "CODE"),
    (0x00000040, "IDATA"),
    (0x00000080, "UDATA"),
    (0x00000800, "COMB"),
    (0x04000000, "NOLOAD"),
    (0x08000000, "SHARED"),
    (0x10000000, "MEMEXEC"),
    (0x20000000, "MEMREAD"),
    (0x40000000, "MEMWRITE"),
]


def entropy(data):
    if not data:
        return 0.0
    counts = [0] * 256
    for b in data:
        counts[b] += 1
    n = len(data)
    e = 0.0
    for c in counts:
        if c:
            p = c / n
            e -= p * math.log2(p)
    return round(e, 3)


def parse_pe(path):
    """Parse a PE file; return dict of metadata or raise on failure."""
    with open(path, "rb") as f:
        data = f.read()

    info = {"path": path, "size": len(data), "error": None}
    try:
        # --- DOS header ---
        if len(data) < 0x40 or data[:2] != b"MZ":
            info["error"] = "not MZ"
            return info
        e_lfanew = struct.unpack_from("<I", data, 0x3C)[0]
        if data[e_lfanew:e_lfanew + 4] != b"PE\0\0":
            info["error"] = "no PE signature"
            return info

        coff = e_lfanew + 4
        (machine, num_sections, timestamp, _, _,
         size_opt, characteristics) = struct.unpack_from("<HHIIIHH", data, coff)
        info["machine"] = MACHINE_NAMES.get(machine, hex(machine))
        info["machine_raw"] = machine
        info["timestamp"] = timestamp
        import time
        info["timestamp_utc"] = time.strftime(
            "%Y-%m-%d %H:%M:%S", time.gmtime(timestamp)) if timestamp else "n/a"
        info["characteristics"] = characteristics
        is_dll = bool(characteristics & 0x2000)
        info["is_dll"] = is_dll

        opt = coff + 20
        magic = struct.unpack_from("<H", data, opt)[0]
        info["pe_magic"] = {0x10B: "PE32", 0x20B: "PE32+"}.get(magic, hex(magic))
        if magic == 0x10B:
            entry_rva, image_base = struct.unpack_from("<II", data, opt + 16)
            ndirs_off = opt + 92          # NumberOfRvaAndSizes at opt+92; dirs at opt+96
            dir_table = opt + 96
        else:
            entry_rva = struct.unpack_from("<I", data, opt + 16)[0]
            image_base = struct.unpack_from("<Q", data, opt + 24)[0]
            ndirs_off = opt + 108         # dirs at opt+112
            dir_table = opt + 112
        info["entry_rva"] = entry_rva
        info["image_base"] = image_base

        # --- Sections ---
        sec_off = opt + size_opt
        sections = []
        sec_map = []  # (rva_start, rva_end, name, raw_ptr, raw_size)
        for i in range(num_sections):
            o = sec_off + i * 40
            name = data[o:o + 8].rstrip(b"\0").decode("latin1", "replace")
            vsize, vaddr, rsize, rptr = struct.unpack_from("<IIII", data, o + 8)
            flags = struct.unpack_from("<I", data, o + 36)[0]
            fl = [n for bit, n in SECTION_FLAGS if flags & bit]
            raw = data[rptr:rptr + rsize] if rptr and rsize else b""
            sections.append({
                "name": name,
                "vsize": vsize,
                "rsize": rsize,
                "entropy": entropy(raw[:min(len(raw), 4 * 1024 * 1024)]),
                "flags": fl,
            })
            if rsize:
                sec_map.append((vaddr, vaddr + max(vsize, rsize), name, rptr, rsize))
        info["sections"] = sections

        def rva_to_off(rva):
            for va0, va1, _n, rptr, rsize in sec_map:
                if va0 <= rva < va1:
                    d = rva - va0
                    if d < rsize:
                        return rptr + d
            return None

        # --- Data directories ---
        num_dirs = struct.unpack_from("<I", data, ndirs_off)[0]
        dirs = {}
        dir_names = ["export", "import", "resource", "exception", "security",
                     "reloc", "debug", "arch", "globalptr", "tls", "loadcfg",
                     "boundimp", "iat", "delayimp", "comdesc"]
        for i in range(min(num_dirs, len(dir_names))):
            rva, sz = struct.unpack_from("<II", data, dir_table + i * 8)
            if rva:
                dirs[dir_names[i]] = (rva, sz)
        info["dirs"] = {k: v for k, v in dirs.items()}

        # --- Imports ---
        imports = {}
        if "import" in dirs:
            rva, _sz = dirs["import"]
            off = rva_to_off(rva)
            if off is not None:
                iat = 0
                while True:
                    desc = data[off + iat * 20: off + (iat + 1) * 20]
                    if len(desc) < 20 or desc == b"\0" * 20:
                        break
                    (chars, imp_ts, fwd, name_rva, first_thunk) = struct.unpack_from("<IIIII", desc, 0)
                    noff = rva_to_off(name_rva)
                    dllname = "<unknown>"
                    if noff is not None:
                        end = data.find(b"\0", noff)
                        dllname = data[noff:end].decode("latin1", "replace")
                    funcs = []
                    # use OriginalFirstThunk if present, else FirstThunk
                    thunk_rva = chars if chars else first_thunk
                    # iterate thunk array
                    t = 0
                    base_off = rva_to_off(thunk_rva) if thunk_rva else (rva_to_off(first_thunk) if first_thunk else None)
                    while base_off is not None:
                        thunk = struct.unpack_from("<I", data, base_off + t * 4)[0]
                        if thunk == 0:
                            break
                        if thunk & 0x80000000:  # ordinal import
                            funcs.append("#%d" % (thunk & 0xFFFF))
                        else:
                            fo = rva_to_off(thunk)
                            if fo is not None:
                                fo += 2  # skip hint
                                end = data.find(b"\0", fo)
                                funcs.append(data[fo:end].decode("latin1", "replace"))
                        t += 1
                        if t > 4096:
                            break
                    imports[dllname] = funcs
                    iat += 1
                    if iat > 128:
                        break
        info["imports"] = imports

        # --- Strings ---
        # ASCII: printable runs >= 5, in raw file
        ascii_re = re.compile(rb"[\x20-\x7E]{5,}")
        astrings = [m.group().decode("latin1") for m in ascii_re.finditer(data)]
        info["ascii_strings"] = astrings
        # UTF-16LE: pairs of printable ASCII chars with \x00, run >= 5 chars
        u16_re = re.compile(rb"(?:[\x20-\x7E]\x00){5,}")
        ustrings = [m.group().decode("utf-16le") for m in u16_re.finditer(data)]
        info["utf16_strings"] = ustrings
        info["counts"] = {
            "ascii": len(astrings),
            "utf16": len(ustrings),
            "imports": sum(len(v) for v in imports.values()),
            "import_dlls": len(imports),
        }
        return info
    except Exception as e:
        info["error"] = repr(e)
        return info


def default_ladder():
    """Auto-detect the version ladder binaries under original_files/."""
    import glob as g
    entries = []
    root = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))),
                        "original_files")
    for d in sorted(g.glob(os.path.join(root, "maxovehmod*"))):
        if not os.path.isdir(d):
            continue
        ver = os.path.basename(d).replace("maxovehmod", "") or "0.9"
        for binname in ("mss\\vehmod.flt", "mvl\\vehmod_retail.dll",
                        "mvl\\vehmod_steam.dll", "mss\\vehmod_chooser.flt",
                        "mvl\\libpng15.dll"):
            p = os.path.join(d, binname)
            if os.path.isfile(p):
                entries.append((ver, p))
    return entries


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 1
    cmd = sys.argv[1]

    if cmd == "strings":
        info = parse_pe(sys.argv[2])
        print("== ASCII ==")
        for s in info.get("ascii_strings", []):
            print(s)
        print("== UTF16 ==")
        for s in info.get("utf16_strings", []):
            print(s)
        return 0

    if cmd in ("scan", "ladder"):
        if cmd == "scan":
            out = [parse_pe(p) for p in sys.argv[2:]]
        else:
            entries = default_ladder()
            out = []
            for ver, p in entries:
                info = parse_pe(p)
                info["version"] = ver
                out.append(info)
        print(json.dumps(out, indent=1))
        return 0

    if cmd in ("diff", "fullnew"):
        # Load a previously saved ladder scan JSON (argv[2]).
        scan = json.load(open(sys.argv[2], encoding="utf-8"))
        # main binary per version, in ladder order
        by_ver = {}
        for e in scan:
            ver = e.get("version")
            if ver is None:
                continue
            if ver not in by_ver:
                by_ver[ver] = e
            else:
                # keep main binary (the .flt/.dll, not libpng/chooser)
                cur = by_ver[ver]
                curmn = os.path.basename(cur["path"]).lower()
                mn = os.path.basename(e["path"]).lower()
                if mn.startswith(("vehmod.flt", "vehmod_retail", "vehmod_steam")) \
                        and not curmn.startswith(("vehmod.flt", "vehmod_retail", "vehmod_steam")):
                    by_ver[ver] = e
        vers = sorted(by_ver.keys(), key=version_key)
        if cmd == "fullnew":
            target = sys.argv[3]
            prev, prevv = None, None
            for v in vers:
                if v == target:
                    prevv = prev
                    break
                prev = v
            if prevv is None and target != vers[0]:
                print("unknown version %r; ladder: %s" % (target, vers), file=sys.stderr)
                return 1
            cur = set(by_ver[target]["ascii_strings"])
            old = set(by_ver[prev]["ascii_strings"]) if prevv else set()
            for s in sorted(cur - old):
                print(s)
            return 0

        # diff: per version, delta vs previous main binary
        for i, v in enumerate(vers):
            e = by_ver[v]
            prev = by_ver[vers[i - 1]] if i else None
            print("=" * 70)
            print("VERSION %s  (%s, %d bytes)" % (
                v, os.path.basename(e["path"]), e["size"]))
            if prev:
                print("  prev %s: %d bytes -> size delta %+d" % (
                    vers[i - 1], prev["size"], e["size"] - prev["size"]))
                pim = {d: set(f) for d, f in prev["imports"].items()}
                nim = {d: set(f) for d, f in e["imports"].items()}
                for d in sorted(set(pim) | set(nim)):
                    added = nim.get(d, set()) - pim.get(d, set())
                    removed = pim.get(d, set()) - nim.get(d, set())
                    if added:
                        print("  +imports %s: %s" % (d, ", ".join(sorted(added))))
                    if removed:
                        print("  -imports %s: %s" % (d, ", ".join(sorted(removed))))
                old = set(prev["ascii_strings"])
                cur = set(e["ascii_strings"])
                texts, tags, paths, idents = [], [], [], []
                for s in sorted(cur - old):
                    cat = classify(s)
                    if cat == "text":
                        texts.append(s)
                    elif cat == "xml":
                        tags.append(s)
                    elif cat == "path":
                        paths.append(s)
                    elif cat == "ident":
                        idents.append(s)
                for name, lst in (("text", texts), ("xml", tags),
                                  ("path", paths), ("ident", idents)):
                    if lst:
                        print("  new %s strings (%d):" % (name, len(lst)))
                        for s in lst[:120]:
                            print("    + %s" % s[:110])
                        if len(lst) > 120:
                            print("    ... %d more" % (len(lst) - 120))
                print("  (new-strings total %d, notable %d)" % (len(cur - old),
                      len(texts) + len(tags) + len(paths) + len(idents)))
            else:
                print("  first version in ladder (no delta)")
                tags = [s for s in e["ascii_strings"] if s.startswith("<")]
                idents = [s for s in e["ascii_strings"] if classify(s) in ("xml", "ident", "path")]
                print("  xml/ident strings (%d):" % len(idents))
                for s in idents[:200]:
                    print("    * %s" % s[:110])
            print("  imports: " + ", ".join("%s(%d)" % (d, len(f))
                                            for d, f in sorted(e["imports"].items())))
            print("  strings: ascii=%d utf16=%d  sections: %s" % (
                e["counts"]["ascii"], e["counts"]["utf16"],
                ", ".join("%s:r%d" % (s["name"], s["rsize"]) for s in e["sections"])))
        return 0

    print(__doc__)
    return 1


def version_key(v):
    import re
    nums = re.findall(r"\d+", v)
    letters = re.findall(r"[a-e]$", v)
    return (int(nums[0]) if nums else 0,
            int(nums[1]) if len(nums) > 1 else 0,
            letters[0] if letters else "")


def classify(s):
    """Categorize a candidate string; return None for binary junk.

    Categories: 'text' (English-ish), 'xml' (tag-ish), 'path',
    'ident' (CamelCase identifier, possibly internal function name).
    """
    letters = sum(1 for c in s if c.isalpha())
    if letters < 5:
        return None
    vowels = sum(1 for c in s.lower() if c in "aeiou")
    if "\\" in s or "/" in s:
        # path-like: must contain a plausible directory/file word
        if letters >= 4 and re.search(r"[a-z]{3}", s):
            return "path"
        return None
    if s.startswith("<") and s.endswith(">"):
        return "xml"
    # XML single tag names: all-lowercase identifier
    if re.fullmatch(r"[a-z][a-z0-9_]{4,}", s) and vowels >= 2:
        return "xml"
    # English-ish text: spaces + decent word structure
    if " " in s and letters >= 6 and vowels >= 3:
        # reject junk rows that happen to contain spaces but no word structure
        words = re.findall(r"[A-Za-z]{3,}", s)
        if len(words) >= 2 and letters * 2 >= len(s):
            return "text"
        return None
    # CamelCase identifier (may be internal function name or import thunk)
    if re.fullmatch(r"[A-Za-z][A-Za-z0-9_$@?]{5,}", s) and vowels >= 2:
        return "ident"
    return None


NOISE_RE = re.compile(
    r"(?i)\.pdb|\.obj\b|microsoft|visual|__|exception|conout|conin|api-ms-")


if __name__ == "__main__":
    sys.exit(main())
