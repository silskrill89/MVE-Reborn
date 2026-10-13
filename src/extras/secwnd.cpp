// secwnd.cpp — MvlSecWnd port (see secwnd.h).
//
// Ported 1:1 from vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/missing33/decomp-chain.txt, depth=0 FUN_10012170)
// and verified against the raw disassembly of vehmod_retail.dll (all field
// stores/args below are disasm-confirmed; IAT slots are plain Win32 calls).
#include "secwnd.h"

#include "../loader/log.h"

// Class-registered flag — original byte DAT_10124535 @ RVA 0x124535 (checked
// at entry, set to 1 after a successful RegisterClassExA).
unsigned char MvlSecWndClassRegistered = 0;  // DAT_10124535 @ RVA 0x124535

// Window procedure seam — target VA 0x10013310 (lpfnWndProc stored into the
// WNDCLASSEXA below). HOOK POINT: not in this dispatch's decomp range; logs
// via MvlErrorLog (once) and returns 0. Never fakes window behaviour.
LRESULT CALLBACK
MvlSecWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) // FUN_10013310 @ RVA 0x13310
{
	static bool s_logged = false;
	if(!s_logged){
		MvlErrorLog("HOOK POINT 0x13310 FUN_10013310 (MvlSecWnd WNDPROC) "
		            "not ported - window messages ignored");
		s_logged = true;
	}
	(void)hWnd; (void)uMsg; (void)wParam; (void)lParam;
	return 0;
}

// MvlSecWnd — 0.98e @ RVA 0x12170 (236 bytes, __fastcall with the HWND out-
// pointer in ECX; plain pointer argument here).
bool
MvlSecWnd(HWND *phwndOut)
{
	// BUGFIX(stable-1.0): guard the out-pointer — the original writes
	// `mov [ebx], eax` unconditionally (caller contract); the clone refuses
	// to deref NULL so a future caller passing NULL fails cleanly instead of
	// crashing.
	if(phwndOut == NULL)
		return false;

	// cmp byte [DAT_10124535], 0 / jne <create window>
	if(!MvlSecWndClassRegistered){
		WNDCLASSEXA wc;
		// Field stores in the original's fill order (disasm @ 0x12186..0x121fd).
		wc.cbSize        = sizeof(wc);              // 0x30 (asm [ebp-0x30])
		wc.cbClsExtra    = 0;                       // [ebp-0x24]
		wc.cbWndExtra    = 0;                       // [ebp-0x20]
		wc.hbrBackground  = NULL;                   // [ebp-0x10]
		wc.hCursor        = LoadCursorA(NULL, IDC_ARROW);    // [ebp-0x14], id 0x7f00
		wc.hIcon          = LoadIconA(NULL, (LPCSTR)0x7f00); // [ebp-0x18], IDI_APPLICATION
		wc.hIconSm        = LoadIconA(NULL, (LPCSTR)0x7f00); // [ebp-4]
		wc.hInstance      = GetModuleHandleA(NULL);          // [ebp-0x1c]
		wc.lpfnWndProc    = MvlSecWndProc;                    // [ebp-0x28] = 0x10013310
		wc.lpszClassName  = "MvlSecWnd";                     // [ebp-8]  @ RVA 0x1007b2d4
		wc.lpszMenuName   = NULL;                            // [ebp-0xc]
		wc.style          = 8;                               // [ebp-0x2c] CS_DBLCLKS

		if(RegisterClassExA(&wc) == 0){
			return false;                                    // xor al,al / ret
		}
		MvlSecWndClassRegistered = 1;                        // mov byte [DAT_10124535], 1
	}

	// CreateWindowExA args (push order reversed in asm @ 0x12216..0x1224a):
	//   dwExStyle 0, "MvlSecWnd", "MVL camera view", style 0x10CF0000,
	//   x/y CW_USEDEFAULT, w 0x280 (640), h 0x1e0 (480), parent/menu 0,
	//   hInstance = GetModuleHandleA(NULL), lpParam 0.
	HWND hWnd = CreateWindowExA(
		0,
		"MvlSecWnd",                // @ RVA 0x1007b2d4
		"MVL camera view",          // @ RVA 0x1007b2e0
		0x10CF0000,                 // raw original style (disasm @ 0x12239)
		CW_USEDEFAULT, CW_USEDEFAULT,
		640, 480,
		NULL, NULL,
		GetModuleHandleA(NULL),
		NULL);
	*phwndOut = hWnd;                                     // mov [ebx], eax
	return hWnd != NULL;                                  // setne al
}
