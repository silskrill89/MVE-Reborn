// log.cpp — implementation of the MVL logging helper (see log.h).
#include "../core/stdafx.h"
#include "log.h"

#include <stdarg.h>

static FILE *g_msgLog = NULL; // mvl\messagelog.txt
static FILE *g_errLog = NULL; // mvl\errorlog.txt

static const char *const kMsgLogPath = MVL_LOG_DIR "\\messagelog.txt";
static const char *const kErrLogPath = MVL_LOG_DIR "\\errorlog.txt";

BOOL
MvlLogInit(void)
{
	// Original creates its files under mvl\ relative to the game dir (CWD);
	// make sure the directory exists before opening for append.
	CreateDirectoryA(MVL_LOG_DIR, NULL); // ERROR_ALREADY_EXISTS is fine

	if(fopen_s(&g_msgLog, kMsgLogPath, "a") != 0)
		g_msgLog = NULL;
	if(fopen_s(&g_errLog, kErrLogPath, "a") != 0)
		g_errLog = NULL;

	return g_msgLog != NULL ? TRUE : FALSE;
}

void
MvlLogShutdown(void)
{
	if(g_msgLog){ fclose(g_msgLog); g_msgLog = NULL; }
	if(g_errLog){ fclose(g_errLog); g_errLog = NULL; }
}

// Format "[timestamp] <fmt>\n" into buf (truncation-safe), write to f and
// mirror to the debugger. Never overruns: every append is bounds-checked and
// vsnprintf/snprintf always NUL-terminate.
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

	if(f){
		fputs(buf, f);
		fflush(f);
	}
	OutputDebugStringA(buf);
}

// Lazy reopen so logging survives an init-order surprise; a missing mvl\
// dir just fails fast and the call no-ops.
static FILE *
MvlLogGet(FILE **slot, const char *path)
{
	if(!*slot && fopen_s(slot, path, "a") != 0)
		*slot = NULL;
	return *slot;
}

void
MvlLog(const char *fmt, ...)
{
	va_list args;
	FILE *f = MvlLogGet(&g_msgLog, kMsgLogPath);
	if(!f)
		return;
	va_start(args, fmt);
	MvlLogFormat(f, NULL, fmt, args);
	va_end(args);
}

void
MvlErrorLog(const char *fmt, ...)
{
	va_list args;
	FILE *f = MvlLogGet(&g_errLog, kErrLogPath);
	if(!f)
		return;
	va_start(args, fmt);
	MvlLogFormat(f, " [ERROR] ", fmt, args);
	va_end(args);
}
