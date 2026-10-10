---
name: hybrid-re-setup
description: >
  Self-contained guide for the Ghidra-headless + IDA-Free hybrid reverse-engineering
  workflow on Windows x86 binaries: batch version-ladder diffs in Ghidra, interactive
  deep-dives in IDA Free, with exact invocation patterns, version-diff methodology,
  export/citation conventions, and division of labor. Clone this skill into any
  project by swapping the binary/tool paths marked [PROJECT-SPECIFIC].
---

# Hybrid RE Setup — Ghidra headless + IDA Free

Two complementary tools, two complementary jobs:

| Tool | Mode | Use for |
|------|------|---------|
| **Ghidra** (headless, `analyzeHeadless`) | BATCH | Import + auto-analyze **many binaries** (version ladders, loader shims, aux exes) with zero GUI; scripted summary dumps; repeatable diff runs |
| **IDA Free** | INTERACTIVE | Deep-dive of the **one primary binary**; fast manual navigation, decompiler cross-checks, patch-site confirmation |

Rule of thumb: Ghidra tells you *what changed between versions and where*; IDA tells you *what the code at that address actually does*.

---

## 1. Install layout

> **[PROJECT-SPECIFIC]** Paths below are the MVL-clone examples. Swap per project;
> the tool installs themselves are machine-global and shared across projects.

| Tool | Path | Notes |
|------|------|-------|
| Ghidra | `E:\dev(dave)\tools\ghidra` | 12.1.4 PUBLIC (verified 2026-10-10); headless runner at `<ghidra>\support\analyzeHeadless.bat` |
| JDK 21 | `E:\dev(dave)\tools\jdk-21` | Required by Ghidra; set `JAVA_HOME` per-invocation (see §2) |
| Ghidra projects dir | `E:\dev(dave)\tools\ghidra-projects\<project>` | One Ghidra project per RE project (e.g. `MVL_clone`) |
| Ghidra scripts dir | `E:\dev(dave)\tools\ghidra-projects\scripts` | Shared postScript dumps (`DumpSummary.java` lives here) |
| IDA Free | `E:\dev(dave)\tools\ida-free` | **9.3 installed 2026-10-10** (`ida.exe` verified); GUI interactive use only |

### IDA Free — install record (resolved 2026-10-10)

- IDA Free **9.3** installed at `E:\dev(dave)\tools\ida-free` (user manual step
  completed; `ida.exe`, `ida.dll`, `ida.int` verified on disk).
- Historical note: 8.x direct downloads were retired; 9.x was only obtainable via
  the my.hex-rays.com registration form (see
  `E:\dev(dave)\tools\installers\README-ida-free-manual-step.txt`).
- **Do NOT launch the IDA GUI from automation — interactive use only.**
- Database files: create IDA databases (.i64) per binary under
  `E:\dev(dave)\tools\ghidra-projects\<project>\ida\` to keep RE artifacts together.

---

## 2. Ghidra headless — exact invocation pattern (Win32)

One-liner that creates the project (if missing), imports the binary, and runs
full auto-analysis:

```bat
set "JAVA_HOME=E:\dev(dave)\tools\jdk-21"
set "GHIDRA=E:\dev(dave)\tools\ghidra"
call "%GHIDRA%\support\analyzeHeadless.bat" "E:\dev(dave)\tools\ghidra-projects" MVL_clone ^
  -import "E:\dev(dave)\projects\<project>\original_files\<version-dir>\<binary>.dll" ^
  -scriptPath "E:\dev(dave)\tools\ghidra-projects\scripts" ^
  -postScript DumpSummary.java ^
  -analysisTimeoutPerFile 900
```

Notes:

- **`JAVA_HOME` must point at the JDK 21 dir** (not `bin`); Ghidra's launcher
  resolves `%JAVA_HOME%\bin\java.exe`. Set it in the same shell as the call —
  do not rely on a machine-wide variable.
- First invocation **creates** the project `MVL_clone.gpr` in the projects dir;
  later invocations with the same project name **reuse** it.
- To add more binaries to the same project, repeat `-import` with the same
  project name/location — each binary becomes a separate *program* inside the
  one `.gpr`. This is what makes cross-version work possible (§3).
- `-analysisTimeoutPerFile 900` (15 min) guards against pathological auto-analysis.
- **Typical runtime** for a ~750 KB x86 PE32 DLL: **~2–5 minutes** end-to-end
  (import + auto-analysis + postScript). Tiny shims (a few hundred bytes) finish
  in seconds.
- Useful variants:
  - `-process <binary>.dll -noanalysis -postScript DumpSummary.java` — re-run a
    script on an already-analyzed program without re-importing.
  - `-deleteProject` — nuke and rebuild the project from scratch if it corrupts.
- A ready-made runner for MVL-clone exists at
  `E:\dev(dave)\tools\ghidra-projects\run_headless.cmd` (sibling-lane owned —
  read it, don't edit it; copy the pattern into per-project runners).

### PostScript summary dump

`DumpSummary.java` (in the shared scripts dir) dumps the function inventory —
names, entry points, sizes — to stdout; capture it to a file per run:

```bat
call run_headless.cmd > re-ghidra-dump-<version>.txt 2>&1
```

---

## 3. Version-diff methodology (batch lane)

1. **Import the whole version ladder into ONE Ghidra project** — every version's
   main binary becomes a program in the same `.gpr`. For MVL-clone:
   0.9 → 0.91 → … → 0.98e (23 versions, ~14 growth steps), plus loader shims
   (`.flt`) and aux binaries (`xmlgen_launcher.exe`).
2. **Diff adjacent versions**, never jump across the ladder. Growth bursts
   correlate with changelog features — isolate each feature's code by comparing
   version N vs N+1.
3. **Matching functions across versions**, in order of preference:
   - **Function-name/hash matching** on the postScript dumps (string anchors,
     called-API signatures, function sizes) — cheap, works headless.
   - **Ghidra Version Tracking** (GUI: `Tool → Version Tracking`) — program
     comparison + suggested matches; interactive but powerful.
   - **BSim** (function-signature database similarity) — available in recent
     Ghidra releases (12.x includes it); best when code was recompiled with
     layout shifts.
4. **Cite every finding in the mandatory format**:
   `binary + version @ RVA 0xXXXX` — e.g. `vehmod_retail.dll 0.98e @ RVA 0x1234`.
   Unattributed addresses are worthless. Always record which file (`.dll` vs
   `.flt`) and which host-exe variant (retail vs steam) the RVA belongs to.
5. Log per-function findings to `.serena/memories/re-<subsystem>.md` (one claim
   per line) and patch sites to `docs/<Project>/patch-sites.md`
   (version, RVA, original bytes, purpose).

---

## 4. Export conventions

- **Ghidra postScript dumps** → distilled into `.serena/memories/re-<subsystem>.md`
  per project convention (raw dumps stay in the run log files, findings get
  curated into the memory notes).
- **Ghidra program annotations** (comments, labels, bookmarks made in the GUI)
  → exported into the docs/ Obsidian vault as durable notes
  (`docs/<Project>/…`), then synced into the project's knowledge index
  (e.g. `python .sadie/sadie_vault_sync.py` in MVL-clone).
- Keep raw artifacts (dump txts, project `.gpr`) OUT of the repo — they live
  under the shared tools dir; only curated notes enter the repo.

---

## 5. Division of labor

| Task | Tool | Why |
|------|------|-----|
| Chooser/loader shims, tiny binaries (≤ a few KB) | Ghidra quick import | Auto-analysis is instant; one headless run answers everything |
| Version-ladder sweeps (many binaries) | Ghidra headless batch | Scriptable, repeatable, no GUI babysitting |
| Primary plugin deep-dive (the big DLL) | IDA Free interactive | Fastest manual navigation + decompiler for sustained reading |
| Cross-checking decompilation disagreements | Both | When Ghidra's and IDA's decompilers disagree on a function, resolve by reading the disassembly in both and by dynamic evidence (debugger, log strings) |

Workflow shape: Ghidra batch run produces the *map* (what's new/changed per
version, candidate RVAs) → IDA interactive session *reads the terrain* (what the
candidate functions do) → findings land in the memory notes with citations.

---

## 6. Clone-me checklist (for other projects)

1. Copy this skill dir into `<project>/.opencode/skills/hybrid-re-setup/`.
2. Swap the `[PROJECT-SPECIFIC]` paths: project name in the Ghidra project
   (`MVL_clone` → `<yourproject>`), binary paths, scripts dir if separate.
3. Create a per-project `run_headless.cmd` modeled on
   `E:\dev(dave)\tools\ghidra-projects\run_headless.cmd` (keep it OUT of the
   shared tools dir unless the sibling lane owns it).
4. Ensure a `DumpSummary.java`-class postScript exists in the scripts dir.
5. Do the IDA Free manual install once per machine (§1) — it is NOT
   project-specific; record the install path in this file.
6. Wire the citation format (`binary + version @ RVA`) into the project's
   AGENTS.md RE rules if not already present.
