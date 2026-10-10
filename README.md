# MVL-clone (Maxos_vehicle_revnres)

Reverse-engineering rebuild of **Maxo's Vehicle Loader (MVL)** — a GTA Vice City 1.0 DLL plugin (`vehmod_retail.dll`) with an XML + 7z vehicle-archive config system, Python 3.3 scripting, and direct engine patching. Goal: clone MVL 0.98e functionality from binary RE, then later port to GTA San Andreas.

## Status

**Scaffolding.** Harness (memory/agent infrastructure) is set up; RE and clone implementation have not started.

- RE targets: `original_files/maxovehmod*` (0.9 → 0.98e) — READ-ONLY
- Clone implementation: `src/` (empty)
- Master plan: `PLAN.md`
- Agent instructions: `AGENTS.md`
- Knowledge vault: `docs/`

## Build

```bash
premake5.cmd
msbuild build/MVL-clone.sln /p:Configuration=Release /p:Platform=Win32
```

Output: `bin/Release/vehmod.dll` (x86, /MT). Deploy target: `%MVL_GAME_DIR%\mvl\vehmod_retail.dll` (game dir = `H:\games\VC`).
