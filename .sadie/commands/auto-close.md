# /auto-close — MVL Build Cycle

Build vehmod.dll, deploy to the game dir, and check the plugin log.

## Steps

1. Build:
   ```bash
   premake5.cmd                    # regenerate VS2022 solution from premake5.lua
   msbuild build/MVL-clone.sln /p:Configuration=Release /p:Platform=Win32
   ```

2. Verify build output:
   ```powershell
   Test-Path "bin\Release\vehmod.dll"
   (Get-Item "bin\Release\vehmod.dll").LastWriteTime
   ```

3. Deploy (game dir = `H:\games\VC`, persisted as user env var `MVL_GAME_DIR` via setx):
   ```powershell
   Test-Path "$env:MVL_GAME_DIR\mvl"   # target: $env:MVL_GAME_DIR\mvl\vehmod_retail.dll
   # Note: only processes started AFTER the setx see the variable; create mvl\ if missing.
   ```

4. Check build log for errors:
   ```powershell
   Select-String -Path "premake_log.txt" -Pattern "error|warning" | Select-Object -Last 20
   ```
