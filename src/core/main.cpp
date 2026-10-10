// main.cpp — DLL entry point for the MVL clone rebuild.
// Minimal, dependency-light: opens a log file next to the module and writes
// a version banner. Game-code hooking will be added module by module.
#include "stdafx.h"

static FILE *g_log = nullptr;

static void
WriteTimestamp(FILE *f)
{
	SYSTEMTIME st;
	GetLocalTime(&st);
	fprintf(f, "[%04u-%02u-%02u %02u:%02u:%02u.%03u] ",
	        st.wYear, st.wMonth, st.wDay,
	        st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
}

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID)
{
	if(reason == DLL_PROCESS_ATTACH){
		char modulePath[MAX_PATH];
		GetModuleFileNameA(hInst, modulePath, MAX_PATH);

		// Log file lives next to the DLL: mvlclone_log.txt
		char logPath[MAX_PATH];
		strncpy_s(logPath, modulePath, _TRUNCATE);
		char *slash = strrchr(logPath, '\\');
		char *bslash = strrchr(logPath, '/');
		char *base = slash > bslash ? slash : bslash;
		if(base) base[1] = '\0';
		else logPath[0] = '\0';
		strncat(logPath, "mvlclone_log.txt", sizeof(logPath) - strlen(logPath) - 1);

		g_log = fopen(logPath, "a");
		if(g_log){
			WriteTimestamp(g_log);
			fprintf(g_log, "MVL-clone " MVL_VERSION_STRING " build attached (module: %s)\n", modulePath);
			fflush(g_log);
		}

		DisableThreadLibraryCalls(hInst);
	}
	else if(reason == DLL_PROCESS_DETACH){
		if(g_log){
			WriteTimestamp(g_log);
			fprintf(g_log, "MVL-clone detached\n");
			fclose(g_log);
			g_log = nullptr;
		}
	}
	return TRUE;
}

// TODO — planned modules for this rebuild (from reverse engineering):
//   archive/   : 7z-based vehicle archive loader (mounted IMG replacement)
//   xml/       : XML vehicle configuration parsing (vehicle defs, upgrades)
//   modelstore/: model storage + vehicle limits expansion (CVehicleModelInfo,
//                pool sizes, streaming buffers)
//   streaming/ : custom streaming hooks (CDirectory, CStreaming request paths)
//   audio/     : vehicle audio hooks (engine sound lookup)
//   scm/       : custom SCM opcodes for spawn/vehicle control
//   scripting/ : optional python scripting bridge
