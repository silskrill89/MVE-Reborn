# Technical Context — MVL-clone

## Build System
- **Primary**: `premake5.cmd` → `build/MVL-clone.sln` → MSBuild Release|Win32
- **MSBuild**: VS2022 at `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe`
- **SDK**: Windows 10 SDK 26100
- **Platform**: x86 (Win32), static CRT (/MT)
- **Output**: `bin/Release/vehmod.dll`

## Deploy
- Game dir: env var `MVL_GAME_DIR` (unknown — placeholder)
- Target: `%MVL_GAME_DIR%\mvl\vehmod_retail.dll`
- Host: GTA Vice City 1.0 (gta-vc.exe, x86, D3D8)

## RE Targets
| Version | Path |
|---------|------|
| 0.98e (primary) | `original_files/maxovehmod098e/` |
| 0.9–0.98d | `original_files/maxovehmod*/` |
| xmlgen_launcher | `original_files/xmlgen_launcher/` |

## Domain Tech
- XML config system (schema TBD by RE)
- 7z vehicle archives
- Python 3.3 embedding (python33.dll)
- Engine patching (x86 code patches)

## Key Files
| Path | Purpose |
|------|---------|
| `premake5.lua` | Build definition (orchestrator-owned) |
| `src/` | Clone implementation (orchestrator-owned) |
| `original_files/` | READ-ONLY RE targets |
| `docs/` | Obsidian vault: RE notes |
