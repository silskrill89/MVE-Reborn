# MVL-clone (Maxos_vehicle_revnres)

Reverse-engineering rebuild of **Maxo's Vehicle Loader (MVL)** — a GTA Vice City 1.0 DLL plugin (`vehmod_retail.dll`) with an XML + 7z vehicle-archive config system, Python 3.3 scripting, and direct engine patching. Goal: clone MVL 0.98e functionality from binary RE, then later port to GTA San Andreas.

## Status

**Active recomp** (2026-10-12). MVL 0.98e is being cloned from binary RE, module by module; every tranche ends with a green build.

### Progress metrics (2026-10-12, tranche-4)

| Metric | Value | Method |
|--------|------:|--------|
| **Recompiled** | **63%** | 105 of 167 MVL-specific census functions ported 1:1 (census = RTTI-named + subsystem-mapped fns in `.serena/memories/re-098e-map.md`; excludes CRT/thunk noise from the 2,922 total functions Ghidra found) |
| **Accuracy** | **~100% of ported code is 1:1** | Every ported function cites `0.98e @ RVA 0x…` and mirrors the decompiled original; remaining gaps are explicit `TODO(...)`/`HOOK POINT` stubs (287 + 84 markers) that log-and-skip rather than fake behavior |

Honest caveat: the accuracy number measures *fidelity of what is ported*, not completeness — the TODO/hook-point stubs are placeholders for engine-internal calls whose bodies aren't ported yet.

- RE targets: `original_files/maxovehmod*` (0.9 → 0.98e) — READ-ONLY
- Clone implementation: `src/` — modules: loader, config, archive, engine (model-ID/limits/hooks/helpers), vehicle (XML parsers + save), weapons, scm
- Master plan: `PLAN.md` · Agent instructions: `AGENTS.md` · Knowledge vault: `docs/`

## Build

```bash
premake5.cmd
msbuild build/mvlclone.sln /p:Configuration=Release /p:Platform=Win32
```

Output: `bin/Release/vehmod.dll` (x86, /MT). Deploy target: `%MVL_GAME_DIR%\vehmod_retail.dll` (game dir = `H:\games\VC`).
