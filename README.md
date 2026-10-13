# MVL-clone (Maxos_vehicle_revnres)

Reverse-engineering rebuild of **Maxo's Vehicle Loader (MVL)** — a GTA Vice City 1.0 DLL plugin (`vehmod_retail.dll`) with an XML + 7z vehicle-archive config system, Python 3.3 scripting, and direct engine patching. Goal: clone MVL 0.98e functionality from binary RE, then later port to GTA San Andreas.

## Status

**Active recomp** (2026-10-12). MVL 0.98e is being cloned from binary RE, module by module; every tranche ends with a green build.

### Progress metrics — milestone history

Each commit is a milestone; the previous values are struck through as a cross-check
(verify any row against the tree state at that commit).

| Milestone commit | Recompiled | Accuracy | Notes |
|---|---:|---:|---|
| `1916452` scaffold | ~~0%~~ → **0%** | — | premake skeleton, harness, plan |
| `5a3e2a5` recomp milestone | ~~0%~~ → **~10%** | ~100% of ported | vendored TinyXML+LZMA, loader/config/archive |
| `07be648` engine tranche | ~~10%~~ → **~30%** | ~100% of ported | limits processors, model-ID engine (2,288 patch sites), hooks |
| `4af9792` tranche-3 | ~~30%~~ → **~50%** | ~100% of ported | hook helpers, dir manager, save system |
| `7a0fc6e` tranche-4 | ~~50%~~ → **63%** | ~100% of ported | weapons + SCM clusters |
| `272770b` tranche-5-prep | ~~63%~~ → **67%** | ~100% of ported | globals consolidation (engine_globals), naming rules, workflow template |
| `9577e6f` tranche-5 | ~~67%~~ → **81%** (125/155) | ~100% of ported | Python host + init chain, SCM bodies (0801/0802/IgnoreOpcode), media cluster (audio/png/ifp/emlights) |
| `a19783e` tranche-6 | ~~81% (125/155)~~ → **81%** (144/177) | ~100% of ported | weapon save-write + ApplySaveData, Python tuning methods/pool getter; census refined with 20 function addenda (+22 denominator, +19 ported) |
| *this commit* tranche-7 | ~~81% (144/177)~~ → **92%** (163/177) | ~100% of ported | vehicle archive-scanning cluster (LoadVehicleArchives/AnalyzeStartIndex/LoadSanAndreasVehiclesTxd), tuning parts (LoadTuningFile/BuildPartDummyName), SA COL loader (COL2/COL3/COLL), GenerateDefaultXML (Alt+4 xmlgen), MvlSecWnd, DX9TextureFix |

| Metric (current) | Value | Method |
|--------|------:|--------|
| **Recompiled** | **92%** | 163 of 177 MVL-specific census functions ported (census = RTTI-named + subsystem-mapped fns in `.serena/memories/re-098e-map.md`, incl. the tranche-5/6 addenda; excludes CRT/thunk noise from the 2,922 total functions Ghidra found). Of the 14 not counted: 11 are file/string data addresses leaked into the census (not functions) and 1 is a catch-block of an already-ported function — so the true unported-function count is 2 (both tiny: 0x24110 manim.initAnimType 105 B, 0x290d0 event-handler map insert 520 B) |
| **Accuracy** | **~100% of ported code is 1:1** | Every ported function cites `0.98e @ RVA 0x…` and mirrors the decompiled original; remaining gaps are explicit `TODO(...)`/`HOOK POINT` stubs (388 + 138 markers) that log-and-skip rather than fake behavior |

Honest caveat: the accuracy number measures *fidelity of what is ported*, not completeness — the TODO/hook-point stubs are placeholders for engine-internal calls whose bodies aren't ported yet. 1,074 RVA-cited sites, 20,687 LOC in `src/`.

- RE targets: `original_files/maxovehmod*` (0.9 → 0.98e) — READ-ONLY
- Clone implementation: `src/` — modules: loader, config, archive (7z + SA COL), engine (model-ID/limits/hooks/helpers), vehicle (XML parsers + save + tuning), weapons, scm, script (Python 3.3 host), extras (xmlgen), render (png/DX9), audio, anim
- Master plan: `PLAN.md` · Agent instructions: `AGENTS.md` · Knowledge vault: `docs/`

## Build

```bash
premake5.cmd
msbuild build/mvlclone.sln /p:Configuration=Release /p:Platform=Win32
```

Output: `bin/Release/vehmod.dll` (x86, /MT). Deploy target: `%MVL_GAME_DIR%\vehmod_retail.dll` (game dir = `H:\games\VC`).
