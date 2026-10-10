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
