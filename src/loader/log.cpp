// log.cpp — implementation of the MVL logging helper (see log.h).
//
// Diagnostics toolkit P0: module-relative log dirs with CWD + temp fallbacks,
// rotation at init, ring buffer of the last 32 lines, MVL_DEBUG verbosity
// gate, throttled logging, and crash-path (share-mode) appends.
//
// NOTE on _fsopen: log files are opened with _fsopen(_SH_DENYNO), NOT
// fopen_s. Repro'd quirk (C:\test\flt-diag\sharetest4/5.c): after a
// MoveFileExA rotation, a fopen_s("a") handle on the recreated file is
// EXCLUSIVE (share=0) — even a same-process CreateFileA with full sharing
// fails with ERROR_SHARING_VIOLATION (gle=32), which broke the crash-path
// appends. _fsopen(_SH_DENYNO) shares correctly in the same scenario.
#include "../core/stdafx.h"
#include "log.h"

#include <stdarg.h>
#include <share.h> // _SH_DENYNO

#if !MVL_DIAGNOSTICS_DISABLED

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

static FILE *g_msgLog = NULL; // messagelog.txt
static FILE *g_errLog = NULL; // errorlog.txt

// Resolved absolute paths (g_*Path[0] == 0 until init ran).
static char g_msgPath[MAX_PATH];
static char g_errPath[MAX_PATH];
static char g_moduleDir[MAX_PATH]; // our DLL's directory, no trailing slash

// Verbosity gate: env var MVL_DEBUG=1, read once at init.
static BOOL g_verbose = FALSE;

// Init-once guard: 0 = not init, 1 = initializing, 2 = ready.
static volatile LONG g_logState = 0;

// Guards g_msgLog/g_errLog and the ring buffer. Kept simple per spec.
static CRITICAL_SECTION g_logCS;
static BOOL g_logCSValid = FALSE;

// ---------------------------------------------------------------------------
// Ring buffer: 32 entries x 160 chars, every MvlLog/MvlErrorLog line lands
// here; MvlRingDump/MvlCrashDumpRing write all entries raw.
// ---------------------------------------------------------------------------
#define MVL_RING_SLOTS 32
#define MVL_RING_LINE  160
static char g_ring[MVL_RING_SLOTS][MVL_RING_LINE];
static DWORD g_ringHead = 0; // next write index (wraps)

// ---------------------------------------------------------------------------
// Throttle: 64 hashed tag slots, 2-second window per tag (Earshot pattern).
// ---------------------------------------------------------------------------
#define MVL_THROTTLE_SLOTS   64
#define MVL_THROTTLE_WINDOW_MS 2000
static DWORD g_throttleLast[MVL_THROTTLE_SLOTS]; // 0 = never logged

// ---------------------------------------------------------------------------
// Path resolution
// ---------------------------------------------------------------------------

// Derive OUR OWN module (the DLL this code is linked into) from the address of
// a log.cpp function — never from CWD or GetModuleHandle(NULL) (that is the
// exe). Returns the module's directory without trailing slash.
static BOOL
MvlResolveModuleDir(void)
{
	HMODULE hMod = NULL;
	char path[MAX_PATH];
	char *slash;
	DWORD n;

	if(g_moduleDir[0])
		return TRUE; // already resolved

	if(!GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
	                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
	                       (LPCSTR)&MvlResolveModuleDir, &hMod))
		return FALSE;

	n = GetModuleFileNameA(hMod, path, MAX_PATH);
	if(n == 0 || n >= MAX_PATH)
		return FALSE;

	slash = strrchr(path, '\\');
	if(!slash)
		return FALSE; // no directory component
	*slash = '\0';

	strncpy(g_moduleDir, path, MAX_PATH - 1);
	g_moduleDir[MAX_PATH - 1] = '\0';
	return TRUE;
}

// base + "\\" + leaf into dst (bounds-safe). base may be "" (CWD-relative).
static BOOL
MvlJoinPath(char *dst, size_t cb, const char *base, const char *leaf)
{
	size_t len = 0;

	if(base && *base){
		len = strlen(base);
		if(len + 1 + strlen(leaf) + 1 > cb)
			return FALSE;
		memcpy(dst, base, len);
		dst[len++] = '\\';
	}else{
		if(strlen(leaf) + 1 > cb)
			return FALSE;
	}
	strcpy(dst + len, leaf); // bounds checked above
	return TRUE;
}

// Rotate an existing log file aside before opening fresh:
//   <name> -> <stem>.prev.log  (MoveFileExA, CopyFileA fallback).
// Never truncates: the fresh file is opened in append mode afterwards.
static void
MvlRotateLog(const char *dir, const char *name, const char *prevName)
{
	char oldPath[MAX_PATH];
	char newPath[MAX_PATH];

	if(!MvlJoinPath(oldPath, sizeof(oldPath), dir, name) ||
	   !MvlJoinPath(newPath, sizeof(newPath), dir, prevName))
		return;

	if(GetFileAttributesA(oldPath) == INVALID_FILE_ATTRIBUTES)
		return; // nothing to rotate

	if(!MoveFileExA(oldPath, newPath, MOVEFILE_REPLACE_EXISTING)){
		// Move can fail if another handle holds the file — copy instead,
		// leaving the original in place for continued appends.
		CopyFileA(oldPath, newPath, FALSE);
	}
}

// Open one log file in dir; returns FILE* or NULL (gle receives GetLastError).
static FILE *
MvlOpenLogIn(const char *dir, const char *name, char *pathOut, size_t cbPath,
             DWORD *gle)
{
	char path[MAX_PATH];
	FILE *f = NULL;

	*gle = 0;
	if(!MvlJoinPath(path, sizeof(path), dir, name)){
		*gle = ERROR_BUFFER_OVERFLOW;
		return NULL;
	}
	CreateDirectoryA(dir, NULL); // ERROR_ALREADY_EXISTS is fine

	// _fsopen(_SH_DENYNO), never fopen_s — see the file-header NOTE (the
	// crash-path appends need this handle to share write access).
	f = _fsopen(path, "a", _SH_DENYNO);
	if(!f){
		*gle = GetLastError();
		return NULL;
	}
	strncpy(pathOut, path, cbPath - 1);
	pathOut[cbPath - 1] = '\0';
	return f;
}

// Resolve candidate dirs, rotate + open in order (module dir -> CWD -> temp),
// log the whole resolution story as the FIRST log lines of the winner.
static BOOL
MvlLogInitImpl(void)
{
	char primaryDir[MAX_PATH];
	char tempRoot[MAX_PATH];
	char tempDir[MAX_PATH];
	const char *cwdDir = MVL_LOG_DIR; // CWD-relative, original behavior

	char msgPath[MAX_PATH] = "";
	char errPath[MAX_PATH] = "";
	FILE *msg = NULL, *err = NULL;
	const char *winner = NULL;
	DWORD gleMsg = 0, gleErr = 0;
	char absMsg[MAX_PATH];

	primaryDir[0] = '\0';
	tempRoot[0] = '\0';
	tempDir[0] = '\0';

	// Candidate 1: directory of our own module + "\mvl".
	if(MvlResolveModuleDir())
		MvlJoinPath(primaryDir, sizeof(primaryDir), g_moduleDir, MVL_LOG_DIR);

	// Candidate 3: temp dir + "\mvl" (last-ditch proof-of-life location).
	if(GetTempPathA(MAX_PATH, tempRoot) > 0){
		size_t l = strlen(tempRoot);
		if(l > 0 && tempRoot[l - 1] == '\\')
			tempRoot[l - 1] = '\0'; // MvlJoinPath adds the separator
		MvlJoinPath(tempDir, sizeof(tempDir), tempRoot, MVL_LOG_DIR);
	}

	// --- messagelog: try candidates in order -------------------------------
	if(primaryDir[0]){
		MvlRotateLog(primaryDir, "messagelog.txt", "messagelog.prev.log");
		msg = MvlOpenLogIn(primaryDir, "messagelog.txt", msgPath,
		                   sizeof(msgPath), &gleMsg);
		if(msg)
			winner = primaryDir;
	}
	if(!msg){
		MvlRotateLog(cwdDir, "messagelog.txt", "messagelog.prev.log");
		msg = MvlOpenLogIn(cwdDir, "messagelog.txt", msgPath,
		                   sizeof(msgPath), &gleMsg);
		if(msg)
			winner = cwdDir;
	}
	if(!msg && tempDir[0]){
		MvlRotateLog(tempDir, "messagelog.txt", "messagelog.prev.log");
		msg = MvlOpenLogIn(tempDir, "messagelog.txt", msgPath,
		                   sizeof(msgPath), &gleMsg);
		if(msg)
			winner = tempDir;
	}

	// --- errorlog: same dir as the messagelog winner ------------------------
	if(winner){
		MvlRotateLog(winner, "errorlog.txt", "errorlog.prev.log");
		err = MvlOpenLogIn(winner, "errorlog.txt", errPath,
		                   sizeof(errPath), &gleErr);
	}

	// Commit handles + paths.
	g_msgLog = msg;
	g_errLog = err;
	if(msgPath[0]){
		strncpy(g_msgPath, msgPath, MAX_PATH - 1);
		g_msgPath[MAX_PATH - 1] = '\0';
	}
	if(errPath[0]){
		strncpy(g_errPath, errPath, MAX_PATH - 1);
		g_errPath[MAX_PATH - 1] = '\0';
	}

	// --- resolution story: the FIRST log lines ------------------------------
	// (Written through the lazy formatter so they land even if this was
	// triggered by an early MvlLog from the crash-handler constructor.)
	if(msg){
		MvlLog("log: module dir = %s", g_moduleDir[0] ? g_moduleDir : "<unresolved>");
		if(primaryDir[0])
			MvlLog("log: primary   %s\\messagelog.txt -> %s", primaryDir,
			       msg && winner == primaryDir ? "OPENED" : "failed");
		MvlLog("log: cwd       %s\\messagelog.txt -> %s (gle=%lu)", cwdDir,
		       msg && winner == cwdDir ? "OPENED" : "not used", gleMsg);
		if(tempDir[0])
			MvlLog("log: temp      %s\\messagelog.txt -> %s (gle=%lu)", tempDir,
			       msg && winner == tempDir ? "OPENED" : "not used", gleMsg);
		if(gleErr && !err)
			MvlLog("log: errorlog open FAILED in winner dir (gle=%lu)", gleErr);

		// Resolved absolute paths, as required first-class lines.
		if(GetFullPathNameA(msgPath, MAX_PATH, absMsg, NULL) > 0)
			MvlLog("log: messagelog = %s", absMsg);
		if(err)
			MvlLog("log: errorlog   = %s", g_errPath);
	}else{
		// Nowhere writable — last-ditch: say so via the debugger only.
		OutputDebugStringA("mvl: no log file could be opened (module/cwd/temp all failed)\n");
	}

	// Verbosity gate (P2): MVL_DEBUG=1.
	{
		char val[8];
		val[0] = '\0';
		if(GetEnvironmentVariableA("MVL_DEBUG", val, sizeof(val)) > 0 &&
		   strcmp(val, "1") == 0)
			g_verbose = TRUE;
	}

	return msg ? TRUE : FALSE;
}

// Init-once wrapper. Idempotent; safe to call from DllMain AND from the
// crash-handler static constructor (whichever runs first wins, order-independent).
BOOL
MvlLogInit(void)
{
	LONG prev = InterlockedCompareExchange(&g_logState, 1, 0);
	if(prev != 0)
		return g_msgLog != NULL ? TRUE : FALSE; // already init(ing)

	if(!g_logCSValid){
		InitializeCriticalSection(&g_logCS);
		g_logCSValid = TRUE;
	}

	BOOL ok = MvlLogInitImpl();
	InterlockedExchange(&g_logState, 2);
	return ok;
}

void
MvlLogShutdown(void)
{
	if(g_logCSValid)
		EnterCriticalSection(&g_logCS);
	if(g_msgLog){ fclose(g_msgLog); g_msgLog = NULL; }
	if(g_errLog){ fclose(g_errLog); g_errLog = NULL; }
	if(g_logCSValid)
		LeaveCriticalSection(&g_logCS);
}

// ---------------------------------------------------------------------------
// Core formatter
// ---------------------------------------------------------------------------

// Format "[timestamp] <tag><fmt>\n" into buf (truncation-safe), append to the
// ring, write to f and mirror to the debugger. Never overruns: every append
// is bounds-checked and vsnprintf/snprintf always NUL-terminate.
static void
MvlLogFormat(FILE *f, const char *tag, const char *fmt, va_list args)
{
	char buf[1024];
	SYSTEMTIME st;
	size_t len;

	GetLocalTime(&st);
	snprintf(buf, sizeof(buf), "[%04u-%02u-%02u %02u:%02u:%02u.%03u]%s",
	         st.wYear, st.wMonth, st.wDay,
	         st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
	         tag ? tag : " ");
	len = strlen(buf);

	vsnprintf(buf + len, sizeof(buf) - len, fmt, args);
	len = strlen(buf); // authoritative after possible truncation

	if(len == 0 || buf[len - 1] != '\n'){
		if(len < sizeof(buf) - 1){
			buf[len++] = '\n';
			buf[len] = '\0';
		} else {
			buf[len - 1] = '\n'; // overwrite last char on full truncation
		}
	}

	// Ring copy (line WITHOUT the trailing newline — dumps add their own;
	// truncated to slot size).
	if(g_logCSValid){
		size_t rlen = (buf[len - 1] == '\n') ? len - 1 : len;
		if(rlen > MVL_RING_LINE - 1)
			rlen = MVL_RING_LINE - 1;
		EnterCriticalSection(&g_logCS);
		memcpy(g_ring[g_ringHead], buf, rlen);
		g_ring[g_ringHead][rlen] = '\0';
		g_ringHead = (g_ringHead + 1) % MVL_RING_SLOTS;
		LeaveCriticalSection(&g_logCS);
	}

	if(f){
		fputs(buf, f);
		fflush(f);
	}
	OutputDebugStringA(buf);
}

// Lazy reopen so logging survives an init-order surprise; uses the RESOLVED
// paths (never the CWD-relative constants) so early callers land in the
// module-relative dir too.
static FILE *
MvlLogGet(FILE **slot, const char *path)
{
	if(!*slot && path[0]){
		*slot = _fsopen(path, "a", _SH_DENYNO); // see file-header NOTE
		if(!*slot)
			*slot = NULL;
	}
	return *slot;
}

// Ensure init ran (crash-handler ctor may log before DllMain calls MvlLogInit).
static void
MvlLogEnsureInit(void)
{
	if(g_logState != 2)
		MvlLogInit();
}

// Core write path shared by MvlLog / MvlErrorLog / MvlLogVerbose.
static void
MvlLogV(FILE **slot, const char *path, const char *tag, const char *fmt,
        va_list args)
{
	FILE *f;

	MvlLogEnsureInit();
	if(g_logCSValid)
		EnterCriticalSection(&g_logCS);
	f = MvlLogGet(slot, path);
	if(f)
		MvlLogFormat(f, tag, fmt, args);
	if(g_logCSValid)
		LeaveCriticalSection(&g_logCS);
}

void
MvlLog(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	MvlLogV(&g_msgLog, g_msgPath, NULL, fmt, args);
	va_end(args);
}

void
MvlErrorLog(const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	MvlLogV(&g_errLog, g_errPath, " [ERROR] ", fmt, args);
	va_end(args);
}

void
MvlLogVerbose(const char *fmt, ...)
{
	va_list args;

	if(!g_verbose)
		return;
	va_start(args, fmt);
	MvlLogV(&g_msgLog, g_msgPath, "[verbose] ", fmt, args);
	va_end(args);
}

// ---------------------------------------------------------------------------
// Accessors
// ---------------------------------------------------------------------------

BOOL
MvlVerboseEnabled(void)
{
	return g_verbose;
}

const char *
MvlMessageLogPath(void)
{
	return g_msgPath;
}

const char *
MvlErrorLogPath(void)
{
	return g_errPath;
}

const char *
MvlModuleDir(void)
{
	if(!g_moduleDir[0])
		MvlResolveModuleDir();
	return g_moduleDir;
}

// ---------------------------------------------------------------------------
// Throttled logging (P2): 64 hashed tag slots, 2-second window.
// ---------------------------------------------------------------------------

static DWORD
MvlHashTag(const char *tag)
{
	DWORD h = 2166136261u; // FNV-1a 32
	while(*tag){
		h ^= (unsigned char)*tag++;
		h *= 16777619u;
	}
	return h & (MVL_THROTTLE_SLOTS - 1);
}

void
MvlLogThrottled(const char *tag, const char *fmt, ...)
{
	char msg[200];
	va_list args;
	DWORD now, slot;
	DWORD last;

	if(!tag || !fmt)
		return;

	now = GetTickCount();
	slot = MvlHashTag(tag);
	last = g_throttleLast[slot];
	if(last != 0 && now - last < MVL_THROTTLE_WINDOW_MS)
		return; // inside the window — drop
	g_throttleLast[slot] = now;

	va_start(args, fmt);
	vsnprintf(msg, sizeof(msg), fmt, args);
	va_end(args);
	MvlLog("[%s] %s", tag, msg);
}

// ---------------------------------------------------------------------------
// Ring dump
// ---------------------------------------------------------------------------

void
MvlRingDump(FILE *f)
{
	DWORD i, idx;

	if(!f)
		return;
	if(!g_logCSValid)
		return;
	EnterCriticalSection(&g_logCS);
	// Oldest -> newest: head is the NEXT write slot.
	for(i = 0; i < MVL_RING_SLOTS; i++){
		idx = (g_ringHead + i) % MVL_RING_SLOTS;
		if(g_ring[idx][0])
			fprintf(f, "%s\n", g_ring[idx]);
	}
	LeaveCriticalSection(&g_logCS);
}

// ---------------------------------------------------------------------------
// Crash-path writes (P1): no stdio handles, no malloc, FILE_SHARE_WRITE.
// ---------------------------------------------------------------------------

// Append one raw line to the resolved errorlog via CreateFileA + WriteFile.
// NOT taking g_logCS — callers on the crash path may already hold it (ring
// dump) or may be in a state where taking it would deadlock.
void
MvlCrashAppendLine(const char *line)
{
	char path[MAX_PATH];
	char stamped[512];
	SYSTEMTIME st;
	DWORD written, len;
	HANDLE h;

	if(!line)
		return;

	// Resolved path if init ran; CWD-relative fallback otherwise (still
	// better than silence).
	if(g_errPath[0])
		strncpy(path, g_errPath, MAX_PATH - 1);
	else
		MvlJoinPath(path, sizeof(path), MVL_LOG_DIR, "errorlog.txt");
	path[MAX_PATH - 1] = '\0';

	h = CreateFileA(path, GENERIC_WRITE,
	                FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
	                NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if(h == INVALID_HANDLE_VALUE)
		return; // nowhere to write — stay silent, never block

	SetFilePointer(h, 0, NULL, FILE_END);

	GetLocalTime(&st);
	len = (DWORD)snprintf(stamped, sizeof(stamped),
	                      "[%04u-%02u-%02u %02u:%02u:%02u.%03u] %s\n",
	                      st.wYear, st.wMonth, st.wDay,
	                      st.wHour, st.wMinute, st.wSecond, st.wMilliseconds,
	                      line);
	if(len >= sizeof(stamped))
		len = sizeof(stamped) - 1;
	WriteFile(h, stamped, len, &written, NULL);
	CloseHandle(h);
}

// Dump the ring into errorlog on the crash path. Uses TryEnterCriticalSection
// so a crash while the faulting thread held the lock degrades to a note
// instead of a deadlock.
void
MvlCrashDumpRing(void)
{
	DWORD i, idx;

	if(!g_logCSValid){
		MvlCrashAppendLine("(ring: log CS not initialized - skipped)");
		return;
	}
	if(!TryEnterCriticalSection(&g_logCS)){
		MvlCrashAppendLine("(ring locked by faulting thread - skipped)");
		return;
	}
	MvlCrashAppendLine("--- last 32 log lines ---");
	for(i = 0; i < MVL_RING_SLOTS; i++){
		idx = (g_ringHead + i) % MVL_RING_SLOTS;
		if(g_ring[idx][0])
			MvlCrashAppendLine(g_ring[idx]);
	}
	MvlCrashAppendLine("--- end ring dump ---");
	LeaveCriticalSection(&g_logCS);
}

#else // MVL_DIAGNOSTICS_DISABLED — compile-time escape hatch: all stubs.

BOOL MvlLogInit(void) { return TRUE; }
void MvlLogShutdown(void) { }
void MvlLog(const char *fmt, ...) { (void)fmt; }
void MvlErrorLog(const char *fmt, ...) { (void)fmt; }
void MvlLogVerbose(const char *fmt, ...) { (void)fmt; }
BOOL MvlVerboseEnabled(void) { return FALSE; }
void MvlLogThrottled(const char *tag, const char *fmt, ...)
{
	(void)tag; (void)fmt;
}
const char *MvlMessageLogPath(void) { return ""; }
const char *MvlErrorLogPath(void) { return ""; }
const char *MvlModuleDir(void) { return ""; }
void MvlRingDump(FILE *f) { (void)f; }
void MvlCrashAppendLine(const char *line) { (void)line; }
void MvlCrashDumpRing(void) { }

#endif // MVL_DIAGNOSTICS_DISABLED
