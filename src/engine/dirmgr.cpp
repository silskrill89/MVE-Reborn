// dirmgr.cpp - ports of the 3 MVL 0.98e directory-manager functions.
//
// Source: .harness-memory/limout/dirmgr/range-decomp.txt (Ghidra decomp of
// vehmod_retail.dll 0.98e). Every function cites its original RVA; FUN_<VA>
// names kept per the binding naming policy.
//
// Porting notes:
//   * FUN_10017f60 / FUN_10018030 are byte-for-byte the bodies already ported
//     as CModelIndexLimit::VerifyModelHasColModel / VerifyDirectoryCanAdd in
//     model_id.cpp (same RVAs, same messages, same AL semantics). They read
//     the shared scratch MvlCreatingModelIndex / MvlCreatingModelName /
//     MvlCreatingModelDirectory (originals DAT_10124f14/f24/f30) and the
//     model-lookup registry MvlModelLookupRegistry (original DAT_100a6314),
//     which lives inside model_id.cpp's CModelIndexLimit methods. To keep ONE
//     implementation and a single owner for that buffer base, these two
//     functions delegate to the already-ported methods instead of duplicating
//     the logic with a second, disconnected copy.
//   * FUN_100180a0 is ported in full, but its 9 E9-JMP installs target plugin
//     handlers at RVAs 0x17DB0..0x18060 whose bodies are absent from the
//     limout dumps; the installs are therefore NOT applied yet (logged once).
#include "../core/stdafx.h"
#include "dirmgr.h"
#include "model_id.h"      // Verify* methods + engine_globals.h via model_id.h
#include "engine_globals.h"
#include "game_hooks.h"
#include "../loader/log.h"

#if !defined(_M_IX86)
#error "MVL clone: dirmgr.cpp is x86-only (VC 1.0 plugin)."
#endif

// 0.98e @ RVA 0x17f60 - CModelIndexLimit::VerifyModelHasColModel.
uint32_t FUN_10017f60(void)
{
	return CModelIndexLimit::VerifyModelHasColModel();
}

// 0.98e @ RVA 0x18030 - CModelIndexLimit::VerifyDirectoryCanAdd.
uint32_t FUN_10018030(void)
{
	return CModelIndexLimit::VerifyDirectoryCanAdd();
}

// 0.98e @ RVA 0x180a0 - directory hook-site installer (guarded, one-shot).
// Guard flag: MvlDirHooksInstalled (original DAT_100b0c43 @ RVA 0xb0c43,
// single storage in engine_globals.cpp).

void FUN_100180a0(void)
{
	if (MvlDirHooksInstalled != '\0')
		return;

	// Original install list (FUN_10022560 = MvlWriteJmp):
	//   VC 0x48BDFC -> plugin 0x10017DB0
	//   VC 0x48BF2C -> plugin 0x10017DF0
	//   VC 0x48CAD3 -> plugin 0x10017E30
	//   VC 0x48CA41 -> plugin 0x10017E70
	//   VC 0x48C4AE -> plugin 0x10017EB0
	//   VC 0x48C69E -> plugin 0x10017EF0
	//   VC 0x55F7D0 -> plugin 0x10017F30
	//   VC 0x48ACB0 -> plugin 0x10017FF0
	//   VC 0x4873F0 -> plugin 0x10018060
	// The 9 handler bodies (0x10017DB0..0x10018060) are not present in the
	// limout range dumps (only 0x17F60 / 0x18030 / 0x180A0 were recovered),
	// so the jumps are NOT installed yet.
	// TODO(helpers): 0x10017db0 unported; TODO(helpers): 0x10017df0 unported;
	// TODO(helpers): 0x10017e30 unported; TODO(helpers): 0x10017e70 unported;
	// TODO(helpers): 0x10017eb0 unported; TODO(helpers): 0x10017ef0 unported;
	// TODO(helpers): 0x10017f30 unported; TODO(helpers): 0x10017ff0 unported;
	// TODO(helpers): 0x10018060 unported.
	MvlErrorLog("helpers: TODO(helpers): FUN_100180a0 - directory hooks not "
	            "installed (handlers 0x10017db0..0x10018060 unported)");

	MvlDirHooksInstalled = '\x01';
}
