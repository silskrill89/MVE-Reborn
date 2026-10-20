// mvl.h — project-wide header for Maxo's Vehicle Loader clone rebuild.
#pragma once

#define MVL_VERSION_MAJOR 0
#define MVL_VERSION_MINOR 0
#define MVL_VERSION_PATCH 1
#define MVL_STRINGIFY_(x) #x
#define MVL_STRINGIFY(x)  MVL_STRINGIFY_(x)
#define MVL_VERSION_STRING \
	MVL_STRINGIFY(MVL_VERSION_MAJOR) "." \
	MVL_STRINGIFY(MVL_VERSION_MINOR) "." \
	MVL_STRINGIFY(MVL_VERSION_PATCH) " dev"

// Target: GTA Vice City 1.0 (US) — game-code-patching DLL plugin, x86 only.

// All original MVL paths resolve against the game directory (process CWD):
// mvl\messagelog.txt, mvl\errorlog.txt, mvl\vehicles\globalv.xml, ...
// (vehmod_retail.dll 0.98e strings @ 0x1007cad0/0x1007c180, re-098e-map.md §3).
#define MVL_LOG_DIR "mvl"

// Diagnostics toolkit escape hatch (canonical default; log.h/crashhandler.h
// re-guard with #ifndef). 0 = diagnostics ON, 1 = everything compiles to
// no-op stubs.
#ifndef MVL_DIAGNOSTICS_DISABLED
#define MVL_DIAGNOSTICS_DISABLED 0
#endif
