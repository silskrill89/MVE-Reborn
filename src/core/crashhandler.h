// crashhandler.h — MVL diagnostics toolkit P1: self-installing crash handler.
//
// Doctrine (per proven skygfx/Earshot patterns):
//   * Installed via a CRT static constructor — NEVER from DllMain (loader-lock
//     hygiene); runs before the user attach body, so init crashes are covered.
//   * SetUnhandledExceptionFilter chains the PREVIOUS filter: our filter logs
//     first, then calls the previous filter and returns ITS result.
//   * Re-entrancy guard (InterlockedCompareExchange) as the very first act.
//   * Every crash-path step runs in its own __try/__except — a crash inside
//     the crash handler degrades to a shorter record, never a hang.
//   * Crash-path file writes use CreateFileA + FILE_SHARE_WRITE (see
//     log.h MvlCrashAppendLine) — no stdio, no malloc on the faulting thread.
//   * Minidump via LoadLibraryA("dbghelp.dll") + GetProcAddress — no .lib.
#pragma once

#include <windows.h>

// Install the filter + register init-chain scope tags. Idempotent; also
// invoked automatically by a static constructor in crashhandler.cpp.
void MvlCrashHandlerInstall(void);

// Scope-tag registry: fixed table of 32 {name, fn} entries. At crash time the
// tag nearest (within +/-0x400 bytes) to the faulting EIP is logged, giving a
// coarse "where in our init chain were we" signal.
void MvlRegisterScope(const char *name, void *fn);

// Human-readable one/two-line exception report through MvlErrorLog (normal
// logging path — used by the DllMain attach guard, not the crash filter).
void MvlLogException(const char *ctx, EXCEPTION_POINTERS *ep);
