// engine_globals.h - ONE canonical declaration per MVL 0.98e plugin .data/.bss
// global shared across clone TUs (fixes port-notes-helpers.md Q1 fragmentation:
// the same global previously existed as file-static copies in game_hooks.cpp /
// model_helpers.cpp / model_id.cpp / dirmgr.cpp).
//
// Binary: vehmod_retail.dll 0.98e, image base 0x10000000 -> DLL VA 0x100xxxxx
// is the plugin's OWN .data/.bss (RVA = VA - 0x10000000). Every declaration
// carries the old Ghidra DAT_ name (grep-able) + "0.98e @ RVA 0x..." + the
// semantic Mvl* name derived from the ported usage. Storage lives in
// engine_globals.cpp (single TU); all other TUs include this header.
//
// Confidence + shared-by census: .harness-memory/limout/globals-map.md.
// VC 1.0 GAME addresses (0x00xxxxxx) are NOT here - they are accessed via
// VcFunc/address constants in the ported TUs (see game-side plugin-sdk names
// in globals-map.md).
#pragma once

#include <stdint.h>

// ===========================================================================
// 1. Model-ID / streaming subsystem
//    (model_id.cpp, model_helpers.cpp, dirmgr.cpp)
// ===========================================================================

// DAT_100a6314 - 0.98e @ RVA 0xa6314 - model-lookup registry head: base of the
// model-pointer array indexed by MVL model id (RegisterModelForLookup writes
// registry[modelIndex] = modelPtr; vehicle archive seam writes info pointers).
extern uint32_t *MvlModelLookupRegistry;

// DAT_100a6318 - 0.98e @ RVA 0xa6318 - COL-end ID boundary (port-notes-limits);
// also read as the OLD anim-start index by CaptureVcLimits and as the
// streaming-buffer element count by ReplaceStreamingBufferRefs / FUN_10017290.
extern int32_t MvlColIdBoundary;

// DAT_100a6320 - 0.98e @ RVA 0xa6320 - model-end ID boundary (= old texture
// start index; mvl_settings curModelLimit mirror).
extern int32_t MvlModelIdBoundary;

// DAT_100a6328 - 0.98e @ RVA 0xa6328 - TXD-end ID boundary (= old collision
// start index; mvl_settings curTxdLimit mirror).
extern int32_t MvlTxdIdBoundary;

// DAT_100a631c - 0.98e @ RVA 0xa631c - streaming model-info buffer base
// (old/new; ReplaceStreamingBufferRefs repoints it + code-site refs).
extern uint8_t *MvlStreamingBufferBase;

// DAT_100b0cd8 - 0.98e @ RVA 0xb0cd8 - MVL-owned streaming buffer (realloc'd).
extern void *MvlStreamingBuffer;

// DAT_100b0ce0 - 0.98e @ RVA 0xb0ce0 - MvlStreamingBuffer capacity (bytes/4).
extern int32_t MvlStreamingBufferCapacity;

// DAT_100b0cdc - 0.98e @ RVA 0xb0cdc - MVL-owned model-pointer buffer.
extern void *MvlModelPointerBuffer;

// DAT_100b0ce4 - 0.98e @ RVA 0xb0ce4 - MvlModelPointerBuffer capacity.
extern int32_t MvlModelPointerBufferCapacity;

// DAT_100b0ce8 - 0.98e @ RVA 0xb0ce8 - saved new texture pool pointer
// (ReplaceTexStartIndexRefs swaps it via VC 0x581010/0x580FF0).
extern uint32_t *MvlSavedTexturePool;

// DAT_100a6d48 - 0.98e @ RVA 0xa6d48 - model-index base offset subtracted by
// MvlGetModelDirRecord (directory-entry lookup by model index).
extern int32_t MvlModelIndexBaseOffset;

// DAT_100b0c41 - 0.98e @ RVA 0xb0c41 - experimental long-model-ID mode flag
// (MvlSetLongModelIdsEnabled; mvl_settings longModelIdMode mirror; gates the
// CLongModelId family + FUN_10017290 anim-pool grow).
extern uint8_t MvlLongModelIdsEnabled;

// DAT_100b17e9 - 0.98e @ RVA 0xb17e9 - paranoid model-name checks flag
// (VerifyModelHasColModel name mismatch warning gate).
extern uint8_t MvlParanoidModelNameChecks;

// DAT_100b0c42 - 0.98e @ RVA 0xb0c42 - col/anim code-patch installed flag
// (FUN_10017940 one-shot guard).
extern uint8_t MvlColAnimPatchesInstalled;

// DAT_100b0c43 - 0.98e @ RVA 0xb0c43 - directory-hook installer-done flag
// (FUN_100180a0 one-shot guard).
extern uint8_t MvlDirHooksInstalled;

// DAT_100b0cd0 - 0.98e @ RVA 0xb0cd0 - anim pool buffer (FUN_10017290 grow).
extern void *MvlAnimPoolBuffer;

// DAT_100b0cd4 - 0.98e @ RVA 0xb0cd4 - anim pool capacity (entries).
extern int32_t MvlAnimPoolCapacity;

// DAT_10124f14 - 0.98e @ RVA 0x124f14 - model index being created (dirmgr
// scratch written by the VC directory hooks FUN_10017DB0..0x10018060).
extern uint32_t MvlCreatingModelIndex;

// DAT_10124f24 - 0.98e @ RVA 0x124f24 - model name being created (24 bytes).
extern char MvlCreatingModelName[24];

// DAT_10124f30 - 0.98e @ RVA 0x124f30 - ID-less model directory scratch
// (16 bytes; +4 = capacity, +8 = used; sizes provisional).
extern uint8_t MvlCreatingModelDirectory[16];

// ===========================================================================
// 2. Configured limits (settings-parsed, engine-consumed)
//    (model_helpers.cpp driver, mvl_settings.cpp mirrors)
// ===========================================================================

// DAT_100b17bc - 0.98e @ RVA 0xb17bc - configured <limits> colfiles count
// (SMvlLimitsIds::colfiles mirror; driver FUN_10017820 colCount).
extern int32_t MvlConfiguredColFileLimit;

// DAT_100b17e0 - 0.98e @ RVA 0xb17e0 - configured <limits> texdicts count
// (SMvlLimitsIds::texdicts mirror; driver texCount).
extern int32_t MvlConfiguredTexDictLimit;

// DAT_100b17e4 - 0.98e @ RVA 0xb17e4 - configured <limits> models count
// (SMvlLimitsIds::models mirror; driver modelCount).
extern int32_t MvlConfiguredModelLimit;

// DAT_100b178f - 0.98e @ RVA 0xb178f - ID-limits dirty flag (a model/TXD/COL
// limit was requested; mvl_settings idLimitsDirty mirror).
extern uint8_t MvlIdLimitsDirty;

// ===========================================================================
// 3. Hook-handler entity / colour / paint scratch (game_hooks.cpp handlers
//    feed these into the ported helpers)
// ===========================================================================

// DAT_100b2450 - 0.98e @ RVA 0xb2450 - active-entity temp shared by handlers
// 0x3e230/250/270 around FUN_1003af20; also passed as repaint context.
extern int32_t MvlActiveEntityTemp;

// DAT_1013dc38 - 0.98e @ RVA 0x13dc38 - last key/class id (handler 0x3e010).
extern uint32_t MvlLastKeyClassParam;

// DAT_1013c598 - 0.98e @ RVA 0x13c598 - AI-class set: model id (handler 0x3e0d0).
extern int32_t MvlSetAiClassModelId;

// DAT_1013dbc8 - 0.98e @ RVA 0x13dbc8 - AI-class set: slot (handler 0x3e0d0).
extern int32_t MvlSetAiClassSlot;

// DAT_1013c268 - 0.98e @ RVA 0x13c268 - entity whose parts block (+0x1D4)
// handler 0x3e120 clears.
extern int32_t MvlClearPartsEntity;

// DAT_1013dc2c - 0.98e @ RVA 0x13dc2c - entity passed to FUN_1003a810
// (parts teardown; handler 0x3e140).
extern int32_t MvlPartsTeardownEntity;

// DAT_1013dbdc - 0.98e @ RVA 0x13dbdc - repaint colour index 1 (handler 0x3e170).
extern uint32_t MvlRepaintColor1;

// DAT_1013c280 - 0.98e @ RVA 0x13c280 - repaint colour index 2 (handler 0x3e170).
extern uint32_t MvlRepaintColor2;

// DAT_1013c558 - 0.98e @ RVA 0x13c558 - repaint entity (handler 0x3e170).
extern int32_t MvlRepaintEntity;

// DAT_1013a5a8 - 0.98e @ RVA 0x13a5a8 - random-paint byte-1 output buffer
// (handler 0x3e1d0 -> FUN_1003b9d0; game-side char*).
extern char *MvlPaintRandomByte1Ptr;

// DAT_1013c1fc - 0.98e @ RVA 0x13c1fc - random-paint byte-2 output buffer.
extern char *MvlPaintRandomByte2Ptr;

// DAT_1013dc50 - 0.98e @ RVA 0x13dc50 - entity for FUN_1003b9d0 (handler 0x3e1d0).
extern int32_t MvlPaintRandomEntity;

// DAT_1013a5a4 - 0.98e @ RVA 0x13a5a4 - paint-variant byte-1 output buffer
// (handler 0x3e200 -> FUN_1003bae0).
extern char *MvlPaintVariantByte1Ptr;

// DAT_1013db88 - 0.98e @ RVA 0x13db88 - paint-variant byte-2 output buffer.
extern char *MvlPaintVariantByte2Ptr;

// DAT_1013c568 - 0.98e @ RVA 0x13c568 - entity for FUN_1003bae0 (handler 0x3e200).
extern int32_t MvlPaintVariantEntity;

// DAT_1013c55c - 0.98e @ RVA 0x13c55c - entity for FUN_1003a450/0x3b580
// (handler 0x3e290).
extern int32_t MvlWheelCacheEntity;

// DAT_1013c540 - 0.98e @ RVA 0x13c540 - entity for FUN_1003b580 (handler 0x3e2b0).
extern int32_t MvlPartFlagEntity;

// DAT_1013a594 - 0.98e @ RVA 0x13a594 - part object* for FUN_1003b5d0
// (handler 0x3e2d0, stack +0x1B8).
extern int32_t *MvlFadePartArg;

// DAT_1013dc04 - 0.98e @ RVA 0x13dc04 - callback arg for FUN_1003b5d0
// (handler 0x3e2d0, stack +0x1B4).
extern uint32_t MvlFadeStackArg;

// DAT_1013a5f8 - 0.98e @ RVA 0x13a5f8 - fade vector* (handler 0x3e2d0, EBP).
extern float *MvlFadeVecArg;

// DAT_1013dc88 - 0.98e @ RVA 0x13dc88 - part object* alt (handler 0x3e320,
// stack +0x40).
extern int32_t *MvlFadePartArgAlt;

// DAT_1013c50c - 0.98e @ RVA 0x13c50c - callback arg alt (handler 0x3e320,
// *(param_1+4)).
extern uint32_t MvlFadeCallbackArgAlt;

// DAT_1013c594 - 0.98e @ RVA 0x13c594 - fade scratch vector* (handler 0x3e320
// local frame).
extern float *MvlFadeScratchVec;

// DAT_1013a59c - 0.98e @ RVA 0x13a59c - attach-fixup entity (handler 0x43760
// -> FUN_1001b610).
extern void *MvlAttachFixupEntity;

// DAT_1013dbc0 - 0.98e @ RVA 0x13dbc0 - generated "vc%u" name ptr (handler
// 0x43730 stores FUN_1003de20's buffer ptr here).
extern uint32_t MvlGeneratedVcNamePtr;

// DAT_1013c290 - 0.98e @ RVA 0x13c290 - "vc%u" name buffer (FUN_1003de20 dest).
extern char MvlGeneratedVcNameBuffer[32];

// DAT_100b0dac - 0.98e @ RVA 0xb0dac - last hook continuation address
// (SetContinuation breadcrumb written before every continuation call).
extern uint32_t MvlLastHookContinuation;

// ===========================================================================
// 4. Frame-wait throttle (handler FUN_10043780, VC site 0x60048c)
// ===========================================================================

// DAT_100b17dc - 0.98e @ RVA 0xb17dc - timing constant subtracted before
// MsgWaitForMultipleObjects.
extern uint32_t MvlFrameWaitTimingConstant;

// DAT_100b1791 - 0.98e @ RVA 0xb1791 - conditional 0x60048c hook toggle
// (MvlInstallStubHooks gate).
extern uint8_t MvlFrameWaitHookEnabled;

// DAT_1013dca0 - 0.98e @ RVA 0x13dca0 - FUN_10043780 x87 ST(0) save.
extern float MvlFrameWaitTimeNow;

// DAT_1013a5bc - 0.98e @ RVA 0x13a5bc - FUN_10043780 x87 ST(1) save.
extern float MvlFrameWaitTimePrev;

// DAT_1013dbf4 - 0.98e @ RVA 0x13dbf4 - computed wait milliseconds.
extern int32_t MvlFrameWaitRemainingMs;

// ===========================================================================
// 5. Key/class tables + livery colours (model_helpers.cpp)
// ===========================================================================

// DAT_100b641e - 0.98e @ RVA 0xb641e - bypass class table; the 0.98e write
// paths range-check ids to [0x82, 0xEF] (handler 0x3e010 read-side stops at
// 0xEC; the InitVehicles default-car clear writes up to 0xEF).
// BUGFIX(stable-1.0): sized [0xF0] (was [0xED]) to cover the full 0.98e write
// range - see engine_globals.cpp.
extern uint8_t MvlKeyClassBypassTable[0xF0];

// DAT_100b2804 - 0.98e @ RVA 0xb2804 - per-slot AI class table (FUN_1004f7e0;
// size provisional 16).
extern int32_t MvlAiClassSlotTable[16];

// DAT_100b51d8 - 0.98e @ RVA 0xb51d8 - per-slot AI class list, 0x61 entries
// per slot (FUN_1004f7e0; slot count provisional 16).
extern int32_t MvlAiClassSlotList[16 * 0x61];

// DAT_100b6498 - 0.98e @ RVA 0xb6498 - primary livery colour (FUN_1003b840).
extern int32_t MvlPrimaryLiveryColor;

// DAT_100b649c - 0.98e @ RVA 0xb649c - alternate livery colour (FUN_1003b840).
extern int32_t MvlAlternateLiveryColor;

// DAT_100b263b - 0.98e @ RVA 0xb263b - flag toggling the VC 0x571820 byte
// (FUN_1003b580 patch value 0xC3/0xD9).
extern uint8_t MvlPartFlagDisable;

// DAT_100b1a1c - 0.98e @ RVA 0xb1a1c - skip-parts-free flag (FUN_1003a810).
extern uint8_t MvlSkipPartsFreeFlag;

// DAT_100b1793 - 0.98e @ RVA 0xb1793 - wheel-cache feature flag (FUN_1003a350).
extern uint8_t MvlWheelCacheFeatureFlag;

// DAT_100b0c3c - 0.98e @ RVA 0xb0c3c - wheel-cache generation stamp
// (FUN_1003a450 staleness compare).
extern uint32_t MvlWheelCacheGenerationStamp;

// ===========================================================================
// 6. Settings .data arrays (config lane mirrors; engine-patch lane consumes)
//    Declared here for completeness - the clone currently stores parsed
//    values in CMvlSettings::m_data (see globals-map.md); the raw-array
//    storage is NOT yet needed, so these are comments-only unless wired.
// ===========================================================================
// DAT_100b1794 - 0.98e @ RVA 0xb1794 - <limits><instances> 10 ints
//   (SMvlLimitsInstances mirror)          -> m_data.instances
// DAT_100b17c0 - 0.98e @ RVA 0xb17c0 - <limits><modelstore> 7 ints
//   (SMvlLimitsModelStore mirror)         -> m_data.modelstore
// DAT_100a6a8c - 0.98e @ RVA 0xa6a8c - streamingmem MB (-1 unset)
//   (SMvlLimitsStreamingMem mirror)       -> m_data.streamingmem
// DAT_100ae6a0/0x100ae70c/0x100ae6e4 - engine pool size/min/max pointer
//   tables (ProcessPoolLimits seam)       -> HOOK POINT, not yet storage
// DAT_100ae940/0x100ae76c/0x100ae750 - modelstore min/elem-size/ctor tables
//   (ProcessModelStoreLimits seam)        -> HOOK POINT, not yet storage

// ===========================================================================
// 7. Lane-replaced globals (original .data now modelled as class statics).
//    NO storage here - clone equivalents listed in globals-map.md. Listed
//    so future ports can grep this header; do not take their address until
//    a lane migrates them.
// ===========================================================================
// DAT_101244d0 / DAT_101244c4 - save slot maps (pool/garage)
//   -> CVehicleHandler::s_poolSlots / s_garageSlots (vehicle_handler.cpp)
// DAT_101247b4/b8/c0/c4 - save identity scratch (hash/name/parts/slot)
//   -> CVehicleHandler::s_nLastArchiveHash / s_szLastArchiveName /
//      s_ainLastParts / s_nLastSlotIndex
// DAT_101247e4/e8/ec - weapon-save identity scratch
//   -> CWeaponHandler statics (weapon_handler.cpp)
// DAT_10117820..0x101179b0 - weapon registry + player-save slots
//   -> CWeaponHandler::s_weapons / s_anPlayerSaveWeapon / s_anPlayerSaveAmmo
// DAT_1011c210 - original std::map name registry
//   -> model_id.cpp hash registry (512 buckets)
// DAT_10124500..0x10124504 - pickup spawn MSVC vector
//   -> weapon_pickups.cpp lane static
// DAT_100b3008 - garage record store (0x50 records)
//   -> vehicle_handler.cpp seam (MvlGarageSlotRecord)
// DAT_1013dd20/dd0c/dcf8/dce4/dcd0 - save-apply temps
//   -> MvlSaveApply* accessors (vehicle_handler.h TODO stubs)
// DAT_1013dd18/dd1c/dcec/dcfc - save-apply block (FUN_100518b0/51900)
//   -> not yet ported (save-apply lane)
// DAT_100b77d8 - SCM next-text buffer (200 wchar_t)
//   -> CVehicleSCM::s_nextText (vehicle_scm.cpp)
// DAT_100b77d0/0x100b3f08/0x100b3f09/0x100b51d0 - archive loading context
//   -> vehicle_archive.cpp hook points (MvlSetLoadingContext etc.)
// DAT_100af590 - SCM hook-stub table (19 entries, 0x4c bytes)
//   -> vehicle_scm.cpp ProcessHookStubTable seam
// DAT_100a6b98 - pickup object base (0x34-byte records)
//   -> weapon_pickups.cpp MvlPickupRecord seam
// DAT_100af138 - PTR_ basic-element bind table (12 entries)
//   -> vehicle_xml.cpp LoadXMLBasic explicit dispatch (table not stored)
// DAT_1008d2a8 - weapon name-row table
//   -> weapon_handler.cpp DumpWeaponXML seam
// DAT_1013dd28 / DAT_1008c928 - wide-char compare operands (FUN_10051dd0)
//   -> not yet ported

// ===========================================================================
// 8. Float/.rdata constants (declared, NOT defined here - values live in the
//    binary .data and were never dumped; lanes keep hardcoded TODO values.
//    Do NOT reference these until a binary .data extraction defines them.)
// ===========================================================================
// extern const float MvlAccelScaleConst;      // DAT_1008ddfc - accel scale (assumed 1.0f)
// extern const float MvlBasicDefaultFloat;    // DAT_1008de28 - 0x2838 default (assumed 1.0f)
// extern const float MvlSaBasicDefaultFloat;  // DAT_1008de40 - SA 0x2838 default (unknown)
// extern const float MvlDegToRadNumerator;    // DAT_1008de7c - deg->rad numerator (pi)
// extern const float MvlDegOneEighty;         // DAT_1008df08 - deg->rad denominator (180)
// extern const float MvlAnimFrameScale;       // DAT_1008dec0 - anim frame scale (assumed 30)
// extern const float MvlAccuracyMaxDeg;       // DAT_1008ded4 - machinegun clamp (assumed 45)
// extern const float MvlAudioSkipDistance;    // DAT_1008df04 - audio skip (== 120.0f)
// extern const float MvlPickupMinZ;           // DAT_1008dfc0 - pickup Z floor (unknown)
// extern const float MvlPickupSpawnBump;      // DAT_1008de00 - pickup spawn bump (unknown)
// extern const double MvlIntToDoubleFix;      // DAT_1008e010 - int->double neg-fix (2^32)
