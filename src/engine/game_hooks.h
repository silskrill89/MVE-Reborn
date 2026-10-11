// game_hooks.h - MVL 0.98e hook machinery (JMP patcher / dword writer /
// .text unlocker) and the ported VC 1.0 hook-handler stubs.
//
// Sources:
//   * Hook trio: vehmod_retail.dll 0.98e @ RVA 0x22560 / 0x22590 / 0x225a0
//     (FUN_10022560 / FUN_10022590 / FUN_100225a0).
//   * Handlers: .harness-memory/limout/stubs/range-decomp.txt (19 fns,
//     RVA 0x3dff0..0x3e320) and .../stubs2/range-decomp.txt (3 fns,
//     RVA 0x43730..0x43780).
//   * Installation pairs: FUN_10043c00 (0.98e @ RVA 0x43c00, main init) call
//     sites of FUN_10022560 / FUN_10022590.
//
// All handler names are the recovered original symbol names (Ghidra convention
// FUN_<VA>); the naming policy mandates the 1:1 mapping. Handlers are installed
// mid-function by an E9 JMP or by overwriting a function-pointer dword; they
// mirror the original's displaced code and then fall through to a VC 1.0
// continuation address. See port-notes-engine.md for the address tables.
#pragma once

#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Hook machinery (original trio).
// ---------------------------------------------------------------------------

// Writes `E9 rel32` at target so it jumps to destination.
// Original: FUN_10022560 (0.98e @ RVA 0x22560).
void MvlWriteJmp(void *target, const void *destination);

// Raw dword store (used for function-pointer slots).
// Original: FUN_10022590 (0.98e @ RVA 0x22590).
void MvlWriteDword(void *target, uint32_t value);

// VirtualProtect(0x401000, 0x27CE00, PAGE_EXECUTE_READWRITE) - unlocks the
// whole VC 1.0 .text range once so the patches above land.
// Original: FUN_100225a0 (0.98e @ RVA 0x225a0).
void MvlUnlockGameText(void);

// ---------------------------------------------------------------------------
// Hook handlers (stub ports, original order = RVA order).
// ---------------------------------------------------------------------------
void     FUN_1003dff0(void);                                       // 0.98e @ RVA 0x3dff0
uint64_t __fastcall FUN_1003e010(uint32_t, uint32_t, uint32_t, int32_t); // 0.98e @ RVA 0x3e010
void     FUN_1003e070(void);                                       // 0.98e @ RVA 0x3e070
void     FUN_1003e090(void);                                       // 0.98e @ RVA 0x3e090
void     FUN_1003e0b0(void);                                       // 0.98e @ RVA 0x3e0b0
uint64_t __fastcall FUN_1003e0d0(uint32_t, uint32_t, int32_t, int32_t); // 0.98e @ RVA 0x3e0d0
void     FUN_1003e100(void);                                       // 0.98e @ RVA 0x3e100
uint64_t __fastcall FUN_1003e120(int32_t, uint32_t);               // 0.98e @ RVA 0x3e120
void     __fastcall FUN_1003e140(uint32_t);                        // 0.98e @ RVA 0x3e140
// thiscall ABI on a free function: this in ECX, args on stack -> __fastcall
// with a dummy EDX 2nd param maps identically (ECX/EDX = first two args).
void     __fastcall FUN_1003e170(int32_t, void* /*edx*/, uint32_t, uint32_t); // 0.98e @ RVA 0x3e170
uint64_t __fastcall FUN_1003e1d0(uint32_t, uint32_t, uint32_t, uint32_t); // 0.98e @ RVA 0x3e1d0
uint64_t __fastcall FUN_1003e200(uint32_t, uint32_t, uint32_t, uint32_t); // 0.98e @ RVA 0x3e200
void     __fastcall FUN_1003e230(uint32_t);                        // 0.98e @ RVA 0x3e230
void     __fastcall FUN_1003e250(uint32_t);                        // 0.98e @ RVA 0x3e250
void     __fastcall FUN_1003e270(uint32_t);                        // 0.98e @ RVA 0x3e270
void     __fastcall FUN_1003e290(uint32_t);                        // 0.98e @ RVA 0x3e290
void     __fastcall FUN_1003e2b0(uint32_t);                        // 0.98e @ RVA 0x3e2b0
// Naked trampolines: consume raw stack/register state from the hook site
// (see the definitions; Ghidra stack offsets preserved).
void     FUN_1003e2d0(void);                                       // 0.98e @ RVA 0x3e2d0 (naked on definition)
void     FUN_1003e320(void);                                       // 0.98e @ RVA 0x3e320 (naked on definition)
void     FUN_10043730(void);                                       // 0.98e @ RVA 0x43730
void     __fastcall FUN_10043760(uint32_t);                        // 0.98e @ RVA 0x43760
void     FUN_10043780(void);                                       // 0.98e @ RVA 0x43780 (naked on definition)

// ---------------------------------------------------------------------------
// Patch table: (VC 1.0 exe address -> MVL handler) pairs extracted from the
// FUN_10022560 / FUN_10022590 call sites in FUN_10043c00 (main init).
// dwordWrite != 0 means MvlWriteDword(handler) instead of MvlWriteJmp.
// The conditional site VC 0x60048c -> FUN_10043780 (guarded by plugin flag
// DAT_100b1791) is applied by MvlInstallStubHooks(), not tabled here.
// ---------------------------------------------------------------------------
struct MvlHookSite
{
	uint32_t  vcTarget;   // VC 1.0 exe address (image base 0x400000)
	uintptr_t handler;    // plugin handler to install
	uint8_t   dwordWrite; // 0 = E9 JMP, 1 = raw dword write
};

extern const MvlHookSite g_MvlStubHookSites[];
extern const size_t     g_MvlStubHookSiteCount;

// Unlocks VC .text and installs every tabled pair (+ the conditional one).
// Call once during plugin init, after the VC module is loaded.
void MvlInstallStubHooks(void);
