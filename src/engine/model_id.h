// model_id.h - CLongModelId / CModelIndexLimit: long-model-ID support for the
// GTA Vice City 1.0 model-index tables (MVL 0.98e subsystem).
//
// Ported 1:1 from the Ghidra decompilation of vehmod_retail.dll 0.98e
// (.harness-memory/limout/decomp-chain.txt, depth=0 sections 0x17170..0x18030).
// Every function carries its original RVA. Game-side VC 1.0 addresses and
// plugin-side .data globals are documented at their use sites; see also
// .harness-memory/limout/port-notes-engine.md.
//
// Invariants (original behavior):
//   * Every code-site rewrite requires the VC .text range
//     (0x401000 .. 0x401000+0x27CE00) to be writable: call MvlUnlockGameText()
//     (game_hooks.h) once first.
//   * The CLongModelId rewrites run only when the long-model-id feature is
//     enabled (plugin flag DAT_100b0c41); use MvlSetLongModelIdsEnabled().
#pragma once

#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// Shared scratch written by the VC streaming / directory hook handlers:
// MvlCreatingModelIndex / MvlCreatingModelName / MvlCreatingModelDirectory
// (originals DAT_10124f14 / DAT_10124f24 / DAT_10124f30). Single storage now
// lives in engine_globals.cpp; declared there with the other shared globals.
// ---------------------------------------------------------------------------
#include "engine_globals.h"

// ---------------------------------------------------------------------------
// CLongModelId  (0.98e @ RVA 0x17170 .. 0x17250)
//
// Rewrites opcode bytes at pre-scanned VC 1.0 code sites so 32-bit model IDs /
// entity levels / txd indices are allowed where the game used 16-bit forms.
// The site lists are the plugin .data arrays dumped from the original binary
// (see model_id.cpp, section "generated site tables").
// ---------------------------------------------------------------------------
class CLongModelId
{
public:
	// 0.98e @ RVA 0x17170 - 27 sites; VC byte 0x5E ('^') -> 0x56 ('V').
	static void RelocateEntityLevel(void);

	// 0.98e @ RVA 0x171a0 - 27 sites; VC byte 0x5F ('_') -> 0x57 ('W').
	static void RelocateEntityZone(void);

	// 0.98e @ RVA 0x171d0 - 664 sites; VC word 0BF0Fh (MOVSX r32,r/m16) ->
	// 8B90h (NOP + MOV r32,r/m32), same instruction length. Skips site
	// 0x5B8520 (VC 1.0 exe address).
	static void ExtendModelIdMovsx(void);

	// 0.98e @ RVA 0x17220 - 7 sites; VC byte 0x66 (operand-size prefix) -> 0x90 (NOP).
	static void ExtendModelIdOr(void);

	// 0.98e @ RVA 0x17250 - 29 sites; VC word 0BF0Fh (MOVSX) -> 0B70Fh (MOVZX).
	static void ExtendTxdIndexMovsx(void);
};

// Enables / disables the CLongModelId rewrite family (plugin flag DAT_100b0c41,
// 0.98e @ RVA 0xb0c41; checked by the original via FUN_100173d0 / FUN_10017390).
void MvlSetLongModelIdsEnabled(bool enabled);

// ---------------------------------------------------------------------------
// CModelIndexLimit  (0.98e @ RVA 0x17410 .. 0x18030)
//
// Relocates the VC streaming buffers (model-info, model-pointer, texture, col
// and anim start-index tables) to MVL-owned buffers with extended capacity and
// rewrites every code-site reference to the old buffers. Site lists are dumped
// from the original plugin .data arrays (see model_id.cpp).
// ---------------------------------------------------------------------------
class CModelIndexLimit
{
public:
	// 0.98e @ RVA 0x17410 - reallocates the streaming model-info buffer to
	// (maxModelId*5 + 0x23)*4 bytes, repoints code sites in
	// [oldBase, oldBase+0x26D2C).
	static void ReplaceStreamingBufferRefs(int maxModelId);

	// 0.98e @ RVA 0x17510 - reallocates the model-pointer buffer to
	// maxModelId*4 + 0x1C bytes, repoints code sites in [oldBase, oldBase+26000).
	static void ReplaceModelPointerBufferRefs(int maxModelId);

	// 0.98e @ RVA 0x175f0 - repoints texture start-index references
	// (old -> param_1, counts -> param_2) and rebuilds the texture pool via
	// VC functions 0x581010 / 0x580FF0.
	static void ReplaceTexStartIndexRefs(int texStart, uint32_t texCount);

	// 0.98e @ RVA 0x17720 - repoints collision start-index references and the
	// VC code immediates at 0x62A01B / 0x62A309 / 0x62AA50 / 0x62AA52 / 0x62AA53.
	static void ReplaceColStartIndexRefs(int colStart, uint32_t colCount);

	// 0.98e @ RVA 0x177c0 - repoints animation start-index references.
	static void ReplaceAnimStartIndexRefs(int animStart);

	// 0.98e @ RVA 0x179e0 - stores the model-info pointer at modelIndex and
	// records the lowercased model name in the lookup registry (duplicate
	// detection when plugin flag DAT_100b17e9 is set).
	static void RegisterModelForLookup(uint32_t modelIndex, int modelPtr);

	// 0.98e @ RVA 0x17f60 - verifies the model being created (DAT_10124f14 /
	// DAT_10124f24) is registered, has the expected name and a colmodel
	// (field +0x1C). Returns 1 in AL on success, 0 on failure (original EAX
	// semantics preserved).
	static uint32_t VerifyModelHasColModel(void);

	// 0.98e @ RVA 0x18030 - verifies the ID-less model directory
	// (DAT_10124f30) can accept one more entry. Returns 1 in AL on success.
	static uint32_t VerifyDirectoryCanAdd(void);

	// Seeds the cached "old" VC limits from the VC exe code immediates.
	// Mirror of FUN_10018180 (0.98e @ RVA 0x18180), which the 0.98e init chain
	// calls before the Replace* family; required for the clone to know the
	// original buffer bases. Not a requested port target - infrastructure.
	static void CaptureVcLimits(void);
};
