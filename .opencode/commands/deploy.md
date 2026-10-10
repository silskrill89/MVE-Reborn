---
name: deploy
description: Build and deploy vehmod.dll to the MVL game dir (H:\games\VC) with running-game safety
---

# /deploy — MVL-clone deploy ritual

Build mode: premake5.cmd → MSBuild Release|Win32 → `bin/Release/vehmod.dll` → deployed as `%MVL_GAME_DIR%\mvl\vehmod_retail.dll` (game dir = `H:\games\VC`, persisted as user env var `MVL_GAME_DIR` via setx; shells started before the setx must re-read it or use the literal path).

## Steps

1. Generate solution: `premake5.cmd` — verify `build/MVL-clone.sln` regenerated.
2. Build: `msbuild build/MVL-clone.sln /p:Configuration=Release /p:Platform=Win32`.
3. Verify output: `bin\Release\vehmod.dll` timestamp/size changed. PE32 x86, statically linked CRT (/MT).
4. Sanity: `premake_log.txt` has no warn/error lines; LSAI/clangd diagnostics 0 errors on touched files.
5. Deploy: copy to `%MVL_GAME_DIR%\mvl\vehmod_retail.dll` (requires `MVL_GAME_DIR` set).
6. In-game verify: launch VC 1.0, check vehicle loading via XML config behaves as expected against RE notes.

## Rules

- If deploy says `Device or resource busy` / access denied → **the game is running**; never force — report and redeploy after user closes it.
- Always keep a `.bak-prev` copy of `vehmod_retail.dll` before overwriting a deployed binary.
- Never deploy with build failures.
- `original_files/` binaries are READ-ONLY RE targets — never deploy from/into them, and never modify them.
