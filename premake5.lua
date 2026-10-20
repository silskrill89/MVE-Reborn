-- Maxo's Vehicle Loader (clone rebuild) build script
-- Adapted from skygfx_plus_expIV premake5.lua.
-- x86 only: GTA Vice City 1.0 game-code-patching plugins are 32-bit DLLs.

workspace "mvlclone"
	configurations { "Release", "Debug" }
	platforms { "x86" }
	defaultplatform "x86"
	location "build"

	files { "src/**.*" }

	-- Vendored third-party sources (compiled into the project, not as libs)
	files {
		"external/tinyxml/**.cpp",
		"external/lzma/C/7zAlloc.c",
		"external/lzma/C/7zArcIn.c",
		"external/lzma/C/7zBuf.c",
		"external/lzma/C/7zBuf2.c",
		"external/lzma/C/7zCrc.c",
		"external/lzma/C/7zCrcOpt.c",
		"external/lzma/C/7zDec.c",
		"external/lzma/C/7zFile.c",
		"external/lzma/C/7zStream.c",
		"external/lzma/C/Bcj2.c",
		"external/lzma/C/Bra.c",
		"external/lzma/C/Bra86.c",
		"external/lzma/C/BraIA64.c",
		"external/lzma/C/CpuArch.c",
		"external/lzma/C/Delta.c",
		"external/lzma/C/LzmaDec.c",
		"external/lzma/C/Lzma2Dec.c",
		"external/lzma/C/Ppmd7.c",
		"external/lzma/C/Ppmd7Dec.c",
		"external/lzma/C/Alloc.c",
	}

	includedirs { "src" }
	includedirs { "external/injector/include" }
	includedirs { "external" }
	includedirs { "external/tinyxml" }
	includedirs { "external/lzma/C" }
	includedirs { "external/lzma/C/Util/7z" }

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

	filter "files:external/lzma/**"
		-- Vendored LZMA SDK: plain C, tolerate SDK-style warnings (never WAE here)
		buildoptions { "/wd4100", "/wd4189", "/wd4245", "/wd4244", "/wd4267" }

	-- Diagnostics toolkit (src/loader/log.cpp, src/core/crashhandler.cpp,
	-- src/core/main.cpp): /W4 warnings-as-errors. The rest of src/ keeps the
	-- /W3 baseline until its pre-existing W4 warnings (C4702/C4100/C4505/...
	-- in custom_anim, mvl_weapon, vehicle_xml, png_funcs, ...) are cleaned
	-- up in a dedicated pass — enabling /WX tree-wide breaks the build today.
	filter "files:src/loader/log.cpp"
		buildoptions { "/W4", "/WX" }
	filter "files:src/core/crashhandler.cpp"
		buildoptions { "/W4", "/WX" }
	filter "files:src/core/main.cpp"
		buildoptions { "/W4", "/WX" }

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
