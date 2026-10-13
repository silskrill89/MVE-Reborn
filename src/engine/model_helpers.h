// model_helpers.h - MVL 0.98e hook-helper functions (the 11 depth-0 helper
// roots + the CModelIndexLimit driver), ported from the Ghidra decompilation
// of vehmod_retail.dll 0.98e.
//
// Source: .harness-memory/limout/helpers/decomp-chain.txt (54 functions;
// the 12 depth-0 roots below are declared here; depth-1 callees are kept
// file-local inside model_helpers.cpp).
//
// Naming policy: each function has a semantic Mvl* name (rename sweep); the
// original Ghidra identity survives as a cite: "FUN_<VA> (0.98e @ RVA 0x...)".
//
// Calling conventions (MSVC x86, C3865): __thiscall is ILLEGAL on free
// functions, so Ghidra's __thiscall reads are lowered to __fastcall with a
// dummy `void* /*edx*/` 2nd parameter where applicable (this-in-ECX maps to
// the 1st __fastcall arg; EDX is consumed by the dummy). Functions Ghidra
// reports as __fastcall keep __fastcall; plain cdecl roots stay cdecl.
//
// These are the real implementations; game_hooks.cpp still keeps its own
// file-local inert stubs with the same names (separate TUs, no ODR clash)
// until the orchestrator rewires the stub handlers to call through here.
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Depth-0 roots (11 helpers + 1 driver).
// ---------------------------------------------------------------------------

// FUN_1001b610 (0.98e @ RVA 0x1b610) - runs the attach/detach fixup pair on the entity at
// DAT_1013a59c (FUN_1001b640 x2 + FUN_1001b5b0).
void MvlRunAttachFixups(void);

// FUN_1003a450 (0.98e @ RVA 0x3a450) - per-frame refresh of the wheel-part cache on the
// entity at DAT_1013c55c (calls FUN_1003a410 / FUN_1003a350 / FUN_1002f890).
void MvlRefreshWheelCache(void);

// FUN_1003a810 (0.98e @ RVA 0x3a810) - tears down the cached parts block of the entity at
// DAT_1013dc2c (IsBadReadPtr-guarded walk + free). Returns 1 on success,
// 0 otherwise (original EAX semantics preserved).
uint32_t MvlTeardownPartsCache(void);

// FUN_1003af20 (0.98e @ RVA 0x3af20) - per-frame part (door/window/extra) state machine:
// key polling, animation stepping and matrix writes via the CMatrix helpers.
void MvlStepPartStateMachine(void);

// FUN_1003b580 (0.98e @ RVA 0x3b580) - writes the single-byte code patch at VC 0x571820
// (0xC3 / 0xD9) depending on the current model directory flag.
void MvlPatchPartFlagByte(void);

// FUN_1003b5d0 (0.98e @ RVA 0x3b5d0) - part visibility/fade: scales the part matrix by the
// directory-entry fade value and triggers the vtable+0x80 callback under 0.2.
// (Ghidra: 3 params; 1st/2nd in ECX/EDX, 3rd on stack.)
void __fastcall MvlUpdatePartFade(int *part, float *fadeVec, uint32_t callbackArg);

// FUN_1003b840 (0.98e @ RVA 0x3b840) - repaint: pushes the resolved colours from the
// 0x939228 game table (or the directory-entry override when color1 == 0xFF)
// into the 4 part groups of the model directory entry.
// (Ghidra: 4 params; 1st/2nd in ECX/EDX, 3rd/4th on stack.)
void __fastcall MvlApplyRepaintColors(int32_t entity, int32_t color1, int32_t color2,
                             int32_t liveryEntity);

// FUN_1003b9d0 (0.98e @ RVA 0x3b9d0) - randomizes the two paint bytes of the entity at
// DAT_1013dc50 into DAT_1013a5a8 / DAT_1013c1fc (skips when a directory
// override is active; 8 attempts).
void MvlRandomizePaint(void);

// FUN_1003bae0 (0.98e @ RVA 0x3bae0) - advances the paint-variant index of the entity at
// DAT_1013c568 (round-robin over the directory entry paint arrays).
void MvlAdvancePaintVariant(void);

// FUN_1003de20 (0.98e @ RVA 0x3de20) - formats "vc%u" (GetTickCount) into DAT_1013c290 and
// returns that buffer (original returned &DAT_1013c290).
char *MvlFormatVcName(void);

// FUN_1004f7e0 (0.98e @ RVA 0x4f7e0) - "set AI class": either flags the class in the
// DAT_100b2804 slot table (bypass classes 0x82..0xEC gated by
// DAT_100b641e[]) or appends it to the per-slot 0x61-entry list at
// DAT_100b51d8[]. (Ghidra: 2 params in ECX/EDX.)
void __fastcall MvlSetAiClass(int32_t modelId, int32_t slot);

// FUN_10017820 (0.98e @ RVA 0x17820) - DRIVER: seeds the old VC limits then runs the
// CModelIndexLimit relocation chain (streaming / model-pointer / tex / col /
// anim buffers), installs the col/anim code patches (FUN_10017940), the
// directory-manager hooks (FUN_100180a0, dirmgr.h) and rebuilds the anim
// pool (FUN_10017290). Reads the configured counts DAT_100b17bc / e0 / e4.
void MvlRunModelIndexLimitChain(void);
