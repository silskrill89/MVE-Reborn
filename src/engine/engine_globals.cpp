// engine_globals.cpp - SINGLE storage owner for the shared MVL 0.98e plugin
// .data/.bss globals (see engine_globals.h for declarations + globals-map.md
// for the census). Fixes the per-TU fragmentation documented in
// port-notes-helpers.md Q1: previously game_hooks.cpp / model_helpers.cpp /
// model_id.cpp / dirmgr.cpp each held disconnected file-static copies.
//
// Binary: vehmod_retail.dll 0.98e (image base 0x10000000). Definitions are
// zero-initialized exactly like the original .bss/.data-at-startup; runtime
// writers (hook handlers, CModelIndexLimit::CaptureVcLimits, settings
// processors) assign real values. No fake seeding.
//
// x86-only TU (all consumers are VC 1.0 plugin code).
#include "../core/stdafx.h"
#include "engine_globals.h"

#if !defined(_M_IX86)
#error "MVL clone: engine_globals.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// 1. Model-ID / streaming subsystem.
// ---------------------------------------------------------------------------
uint32_t *MvlModelLookupRegistry      = NULL;    // DAT_100a6314 @ RVA 0xa6314
int32_t    MvlColIdBoundary           = 0;       // DAT_100a6318 @ RVA 0xa6318
int32_t    MvlModelIdBoundary         = 0;       // DAT_100a6320 @ RVA 0xa6320
int32_t    MvlTxdIdBoundary           = 0;       // DAT_100a6328 @ RVA 0xa6328
uint8_t   *MvlStreamingBufferBase     = NULL;    // DAT_100a631c @ RVA 0xa631c
void      *MvlStreamingBuffer         = NULL;    // DAT_100b0cd8 @ RVA 0xb0cd8
int32_t    MvlStreamingBufferCapacity = 0;       // DAT_100b0ce0 @ RVA 0xb0ce0
void      *MvlModelPointerBuffer      = NULL;    // DAT_100b0cdc @ RVA 0xb0cdc
int32_t    MvlModelPointerBufferCapacity = 0;    // DAT_100b0ce4 @ RVA 0xb0ce4
uint32_t  *MvlSavedTexturePool        = NULL;    // DAT_100b0ce8 @ RVA 0xb0ce8
int32_t    MvlModelIndexBaseOffset    = 0;       // DAT_100a6d48 @ RVA 0xa6d48
uint8_t    MvlLongModelIdsEnabled     = 0;       // DAT_100b0c41 @ RVA 0xb0c41
uint8_t    MvlParanoidModelNameChecks = 0;       // DAT_100b17e9 @ RVA 0xb17e9
uint8_t    MvlColAnimPatchesInstalled = 0;       // DAT_100b0c42 @ RVA 0xb0c42
uint8_t    MvlDirHooksInstalled       = 0;       // DAT_100b0c43 @ RVA 0xb0c43
void      *MvlAnimPoolBuffer          = NULL;    // DAT_100b0cd0 @ RVA 0xb0cd0
int32_t    MvlAnimPoolCapacity        = 0;       // DAT_100b0cd4 @ RVA 0xb0cd4
uint32_t   MvlCreatingModelIndex      = 0;       // DAT_10124f14 @ RVA 0x124f14
char       MvlCreatingModelName[24]   = {};      // DAT_10124f24 @ RVA 0x124f24
uint8_t    MvlCreatingModelDirectory[16] = {};   // DAT_10124f30 @ RVA 0x124f30

// ---------------------------------------------------------------------------
// 2. Configured limits.
// ---------------------------------------------------------------------------
int32_t MvlConfiguredColFileLimit = 0; // DAT_100b17bc @ RVA 0xb17bc
int32_t MvlConfiguredTexDictLimit = 0; // DAT_100b17e0 @ RVA 0xb17e0
int32_t MvlConfiguredModelLimit   = 0; // DAT_100b17e4 @ RVA 0xb17e4
uint8_t MvlIdLimitsDirty          = 0; // DAT_100b178f @ RVA 0xb178f

// ---------------------------------------------------------------------------
// 3. Hook-handler entity / colour / paint scratch.
// ---------------------------------------------------------------------------
int32_t MvlActiveEntityTemp       = 0;    // DAT_100b2450 @ RVA 0xb2450
uint32_t MvlLastKeyClassParam     = 0;    // DAT_1013dc38 @ RVA 0x13dc38
int32_t  MvlSetAiClassModelId     = 0;    // DAT_1013c598 @ RVA 0x13c598
int32_t  MvlSetAiClassSlot        = 0;    // DAT_1013dbc8 @ RVA 0x13dbc8
int32_t  MvlClearPartsEntity      = 0;    // DAT_1013c268 @ RVA 0x13c268
int32_t  MvlPartsTeardownEntity   = 0;    // DAT_1013dc2c @ RVA 0x13dc2c
uint32_t MvlRepaintColor1         = 0;    // DAT_1013dbdc @ RVA 0x13dbdc
uint32_t MvlRepaintColor2         = 0;    // DAT_1013c280 @ RVA 0x13c280
int32_t  MvlRepaintEntity         = 0;    // DAT_1013c558 @ RVA 0x13c558
char    *MvlPaintRandomByte1Ptr   = NULL; // DAT_1013a5a8 @ RVA 0x13a5a8
char    *MvlPaintRandomByte2Ptr   = NULL; // DAT_1013c1fc @ RVA 0x13c1fc
int32_t  MvlPaintRandomEntity     = 0;    // DAT_1013dc50 @ RVA 0x13dc50
char    *MvlPaintVariantByte1Ptr  = NULL; // DAT_1013a5a4 @ RVA 0x13a5a4
char    *MvlPaintVariantByte2Ptr  = NULL; // DAT_1013db88 @ RVA 0x13db88
int32_t  MvlPaintVariantEntity    = 0;    // DAT_1013c568 @ RVA 0x13c568
int32_t  MvlWheelCacheEntity      = 0;    // DAT_1013c55c @ RVA 0x13c55c
int32_t  MvlPartFlagEntity        = 0;    // DAT_1013c540 @ RVA 0x13c540
int32_t *MvlFadePartArg           = NULL; // DAT_1013a594 @ RVA 0x13a594
uint32_t MvlFadeStackArg          = 0;    // DAT_1013dc04 @ RVA 0x13dc04
float   *MvlFadeVecArg            = NULL; // DAT_1013a5f8 @ RVA 0x13a5f8
int32_t *MvlFadePartArgAlt        = NULL; // DAT_1013dc88 @ RVA 0x13dc88
uint32_t MvlFadeCallbackArgAlt    = 0;    // DAT_1013c50c @ RVA 0x13c50c
float   *MvlFadeScratchVec        = NULL; // DAT_1013c594 @ RVA 0x13c594
void    *MvlAttachFixupEntity     = NULL; // DAT_1013a59c @ RVA 0x13a59c
uint32_t MvlGeneratedVcNamePtr    = 0;    // DAT_1013dbc0 @ RVA 0x13dbc0
char     MvlGeneratedVcNameBuffer[32] = {}; // DAT_1013c290 @ RVA 0x13c290
uint32_t MvlLastHookContinuation      = 0;   // DAT_100b0dac @ RVA 0xb0dac

// ---------------------------------------------------------------------------
// 4. Frame-wait throttle (FUN_10043780).
// ---------------------------------------------------------------------------
uint32_t MvlFrameWaitTimingConstant = 0;   // DAT_100b17dc @ RVA 0xb17dc
uint8_t  MvlFrameWaitHookEnabled    = 0;   // DAT_100b1791 @ RVA 0xb1791
float    MvlFrameWaitTimeNow        = 0.0f;// DAT_1013dca0 @ RVA 0x13dca0
float    MvlFrameWaitTimePrev       = 0.0f;// DAT_1013a5bc @ RVA 0x13a5bc
int32_t  MvlFrameWaitRemainingMs    = 0;   // DAT_1013dbf4 @ RVA 0x13dbf4

// ---------------------------------------------------------------------------
// 5. Key/class tables + livery colours.
// ---------------------------------------------------------------------------
uint8_t  MvlKeyClassBypassTable[0xED] = {};          // DAT_100b641e @ RVA 0xb641e
int32_t  MvlAiClassSlotTable[16]      = {};          // DAT_100b2804 @ RVA 0xb2804
int32_t  MvlAiClassSlotList[16 * 0x61] = {};         // DAT_100b51d8 @ RVA 0xb51d8
int32_t  MvlPrimaryLiveryColor        = 0;           // DAT_100b6498 @ RVA 0xb6498
int32_t  MvlAlternateLiveryColor      = 0;           // DAT_100b649c @ RVA 0xb649c
uint8_t  MvlPartFlagDisable           = 0;           // DAT_100b263b @ RVA 0xb263b
uint8_t  MvlSkipPartsFreeFlag         = 0;           // DAT_100b1a1c @ RVA 0xb1a1c
uint8_t  MvlWheelCacheFeatureFlag     = 0;           // DAT_100b1793 @ RVA 0xb1793
uint32_t MvlWheelCacheGenerationStamp = 0;           // DAT_100b0c3c @ RVA 0xb0c3c
