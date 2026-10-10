// chooser.cpp — faithful clone of the vehmod_chooser.flt loader shim.
//
// Citation: vehmod_chooser.flt 0.98e @ RVA 0x154, see .serena/memories/re-loader-shim.md
// (532-byte PE32 shim, image base 0x10000000, entire .text = 0x40 bytes,
// byte-identical across 0.98c/d/e).
//
// Original logic (full disassembly, re-loader-shim.md):
//   DllMain(hInst, reason, reserved):           ; stdcall, ret 0xC
//     if(reason != DLL_PROCESS_ATTACH) return;  ; cmp [esp+0xC],1 / jne ret
//     eax = *(dword*)0x61A7F8;                  ; fixed gta-vc.exe VA read
//     if(eax == 0x9753C405) name = "mvl\vehmod_retail.dll";   ; [ebx+0x55] @ 0x100001E9
//     else if(eax == 0xE8535004) name = "mvl\vehmod_steam.dll";; [ebx+0x6B] @ 0x100001FF
//     else return;                              ; unknown exe -> silent no-op
//     LoadLibraryA(name);                       ; sole IAT import
//
// Clone note: in this single-DLL build the real DllMain lives in core/main.cpp,
// so the shim's DllMain is exposed under a provisional name (naming policy:
// original was an unnamed EP; confidence: COMPLETE per re-loader-shim.md).
// When the standalone .flt shim target is split out, this body becomes its
// DllMain verbatim. It is compiled in but NOT invoked from our DllMain —
// dispatching it here would just LoadLibrary our own module path.
#include "../core/stdafx.h"

// Fingerprint constants (re-loader-shim.md): one dword at fixed gta-vc.exe
// VA 0x61A7F8 decides the exe variant. No pattern scan, no checksum.
static const DWORD kFingerprintRetail = 0x9753C405;
static const DWORD kFingerprintSteam  = 0xE8535004;
static const DWORD kVcFingerprintVa   = 0x61A7F8;

extern "C" BOOL WINAPI
VehmodChooserDllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
	const char *dllName;
	DWORD fingerprint;
	volatile const DWORD *pFingerprint;

	(void)hInst;
	(void)reserved;

	if(reason != DLL_PROCESS_ATTACH)
		return TRUE; // cmp [esp+0xC],1 / jne ret

	// Guard the fixed-VA read with SEH: the original never guards (it is only
	// ever loaded by gta-vc.exe), but during testing we may be attached to
	// non-VC exes where 0x61A7F8 is unmapped. Plain-C locals only — no
	// unwinding objects allowed alongside __try (MSVC C2712).
	pFingerprint = (volatile const DWORD *)(uintptr_t)kVcFingerprintVa;
	__try{
		fingerprint = *pFingerprint;
	}
	__except(EXCEPTION_EXECUTE_HANDLER){
		return TRUE; // unreadable -> treat like the original's silent failure
	}

	if(fingerprint == kFingerprintRetail)
		dllName = "mvl\\vehmod_retail.dll";
	else if(fingerprint == kFingerprintSteam)
		dllName = "mvl\\vehmod_steam.dll";
	else
		return TRUE; // neither fingerprint -> load nothing (silent, as original)

	LoadLibraryA(dllName); // sole IAT import: KERNEL32!LoadLibraryA
	return TRUE;
}
