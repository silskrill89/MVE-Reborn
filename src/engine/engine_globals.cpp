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
#include "game_hooks.h" // MvlHookSite + MvlHook* handler decls (retail sites table)
#include "../loader/log.h" // MvlErrorLog (unknown-variant fail-safe log)

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
// BUGFIX(stable-1.0): size [0xED] -> [0xF0]. The original table region spans
// RVA 0xb641e..0xb650e (the InitVehicles port range-checks default car ids to
// [0x82, 0xEF] and writes MvlKeyClassBypassTable[id]); the old 0xED bound
// excluded indices 0xED..0xEF, so that (out-of-scope, vehicle lane) write went
// 1-3 bytes past this array. 0xF0 covers every index the 0.98e code paths use.
uint8_t  MvlKeyClassBypassTable[0xF0] = {};          // DAT_100b641e @ RVA 0xb641e
int32_t  MvlAiClassSlotTable[16]      = {};          // DAT_100b2804 @ RVA 0xb2804
int32_t  MvlAiClassSlotList[16 * 0x61] = {};         // DAT_100b51d8 @ RVA 0xb51d8
int32_t  MvlPrimaryLiveryColor        = 0;           // DAT_100b6498 @ RVA 0xb6498
int32_t  MvlAlternateLiveryColor      = 0;           // DAT_100b649c @ RVA 0xb649c
uint8_t  MvlPartFlagDisable           = 0;           // DAT_100b263b @ RVA 0xb263b
uint8_t  MvlSkipPartsFreeFlag         = 0;           // DAT_100b1a1c @ RVA 0xb1a1c
uint8_t  MvlWheelCacheFeatureFlag     = 0;           // DAT_100b1793 @ RVA 0xb1793
uint32_t MvlWheelCacheGenerationStamp = 0;           // DAT_100b0c3c @ RVA 0xb0c3c

// ---------------------------------------------------------------------------
// 9. Per-variant VC 1.0 exe address map (PLAN.md §10.2 step 1 / M8a).
// ---------------------------------------------------------------------------

// Retail hook-site table - moved verbatim from game_hooks.cpp (was
// g_MvlStubHookSites). (VC 1.0 target -> handler) pairs from FUN_10043c00
// (0.98e @ RVA 0x43c00, main init); order matches the original install order.
static const MvlHookSite kRetailHookSites[] = {
	{ 0x00426748u, reinterpret_cast<uintptr_t>(&MvlHookTrampoline426748), 0 }, // -> continues 0x42674f
	{ 0x004267DEu, reinterpret_cast<uintptr_t>(&MvlHookKeyClassBypass), 0 }, // -> continues 0x4267e5
	{ 0x00426AC6u, reinterpret_cast<uintptr_t>(&MvlHookTrampoline426ac6), 0 }, // -> continues 0x426acd
	{ 0x00426C80u, reinterpret_cast<uintptr_t>(&MvlHookTrampoline426c80), 0 }, // -> continues 0x426c87
	{ 0x00427470u, reinterpret_cast<uintptr_t>(&MvlHookTrampoline427470), 0 }, // -> continues 0x427477
	{ 0x00426820u, reinterpret_cast<uintptr_t>(&MvlHookSetAiClass), 0 }, // -> no continuation
	{ 0x00426A4Cu, reinterpret_cast<uintptr_t>(&MvlHookTrampoline426a4c), 0 }, // -> continues 0x426a53
	{ 0x005BB40Bu, reinterpret_cast<uintptr_t>(&MvlHookClearParts), 0 }, // -> no continuation
	{ 0x005BAC04u, reinterpret_cast<uintptr_t>(&MvlHookTeardownParts), 0 }, // -> continues 0x5bac09
	{ 0x00579290u, reinterpret_cast<uintptr_t>(&MvlHookRepaint), 0 }, // -> continues 0x579295
	{ 0x00579090u, reinterpret_cast<uintptr_t>(&MvlHookPaintRandom), 0 }, // -> no continuation
	{ 0x00579190u, reinterpret_cast<uintptr_t>(&MvlHookPaintVariant), 0 }, // -> no continuation
	{ 0x0069ADC4u, reinterpret_cast<uintptr_t>(&MvlHookPartUpdate69adc4), 1 }, // dword slot
	{ 0x006D7B68u, reinterpret_cast<uintptr_t>(&MvlHookPartUpdate6d7b68), 1 }, // dword slot
	{ 0x0069B0E8u, reinterpret_cast<uintptr_t>(&MvlHookPartUpdate69b0e8), 1 }, // dword slot
	{ 0x0069ADC0u, reinterpret_cast<uintptr_t>(&MvlHookWheelCache69adc0), 1 }, // dword slot
	{ 0x006D7B64u, reinterpret_cast<uintptr_t>(&MvlHookPartFlag6d7b64), 1 }, // dword slot
	{ 0x005CF9F3u, reinterpret_cast<uintptr_t>(&MvlHookFadePart), 0 }, // -> continues 0x5cf9f9
	{ 0x005C4341u, reinterpret_cast<uintptr_t>(&MvlHookFadePartAlt), 0 }, // -> continues 0x5c4348
	{ 0x00408967u, reinterpret_cast<uintptr_t>(&MvlHookGenerateVcName), 0 }, // -> continues 0x40896c
	{ 0x0069ADB0u, reinterpret_cast<uintptr_t>(&MvlHookAttachFixup), 1 }, // dword slot

	// Conditional site not tabled (applied by MvlInstallStubHooks):
	//   VC 0x60048c -> FUN_10043780, guarded by plugin flag MvlFrameWaitHookEnabled.
	// Other FUN_10043c00 pairs target handlers outside this port batch
	// (FUN_1003e380 .. FUN_10043b50); extend this table as they get ported.
};

static_assert(sizeof(kRetailHookSites) / sizeof(kRetailHookSites[0]) == 21,
              "retail hook-site count (FUN_10043c00 pairs)");

// Drift guard: every MvlExeAddrs member is 4 bytes on x86 (uint32_t / pointer),
// so sizeof == field count * 4. If a field is added to the struct without a
// matching kExeAddrsRetail initializer (positional aggregate init would
// silently zero it), this fires. 64 fields today.
static_assert(sizeof(MvlExeAddrs) == 64 * sizeof(uint32_t),
              "MvlExeAddrs field count drifted - update kExeAddrsRetail (positional init)");

// Retail map - every value verbatim from the previous per-TU constexpr tables
// (model_id.cpp / model_helpers.cpp kVC_*) + game_hooks.cpp literals. Field
// order MUST match struct MvlExeAddrs (engine_globals.h).
constexpr MvlExeAddrs kExeAddrsRetail = {
	0x00401000u, // textUnlockBase         (FUN_100225a0 VirtualProtect)
	0x0027CE00u, // textUnlockSize
	0x0055F636u, // modelPtrBaseImm        kVC_ModelPtrBaseImm      0.98e @ RVA 0x15F636
	0x0040AF00u, // streamingBaseImm       kVC_StreamingBaseImm     0.98e @ RVA 0x00AF00
	0x0040C417u, // texStartImm            kVC_TexStartImm          0.98e @ RVA 0x00C417
	0x0040C507u, // colStartImm            kVC_ColStartImm          0.98e @ RVA 0x00C507
	0x0040C582u, // animStartImm           kVC_AnimStartImm         0.98e @ RVA 0x00C582
	0x004102B2u, // streamingCapacity      kVC_StreamingCapacity    0.98e @ RVA 0x0102B2
	0x0040BD2Du, // texStartRef1           kVC_TexStartRef1         0.98e @ RVA 0x00BD2D
	0x00410651u, // texStartRef2           kVC_TexStartRef2         0.98e @ RVA 0x010651
	0x0041065Fu, // texCountRef1           kVC_TexCountRef1         0.98e @ RVA 0x01065F
	0x0055FB1Cu, // texStartRef3           kVC_TexStartRef3         0.98e @ RVA 0x15FB1C
	0x00580FD6u, // texCountRef2           kVC_TexCountRef2         0.98e @ RVA 0x180FD6
	0x0058102Du, // texCountRef3           kVC_TexCountRef3         0.98e @ RVA 0x18102D
	0x009B6A4Cu, // texPoolGlobal          kVC_TexPoolGlobal        0.98e @ RVA 0x5B6A4C (VC .data ptr)
	0x00581010u, // texPoolRebuild1        kVC_TexPoolRebuild1      0.98e @ RVA 0x181010 (VC fn)
	0x00580FF0u, // texPoolRebuild2        kVC_TexPoolRebuild2      0.98e @ RVA 0x180FF0 (VC fn)
	0x0062A01Bu, // colStartRef1           kVC_ColStartRef1         0.98e @ RVA 0x22A01B
	0x0062A309u, // colStartRef2           kVC_ColStartRef2         0.98e @ RVA 0x22A309
	0x0062AA50u, // colExtraRef1           kVC_ColExtraRef1         0.98e @ RVA 0x22AA50
	0x0062AA52u, // colExtraRef2           kVC_ColExtraRef2         0.98e @ RVA 0x22AA52
	0x0062AA53u, // colExtraRef3           kVC_ColExtraRef3         0.98e @ RVA 0x22AA53
	0x0040BB6Bu, // streamingSkipSite1     kVC_StreamingSkipSite1   0.98e @ RVA 0x00BB6B
	0x0040FDC6u, // streamingSkipSite2     kVC_StreamingSkipSite2   0.98e @ RVA 0x00FDC6
	0x00578A77u, // modelPtrSkipSite       kVC_ModelPtrSkipSite     0.98e @ RVA 0x178A77
	0x0045E866u, // modelPtrSpecialSite    kVC_ModelPtrSpecialSite  0.98e @ RVA 0x05E866
	0x0045E865u, // modelPtrSpecialValue   kVC_ModelPtrSpecialValue 0.98e @ RVA 0x05E865 (unaligned)
	0x005B8520u, // wideModelIdSkipSite    kVC_WideModelIdSkipSite  0.98e @ RVA 0x1B8520
	0x00A10B26u, // paintByteGlobal1       kVC_PaintByteGlobal1     0.98e @ RVA 0x610B26
	0x00A10B82u, // paintByteGlobal2       kVC_PaintByteGlobal2     0.98e @ RVA 0x610B82
	0x00571820u, // partFlagByte           kVC_PartFlagByte         0.98e @ RVA 0x171820
	0x00939228u, // liveryColorTable       kVC_LiveryColorTable     0.98e @ RVA 0x539228
	0x0094AD28u, // speedCheckObject       kVC_SpeedCheckObject     0.98e @ RVA 0x54AD28
	0x00974B2Cu, // frameCounter           kVC_FrameCounter         0.98e @ RVA 0x574B2C
	0x009785A8u, // classCounterBase       kVC_ClassCounterBase     0.98e @ RVA 0x5785A8 (int per slot)
	0x0063C6F2u, // cacheByte1             kVC_CacheByte1           0.98e @ RVA 0x23C6F2 (.text byte)
	0x0063C71Du, // cacheByte2             kVC_CacheByte2           0.98e @ RVA 0x23C71D
	0x0063CE27u, // cacheByte3             kVC_CacheByte3           0.98e @ RVA 0x23CE27
	0x006DDE20u, // cacheRestoreByte       kVC_CacheRestoreByte     0.98e @ RVA 0x2DDE20 (.data byte)
	0x007DB888u, // classSlotBase          kVC_ClassSlotBase        0.98e @ RVA 0x3DB888 (stride 0x1C)
	0x0094B708u, // classObjTable          kVC_ClassObjTable        0.98e @ RVA 0x54B708
	kRetailHookSites, // stubHookSites     (was g_MvlStubHookSites, game_hooks.cpp)
	21u,         // stubHookSiteCount      (static_assert above keeps this in sync)
	0x0060048Cu, // frameWaitHookTarget    VC 0x60048c (conditional site, FUN_10043780)
	0x0042674Fu, // contTrampoline426748   (MvlHookTrampoline426748)
	0x004267E5u, // contKeyClassBypass     (MvlHookKeyClassBypass)
	0x00426ACDu, // contTrampoline426ac6   (MvlHookTrampoline426ac6)
	0x00426C87u, // contTrampoline426c80   (MvlHookTrampoline426c80)
	0x00427477u, // contTrampoline427470   (MvlHookTrampoline427470)
	0x00426A53u, // contTrampoline426a4c   (MvlHookTrampoline426a4c)
	0x004BB630u, // teardownPartsCallee    (MvlHookTeardownParts mid-call)
	0x005BAC09u, // contTeardownParts      (MvlHookTeardownParts)
	0x00579295u, // contRepaint            (MvlHookRepaint)
	0x00589790u, // partUpdateCallee69adc4 (MvlHookPartUpdate69adc4)
	0x0060AD50u, // partUpdateCallee6d7b68 (MvlHookPartUpdate6d7b68)
	0x005A4DE0u, // partUpdateCallee69b0e8 (MvlHookPartUpdate69b0e8)
	0x00589AD0u, // wheelCacheCallee       (MvlHookWheelCache69adc0)
	0x0060AD90u, // partFlagCallee         (MvlHookPartFlag6d7b64)
	0x005CF9F9u, // contFadePart           (MvlHookFadePart)
	0x005C4348u, // contFadePartAlt        (MvlHookFadePartAlt)
	0x0040896Cu, // contGenerateVcName     (MvlHookGenerateVcName)
	0x00593030u, // attachFixupCallee      (MvlHookAttachFixup)
	0x006006B8u, // contFrameWaitThrottled (MvlHookFrameWait)
	0x0060049Cu, // contFrameWaitNormal    (MvlHookFrameWait)
};

// Steam hook-site table - steam targets recovered by M8b; handlers and
// dwordWrite flags identical to retail (installer call bytes verified
// byte-identical per site, see re-steam-variant.md).
static const MvlHookSite kSteamHookSites[] = {
	{ 0x00426718u, reinterpret_cast<uintptr_t>(&MvlHookTrampoline426748), 0 }, // steam: imm @ dll RVA 0x43C4A (retail 0x426748)
	{ 0x004267AEu, reinterpret_cast<uintptr_t>(&MvlHookKeyClassBypass), 0 }, // steam: imm @ dll RVA 0x43C59 (retail 0x4267DE)
	{ 0x00426A96u, reinterpret_cast<uintptr_t>(&MvlHookTrampoline426ac6), 0 }, // steam: imm @ dll RVA 0x43C68 (retail 0x426AC6)
	{ 0x00426C50u, reinterpret_cast<uintptr_t>(&MvlHookTrampoline426c80), 0 }, // steam: imm @ dll RVA 0x43C77 (retail 0x426C80)
	{ 0x00427440u, reinterpret_cast<uintptr_t>(&MvlHookTrampoline427470), 0 }, // steam: imm @ dll RVA 0x43C86 (retail 0x427470)
	{ 0x004267F0u, reinterpret_cast<uintptr_t>(&MvlHookSetAiClass), 0 }, // steam: imm @ dll RVA 0x43C95 (retail 0x426820)
	{ 0x00426A1Cu, reinterpret_cast<uintptr_t>(&MvlHookTrampoline426a4c), 0 }, // steam: imm @ dll RVA 0x43CA4 (retail 0x426A4C)
	{ 0x005BB23Bu, reinterpret_cast<uintptr_t>(&MvlHookClearParts), 0 }, // steam: imm @ dll RVA 0x43CB3 (retail 0x5BB40B)
	{ 0x005BAA34u, reinterpret_cast<uintptr_t>(&MvlHookTeardownParts), 0 }, // steam: imm @ dll RVA 0x43CC2 (retail 0x5BAC04)
	{ 0x00579180u, reinterpret_cast<uintptr_t>(&MvlHookRepaint), 0 }, // steam: imm @ dll RVA 0x43CD1 (retail 0x579290)
	{ 0x00578F80u, reinterpret_cast<uintptr_t>(&MvlHookPaintRandom), 0 }, // steam: imm @ dll RVA 0x43CE0 (retail 0x579090)
	{ 0x00579080u, reinterpret_cast<uintptr_t>(&MvlHookPaintVariant), 0 }, // steam: imm @ dll RVA 0x43CEF (retail 0x579190)
	{ 0x00699DC4u, reinterpret_cast<uintptr_t>(&MvlHookPartUpdate69adc4), 1 }, // steam: imm @ dll RVA 0x43CFE (retail 0x69ADC4)
	{ 0x006D6B38u, reinterpret_cast<uintptr_t>(&MvlHookPartUpdate6d7b68), 1 }, // steam: imm @ dll RVA 0x43D0D (retail 0x6D7B68)
	{ 0x0069A0E8u, reinterpret_cast<uintptr_t>(&MvlHookPartUpdate69b0e8), 1 }, // steam: imm @ dll RVA 0x43D1C (retail 0x69B0E8)
	{ 0x00699DC0u, reinterpret_cast<uintptr_t>(&MvlHookWheelCache69adc0), 1 }, // steam: imm @ dll RVA 0x43D2B (retail 0x69ADC0)
	{ 0x006D6B34u, reinterpret_cast<uintptr_t>(&MvlHookPartFlag6d7b64), 1 }, // steam: imm @ dll RVA 0x43D3A (retail 0x6D7B64)
	{ 0x005CF7C3u, reinterpret_cast<uintptr_t>(&MvlHookFadePart), 0 }, // steam: imm @ dll RVA 0x43D49 (retail 0x5CF9F3)
	{ 0x005C4171u, reinterpret_cast<uintptr_t>(&MvlHookFadePartAlt), 0 }, // steam: imm @ dll RVA 0x43D58 (retail 0x5C4341)
	{ 0x00408967u, reinterpret_cast<uintptr_t>(&MvlHookGenerateVcName), 0 }, // steam: imm @ dll RVA 0x4477B (retail 0x408967)
	{ 0x00699DB0u, reinterpret_cast<uintptr_t>(&MvlHookAttachFixup), 1 }, // steam: imm @ dll RVA 0x4478A (retail 0x69ADB0)
};

static_assert(sizeof(kSteamHookSites) / sizeof(kSteamHookSites[0]) == 21,
              "steam hook-site count (FUN_10043c00 pairs)");

// Steam map - M8b recovery (tools/steam_addr_diff.py, byte-diff of
// vehmod_retail.dll vs vehmod_steam.dll; per-field cites in
// .serena/memories/re-steam-variant.md). Same 64-field order as retail.
// "identical" = steam value == retail at every match; "imm" = substituted
// immediate recovered at the cited DLL offset. "cluster-verified" = the
// +/-6-byte context check failed only because a NEIGHBORING substituted
// address sits inside the window; a wider (+/-24) masked-window check
// proved every differing byte is itself a plausible address substitution
// (tools/steam_addr_diff.py cluster_verify). All 83 fields HIGH.
constexpr MvlExeAddrs kExeAddrsSteam = {
	0x00401000u, // textUnlockBase         steam == retail (identical @ dll RVA 0x225B0/.text) HIGH
	0x0027CE00u, // textUnlockSize         steam == retail (identical @ dll RVA 0x225AB/.text) HIGH
	0x0055F526u, // modelPtrBaseImm        steam: imm @ dll RVA 0x18181/.text (retail RVA 0x15F636) HIGH
	0x0040AF00u, // streamingBaseImm       steam == retail (identical @ dll RVA 0x1818B/.text) HIGH
	0x0040C417u, // texStartImm            steam == retail (identical @ dll RVA 0x18195/.text) HIGH
	0x0040C507u, // colStartImm            steam == retail (identical @ dll RVA 0x1819F/.text) HIGH
	0x0040C582u, // animStartImm           steam == retail (identical @ dll RVA 0x181A9/.text) HIGH
	0x004102B2u, // streamingCapacity      steam == retail (identical @ dll RVA 0x17501/.text) HIGH
	0x0040BD2Du, // texStartRef1           steam == retail (identical @ dll RVA 0x174AC/.text) HIGH
	0x00410651u, // texStartRef2           steam == retail (identical @ dll RVA 0x174B3/.text) HIGH cluster-verified
	0x0041065Fu, // texCountRef1           steam == retail (identical @ dll RVA 0x1767A/.text) HIGH cluster-verified
	0x0055FA0Cu, // texStartRef3           steam: imm @ dll RVA 0x17644/.text (retail RVA 0x15FB1C) HIGH
	0x00580E06u, // texCountRef2           steam: imm @ dll RVA 0x17675/.text (retail RVA 0x180FD6) HIGH cluster-verified
	0x00580E5Du, // texCountRef3           steam: imm @ dll RVA 0x17670/.text (retail RVA 0x18102D) HIGH cluster-verified
	0x009B5A54u, // texPoolGlobal          steam: imm @ dll RVA 0x1767F/.text (retail RVA 0x5B6A4C) HIGH
	0x00580E40u, // texPoolRebuild1        steam: imm @ dll RVA 0x17695/.text (retail RVA 0x181010) HIGH
	0x00580E20u, // texPoolRebuild2        steam: imm @ dll RVA 0x176F3/.text (retail RVA 0x180FF0) HIGH cluster-verified
	0x00629CDBu, // colStartRef1           steam: imm @ dll RVA 0x174C1/.text (retail RVA 0x22A01B) HIGH
	0x00629FC9u, // colStartRef2           steam: imm @ dll RVA 0x174BA/.text (retail RVA 0x22A309) HIGH
	0x0062A710u, // colExtraRef1           steam: imm @ dll RVA 0x1779C/.text (retail RVA 0x22AA50) HIGH
	0x0062A712u, // colExtraRef2           steam: imm @ dll RVA 0x177AB/.text (retail RVA 0x22AA52) HIGH cluster-verified
	0x0062A713u, // colExtraRef3           steam: imm @ dll RVA 0x177B1/.text (retail RVA 0x22AA53) HIGH cluster-verified
	0x0040BB6Bu, // streamingSkipSite1     steam == retail (identical @ dll RVA 0x17480/.text) HIGH
	0x0040FDC6u, // streamingSkipSite2     steam == retail (identical @ dll RVA 0x17487/.text) HIGH
	0x00578967u, // modelPtrSkipSite       steam: imm @ dll RVA 0x17568/.text (retail RVA 0x178A77) HIGH
	0x0045E746u, // modelPtrSpecialSite    steam: imm @ dll RVA 0x17588/.text (retail RVA 0x05E866) HIGH
	0x0045E745u, // modelPtrSpecialValue   steam: imm @ dll RVA 0x17590/.text (retail RVA 0x05E865) HIGH
	0x005B8350u, // wideModelIdSkipSite    steam: imm @ dll RVA 0x171E7/.text (retail RVA 0x1B8520) HIGH
	0x00A0FB2Fu, // paintByteGlobal1       steam: imm @ dll RVA 0x3BA08/.text (retail RVA 0x610B26) HIGH
	0x00A0FB8Cu, // paintByteGlobal2       steam: imm @ dll RVA 0x3B9FB/.text (retail RVA 0x610B82) HIGH
	0x00571710u, // partFlagByte           steam: imm @ dll RVA 0x3B5C3/.text (retail RVA 0x171820) HIGH
	0x00938230u, // liveryColorTable       steam: imm @ dll RVA 0x3B867/.text (retail RVA 0x539228) HIGH
	0x00949D30u, // speedCheckObject       steam: imm @ dll RVA 0x798E/.text (retail RVA 0x54AD28) HIGH
	0x00973B34u, // frameCounter           steam: imm @ dll RVA 0xA9AD/.text (retail RVA 0x574B2C) HIGH
	0x009775B0u, // classCounterBase       steam: imm @ dll RVA 0x4F74F/.text (retail RVA 0x5785A8) HIGH
	0x0063B6A2u, // cacheByte1             steam: imm @ dll RVA 0xC2B2/.text (retail RVA 0x23C6F2) HIGH
	0x0063B6CDu, // cacheByte2             steam: imm @ dll RVA 0xC2BE/.text (retail RVA 0x23C71D) HIGH
	0x0063BDD7u, // cacheByte3             steam: imm @ dll RVA 0xC2CA/.text (retail RVA 0x23CE27) HIGH
	0x006DCDB8u, // cacheRestoreByte       steam: imm @ dll RVA 0xC27B/.text (retail RVA 0x2DDE20) HIGH
	0x007DA890u, // classSlotBase          steam: imm @ dll RVA 0x13B4C/.text (retail RVA 0x3DB888) HIGH
	0x0094A710u, // classObjTable          steam: imm @ dll RVA 0x13B31/.text (retail RVA 0x54B708) HIGH
	kSteamHookSites, // stubHookSites          steam targets (below; installer bytes verified identical)
	21u,             // stubHookSiteCount      (static_assert below keeps this in sync)
	0x006000ECu, // frameWaitHookTarget    steam: imm @ dll RVA 0x44802/.text (retail RVA 0x20048C) HIGH
	0x0042671Fu, // contTrampoline426748   steam: imm @ dll RVA 0x3E004/.text (retail RVA 0x02674F) HIGH
	0x004267B5u, // contKeyClassBypass     steam: imm @ dll RVA 0x3E05C/.text (retail RVA 0x0267E5) HIGH
	0x00426A9Du, // contTrampoline426ac6   steam: imm @ dll RVA 0x3E084/.text (retail RVA 0x026ACD) HIGH
	0x00426C57u, // contTrampoline426c80   steam: imm @ dll RVA 0x3E0A4/.text (retail RVA 0x026C87) HIGH
	0x00427447u, // contTrampoline427470   steam: imm @ dll RVA 0x3E0C4/.text (retail RVA 0x027477) HIGH
	0x00426A23u, // contTrampoline426a4c   steam: imm @ dll RVA 0x3E114/.text (retail RVA 0x026A53) HIGH
	0x004BB500u, // teardownPartsCallee    steam: imm @ dll RVA 0x3E154/.text (retail RVA 0x0BB630) HIGH
	0x005BAA39u, // contTeardownParts      steam: imm @ dll RVA 0x3E160/.text (retail RVA 0x1BAC09) HIGH
	0x00579185u, // contRepaint            steam: imm @ dll RVA 0x3E1C0/.text (retail RVA 0x179295) HIGH
	0x005895C0u, // partUpdateCallee69adc4 steam: imm @ dll RVA 0x33331/.text (retail RVA 0x189790) HIGH
	0x0060A970u, // partUpdateCallee6d7b68 steam: imm @ dll RVA 0x33511/.text (retail RVA 0x20AD50) HIGH
	0x005A4C10u, // partUpdateCallee69b0e8 steam: imm @ dll RVA 0x336A1/.text (retail RVA 0x1A4DE0) HIGH
	0x00589900u, // wheelCacheCallee       steam: imm @ dll RVA 0x16C74/.text (retail RVA 0x189AD0) HIGH
	0x0060A9B0u, // partFlagCallee         steam: imm @ dll RVA 0x33501/.text (retail RVA 0x20AD90) HIGH
	0x005CF7C9u, // contFadePart           steam: imm @ dll RVA 0x3E316/.text (retail RVA 0x1CF9F9) HIGH
	0x005C4178u, // contFadePartAlt        steam: imm @ dll RVA 0x3E367/.text (retail RVA 0x1C4348) HIGH
	0x0040896Cu, // contGenerateVcName     steam == retail (identical @ dll RVA 0x43748/.text) HIGH
	0x00592E60u, // attachFixupCallee      steam: imm @ dll RVA 0x33301/.text (retail RVA 0x193030) HIGH
	0x00600318u, // contFrameWaitThrottled steam: imm @ dll RVA 0x437E7/.text (retail RVA 0x2006B8) HIGH
	0x006000FCu, // contFrameWaitNormal    steam: imm @ dll RVA 0x437F8/.text (retail RVA 0x20049C) HIGH
};

// Active map pointer. NULL = exe variant unknown/unreadable -> FAIL-SAFE
// NO-OP: every hook install / game-memory write path guards on this and
// bails (never patch a wrong exe). Before MvlInitExeAddrs() runs it defaults
// to retail (the DllMain ordering is not load-bearing; retail stays the safe
// pre-init assumption exactly as in M8a).
static const MvlExeAddrs *s_mvlExeAddrs = &kExeAddrsRetail;

const MvlExeAddrs *
MvlAddrs(void)
{
	return s_mvlExeAddrs;
}

void
MvlInitExeAddrs(void)
{
	// M8c: fingerprint the host exe ONCE, before any hook install / memory
	// write, and select the matching map. Unknown -> log + NULL map (all
	// write paths no-op; see the guards in game_hooks.cpp / init.cpp /
	// model_helpers.cpp).
	const int variant = MvlDetectExeVariant();
	switch (variant) {
	case 0:
		s_mvlExeAddrs = &kExeAddrsRetail;
		break;
	case 1:
		s_mvlExeAddrs = &kExeAddrsSteam;
		break;
	default:
		MvlErrorLog("engine: unknown gta-vc.exe variant (fingerprint @ 0x61A7F8 "
		            "not retail/steam) - MVL engine patches DISABLED (fail-safe no-op)");
		s_mvlExeAddrs = NULL;
		break;
	}
}

int
MvlDetectExeVariant(void)
{
	// Fingerprint dword @ fixed gta-vc.exe VA 0x61A7F8 (re-loader-shim.md;
	// same probe as src/loader/chooser.cpp VehmodChooserDllMain - the exe is
	// loaded at its preferred base 0x400000, no ASLR, so the fixed VA is
	// directly readable in-process). SEH-guarded: during non-VC-hosted tests
	// that address is unmapped.
	//   0x9753C405 -> 0 retail
	//   0xE8535004 -> 1 steam
	//   anything else / unreadable -> -1 unknown
	DWORD fingerprint;
	volatile const DWORD *pFingerprint =
	    (volatile const DWORD *)(uintptr_t)0x61A7F8u;
	__try {
		fingerprint = *pFingerprint;
	}
	__except (EXCEPTION_EXECUTE_HANDLER) {
		return -1; // unreadable -> unknown
	}

	if (fingerprint == 0x9753C405u)
		return 0; // retail
	if (fingerprint == 0xE8535004u)
		return 1; // steam
	return -1;    // unknown exe
}
