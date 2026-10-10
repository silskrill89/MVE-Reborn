# MVL Build-Test-Iterate Cycle

> premake5 → MSBuild → vehmod.dll in bin/ → deploy to MVL_GAME_DIR → Launch VC 1.0 → Check log → Repeat

## Prerequisites

- MSBuild (VS2022) at `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe`
- Windows 10 SDK 26100
- Toolchain: x86, static CRT (/MT)
- Game dir via env var `MVL_GAME_DIR` (currently unknown/placeholder — ask the user)

## Step-by-step

### 1. Build

```bash
premake5.cmd    # premake5.lua → build/MVL-clone.sln
msbuild build/MVL-clone.sln /p:Configuration=Release /p:Platform=Win32
```

| Error | Fix |
|---|---|
| MSBuild not found | Install VS2022 Build Tools |
| LNK error | Check src/ includes, clean build |
| SDK not found | Verify Win10 SDK 26100 installed |

### 2. Deploy

Target: `$env:MVL_GAME_DIR\mvl\vehmod_retail.dll` (placeholder — game dir unknown; original MVL deploys as `vehmod_retail.dll` in a `mvl/` subdirectory).

Verify:
```powershell
(Get-Item "$env:MVL_GAME_DIR\mvl\vehmod_retail.dll").LastWriteTime
```

### 3. Launch game

Launch `gta-vc.exe` from `MVL_GAME_DIR` manually (no launcher script exists yet).

### 4. Check plugin output

MVL writes its own logs/records in the game dir. Patterns to look for once RE identifies them:

| Pattern | Meaning |
|---|---|
| vehicle XML parse error | Config/XML schema issue |
| 7z archive open failure | Vehicle archive loader issue |
| D3D8 device hook | Engine patching active |

### 5. Failure recovery

- **Plugin not loaded**: Verify DLL is x86 + /MT, check the VC 1.0 ASI/filter loader picked it up.
- **Vehicles missing**: Check XML files next to the DLL, verify 7z archives paths.
- **Crash on startup**: Attach debugger, compare hook sites against RE notes in `.serena/memories/re-*.md`.

## Key paths

| Path | Purpose |
|---|---|
| `original_files/maxovehmod098e/` | Primary RE target (0.98e) |
| `src/` | Clone implementation (orchestrator-owned) |
| `bin/` | Build output (vehmod.dll) |
| `docs/` | Obsidian vault: RE notes |
| `$env:MVL_GAME_DIR\mvl\vehmod_retail.dll` | Deployed plugin (placeholder) |
