-- Maxo's Vehicle Loader (clone rebuild) build script
-- Adapted from skygfx_plus_expIV premake5.lua.
-- x86 only: GTA Vice City 1.0 game-code-patching plugins are 32-bit DLLs.

workspace "mvlclone"
	configurations { "Release", "Debug" }
	platforms { "x86" }
	defaultplatform "x86"
	location "build"

	files { "src/**.*" }

	includedirs { "src" }
	includedirs { "external/injector/include" }
	includedirs { "external" }

	-- Optional deploy target: when MVL_GAME_DIR is set in the environment,
	-- the built DLL is additionally copied to that directory after every
	-- build as vehmod_retail.dll. Unset => no copy step at all.
	deploydir = os.getenv("MVL_GAME_DIR")

project "vehmod"
	kind "SharedLib"
	language "C++"
	targetdir "bin/%{cfg.buildcfg}"
	targetextension ".dll"
	characterset ("MBCS")
	staticruntime "On"
	systemversion "10.0.26100.0"

	defines { "_CRT_NONSTDC_NO_WARNINGS", "_CRT_SECURE_NO_WARNINGS" }
	buildoptions { "/FS" }

	filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

	filter {}
	if deploydir then
		postbuildcommands { 'copy /y "$(TargetPath)" "' .. deploydir .. '\\vehmod_retail.dll"' }
	end
