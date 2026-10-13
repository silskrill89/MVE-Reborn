// secwnd.h — MvlSecWnd: secondary "MVL camera view" window (testmenus /
// trashbin support UI).
//
// Port of vehmod_retail.dll 0.98e @ RVA 0x12170 (FUN_10012170, 236 bytes).
// Name per re-098e-map.md §3 ("MvlSecWnd / 'MVL camera view' 0x12170 (window
// class — testmenus/trashbin UI)"); the window class string itself is
// "MvlSecWnd" (@ RVA 0x1007b2d4) and the title "MVL camera view"
// (@ RVA 0x1007b2e0). Registers the class once (style CS_DBLCLKS == 8,
// WNDPROC = FUN_10013310) then creates a 640x480 popup with
// CW_USEDEFAULT position and raw style 0x10CF0000.
#pragma once

#include "../core/stdafx.h"

// MvlSecWnd — 0.98e @ RVA 0x12170. Creates (or reuses) the "MvlSecWnd"
// window class and stores the new HWND through phwndOut. Returns true on
// success; false if RegisterClassExA fails (original returns false) or
// CreateWindowExA returns NULL.
bool MvlSecWnd(HWND *phwndOut);

// Window procedure for the class (lpfnWndProc = 0x10013310). NOT present in
// the decomp range for this dispatch — HOOK POINT seam: logs once and
// returns 0 (game/UI behaviour is not faked).
LRESULT CALLBACK MvlSecWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam); // 0.98e @ RVA 0x13310

// Class-already-registered flag (original DAT_10124535, byte; 0 = not yet
// registered). Owned by secwnd.cpp; the early-load UI lane may inspect it.
extern unsigned char MvlSecWndClassRegistered;  // DAT_10124535 @ RVA 0x124535
