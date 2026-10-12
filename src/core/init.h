// init.h - MVL 0.98e init-chain patch functions (depth-0 callees of
// FUN_10043c00, the main init orchestrator).
//
// Source: .harness-memory/limout/initchain/decomp-chain.txt (5 roots).
// Binary: vehmod_retail.dll 0.98e, image base 0x10000000.
// Naming policy: unnamed functions keep their Ghidra FUN_<VA> names; the
// plugin-side data they touch gets semantic Mvl* names with trailing
// `DAT_* @ RVA` comments (AGENTS.md Globals rule).
//
// All five re-own engine/code-site memory. Two families:
//   * direct stores with fully-known VA + value (0x43810 / 0x43870) ->
//     real writes through MvlWriteDword (game_hooks.h) after the one-shot
//     .text unlock, with a trace log (never silently skipped);
//   * table-driven patches whose table CONTENTS were never dumped
//     (0x43900 / 0x43a10 / 0x43b30) -> the loop logic is ported verbatim
//     over zero-filled declared tables; undumped rows are HOOK POINT seams
//     (logged, never faked).
#pragma once

#include <stdint.h>

// ---------------------------------------------------------------------------
// Shared redirect buffers (plugin .data; their ADDRESSES are what the patched
// VC code sites consume).
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x124490 .. 0x1244b0 - nine dwords, contiguous.
extern uint32_t MvlModelRedirectSlots[9];  // DAT_10124490..DAT_101244B0
// 0.98e @ RVA 0x1244b4 - auxiliary redirect slot.
extern uint32_t MvlModelRedirectAux;       // DAT_101244B4
// 0.98e @ RVA 0x1241d8 - anim redirect slot (target of two VC sites).
extern uint32_t MvlAnimRedirectSlot;       // DAT_101241D8

// ---------------------------------------------------------------------------
// Patch tables (contents of these plugin .data arrays were NOT in the dump -
// rows are zero until a binary .data extraction fills them; the loops skip
// zero rows and log the seam).
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0xae6d8 - 61 code-site pointers holding immediates to remap.
extern uint32_t *MvlRemapSiteTable[61];     // DAT_100aed68 (0xf4 bytes)
// 0.98e @ RVA 0xaeff0 .. 0xaf00c - 8 pointers to bytes patched 0x10 -> 0x7F.
extern uint8_t *MvlBytePatchPtrs[8];        // DAT_100aeff0..DAT_100af00c
// 0.98e @ RVA 0xaf014 - 14 code sites built up as
//   site[0]=0x80, site[1]=MvlCodePatchByteA[i], site[2]=0x7C,
//   site[3]=MvlCodePatchByteB[i], site[4..0xD]=NOP (0x90).
extern uint8_t *MvlCodePatchSites[14];      // DAT_100af014 (see Q: +4 oddity)
extern uint8_t  MvlCodePatchByteA[14];      // DAT_100af060
extern uint8_t  MvlCodePatchByteB[14];      // DAT_100aee5c
// 0.98e @ RVA 0xaf0a8 - 23 code-site pointers holding RW chunk versions
// remapped 0x34003 -> 0x36003 (clump 3.4/3.5 -> 3.6 acceptance).
extern uint32_t *MvlClumpVersionSites[23];  // DAT_100af0a8 (0x5c bytes)

// ---------------------------------------------------------------------------
// The five roots (unnamed in RE -> FUN_ names kept 1:1).
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x43810 - writes 9 code-site immediates: VC 0x58BFC4/0x58BFBE/
// 0x58BFB8/0x58BFF6/0x58BFF0/0x58BFEA <- &MvlModelRedirectSlots[3..8] and
// VC 0x58C912/0x58C90C/0x58C906 <- &MvlModelRedirectSlots[0..2].
void FUN_10043810(void);

// 0.98e @ RVA 0x43870 - writes 3 code-site immediates: VC 0x597FA4 and
// 0x5980BD <- &MvlAnimRedirectSlot, VC 0x598221 <- &MvlModelRedirectAux.
void FUN_10043870(void);

// 0.98e @ RVA 0x43900 - builds 14 code stubs (0x80 / byteA / 0x7C / byteB /
// 10x NOP) at the sites in MvlCodePatchSites.
void FUN_10043900(void);

// 0.98e @ RVA 0x43a10 - remaps immediates at the 61 MvlRemapSiteTable sites
// (value in (0x8118CF, 0x8118FD) -> +0xF92ACE0, i.e. 0x8118D0..0x8118FC
// retargeted into plugin .data 0x1013C5B0..) and byte-patches the 8
// MvlBytePatchPtrs targets 0x10 -> 0x7F.
void FUN_10043a10(void);

// 0.98e @ RVA 0x43b30 - remaps RW clump chunk-version immediates at the 23
// MvlClumpVersionSites targets: 0x34003 -> 0x36003.
void FUN_10043b30(void);

// Runs the five roots in the recovered FUN_10043c00 order
// (0x43810 -> 0x43a10 -> 0x43900 -> 0x43870 -> 0x43b30).
void MvlInstallInitChainPatches(void);
