// log.h — MVL message/error logging helper (diagnostics toolkit, P0).
//
// Log targets are faithful to the original: vehmod_retail.dll 0.98e references
// "mvl\messagelog.txt" (string @ 0x1007cad0) and "mvl\errorlog.txt"
// (@ 0x1007c180) — see .serena/memories/re-098e-map.md §3 Misc.
//
// Path resolution (diagnostics upgrade, 2026-10-20):
//   1. PRIMARY  : directory of OUR OWN module + "\mvl" — derived via
//                 GetModuleHandleExA(FROM_ADDRESS) on a log.cpp function, so
//                 logs land next to the DLL even when CWD is not the game dir
//                 (e.g. injected loaders, .flt filter hosts).
//   2. FALLBACK : CWD-relative "mvl" — original MVL behavior.
//   3. LAST-DITCH: GetTempPathA() + "mvl" — a log saying where it could not
//                 write is still proof of life.
// The resolved absolute paths are logged as the FIRST log lines.
//
// Rotation at init: an existing messagelog.txt is moved to messagelog.prev.log
// (MoveFileExA, CopyFileA fallback) before a fresh file is opened — never
// truncate without rotating. Same for errorlog.txt.
//
// Ring buffer: the last 32 log lines are kept in a fixed table and can be
// dumped raw via MvlRingDump(); the crash handler uses it for the crash
// record (no malloc on the faulting thread).
//
// Buffer-safety rules: fopen_s/snprintf only — never strcpy/sprintf.
#pragma once

// Compile-time escape hatch: define to 1 to compile the whole diagnostics
// toolkit down to no-op stubs (default 0 — diagnostics ON).
#ifndef MVL_DIAGNOSTICS_DISABLED
#define MVL_DIAGNOSTICS_DISABLED 0
#endif

// Requires windows.h + stdio.h (stdafx.h guarantees the include order).

BOOL MvlLogInit(void);      // resolve dirs, rotate, open both logs (idempotent)
void MvlLogShutdown(void);  // close both logs (DLL_PROCESS_DETACH)

// Append a timestamped line to messagelog.txt (+ OutputDebugStringA mirror).
void MvlLog(const char *fmt, ...);

// Append a timestamped line to errorlog.txt (+ OutputDebugStringA mirror).
// NEVER gated by verbosity — errors always log.
void MvlErrorLog(const char *fmt, ...);

// Verbose logging, gated by env var MVL_DEBUG=1 (read once at init).
// No-ops (cheap flag test) unless the flag is set.
void MvlLogVerbose(const char *fmt, ...);
BOOL MvlVerboseEnabled(void); // flag state, for the boot banner

// Throttled logging: 64 hashed tag slots, 2-second window per tag (~Earshot
// pattern) for future per-event logs (e.g. per-stream, per-frame spam).
// Emits "[tag] <message>" through MvlLog when the tag's window has elapsed.
void MvlLogThrottled(const char *tag, const char *fmt, ...);

// Resolved absolute paths ("" if logging never opened). For the boot banner
// and the crash handler's dmp/record placement.
const char *MvlMessageLogPath(void);
const char *MvlErrorLogPath(void);

// Our own module's directory (no trailing slash), resolved the same way as
// the log dirs. "" if unresolvable. Crash handler uses it for minidumps.
const char *MvlModuleDir(void);

// Ring buffer: dump the last 32 log lines raw (oldest -> newest) to f.
// Used by the crash handler; safe to call anytime.
void MvlRingDump(FILE *f);

// --- Crash-path writes (P1) ------------------------------------------------
// These never touch the stdio FILE* handles and never malloc: they open the
// resolved errorlog with CreateFileA + FILE_SHARE_WRITE share mode, append,
// and close. Safe from inside an exception filter.
void MvlCrashAppendLine(const char *line); // one raw line into errorlog
void MvlCrashDumpRing(void);               // last 32 log lines into errorlog
