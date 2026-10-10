// log.h — MVL message/error logging helper.
//
// Log targets are faithful to the original: vehmod_retail.dll 0.98e references
// "mvl\messagelog.txt" (string @ 0x1007cad0) and "mvl\errorlog.txt"
// (@ 0x1007c180) — see .serena/memories/re-098e-map.md §3 Misc.
// Paths are game-CWD-relative, exactly like the original.
//
// Buffer-safety rules: fopen_s/snprintf only — never strcpy/sprintf.
#pragma once

// Requires windows.h + stdio.h (stdafx.h guarantees the include order).
BOOL MvlLogInit(void);      // create mvl\ dir if missing, open both logs (append)
void MvlLogShutdown(void);  // close both logs (DLL_PROCESS_DETACH)

// Append a timestamped line to mvl\messagelog.txt (+ OutputDebugStringA).
void MvlLog(const char *fmt, ...);

// Append a timestamped line to mvl\errorlog.txt (+ OutputDebugStringA).
void MvlErrorLog(const char *fmt, ...);
