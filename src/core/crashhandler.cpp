// crashhandler.cpp — MVL diagnostics toolkit P1 (see crashhandler.h).
//
// Crash record format (one line into errorlog, share-mode append):
//   [CRASH] code=0x%08X addr=0x%08X op=(READ|WRITE|EXEC) fault=0x%08X eip=<module>+0x%X
// followed by the nearest scope tag (if any), the last 32 log lines (ring
// dump) and a MiniDumpNormal minidump beside our DLL.
#include "stdafx.h"
#include "crashhandler.h"
#include "../loader/log.h"
#include "init.h"                     // MvlInstallInitChainPatches
#include "../engine/engine_globals.h" // MvlInitExeAddrs

#if !MVL_DIAGNOSTICS_DISABLED

#if !defined(_M_IX86)
#error "MVL clone: crashhandler.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static LPTOP_LEVEL_EXCEPTION_FILTER g_prevFilter = NULL;
static volatile LONG g_inFilter = 0;     // re-entrancy guard
static volatile LONG g_installed = 0;    // install-once guard

// Scope-tag registry: 32 fixed entries, init-time writes only.
#define MVL_SCOPE_MAX 32
struct MvlScopeTag {
	const char *name;
	const void *addr;
};
static MvlScopeTag g_scopes[MVL_SCOPE_MAX];
static volatile LONG g_scopeCount = 0;

// ---------------------------------------------------------------------------
// Scope tags
// ---------------------------------------------------------------------------

void
MvlRegisterScope(const char *name, void *fn)
{
	LONG i = g_scopeCount;

	if(!name || !fn)
		return;
	if(i >= MVL_SCOPE_MAX)
		return; // table full — silently drop (init-time only)
	g_scopes[i].name = name;
	g_scopes[i].addr = fn;
	g_scopeCount = i + 1;
}

// Nearest tag within +/-0x400 of eip -> log "MVL SCOPE: name (+0x%X)".
static void
MvlCrashScopeTag(DWORD eip)
{
	char line[256];
	LONG best = -1;
	DWORD bestDelta = 0xFFFFFFFFu;
	LONG i;

	if(eip == 0)
		return;
	for(i = 0; i < g_scopeCount; i++){
		DWORD tagAddr = (DWORD)(uintptr_t)g_scopes[i].addr;
		DWORD delta = (eip >= tagAddr) ? (eip - tagAddr) : (tagAddr - eip);
		if(delta <= 0x400 && delta < bestDelta){
			bestDelta = delta;
			best = i;
		}
	}
	if(best < 0)
		return;

	{
		DWORD tagAddr = (DWORD)(uintptr_t)g_scopes[best].addr;
		if(eip >= tagAddr)
			snprintf(line, sizeof(line), "MVL SCOPE: %s (+0x%X)",
			         g_scopes[best].name, bestDelta);
		else
			snprintf(line, sizeof(line), "MVL SCOPE: %s (-0x%X)",
			         g_scopes[best].name, bestDelta);
		MvlCrashAppendLine(line);
	}
}

// ---------------------------------------------------------------------------
// EIP -> module+offset resolution
// ---------------------------------------------------------------------------

// Resolve which module owns eip; write "name+0x%X" (or the raw eip on
// failure) into out. GetModuleHandleExA(FROM_ADDRESS) — no snapshot, no
// allocation beyond the stack buffers.
static void
MvlResolveEip(DWORD eip, char *out, size_t cb)
{
	HMODULE hMod = NULL;
	char path[MAX_PATH];
	char *slash;
	const char *name;
	DWORD n, base;

	out[0] = '\0';
	if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
	                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       (LPCSTR)(uintptr_t)eip, &hMod)){
		snprintf(out, cb, "0x%08X", eip);
		return;
	}
	n = GetModuleFileNameA(hMod, path, MAX_PATH);
	if(n == 0 || n >= MAX_PATH){
		snprintf(out, cb, "0x%08X", eip);
		return;
	}
	slash = strrchr(path, '\\');
	name = slash ? slash + 1 : path;
	base = (DWORD)(uintptr_t)hMod;
	snprintf(out, cb, "%s+0x%X", name, eip - base);
}

// ---------------------------------------------------------------------------
// Crash record (one line, errorlog, share-mode append)
// ---------------------------------------------------------------------------

static void
MvlCrashRecord(EXCEPTION_POINTERS *ep)
{
	EXCEPTION_RECORD *er;
	CONTEXT *ctx;
	char line[512];
	char eipLoc[160];
	const char *op = "N/A";
	DWORD fault = 0;

	if(!ep || !ep->ExceptionRecord || !ep->ContextRecord)
		return;
	er = ep->ExceptionRecord;
	ctx = ep->ContextRecord;

	// ACCESS_VIOLATION: ExceptionInformation[0] = 0 READ / 1 WRITE / 8 EXEC,
	// [1] = inaccessible address.
	if(er->ExceptionCode == 0xC0000005u && er->NumberParameters >= 2){
		switch(er->ExceptionInformation[0]){
		case 0: op = "READ"; break;
		case 1: op = "WRITE"; break;
		case 8: op = "EXEC"; break;
		default: op = "N/A"; break;
		}
		fault = (DWORD)er->ExceptionInformation[1];
	}

	MvlResolveEip(ctx->Eip, eipLoc, sizeof(eipLoc));
	snprintf(line, sizeof(line),
	         "[CRASH] code=0x%08X addr=0x%08X op=%s fault=0x%08X eip=%s",
	         (DWORD)er->ExceptionCode, (DWORD)ctx->Eip, op, fault, eipLoc);
	MvlCrashAppendLine(line);
}

// ---------------------------------------------------------------------------
// Minidump — LoadLibraryA("dbghelp.dll") + GetProcAddress, NO .lib dependency.
// ---------------------------------------------------------------------------

// Layout-compatible POD for MINIDUMP_EXCEPTION_INFORMATION (dbghelp.h not
// included on purpose — keeps the build free of any dbghelp dependency).
struct MvlMiniDumpExcInfo {
	DWORD ThreadId;
	EXCEPTION_POINTERS *ExceptionPointers;
	DWORD ClientPointers;
};

typedef BOOL(WINAPI *MiniDumpWriteDumpFn)(HANDLE, DWORD, HANDLE, DWORD,
                                          void *, void *, void *);

#define MVL_MINIDUMP_NORMAL 0x00000000u

// Write one minidump attempt; returns the resulting file size (0 = failure).
static DWORD
MvlMinidumpAttempt(EXCEPTION_POINTERS *ep, const char *path)
{
	HMODULE hDbg;
	MiniDumpWriteDumpFn fn;
	char ansiPath[MAX_PATH]; // CreateFileA needs ANSI
	HANDLE hFile;
	MvlMiniDumpExcInfo mei;
	DWORD size = 0;

	// Our paths are ANSI already (GetModuleFileNameA); copy for clarity.
	strncpy(ansiPath, path, MAX_PATH - 1);
	ansiPath[MAX_PATH - 1] = '\0';

	hFile = CreateFileA(ansiPath, GENERIC_WRITE,
	                    FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
	                    CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(hFile == INVALID_HANDLE_VALUE)
		return 0;

	__try{
		hDbg = LoadLibraryA("dbghelp.dll");
		if(hDbg){
			fn = (MiniDumpWriteDumpFn)(void *)GetProcAddress(hDbg, "MiniDumpWriteDump");
			if(fn){
				mei.ThreadId = GetCurrentThreadId();
				mei.ExceptionPointers = ep;
				mei.ClientPointers = 0;
				fn(GetCurrentProcess(), GetCurrentProcessId(), hFile,
				   MVL_MINIDUMP_NORMAL, &mei, NULL, NULL);
			}
		}
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		// dbghelp blew up mid-dump — fall through, size check decides retry.
	}

	size = GetFileSize(hFile, NULL);
	CloseHandle(hFile);
	return size;
}

static void
MvlCrashMinidump(EXCEPTION_POINTERS *ep)
{
	char path[MAX_PATH];
	char line[256];
	SYSTEMTIME st;
	const char *modDir;
	DWORD size;

	if(!ep)
		return;

	modDir = MvlModuleDir();
	if(!modDir[0])
		return; // nowhere beside us to write — skip silently

	GetLocalTime(&st);
	snprintf(path, sizeof(path),
	         "%s\\MvlCrash_%04u-%02u-%02u_%02u%02u%02u.dmp",
	         modDir, st.wYear, st.wMonth, st.wDay,
	         st.wHour, st.wMinute, st.wSecond);

	size = MvlMinidumpAttempt(ep, path);
	if(size == 0){
		// Retry once — first attempt may have failed on a fresh handle.
		size = MvlMinidumpAttempt(ep, path);
	}
	if(size == 0)
		DeleteFileA(path); // don't leave a 0-byte dmp behind
	else{
		snprintf(line, sizeof(line), "[CRASH] minidump = %s (%lu bytes)", path,
		         size);
		MvlCrashAppendLine(line);
	}
}

// ---------------------------------------------------------------------------
// The filter
// ---------------------------------------------------------------------------

// All crash-path steps, each in its own __try/__except. Object-free function
// (POD buffers only) so SEH is legal here.
static void
MvlCrashFilterSteps(EXCEPTION_POINTERS *ep)
{
	__try{
		MvlCrashRecord(ep);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
	}
	__try{
		MvlCrashScopeTag(ep ? ep->ContextRecord->Eip : 0);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
	}
	__try{
		MvlCrashDumpRing(); // last 32 log lines into errorlog
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
	}
	__try{
		MvlCrashMinidump(ep);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
	}
}

static LONG WINAPI
MvlCrashFilter(EXCEPTION_POINTERS *ep)
{
	// First act: re-entrancy guard. A crash while we are handling a crash
	// must not recurse — bail straight out.
	if(InterlockedCompareExchange(&g_inFilter, 1, 0) != 0)
		return EXCEPTION_CONTINUE_SEARCH;

	__try{
		MvlCrashFilterSteps(ep);
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		// Even the step wrapper blew up — nothing more we can do here.
	}

	// Re-arm so a LATER crash is still handled.
	InterlockedExchange(&g_inFilter, 0);

	// ALWAYS chain the previous filter; its return wins.
	if(g_prevFilter)
		return g_prevFilter(ep);
	return EXCEPTION_CONTINUE_SEARCH;
}

// ---------------------------------------------------------------------------
// Install (self-installing via CRT static constructor)
// ---------------------------------------------------------------------------

void
MvlCrashHandlerInstall(void)
{
	if(InterlockedCompareExchange(&g_installed, 1, 0) != 0)
		return; // already installed

	g_prevFilter = SetUnhandledExceptionFilter(MvlCrashFilter);

	// Register the main init-chain functions so a crash during init names
	// the stage (naming policy: original MVL symbol names).
	MvlRegisterScope("PrepareMvlForLoad", (void *)&PrepareMvlForLoad);
	MvlRegisterScope("LoadGlobalVXMLFile", (void *)&LoadGlobalVXMLFile);
	MvlRegisterScope("LoadGlobalMXMLFile", (void *)&LoadGlobalMXMLFile);
	MvlRegisterScope("MvlInstallInitChainPatches",
	                 (void *)&MvlInstallInitChainPatches);
	MvlRegisterScope("MvlInitExeAddrs", (void *)&MvlInitExeAddrs);

	MvlLog("crash handler installed (prev filter=%p, %d scope tags)",
	       (void *)g_prevFilter, (int)g_scopeCount);
}

namespace {
struct MvlCrashAutoInstall {
	MvlCrashAutoInstall() { MvlCrashHandlerInstall(); }
};
} // namespace

// CRT static constructor — runs during DLL_PROCESS_ATTACH CRT init, BEFORE
// the user DllMain body. Never in DllMain itself.
static MvlCrashAutoInstall g_mvlCrashAutoInstall;

// ---------------------------------------------------------------------------
// Exception reporter for the DllMain attach guard (normal logging path)
// ---------------------------------------------------------------------------

void
MvlLogException(const char *ctx, EXCEPTION_POINTERS *ep)
{
	EXCEPTION_RECORD *er;
	CONTEXT *c;
	char eipLoc[160];

	if(!ep || !ep->ExceptionRecord || !ep->ContextRecord){
		MvlErrorLog("%s: exception (no context available)", ctx);
		return;
	}
	er = ep->ExceptionRecord;
	c = ep->ContextRecord;

	MvlResolveEip(c->Eip, eipLoc, sizeof(eipLoc));
	MvlErrorLog("%s: code=0x%08X eip=%s", ctx, (DWORD)er->ExceptionCode,
	            eipLoc);
	MvlErrorLog("%s: regs eax=0x%08X ebx=0x%08X ecx=0x%08X edx=0x%08X "
	            "esi=0x%08X edi=0x%08X ebp=0x%08X esp=0x%08X",
	            ctx, (DWORD)c->Eax, (DWORD)c->Ebx, (DWORD)c->Ecx,
	            (DWORD)c->Edx, (DWORD)c->Esi, (DWORD)c->Edi, (DWORD)c->Ebp,
	            (DWORD)c->Esp);
}

#else // MVL_DIAGNOSTICS_DISABLED — escape hatch stubs.

void MvlCrashHandlerInstall(void) { }
void MvlRegisterScope(const char *name, void *fn) { (void)name; (void)fn; }
void MvlLogException(const char *ctx, EXCEPTION_POINTERS *ep)
{
	(void)ctx; (void)ep;
}

#endif // MVL_DIAGNOSTICS_DISABLED
