# globals-map — MVL 0.98e plugin .data/.bss global census

Scope: every `DAT_10xxxxxx` / `DAT_101xxxxx` symbol (vehmod_retail.dll 0.98e
OWN .data/.bss, image base 0x10000000, RVA = VA − 0x10000000) that appears in
`src/**`. Consolidated into `src/engine/engine_globals.{h,cpp}` (single
storage TU) unless noted. VC 1.0 GAME addresses (0x40xxxx–0x9Exxxxx) are NOT
globals of ours and are handled via VcFunc/constants in the ported TUs.

Legend: conf = HIGH (usage fully pinned by ported code / port-notes), MEDIUM
(usage known, layout/size provisional), LOW (name/type inferred only).

## 1. Top-frequency globals (all consolidated into engine_globals.cpp)

| DAT name | RVA | type | semantic name | shared-by (after refactor) | conf | evidence |
|---|---|---|---|---|---|---|
| DAT_100a6314 | 0xa6314 | uint32_t* | MvlModelLookupRegistry | model_id.cpp, model_helpers.cpp, dirmgr.cpp | HIGH | model_id.cpp RegisterModelForLookup `registry[modelIndex]=ptr`; port-notes-veh §5.8 "registry head"; was 3 disconnected statics |
| DAT_100b2450 | 0xb2450 | int32_t | MvlActiveEntityTemp | game_hooks.cpp (0x3e230/250/270), model_helpers.cpp (FUN_1003af20 entity guess) | HIGH | port-notes-engine §4 "share DAT_100b2450 across 0x3e230/250/270" |
| DAT_100b0c41 | 0xb0c41 | uint8_t | MvlLongModelIdsEnabled | model_id.cpp, model_helpers.cpp, mvl_settings.h mirror | HIGH | model_id.h MvlSetLongModelIdsEnabled; port-notes-limits §ProcessIdLimits "long-model flag" |
| DAT_100a6318 | 0xa6318 | int32_t | MvlColIdBoundary | model_id.cpp, model_helpers.cpp (FUN_10017290 capacity), mvl_settings mirror | HIGH | port-notes-limits "engine ID boundaries @ 0x100a6318 (COL end)"; also old anim-start / streaming count |
| DAT_10124f24 | 0x124f24 | char[24] | MvlCreatingModelName | model_id.cpp, dirmgr.cpp | HIGH | VerifyModelHasColModel name check; model_id.h old extern |
| DAT_100b0cdc | 0xb0cdc | void* | MvlModelPointerBuffer | model_id.cpp | HIGH | ReplaceModelPointerBufferRefs realloc target |
| DAT_100a6320 | 0xa6320 | int32_t | MvlModelIdBoundary | model_id.cpp; mvl_settings curModelLimit mirror | HIGH | port-notes-limits "0x100a6320 (model end)"; old texture-start |
| DAT_100a631c | 0xa631c | uint8_t* | MvlStreamingBufferBase | model_id.cpp; vehicle_archive seam comment | HIGH | ReplaceStreamingBufferRefs; port-notes-veh §5.8 flags write `+8+id*0x14` |
| DAT_1013c290 | 0x13c290 | char[32] | MvlGeneratedVcNameBuffer | model_helpers.cpp (FUN_1003de20), game_hooks.cpp (0x43730 stores ptr) | HIGH | FUN_1003de20 sprintf "vc%u" dest |
| DAT_1013a59c | 0x13a59c | void* | MvlAttachFixupEntity | game_hooks.cpp (0x43760), model_helpers.cpp (FUN_1001b610) | HIGH | handler 0x43760 sets it then calls FUN_1001b610 |
| DAT_10124f30 | 0x124f30 | uint8_t[16] | MvlCreatingModelDirectory | model_id.cpp, dirmgr.cpp | HIGH | VerifyDirectoryCanAdd +4/+8 counts |
| DAT_10124f14 | 0x124f14 | uint32_t | MvlCreatingModelIndex | model_id.cpp, dirmgr.cpp | HIGH | VerifyModelHasColModel index |
| DAT_100b0cd8 | 0xb0cd8 | void* | MvlStreamingBuffer | model_id.cpp | HIGH | ReplaceStreamingBufferRefs MVL-owned buffer |
| DAT_100b0cd0 | 0xb0cd0 | void* | MvlAnimPoolBuffer | model_helpers.cpp (FUN_10017290) | HIGH | FUN_10017290 malloc/free of anim pool |
| DAT_100a6328 | 0xa6328 | int32_t | MvlTxdIdBoundary | model_id.cpp; mvl_settings curTxdLimit mirror | HIGH | port-notes-limits "0x100a6328 (TXD end)" |
| DAT_1013dc50 | 0x13dc50 | int32_t | MvlPaintRandomEntity | game_hooks.cpp (0x3e1d0), model_helpers.cpp (FUN_1003b9d0) | HIGH | handler sets entity then calls FUN_1003b9d0 |
| DAT_1013dc2c | 0x13dc2c | int32_t | MvlPartsTeardownEntity | game_hooks.cpp (0x3e140), model_helpers.cpp (FUN_1003a810) | HIGH | handler 0x3e140 stores entity, calls FUN_1003a810 |
| DAT_1013c568 | 0x13c568 | int32_t | MvlPaintVariantEntity | game_hooks.cpp (0x3e200), model_helpers.cpp (FUN_1003bae0) | HIGH | handler 0x3e200 -> FUN_1003bae0 |
| DAT_1013c55c | 0x13c55c | int32_t | MvlWheelCacheEntity | game_hooks.cpp (0x3e290), model_helpers.cpp (FUN_1003a450) | HIGH | handler 0x3e290 -> FUN_1003a450+0x3b580 |
| DAT_100b77d8 | 0xb77d8 | wchar_t[200] | (SCM next-text) | vehicle_scm.cpp lane static s_nextText | HIGH | port-notes-scm §6; NO engine_globals storage (lane-owned) |
| DAT_100b641e | 0xb641e | uint8_t[0xED] | MvlKeyClassBypassTable | game_hooks.cpp (0x3e010), model_helpers.cpp (FUN_1004f7e0) | HIGH | port-notes-engine "0x3e010 bypass table indexed 0x81..0xEC" |
| DAT_1013dcf8 | 0x13dcf8 | int32_t | (save-apply pool fallback) | vehicle_handler.cpp MvlSaveApplyPoolFallbackIndex stub | MEDIUM | port-notes-veh save section; lane-owned accessor |
| DAT_1013dce4 | 0x13dce4 | int32_t | (save-apply garage fallback) | vehicle_handler.cpp MvlSaveApplyGarageFallbackIndex stub | MEDIUM | same |
| DAT_1013c1fc | 0x13c1fc | char* | MvlPaintRandomByte2Ptr | game_hooks.cpp (0x3e1d0), model_helpers.cpp (FUN_1003b9d0) | HIGH | handler passes game-side output buffer |
| DAT_1013a5a8 | 0x13a5a8 | char* | MvlPaintRandomByte1Ptr | game_hooks.cpp (0x3e1d0), model_helpers.cpp (FUN_1003b9d0) | HIGH | same |
| DAT_101247b8 | 0x1247b8 | const char* | (last archive name) | vehicle_handler.cpp s_szLastArchiveName | HIGH | port-notes-veh save §FindLoadedVehicleForIdentity; lane-owned |
| DAT_101244c4 | 0x1244c4 | std::map | (garage slot map) | vehicle_handler.cpp s_garageSlots | HIGH | port-notes-veh save; lane-owned class static |
| DAT_100b17e9 | 0xb17e9 | uint8_t | MvlParanoidModelNameChecks | model_id.cpp | HIGH | VerifyModelHasColModel name-mismatch gate |
| DAT_100b1791 | 0xb1791 | uint8_t | MvlFrameWaitHookEnabled | game_hooks.cpp (MvlInstallStubHooks) | HIGH | port-notes-engine "conditional site ... guarded by DAT_100b1791" |
| DAT_1008de28 | 0x8de28 | const float | (basic default 1.0f) | vehicle_xml.cpp hardcoded TODO | LOW | port-notes-veh §4.5 assumed 1.0f; NOT consolidated (value undumped) |

## 2. Remaining consolidated globals (engine_globals.cpp, single-TU consumers)

| DAT name | RVA | type | semantic name | consumer | conf | evidence |
|---|---|---|---|---|---|---|
| DAT_100a6d48 | 0xa6d48 | int32_t | MvlModelIndexBaseOffset | model_helpers.cpp FUN_1004f3a0 | MEDIUM | `index - base` subtracted before slot lookup |
| DAT_100b0ce0 | 0xb0ce0 | int32_t | MvlStreamingBufferCapacity | model_id.cpp | HIGH | ReplaceStreamingBufferRefs capacity guard |
| DAT_100b0ce4 | 0xb0ce4 | int32_t | MvlModelPointerBufferCapacity | model_id.cpp | HIGH | same pattern |
| DAT_100b0ce8 | 0xb0ce8 | uint32_t* | MvlSavedTexturePool | model_id.cpp | HIGH | ReplaceTexStartIndexRefs pool swap |
| DAT_100b0c42 | 0xb0c42 | uint8_t | MvlColAnimPatchesInstalled | model_helpers.cpp FUN_10017940 | HIGH | one-shot guard |
| DAT_100b0c43 | 0xb0c43 | uint8_t | MvlDirHooksInstalled | dirmgr.cpp FUN_100180a0 | HIGH | one-shot guard |
| DAT_100b0cd4 | 0xb0cd4 | int32_t | MvlAnimPoolCapacity | model_helpers.cpp FUN_10017290 | HIGH | pool capacity |
| DAT_100b17bc | 0xb17bc | int32_t | MvlConfiguredColFileLimit | model_helpers.cpp driver | HIGH | port-notes-limits "ids: colfiles @ 0x100b17bc" |
| DAT_100b17e0 | 0xb17e0 | int32_t | MvlConfiguredTexDictLimit | model_helpers.cpp driver | HIGH | "texdicts @ 0x100b17e0" |
| DAT_100b17e4 | 0xb17e4 | int32_t | MvlConfiguredModelLimit | model_helpers.cpp driver | HIGH | "models @ 0x100b17e4" |
| DAT_100b178f | 0xb178f | uint8_t | MvlIdLimitsDirty | mvl_settings mirror only | HIGH | port-notes-limits "dirty flag @ 0x100b178f" |
| DAT_100b17dc | 0xb17dc | uint32_t | MvlFrameWaitTimingConstant | game_hooks.cpp FUN_10043780 | HIGH | throttle subtract |
| DAT_1013dc38 | 0x13dc38 | uint32_t | MvlLastKeyClassParam | game_hooks.cpp 0x3e010 | MEDIUM | stores key/class param |
| DAT_1013c598 | 0x13c598 | int32_t | MvlSetAiClassModelId | game_hooks.cpp 0x3e0d0 | MEDIUM | FUN_1004f7e0 modelId channel |
| DAT_1013dbc8 | 0x13dbc8 | int32_t | MvlSetAiClassSlot | game_hooks.cpp 0x3e0d0 | MEDIUM | FUN_1004f7e0 slot channel |
| DAT_1013c268 | 0x13c268 | int32_t | MvlClearPartsEntity | game_hooks.cpp 0x3e120 | MEDIUM | clears field +0x1D4 |
| DAT_1013dbdc | 0x13dbdc | uint32_t | MvlRepaintColor1 | game_hooks.cpp 0x3e170 | HIGH | FUN_1003b840 color index 1 |
| DAT_1013c280 | 0x13c280 | uint32_t | MvlRepaintColor2 | game_hooks.cpp 0x3e170 | HIGH | color index 2 |
| DAT_1013c558 | 0x13c558 | int32_t | MvlRepaintEntity | game_hooks.cpp 0x3e170 | HIGH | repaint entity |
| DAT_1013a5a4 | 0x13a5a4 | char* | MvlPaintVariantByte1Ptr | game_hooks.cpp 0x3e200, model_helpers.cpp | HIGH | FUN_1003bae0 output buffer |
| DAT_1013db88 | 0x13db88 | char* | MvlPaintVariantByte2Ptr | game_hooks.cpp 0x3e200, model_helpers.cpp | HIGH | same |
| DAT_1013c540 | 0x13c540 | int32_t | MvlPartFlagEntity | game_hooks.cpp 0x3e2b0 | MEDIUM | entity for FUN_1003b580 |
| DAT_1013a594 | 0x13a594 | int32_t* | MvlFadePartArg | game_hooks.cpp 0x3e2d0 | HIGH | FUN_1003b5d0 part ptr |
| DAT_1013dc04 | 0x13dc04 | uint32_t | MvlFadeStackArg | game_hooks.cpp 0x3e2d0 | MEDIUM | callback arg |
| DAT_1013a5f8 | 0x13a5f8 | float* | MvlFadeVecArg | game_hooks.cpp 0x3e2d0 | HIGH | fade vector |
| DAT_1013dc88 | 0x13dc88 | int32_t* | MvlFadePartArgAlt | game_hooks.cpp 0x3e320 | MEDIUM | alt part ptr |
| DAT_1013c50c | 0x13c50c | uint32_t | MvlFadeCallbackArgAlt | game_hooks.cpp 0x3e320 | MEDIUM | *(param+4) |
| DAT_1013c594 | 0x13c594 | float* | MvlFadeScratchVec | game_hooks.cpp 0x3e320 | MEDIUM | local-frame scratch |
| DAT_1013dbc0 | 0x13dbc0 | uint32_t | MvlGeneratedVcNamePtr | game_hooks.cpp 0x43730 | HIGH | stores FUN_1003de20 buffer ptr |
| DAT_1013dca0 | 0x13dca0 | float | MvlFrameWaitTimeNow | game_hooks.cpp FUN_10043780 | HIGH | x87 ST(0) save |
| DAT_1013a5bc | 0x13a5bc | float | MvlFrameWaitTimePrev | game_hooks.cpp FUN_10043780 | HIGH | x87 ST(1) save |
| DAT_1013dbf4 | 0x13dbf4 | int32_t | MvlFrameWaitRemainingMs | game_hooks.cpp FUN_10043780 | HIGH | computed wait ms |
| DAT_100b2804 | 0xb2804 | int32_t[16] | MvlAiClassSlotTable | model_helpers.cpp FUN_1004f7e0 | MEDIUM | size provisional (16 slots) |
| DAT_100b51d8 | 0xb51d8 | int32_t[16*0x61] | MvlAiClassSlotList | model_helpers.cpp FUN_1004f7e0 | MEDIUM | size provisional |
| DAT_100b6498 | 0xb6498 | int32_t | MvlPrimaryLiveryColor | model_helpers.cpp FUN_1003b840 | HIGH | livery primary |
| DAT_100b649c | 0xb649c | int32_t | MvlAlternateLiveryColor | model_helpers.cpp FUN_1003b840 | HIGH | livery alternate |
| DAT_100b263b | 0xb263b | uint8_t | MvlPartFlagDisable | model_helpers.cpp FUN_1003b580 | MEDIUM | toggles 0x571820 patch |
| DAT_100b1a1c | 0xb1a1c | uint8_t | MvlSkipPartsFreeFlag | model_helpers.cpp FUN_1003a810 | MEDIUM | skip free |
| DAT_100b1793 | 0xb1793 | uint8_t | MvlWheelCacheFeatureFlag | model_helpers.cpp FUN_1003a350 | MEDIUM | feature gate |
| DAT_100b0c3c | 0xb0c3c | uint32_t | MvlWheelCacheGenerationStamp | model_helpers.cpp FUN_1003a450/3a350 | MEDIUM | staleness stamp |
| DAT_100b0dac | 0xb0dac | uint32_t | MvlLastHookContinuation | game_hooks.cpp SetContinuation | HIGH | crash breadcrumb |

## 3. Lane-owned (clone replaced storage with class statics; NOT in engine_globals)

| DAT name | RVA | original shape | clone equivalent | conf | evidence |
|---|---|---|---|---|---|
| DAT_101244d0 / +0xd4 | 0x1244d0 | MSVC std::map pool + count | CVehicleHandler::s_poolSlots | HIGH | port-notes-veh save §slot maps |
| DAT_101244c4 / +0xc8 | 0x1244c4 | MSVC std::map garage + count | CVehicleHandler::s_garageSlots | HIGH | same |
| DAT_101247b4/b8/c0/c4 | 0x1247b4 | save identity scratch | s_nLastArchiveHash/s_szLastArchiveName/s_ainLastParts/s_nLastSlotIndex | HIGH | vehicle_handler.cpp:68-78 |
| DAT_101247e4/e8/ec | 0x1247e4 | weapon-save identity | CWeaponHandler statics | HIGH | weapon_handler.h:120-121 |
| DAT_10117820..9b0 | 0x117820 | weapon registry + save slots | CWeaponHandler::s_weapons etc | HIGH | weapon_handler.h:113-117 |
| DAT_1011795c | 0x11795c | high-water counter | s_nMaxWeaponId | HIGH | weapon_handler.cpp:381 |
| DAT_1011c210 | 0x11c210 | std::map<string,int> registry | model_id.cpp 512-bucket hash | HIGH | port-notes-engine §53 |
| DAT_10124500..504 | 0x124500 | MSVC vector spawn list | weapon_pickups.cpp static | HIGH | weapon_pickups.h:81-82 |
| DAT_100b3008 | 0xb3008 | garage record store 0x50 recs | vehicle_handler seam | MEDIUM | vehicle_handler.h:88 |
| DAT_1013dd20/dd0c/dcf8/dce4/dcd0 | 0x13dd20 | save-apply temps | MvlSaveApply* accessors (TODO stubs) | MEDIUM | vehicle_handler.h:83-87 |
| DAT_1013dd18/dd1c/dcec/dcfc | 0x13dd18 | save-apply block | not yet ported | LOW | port-notes-scm §1 FUN_100518b0/51900 |
| DAT_100b77d8 | 0xb77d8 | wchar_t[200] next-text | CVehicleSCM::s_nextText | HIGH | port-notes-scm §6 |
| DAT_100af590 | 0xaf590 | 19-entry hook-stub ptr table | vehicle_scm.cpp seam | MEDIUM | port-notes-scm §4 |
| DAT_100a6b98 | 0xa6b98 | pickup record base 0x34B | weapon_pickups MvlPickupRecord | MEDIUM | weapon_pickups.h:103-104 |
| DAT_100b77d0/b3f08/b3f09/b51d0 | 0xb77d0 | archive loading ctx | vehicle_archive hook points | MEDIUM | vehicle_archive.h:65-71 |
| DAT_101244d4 | 0x1244d4 | pool map count | s_poolSlots (map size) | MEDIUM | vehicle_handler.h:106 |
| DAT_100a6a8c | 0xa6a8c | int streamingmem MB | m_data.streamingmem | HIGH | port-notes-limits §ProcessStreamingMemLimit |
| DAT_100b1794..7b8 | 0xb1794 | int[10] instance limits | m_data.instances | HIGH | port-notes-limits §storage |
| DAT_100b17c0..7d8 | 0xb17c0 | int[7] modelstore limits | m_data.modelstore | HIGH | same |
| DAT_100ae6a0/70c/6e4 | 0xae6a0 | pool ptr/min/max tables | HOOK POINT (not storage) | MEDIUM | port-notes-limits §Unportable |
| DAT_100ae940/76c/750/788 | 0xae940 | modelstore tables | HOOK POINT | MEDIUM | same |

## 4. Float/.rdata constants (NOT consolidated — binary .data never dumped)

| DAT name | RVA | inferred value | usage | conf |
|---|---|---|---|---|
| DAT_1008ddfc | 0x8ddfc | 1.0f? | transmission accel scale | LOW |
| DAT_1008de28 | 0x8de28 | 1.0f? | 0x2838 basic pre-default / weapon defaults | LOW |
| DAT_1008de40 | 0x8de40 | 2.0f? | SA 0x2838 default | LOW |
| DAT_1008de7c | 0x8de7c | pi | deg->rad numerator | MEDIUM |
| DAT_1008df08 | 0x8df08 | 180.0f | deg->rad denominator | MEDIUM |
| DAT_1008dec0 | 0x8dec0 | 30.0f? | anim frame scale | LOW |
| DAT_1008ded4 | 0x8ded4 | 45.0f | machinegun accuracy clamp | MEDIUM |
| DAT_1008df04 | 0x8df04 | 120.0f | audio skip distance | HIGH (value asserted in weapon_handler.cpp:73) |
| DAT_1008df08/dfc0 | 0x8dfc0 | 0.0f? | pickup Z floor | LOW |
| DAT_1008de00 | 0x8de00 | 1.0f? | pickup spawn bump | LOW |
| DAT_1008e010 | 0x8e010 | 2^32 fix table | int->double negative fix | MEDIUM (model_helpers.cpp uses 4294967296.0 literal) |
| DAT_1008d2a8 | 0x8d2a8 | name-row table | weapon name echo | LOW |

## 5. Fragmentation fixes (before → after)

| DAT name | previous copies | now |
|---|---|---|
| DAT_100a6314 | model_id.cpp static + model_helpers.cpp static + (dirmgr TODO) | 1 (MvlModelLookupRegistry) |
| DAT_100b2450 | game_hooks.cpp static + model_helpers.cpp static | 1 (MvlActiveEntityTemp) |
| DAT_100b0c41 | model_id.cpp static + model_helpers.cpp static | 1 (MvlLongModelIdsEnabled) |
| DAT_100a6318 | model_id.cpp static + model_helpers.cpp static | 1 (MvlColIdBoundary) |
| DAT_100b641e | game_hooks.cpp static + model_helpers.cpp static | 1 (MvlKeyClassBypassTable) |
| DAT_1013a59c/c55c/dc2c/dc50/c568/a5a8/c1fc/a5a4/db88 | game_hooks.cpp + model_helpers.cpp copies | 1 each (handler-entity/paint globals) |

Type unification conflicts resolved (per-TU types differed; unified type chosen):
- DAT_100b2450: game_hooks uint32_t / helpers int32_t → int32_t (entity id)
- DAT_100b0c41: model_id uint8_t / helpers char → uint8_t
- DAT_1013a59c: game_hooks uint32_t / helpers void* → void*
- DAT_1013a5a8/c1fc/a5a4/db88: game_hooks uint32_t / helpers char* → char*
  (game_hooks handler assignments now reinterpret_cast from uint32_t params)
- DAT_100b17e0: helpers uint32_t / limits int → int32_t (cast at driver call)
