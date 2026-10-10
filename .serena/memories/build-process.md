# Build & Deploy — MVL-clone

## Build Commands

```bash
premake5.cmd    # premake5.lua → build/MVL-clone.sln (VS2022)
msbuild build/MVL-clone.sln /p:Configuration=Release /p:Platform=Win32
```

## Toolchain

- MSBuild: VS2022 at `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe`
- Windows 10 SDK 26100
- Platform: x86 (Win32), static CRT (/MT)
- Output: `bin/Release/vehmod.dll`

## Deploy

- Game dir: env var `MVL_GAME_DIR` (unknown — placeholder; ask the user)
- Target: `%MVL_GAME_DIR%\mvl\vehmod_retail.dll`
- Host game: GTA Vice City 1.0 (gta-vc.exe)

## Paths

| Path | Purpose |
|------|---------|
| `premake5.lua` / `premake5.cmd` | Build definition (orchestrator-owned) |
| `src/` | Clone implementation (orchestrator-owned) |
| `bin/` | Build output |
| `original_files/` | READ-ONLY RE targets |
