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
REM   MVL_GAME_DIR   if set, postbuild copies bin\<cfg>\vehmod.dll
REM                  there as vehmod_retail.dll
REM ============================================================

setlocal
cd /d "%~dp0"

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
endlocal
