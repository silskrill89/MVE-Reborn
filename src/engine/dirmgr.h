// dirmgr.h - MVL 0.98e directory-manager functions in the VA range
// 0x10017D00..0x10018100 (writers/users of the model-ID scratch globals
// DAT_10124f14 / DAT_10124f24 / DAT_10124f30).
//
// Source: .harness-memory/limout/dirmgr/range-functions.txt + range-decomp.txt
// (3 functions; semantic Mvl* names + "FUN_<VA> @ RVA" cites - renames-engine.md).
//
// The scratch globals themselves are declared extern in model_id.h and
// defined in model_id.cpp - referenced here, never redefined.
#pragma once

#include <stdint.h>

// FUN_10017f60 (0.98e @ RVA 0x17f60, 141 bytes) - verifies the model being created
// (DAT_10124f14 / DAT_10124f24) is registered, has the expected name and a
// colmodel (field +0x1C). Returns 1 in AL on success, 0 on failure.
// Original class context: "CModelIndexLimit::VerifyModelHasColModel".
uint32_t MvlVerifyModelHasColModel(void);

// FUN_10018030 (0.98e @ RVA 0x18030, 44 bytes) - verifies the ID-less model directory
// (DAT_10124f30 +4 capacity, +8 used) can accept one more entry. Returns
// 1 in AL on success, 0 on failure.
// Original class context: "CModelIndexLimit::VerifyDirectoryCanAdd".
uint32_t MvlVerifyDirectoryCanAdd(void);

// FUN_100180a0 (0.98e @ RVA 0x180a0, 156 bytes) - one-shot installer of the 9 directory
// hook sites (VC exe addresses -> plugin handlers at RVAs 0x17DB0..0x18060),
// guarded by the plugin flag DAT_100b0c43.
void MvlInstallDirHooks(void);
