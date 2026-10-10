@echo off
REM ============================================================
REM b.bat — build dispatcher for mvlclone (simple version)
REM
REM 1. Runs premake5.cmd to (re)generate build/mvlclone.sln
REM 2. Builds Release|x86 with MSBuild if the solution exists.
REM
REM NOTE: the skygfx project has a fancy python build pipeline;
REM that is NOT in scope here — this is intentionally minimal.
REM
REM Optional env vars:
REM   MVL_GAME_DIR   game dir to deploy bin\<cfg>\vehmod.dll into as
REM                  vehmod_retail.dll (mvl\ subdirectory). Falls back to
REM                  H:\games\VC if unset. Set persistently via:
REM                    setx MVL_GAME_DIR "H:\games\VC"
REM                  (only NEW processes see the variable)
REM ============================================================

setlocal
cd /d "%~dp0"

REM --- deploy target: env var, fallback to confirmed game dir ---
set "GAME_DIR=%MVL_GAME_DIR%"
if not defined GAME_DIR set "GAME_DIR=H:\games\VC"

call premake5.cmd %*

if not exist build\mvlclone.sln (
	echo [b.bat] ERROR: build\mvlclone.sln not found - premake generation failed.
	exit /b 1
)

for /f "usebackq delims=" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -requires Microsoft.Component.MSBuild -find MSBuild\**\Bin\MSBuild.exe`) do set "MSBUILD=%%i"

if not defined MSBUILD (
	echo [b.bat] ERROR: MSBuild not found via vswhere.
	exit /b 1
)

"%MSBUILD%" build\mvlclone.sln /p:Configuration=Release /p:Platform=x86 /m /v:m
if errorlevel 1 (
	echo [b.bat] ERROR: build failed - not deploying.
	exit /b 1
)

REM --- deploy: bin\Release\vehmod.dll -> %GAME_DIR%\mvl\vehmod_retail.dll ---
if exist "bin\Release\vehmod.dll" (
	if not exist "%GAME_DIR%\mvl" mkdir "%GAME_DIR%\mvl"
	copy /y "bin\Release\vehmod.dll" "%GAME_DIR%\mvl\vehmod_retail.dll" >nul && (
		echo [b.bat] Deployed to %GAME_DIR%\mvl\vehmod_retail.dll
	) || (
		echo [b.bat] WARNING: deploy failed ^(game running? dir missing?^) - build output is in bin\Release\vehmod.dll
	)
) else (
	echo [b.bat] WARNING: bin\Release\vehmod.dll not found - nothing to deploy.
)

endlocal
