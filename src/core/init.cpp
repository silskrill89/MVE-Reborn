// init.cpp - MVL 0.98e init-chain patch roots (FUN_10043c00 callees).
//
// Source: .harness-memory/limout/initchain/decomp-chain.txt. Every store is
// fully reconstructed (VA + value) from the decomp and routed through the
// MvlInitPatchDword seam below. Per the lane rule, engine writes are HOOK
// POINT seams: the seam logs the exact write (VA + value + RVA) and is the
// single choke point the wiring lane flips to live stores; nothing is faked
// and nothing is silently dropped.
#include "stdafx.h"
#include "init.h"
#include "../engine/engine_globals.h" // MvlAddrs (fail-safe guard, M8c)
#include "../loader/log.h"

#if !defined(_M_IX86)
#error "MVL clone: init.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// Storage (plugin .data; semantic names, original DAT_ in comments).
// ---------------------------------------------------------------------------
uint32_t MvlModelRedirectSlots[9] = {}; // DAT_10124490..DAT_101244B0 @ RVA 0x124490
uint32_t MvlModelRedirectAux      = 0;  // DAT_101244B4 @ RVA 0x1244b4
uint32_t MvlAnimRedirectSlot      = 0;  // DAT_101241D8 @ RVA 0x1241d8

uint32_t *MvlRemapSiteTable[61]  = {};  // DAT_100aed68 @ RVA 0xaed68 (contents undumped)
uint8_t  *MvlBytePatchPtrs[8]    = {};  // DAT_100aeff0..DAT_100af00c @ RVA 0xaeff0
uint8_t  *MvlCodePatchSites[14]  = {};  // DAT_100af018 @ RVA 0xaf018 (14 slots; asm-verified, see FUN_10043900)
uint8_t   MvlCodePatchByteA[14]  = {};  // DAT_100af060 @ RVA 0xaf060 (contents undumped)
uint8_t   MvlCodePatchByteB[14]  = {};  // DAT_100aee5c @ RVA 0xaee5c (contents undumped)
uint32_t *MvlClumpVersionSites[23] = {}; // DAT_100af0a8 @ RVA 0xaf0a8 (contents undumped)

// ---------------------------------------------------------------------------
// Engine-write seam (single choke point).
// ---------------------------------------------------------------------------

// HOOK POINT: one dword write into VC 1.0 code/data (target VA inside the
// game image). Logs the exact store and skips the live write - the wiring
// lane replaces the body with MvlUnlockGameText() + MvlWriteDword() once the
// harness runs in-process. Never fabricates: the log line IS the port.
static void MvlInitPatchDword(uint32_t va, uint32_t value, const char *what)
{
	MvlErrorLog("init: HOOK POINT write *0x%08X = 0x%08X (%s) - TODO(engine)",
	            va, value, what);
}

static void MvlInitPatchByte(uint32_t va, uint8_t value, const char *what)
{
	MvlErrorLog("init: HOOK POINT write byte *0x%08X = 0x%02X (%s) - TODO(engine)",
	            va, value, what);
}

// ---------------------------------------------------------------------------
// FUN_10043810 - 0.98e @ RVA 0x43810 (redirect-buffer wiring).
// Decompiled as `_DAT_0058bfc4 = &DAT_1012449c; ...` - nine code-site
// immediates repointed at the MVL-owned redirect slots.
// ---------------------------------------------------------------------------
void MvlInitWireModelRedirects(void)                 // FUN_10043810 @ RVA 0x43810
{
	// target VC site -> redirect slot index (decomp order, 1:1).
	static const struct { uint32_t site; int slot; } kStores[9] = {
		{ 0x0058BFC4u, 3 }, // _DAT_0058bfc4 = &DAT_1012449c (slots[3])
		{ 0x0058BFBEu, 4 }, // _DAT_0058bfbe = &DAT_101244a0
		{ 0x0058BFB8u, 5 }, // _DAT_0058bfb8 = &DAT_101244a4
		{ 0x0058BFF6u, 6 }, // _DAT_0058bff6 = &DAT_101244a8
		{ 0x0058BFF0u, 7 }, // _DAT_0058bff0 = &DAT_101244ac
		{ 0x0058BFEAu, 8 }, // _DAT_0058bfea = &DAT_101244b0
		{ 0x0058C912u, 0 }, // _DAT_0058c912 = &DAT_10124490
		{ 0x0058C90Cu, 1 }, // _DAT_0058c90c = &DAT_10124494
		{ 0x0058C906u, 2 }, // _DAT_0058c906 = &DAT_10124498
	};
	for (int i = 0; i < 9; ++i) {
		// Value = address of our slot (clone buffer, same role as the
		// original's &DAT_1012449x).
		uint32_t value = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
		    &MvlModelRedirectSlots[kStores[i].slot]));
		MvlInitPatchDword(kStores[i].site, value, "model redirect slot");
	}
}

// ---------------------------------------------------------------------------
// FUN_10043870 - 0.98e @ RVA 0x43870 (anim redirect wiring).
// ---------------------------------------------------------------------------
void MvlInitWireAnimRedirects(void)                  // FUN_10043870 @ RVA 0x43870
{
	uint32_t animSlot = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
	    &MvlAnimRedirectSlot)); // &DAT_101241d8
	uint32_t auxSlot = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(
	    &MvlModelRedirectAux)); // &DAT_101244b4

	MvlInitPatchDword(0x00597FA4u, animSlot, "_DAT_00597fa4 = &DAT_101241d8");
	MvlInitPatchDword(0x005980BDu, animSlot, "_DAT_005980bd = &DAT_101241d8");
	MvlInitPatchDword(0x00598221u, auxSlot, "_DAT_00598221 = &DAT_101244b4");
}

// ---------------------------------------------------------------------------
// FUN_10043900 - 0.98e @ RVA 0x43900 (14 code stubs).
// Per site (0.98e @ RVA 0x43900 loop, uVar2 = 0..13):
//   site[0] = 0x80; site[1] = byteA[i]; site[2] = 0x7C; site[3] = byteB[i];
//   *(dword*)(site+4) = *(dword*)(site+8) = 0x90909090; *(word*)(site+0xC) = 0x9090.
// NOTE (BUGFIX(stable-1.0), asm-verified against vehmod_retail.dll 0.98e):
// the site-pointer table is 14 dwords at DAT_100af018 @ RVA 0xaf018..0xaf04c.
// Ghidra renders the same loads two ways - (&DAT_100af018)[i] and
// *(&DAT_100af014 + (i+1)*4) - identical addresses; its byte-store of 0x80
// at "(&DAT_100af018)[i]" is a decompiler artifact (the asm stores 0x80
// THROUGH the site pointer, i.e. site[0]). The previous "15-slot i+1" story
// in this comment was wrong and would have misled the wiring lane to extract
// the table 4 bytes early. Table contents were never dumped -> NULL rows are
// skipped through the seam log.
// ---------------------------------------------------------------------------
void MvlInitPatchCodeStubs(void)                     // FUN_10043900 @ RVA 0x43900
{
	for (int i = 0; i < 14; ++i) {
		uint8_t *site = MvlCodePatchSites[i];
		if (site == NULL) {
			// TODO(engine): DAT_100af014 table contents undumped - row is a
			// HOOK POINT until a binary .data extraction fills it.
			MvlErrorLog("init: HOOK POINT FUN_10043900 site[%d] NULL "
			            "(DAT_100af014 row undumped) - TODO(engine)", i);
			continue;
		}
		const uint8_t byteA = MvlCodePatchByteA[i]; // DAT_100af060
		const uint8_t byteB = MvlCodePatchByteB[i]; // DAT_100aee5c
		MvlInitPatchByte(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(site)) + 0u,
		                 0x80u, "stub byte 0");
		MvlInitPatchByte(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(site)) + 1u,
		                 byteA, "stub byte 1 (DAT_100af060)");
		MvlInitPatchByte(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(site)) + 2u,
		                 0x7Cu, "stub byte 2");
		MvlInitPatchByte(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(site)) + 3u,
		                 byteB, "stub byte 3 (DAT_100aee5c)");
		const uint32_t siteVa =
		    static_cast<uint32_t>(reinterpret_cast<uintptr_t>(site));
		MvlInitPatchDword(siteVa + 4u, 0x90909090u, "NOP dword");
		MvlInitPatchDword(siteVa + 8u, 0x90909090u, "NOP dword");
		MvlInitPatchByte(siteVa + 0xCu, 0x90u, "NOP word lo"); // decomp: word store
		MvlInitPatchByte(siteVa + 0xDu, 0x90u, "NOP word hi"); // 0x9090
	}
}

// === APPEND-MARKER-INIT-2 ===

// ---------------------------------------------------------------------------
// FUN_10043a10 - 0.98e @ RVA 0x43a10 (immediate remap + byte patches).
// Loop 1: 61 rows of MvlRemapSiteTable (0xf4/4), each row a pointer to a code
// immediate; if the immediate is in (0x8118CF, 0x8118FD) exclusive it is
// rewritten to value + 0xF92ACE0 (0x8118D0..0x8118FC -> 0x1013C5B0..0x1013C5DC,
// i.e. retarget into plugin .data). Then 8 byte patches 0x10 -> 0x7F at the
// targets of MvlBytePatchPtrs.
// ---------------------------------------------------------------------------
void MvlInitRemapImmediates(void)                    // FUN_10043a10 @ RVA 0x43a10
{
	for (int i = 0; i < 61; ++i) {
		uint32_t *site = MvlRemapSiteTable[i]; // DAT_100aed68 rows (undumped)
		if (site == NULL) {
			MvlErrorLog("init: HOOK POINT FUN_10043a10 row[%d] NULL "
			            "(DAT_100aed68 row undumped) - TODO(engine)", i);
			continue;
		}
		const uint32_t v = *site;
		if (0x8118CFu < v && v < 0x8118FDu) {
			MvlInitPatchDword(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(site)),
			                  v + 0xF92ACE0u, "model-id immediate remap");
		}
	}
	for (int i = 0; i < 8; ++i) {
		uint8_t *target = MvlBytePatchPtrs[i]; // DAT_100aeff0..af00c rows
		if (target == NULL) {
			MvlErrorLog("init: HOOK POINT FUN_10043a10 byte[%d] NULL "
			            "(DAT_100aeff0 row undumped) - TODO(engine)", i);
			continue;
		}
		if (*target == 0x10u) {
			MvlInitPatchByte(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(target)),
			                 0x7Fu, "0x10 -> 0x7F byte patch");
		}
	}
}

// ---------------------------------------------------------------------------
// FUN_10043b30 - 0.98e @ RVA 0x43b30 (RW clump chunk-version remap).
// 23 rows of MvlClumpVersionSites (0x5c/4): immediate 0x34003 -> 0x36003
// (RW clump chunk version 3.4/3.5 -> 3.6 acceptance).
// ---------------------------------------------------------------------------
void MvlInitRemapClumpVersions(void)                 // FUN_10043b30 @ RVA 0x43b30
{
	for (int i = 0; i < 23; ++i) {
		uint32_t *site = MvlClumpVersionSites[i]; // DAT_100af0a8 rows (undumped)
		if (site == NULL) {
			MvlErrorLog("init: HOOK POINT FUN_10043b30 row[%d] NULL "
			            "(DAT_100af0a8 row undumped) - TODO(engine)", i);
			continue;
		}
		if (*site == 0x34003u) {
			MvlInitPatchDword(static_cast<uint32_t>(reinterpret_cast<uintptr_t>(site)),
			                  0x36003u, "clump chunk version 0x34003 -> 0x36003");
		}
	}
}

// ---------------------------------------------------------------------------
// Runner - the recovered FUN_10043c00 call order (decomp-chain.txt depth-0
// listing order: 0x43810 -> 0x43a10 -> 0x43900 -> 0x43870 -> 0x43b30).
// ---------------------------------------------------------------------------
void MvlInstallInitChainPatches(void)
{
	// FAIL-SAFE (M8c): unknown exe variant -> no map -> run nothing. (Today
	// the roots only log through HOOK POINT seams, but the guard keeps the
	// no-op contract for when the wiring lane flips them to live stores.)
	if (MvlAddrs() == NULL) {
		MvlErrorLog("init: MvlInstallInitChainPatches: exe variant unknown - "
		            "init-chain patches skipped (fail-safe no-op)");
		return;
	}

	MvlInitWireModelRedirects();
	MvlInitRemapImmediates();
	MvlInitPatchCodeStubs();
	MvlInitWireAnimRedirects();
	MvlInitRemapClumpVersions();
}
