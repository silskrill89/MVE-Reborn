@echo off
REM Generate Visual Studio solution.
REM Prefers the local premake5 in tools\; falls back to the sibling skygfx copy
REM (note: that copy is 5.0.0-alpha10 and cannot generate vs2022).
setlocal
cd /d "%~dp0"

if exist "tools\premake5.exe" (
	"tools\premake5.exe" vs2022 %*
) else (
	"E:\dev(dave)\skygfx_plus_expIV\premake5.exe" vs2022 %*
)
endlocal
