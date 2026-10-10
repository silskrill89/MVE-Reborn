# Pre-Flight Checks — MVL-clone

Run before starting work to verify the environment is healthy.

## 1. Game directory (deploy target)
```powershell
Test-Path "$env:MVL_GAME_DIR\gta-vc.exe"   # placeholder — MVL_GAME_DIR unknown
```

## 2. Build tools available
```powershell
# MSBuild
Test-Path "C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe"

# premake5 (project root)
Test-Path "premake5.cmd"
```

## 3. RE targets present
```powershell
Test-Path "original_files\maxovehmod098e"
Test-Path "original_files\maxovehmod"
```

## 4. Build output exists
```powershell
Test-Path "bin\Release\vehmod.dll"
```
If not → run `premake5.cmd` then `msbuild build/MVL-clone.sln /p:Configuration=Release /p:Platform=Win32`

## 5. Deploy target matches
```powershell
Test-Path "$env:MVL_GAME_DIR\mvl\vehmod_retail.dll"   # placeholder
```

## 6. Obsidian vault accessible
```powershell
Test-Path "docs\index.md"
```

## 7. Git status clean (optional)
```powershell
git status --short
```

## 8. Sadie database fresh
```powershell
python .sadie/sadie.py stats
```
If stale → run `python .sadie/sadie.py init` + `python .sadie/sadie_vault_sync.py`
