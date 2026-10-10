---
name: mvl-re
description: RE workflow for MVL-clone — version-diff methodology across original_files/maxovehmod* (0.9 → 0.98e), Ghidra/IDA import conventions, and patch-site logging. Load for any binary analysis work.
triggers:
  - Any work on original_files/ binaries
  - "How does MVL do X?"
  - Finding patch sites / hook addresses
  - XML schema or extraflags questions
  - Version comparison questions
---

# MVL RE Workflow — mvl-re

## Core Methodology: Version-Diff First

`original_files/` contains MVL distributions from 0.9 to 0.98e. Diffing adjacent versions isolates the code added per feature — far cheaper than analyzing 0.98e blind.

1. **Pick the feature window**: e.g., if a feature appeared in 0.96, diff 0.95 → 0.96 binaries.
2. **Diff binaries**: compare `vehmod*.dll/.flt` between adjacent versions (size, string tables, import tables, code sections).
3. **Strings first**: MBCS strings (XML tags, file names, log messages) are the fastest anchors — locate them in the diff, then find xrefs.
4. **Isolate new code**: the delta between versions is the feature's implementation; analyze only that.

## Binary Targets

| Version | Path | Notes |
|---------|------|-------|
| 0.98e | `original_files/maxovehmod098e/` | Primary target (latest) |
| 0.9 | `original_files/maxovehmod/` | Baseline |
| 0.91–0.98d | `original_files/maxovehmod*/` | Intermediate versions |
| xmlgen_launcher | `original_files/xmlgen_launcher/` | XML generator |

**Rules**:
- NEVER modify anything in `original_files/` — copy out to a scratch dir for analysis.
- Confirm the plugin format per version (`.dll` vs `.flt`) before diffing.
- Record the exact file hash/version analyzed with every finding.

## Ghidra/IDA Imports

- Import as x86 (32-bit) PE; VC 1.0-era binaries, MBCS strings.
- Set image base correctly; record RVAs relative to the file's base.
- For engine patch sites, also load the target VC 1.0 exe (gta-vc.exe 1.0) and record which exe base a patch address was found against — retail variants differ.

## Patch-Site Logging

Every finding goes to `.serena/memories/re-*.md` (one file per version or subsystem) with this shape:

```
# re-<version>-<subsystem>
Binary: vehmod_retail.dll 0.98e (sha256: <hash>)
Base: <image base>

## Finding: <name>
- RVA 0x1234 — <what it does>
- Called from: <xrefs>
- Evidence: <strings/disasm snippet>
```

Durable notes (XML schema, extraflags semantics, archive naming) go to the docs/ vault (`docs/MVL/`).

## Priority RE Questions

1. DllMain / plugin entry — how it attaches to VC 1.0
2. XML config parser — schema, tags, defaults
3. 7z archive loader — bundled 7z runtime (dll vs exe), naming convention
4. Python 3.3 embedding — python33.dll hosting, script API surface
5. extraflags bitmask — per-vehicle feature flags
6. Engine patch sites — what is patched in the VC 1.0 exe, per version
