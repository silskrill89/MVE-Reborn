# renames-engine - src/engine `FUN_<va>` -> semantic `Mvl*` rename sweep

Scope: `src/engine/` ONLY - `model_helpers.{h,cpp}`, `model_id.{h,cpp}`,
`game_hooks.{h,cpp}`, `dirmgr.{h,cpp}`, `engine_globals.{h,cpp}`.
Branch: `continued`. No builds, no git operations, nothing outside `src/engine/`
written.

## 1. Result

| metric | before | after |
|---|---|---|
| unique `FUN_<va>` **code** identifiers in `src/engine/` | 122 (+ the `thunk_FUN_1002faa0` form = 123 scanner hits) | **0** |
| definitions carrying a `// FUN_<va> ... @ RVA 0x...` cite | ~30 (RVA only, no `FUN_` name) | **135 / 135** |
| `FUN_` mentions (code + comments) | 487 occurrences | 294 (comments only) |
| brace count `{`/`}` and line count per file | - | **unchanged** for all 10 files |

Files actually changed (6): `dirmgr.{h,cpp}`, `game_hooks.{h,cpp}`, `model_helpers.{h,cpp}`.
In scope but **nothing to rename** (4): `model_id.{h,cpp}`, `engine_globals.{h,cpp}` -
they contain zero code-level `FUN_` identifiers (every mention there is a comment
cite).

Method: comment-span-aware Python replacement - word-boundary
`(?<![A-Za-z0-9])FUN_xxxxxxxxx(?![A-Za-z0-9])` applied to code **and string
literals** only; comment text left byte-identical so the original identity stays
grep-able. Only the identifier changed: signatures, calling conventions
(`__fastcall`/`__cdecl`/`__declspec(naked)`), parameter lists and bodies are
untouched. Each definition/declaration then carries a cite - the existing leading
comment was rewritten in place where one existed:

- `// 0.98e @ RVA 0x4f400 - ...` -> `// FUN_1004f400 (0.98e @ RVA 0x4f400) - ...`
- `// TODO(helpers): 0x1001af70 unported - ...` -> `// FUN_1001af70 (0.98e @ RVA 0x1af70) - TODO(helpers): unported - ...`

otherwise a trailing comment was appended on the definition line:

- `void MvlHookTrampoline426748(void) // FUN_1003dff0 @ RVA 0x3dff0`

## 2. Logger reconciliation (special case)

`src/loader/log.h` exposes the **clone-side** wrappers `MvlLog` / `MvlErrorLog`
(thin helpers: timestamp + append to `mvl\messagelog.txt` / `mvl\errorlog.txt`).
The `FUN_` loggers are the **original DLL's** printf-shaped loggers, so they got
the `MvlOrig*` prefix to avoid colliding with the clone wrappers:

| original | clone name | notes |
|---|---|---|
| `FUN_1001cb80` @ RVA 0x1cb80 | `MvlOrigErrorLog` | primary plugin error logger `(context, fmt, ...)`; body forwards to `MvlErrorLog("%s: %s", ...)` |
| `FUN_1001cce0` @ RVA 0x1cce0 | `MvlOrigWarnLog` | second logger flavor (port notes: warn/context channel); identical body, also forwards to `MvlErrorLog` |
| `FUN_1001cc00` @ RVA 0x1cc00 | **not renamed - no definition exists** | the msg logger has *no* definition anywhere under `src/`; it appears only as comment mentions in other lanes (`python_host.cpp`, `vehicle_xml.cpp`, `png_funcs.cpp`, `vehicle_archive.cpp`, ...). Out of scope for this sweep. |

Both engine-side loggers are `static` in `model_helpers.cpp` (internal linkage),
so `MvlOrigErrorLog` / `MvlOrigWarnLog` cannot collide with the extern
`MvlErrorLog` / `MvlLog`. Other lanes name the same originals with their own
module prefix (`MvlScmWarn` = `FUN_1001cce0` in `src/scm/scm_handler.cpp`), i.e.
one lane-local wrapper name per original symbol - the same pattern.

## 3. Cross-module name reuse (same RVA already named elsewhere)

To keep "one original symbol -> one clone name" (AGENTS.md naming doctrine), the
engine port reuses the name another lane already established for the same RVA:

| old | reused name | also used in |
|---|---|---|
| `FUN_100080f0` | `MvlAnimQuatInit` | `src/anim/custom_anim.cpp` |
| `FUN_10008250` | `MvlEulerToQuat` | `src/archive/vehicle_archive.{h,cpp}` |
| `FUN_10008590` | `MvlQuatMid` | `src/archive/vehicle_archive.{h,cpp}` |
| `FUN_10008c30` | `MvlVectorZero` | `src/archive/vehicle_archive.{h,cpp}`, `src/anim/custom_anim.cpp`, `src/weapons/weapon_pickups.cpp` |
| `FUN_10008c90` | `MvlVectorCopy` | `src/archive/vehicle_archive.{h,cpp}`, `src/anim/custom_anim.cpp` |
| `FUN_1002dfd0` | `MvlGameGetPlayerPed` | `src/weapons/weapon_handler.{h,cpp}` |
| `FUN_10031380` | `MvlRwAtomicDestroy` | `src/weapons/mvl_weapon.{h,cpp}` |
| `FUN_10031ca0` | `MvlRwClumpSetup` | `src/archive/vehicle_archive.{h,cpp}` |
| `FUN_10031da0` | `MvlRwPartDestroyGeometry` | `src/vehicle/tuning.cpp` |
| `FUN_1004f380` | `MvlVehicleRegGet` | `src/archive/vehicle_archive.{h,cpp}`, `src/extras/xmlgen.cpp` |

All ten are `static` (internal linkage) inside `model_helpers.cpp`, and no
`src/engine/*.h` is included from outside `src/engine/` (verified) - so the reuse
cannot create an ODR/link clash even though the other lanes' copies are extern.

**Deliberate divergence:** `FUN_1004f7e0` -> `MvlSetAiClass` here, while
`src/archive/vehicle_archive.h:95` has a stub `MvlHandlerSetAiClass` citing the
same RVA 0x4f7e0. The engine copy is **non-static** (`void __fastcall`, declared
in `model_helpers.h`), the archive stub is `void (int, int)` cdecl - reusing the
archive name would put two differently-qualified external symbols with the same
unqualified name into one link. Keep engine-local for now; align when the archive
stub is rewired to call through the engine port.

`FUN_10008c50` also has two competing lane names (`MvlAnimVectorDecode` in the
anim lane, `MvlVectorZero` in the weapons lane) - the engine body is a plain
12-byte zero-fill and a second `MvlVectorZero` static would collide inside the
same TU, so it keeps the local name `MvlVectorZeroVariant`.

## 4. Identifiers deliberately left alone

- **Comment-only `FUN_` mentions (294)** - every remaining `FUN_` in
  `src/engine/` is a comment: definition/declaration cites plus prose
  (`calls FUN_1003a410`, install notes naming `FUN_10022560`). Required for
  grep-ability of the original identity.
- **`FUN_1001cc00`** - no definition exists under `src/` (see section 2).
- **Cross-module comment references** (`FUN_10022560`, `FUN_10022590`,
  `FUN_100225a0`, `FUN_10043c00`, `FUN_1003e380`..`FUN_10043b50`,
  `FUN_10018180` and the `CModelIndexLimit::*` chain) - owned by other lanes or
  not code here (comment-only anyway).
- **`DAT_` / `UNK_`** - none are code identifiers in `src/engine/` (all mentions
  are comments; the storage already lives in `engine_globals.cpp` as `Mvl*`).
- **Cross-module idents already renamed earlier** - `FUN_1002d880`,
  `FUN_1003d120`, `FUN_1004f3a0` were already ported as `MvlGetActiveModelIndex`,
  `MvlGetExtraFlags`, `MvlGetModelDirRecord`; only their comment cites remain.
- **Clone-side `_impl` shims** (not original symbols, no RVA cite added):
  `FUN_1003e2d0_impl` -> `MvlHookFadePart_impl`, `FUN_1003e320_impl` ->
  `MvlHookFadePartAlt_impl`, `FUN_10043780_impl` -> `MvlHookFrameWait_impl`
  (renamed only so no `FUN_` substring survives in code; the inline-asm `call`
  targets were renamed with them).
- `thunk_FUN_1002faa0` -> `MvlPartsCacheDtorThunk`, cited as
  `// thunk_FUN_1002faa0 (0.98e @ RVA 0x2faa0)` on its definition (Ghidra reports
  the thunk form; the bare `FUN_1002faa0` substring it contains is not a symbol).

## 5. Verification

- `FUN_[0-9a-fA-F]{5,}` as a code identifier in `src/engine/`: **0** (comment
  spans excluded by a C/C++ comment scanner) - PASS.
- Definitions checked after the rename: **135**, each carrying a comment with
  its old `FUN_<va>` (or `thunk_FUN_...`) **and** `@ RVA 0x` - missing: **0** - PASS.
- Brace balance `{`/`}` and line counts per file identical before/after
  (`model_helpers.cpp` 183/183 x 1468 lines, `game_hooks.cpp` 73/73 x 464,
  `dirmgr.cpp` 3/3 x 75, `model_id.cpp` 78/78 x 973, `engine_globals.cpp` 6/6 x
  108, headers 0/0, 2/2 or 1/1) - PASS.
- Old engine `FUN_` names used as **code** outside `src/engine/`: **0** - no
  cross-module breakage - PASS.
- Files modified outside `src/engine/`: **0** - PASS.
- No build run (per task); no git operations performed.

## 6. Mapping table

### dirmgr.{h,cpp}

| old FUN_ | new name | behavior (decomp) | confidence |
|---|---|---|---|
| `FUN_10017f60` | `MvlVerifyModelHasColModel` | verifies the model being created is registered, has the expected name and a colmodel (delegates to CModelIndexLimit::VerifyModelHasColModel) | recovered |
| `FUN_10018030` | `MvlVerifyDirectoryCanAdd` | verifies the ID-less model directory can accept one more entry (delegates to CModelIndexLimit::VerifyDirectoryCanAdd) | recovered |
| `FUN_100180a0` | `MvlInstallDirHooks` | one-shot installer of the 9 directory hook sites, guarded by MvlDirHooksInstalled | semantic-provisional |

### game_hooks.{h,cpp}

| old FUN_ | new name | behavior (decomp) | confidence |
|---|---|---|---|
| `FUN_1003dff0` | `MvlHookTrampoline426748` | hook handler @ VC 0x426748: records continuation 0x42674f and falls through (original had an unrecovered jumptable) | semantic-provisional |
| `FUN_1003e010` | `MvlHookKeyClassBypass` | hook handler @ VC 0x4267de: captures the key-class param, bypasses via MvlKeyClassBypassTable[0x82..0xEC], else continues 0x4267e5 | semantic-provisional |
| `FUN_1003e070` | `MvlHookTrampoline426ac6` | hook handler @ VC 0x426ac6: records continuation 0x426acd and falls through (unrecovered jumptable) | semantic-provisional |
| `FUN_1003e090` | `MvlHookTrampoline426c80` | hook handler @ VC 0x426c80: records continuation 0x426c87 and falls through (unrecovered jumptable) | semantic-provisional |
| `FUN_1003e0b0` | `MvlHookTrampoline427470` | hook handler @ VC 0x427470: records continuation 0x427477 and falls through (unrecovered jumptable) | semantic-provisional |
| `FUN_1003e0d0` | `MvlHookSetAiClass` | hook handler @ VC 0x426820: stores the AI-class model/slot globals and calls MvlSetAiClass | semantic-provisional |
| `FUN_1003e100` | `MvlHookTrampoline426a4c` | hook handler @ VC 0x426a4c: records continuation 0x426a53 and falls through (unrecovered jumptable) | semantic-provisional |
| `FUN_1003e120` | `MvlHookClearParts` | hook handler @ VC 0x5bb40b: records the entity and clears its +0x1D4 parts pointer, EAX passthrough | semantic-provisional |
| `FUN_1003e140` | `MvlHookTeardownParts` | hook handler @ VC 0x5bac04: stores the teardown entity, runs MvlTeardownPartsCache, continues 0x5bac09 | semantic-provisional |
| `FUN_1003e170` | `MvlHookRepaint` | hook handler @ VC 0x579290: captures the two repaint colours + entity, runs MvlApplyRepaintColors, continues 0x579295 | semantic-provisional |
| `FUN_1003e1d0` | `MvlHookPaintRandom` | hook handler @ VC 0x579090: feeds the paint-random byte ptrs + entity into MvlRandomizePaint | semantic-provisional |
| `FUN_1003e200` | `MvlHookPaintVariant` | hook handler @ VC 0x579190: feeds the paint-variant byte ptrs + entity into MvlAdvancePaintVariant | semantic-provisional |
| `FUN_1003e230` | `MvlHookPartUpdate69adc4` | dword-slot hook @ VC 0x69adc4: sets the active entity, runs the part state machine, calls VC 0x589790, clears | semantic-provisional |
| `FUN_1003e250` | `MvlHookPartUpdate6d7b68` | dword-slot hook @ VC 0x6d7b68: sets the active entity, runs the part state machine, calls VC 0x60ad50, clears | semantic-provisional |
| `FUN_1003e270` | `MvlHookPartUpdate69b0e8` | dword-slot hook @ VC 0x69b0e8: sets the active entity, runs the part state machine, calls VC 0x5a4de0, clears | semantic-provisional |
| `FUN_1003e290` | `MvlHookWheelCache69adc0` | dword-slot hook @ VC 0x69adc0: stores the wheel-cache entity, runs MvlRefreshWheelCache + MvlPatchPartFlagByte, calls VC 0x589ad0 | semantic-provisional |
| `FUN_1003e2b0` | `MvlHookPartFlag6d7b64` | dword-slot hook @ VC 0x6d7b64: stores the part-flag entity, runs MvlPatchPartFlagByte, calls VC 0x60ad90 | semantic-provisional |
| `FUN_1003e2d0` | `MvlHookFadePart` | naked trampoline @ VC 0x5cf9f3: unpacks stack args (EBP float*) into MvlUpdatePartFade, continues 0x5cf9f9 | semantic-provisional |
| `FUN_1003e320` | `MvlHookFadePartAlt` | naked trampoline @ VC 0x5c4341: unpacks stack args + frame scratch into MvlUpdatePartFade, continues 0x5c4348 | semantic-provisional |
| `FUN_10043730` | `MvlHookGenerateVcName` | hook handler @ VC 0x408967: generates the "vc%u" vehicle name via MvlFormatVcName, continues 0x40896c | semantic-provisional |
| `FUN_10043760` | `MvlHookAttachFixup` | dword-slot hook @ VC 0x69adb0: stores the attach-fixup entity, runs MvlRunAttachFixups, calls VC 0x593030 | semantic-provisional |
| `FUN_10043780` | `MvlHookFrameWait` | conditional hook @ VC 0x60048c: x87 ST0/ST1 frame-wait throttle via MsgWaitForMultipleObjects (0x6006b8/0x60049c) | semantic-provisional |

### model_helpers.{h,cpp}

| old FUN_ | new name | behavior (decomp) | confidence |
|---|---|---|---|
| `FUN_100080d0` | `MvlQuatNormalizeElem` | quaternion normalize element step called by MvlQuatNormalizeChain / MvlQuatSlerp (unported stub) | semantic-provisional |
| `FUN_100080f0` | `MvlAnimQuatInit` | CQuaternion::SetIdentity (local decomp); name reused from the anim lane | recovered (reused: anim lane) |
| `FUN_10008250` | `MvlEulerToQuat` | CQuaternion::Set pass-through (local decomp); name reused from the archive lane | recovered (reused: archive lane) |
| `FUN_10008590` | `MvlQuatMid` | CQuaternion slerp/midpoint (local decomp); name reused from the archive lane | recovered (reused: archive lane) |
| `FUN_10008990` | `MvlQuatGetMatrix` | CQuaternion::Get - quaternion -> 3x3 matrix rows | recovered |
| `FUN_10008c30` | `MvlVectorZero` | CVector zero-init (12-byte memset); name reused from the archive lane | recovered (reused: archive/anim/weapons) |
| `FUN_10008c50` | `MvlVectorZeroVariant` | CVector zero-init variant (12-byte memset) | semantic-provisional |
| `FUN_10008c90` | `MvlVectorCopy` | CVector copy (original __thiscall); name reused from the archive lane | recovered (reused: archive lane) |
| `FUN_10008ce0` | `MvlVectorCtorPass` | CVector ctor pass-through (unported stub) | semantic-provisional |
| `FUN_10008d10` | `MvlVectorProductHelper` | CVector product helper (original __thiscall) | semantic-provisional |
| `FUN_10008d90` | `MvlVectorTransform` | CVector transform helper (original __thiscall) | semantic-provisional |
| `FUN_10008e30` | `MvlQuatNormaliseWrapper` | CQuaternion::Normalise wrapper (original __thiscall) | semantic-provisional |
| `FUN_10008ee0` | `MvlVectorProduct2` | CVector product, 2nd variant (unported stub) | semantic-provisional |
| `FUN_10008f50` | `MvlVectorProduct1` | CVector product, result written through the return pointer (unported stub) | semantic-provisional |
| `FUN_10008fc0` | `MvlQuatNormaliseBody` | CQuaternion::Normalise body (unported stub) | semantic-provisional |
| `FUN_10009060` | `MvlQuatGetPass1` | CQuaternion::Get pass-through 1 (unported stub) | semantic-provisional |
| `FUN_10009190` | `MvlQuatGetPass2` | CQuaternion::Get pass-through 2 (unported stub) | semantic-provisional |
| `FUN_100092f0` | `MvlQuatNormalizeChain` | quaternion normalize chain driver (unported stub) | semantic-provisional |
| `FUN_10009300` | `MvlQuatNormalizeSub` | quaternion normalize sub-step called by MvlQuatNormalizeChain (unported stub) | semantic-provisional |
| `FUN_10009330` | `MvlVectorLengthPass` | CVector length helper pass-through (unported stub) | semantic-provisional |
| `FUN_10009380` | `MvlVectorStorePass` | CVector store pass-through (unported stub) | semantic-provisional |
| `FUN_10009480` | `MvlUpdateEffectFrame` | effect frame update (unported stub) | semantic-provisional |
| `FUN_10009490` | `MvlUpdateEffectPosition` | effect position update (unported stub) | semantic-provisional |
| `FUN_1000b070` | `MvlResetRenderState` | render state reset (unported stub) | semantic-provisional |
| `FUN_1000b080` | `MvlToggleRenderState2` | render state toggle 2 (unported stub) | semantic-provisional |
| `FUN_1000b090` | `MvlToggleRenderState1` | render state toggle 1 (unported stub) | semantic-provisional |
| `FUN_1000b120` | `MvlFlushRenderState` | render state flush (unported stub) | semantic-provisional |
| `FUN_1000b130` | `MvlSetRenderAlphaState` | render alpha state (unported stub) | semantic-provisional |
| `FUN_1000b170` | `MvlRegisterEffect` | effect registration, returns 0 (unported stub) | semantic-provisional |
| `FUN_10017290` | `MvlGrowAnimPool` | grows the anim pool to MvlColIdBoundary + 100 entries, gated on MvlLongModelIdsEnabled | semantic-provisional |
| `FUN_10017820` | `MvlRunModelIndexLimitChain` | DRIVER: seeds VC limits then runs the CModelIndexLimit relocation chain + patches + dirmgr hooks + anim pool | semantic-provisional |
| `FUN_10017940` | `MvlInstallColAnimPatches` | col/anim code-site patches (guarded by MvlColAnimPatchesInstalled; 5 JMPs still unported) | semantic-provisional |
| `FUN_1001af70` | `MvlFindEntityAttachment` | entity -> attachment lookup (unported stub) | semantic-provisional |
| `FUN_1001af90` | `MvlQueryAttachmentSlot` | attachment slot query, returns 0 (unported stub) | semantic-provisional |
| `FUN_1001b080` | `MvlQueryAttachmentOffset` | attachment offset query, returns 0 (unported stub) | semantic-provisional |
| `FUN_1001b100` | `MvlApplyAttachmentFixup` | attachment fixup apply (unported stub) | semantic-provisional |
| `FUN_1001b1a0` | `MvlFetchAttachmentWorldMatrix` | attachment world matrix fetch (unported stub) | semantic-provisional |
| `FUN_1001b240` | `MvlHandleAttachmentFailure` | attachment failure path (unported stub) | semantic-provisional |
| `FUN_1001b280` | `MvlBuildAttachmentWorldPos` | attachment world-position build from an entity CVector (unported stub) | semantic-provisional |
| `FUN_1001b5b0` | `MvlAttachmentPostFixup` | attachment post-fixup when extra-flag 0x400000 is set | semantic-provisional |
| `FUN_1001b610` | `MvlRunAttachFixups` | runs the attach/detach fixup pair on MvlAttachFixupEntity (fixup x2 + post-fixup) | semantic-provisional |
| `FUN_1001b640` | `MvlFixupAttachTransform` | attach/detach transform fixup, implicit XMM inputs lost (unported stub) | semantic-provisional |
| `FUN_1001cb80` | `MvlOrigErrorLog` | ORIGINAL 0.98e primary plugin error logger (context + fmt, va_list) forwarded to MvlErrorLog | semantic-provisional |
| `FUN_1001cce0` | `MvlOrigWarnLog` | ORIGINAL 0.98e second logger flavor (context + fmt, va_list) forwarded to MvlErrorLog | semantic-provisional |
| `FUN_1002dfd0` | `MvlGameGetPlayerPed` | player ped lookup (local decomp stub, 0); name reused from the weapons lane | recovered (reused: weapons lane) |
| `FUN_1002dff0` | `MvlGetLocalPlayerPed` | game call into VC 0x4BC1E0 returning the local player ped (unported stub, 0) | semantic-provisional |
| `FUN_1002e170` | `MvlFixupEffectMatrix` | effect object matrix fixup (unported stub) | semantic-provisional |
| `FUN_1002e340` | `MvlCombineEffectMatrix` | effect matrix combine (unported stub) | semantic-provisional |
| `FUN_1002e750` | `MvlAllocEffectObject` | effect object alloc (unported stub, 0) | semantic-provisional |
| `FUN_1002e780` | `MvlSetupEffectFlags` | effect object flag setup (unported stub) | semantic-provisional |
| `FUN_1002e8d0` | `MvlFlushWheelCacheBody` | wheel-cache flush body (unported stub) | semantic-provisional |
| `FUN_1002e8e0` | `MvlFlushWheelCache` | wheel-cache flush wrapper (delegates to MvlFlushWheelCacheBody) | semantic-provisional |
| `FUN_1002ec60` | `MvlAttachEffectLight` | effect light attach (unported stub) | semantic-provisional |
| `FUN_1002f690` | `MvlPartsObjectCtor` | parts-object constructor (unported pass-through stub) | semantic-provisional |
| `FUN_1002f880` | `MvlPartsObjectSubInitB` | parts-object sub-init B (unported stub) | semantic-provisional |
| `FUN_1002f890` | `MvlInitPartsObject` | parts-object init pair (two sub-inits) | semantic-provisional |
| `FUN_1002f8a0` | `MvlPartsObjectSubInitA` | parts-object sub-init A (unported stub) | semantic-provisional |
| `FUN_1002f8b0` | `MvlPartsObjectFill` | parts-object data fill from a source buffer (unported stub) | semantic-provisional |
| `thunk_FUN_1002faa0` | `MvlPartsCacheDtorThunk` | cached-parts destructor thunk FUN_1002faa0 (unported stub) | semantic-provisional |
| `FUN_1002fb00` | `MvlHasGeometry` | "has geometry": *(obj + 0x2C) != 0 | semantic-provisional |
| `FUN_10031380` | `MvlRwAtomicDestroy` | part/atomic frame teardown returning 1 (local decomp); name reused from the weapons lane | recovered (reused: weapons lane) |
| `FUN_10031ca0` | `MvlRwClumpSetup` | part -> RW clump/frame accessor *(part+4); name reused from the archive lane | recovered (reused: archive lane) |
| `FUN_10031cc0` | `MvlFetchPartFrame` | part frame fetch, returns 0 (unported stub) | semantic-provisional |
| `FUN_10031d40` | `MvlRwMatrixPush` | tail call into VC RW matrix push (VC 0x6437B0, unported stub) | semantic-provisional |
| `FUN_10031d90` | `MvlRwFrameUpdate2` | tail call into VC RW frame update 2 (VC 0x644470, unported stub) | semantic-provisional |
| `FUN_10031da0` | `MvlRwPartDestroyGeometry` | tail call into VC RW code (VC 0x6445F0); name reused from the tuning lane | recovered (reused: tuning lane) |
| `FUN_10031ec0` | `MvlRwFrameSync` | tail call into VC RW frame sync (VC 0x644AD0, unported stub) | semantic-provisional |
| `FUN_10031ee0` | `MvlRwFrameUpdate3` | tail call into VC RW frame update 3 (VC 0x644D00, unported stub) | semantic-provisional |
| `FUN_10032140` | `MvlRwMatrixPop` | tail call into VC RW matrix pop (VC 0x647160, unported stub) | semantic-provisional |
| `FUN_10032300` | `MvlPartFrameStep3` | part frame step 3 (unported stub) | semantic-provisional |
| `FUN_10032810` | `MvlPartFrameStep4` | part frame step 4, returns 0 (unported stub) | semantic-provisional |
| `FUN_10032b30` | `MvlPartFrameStep5` | part frame step 5 (unported stub) | semantic-provisional |
| `FUN_10032b70` | `MvlPartFrameStep2` | part frame step 2 (unported stub) | semantic-provisional |
| `FUN_10032bf0` | `MvlPartFrameStep1` | part frame step 1 (unported stub) | semantic-provisional |
| `FUN_10037a60` | `MvlFetchEffectRngSeed` | RNG/seed fetch used by the emitter (unported stub) | semantic-provisional |
| `FUN_1003a350` | `MvlBuildWheelCache` | (re)builds the wheel-parts cache for the entity (0x58 alloc + 3 .text byte pokes) | semantic-provisional |
| `FUN_1003a410` | `MvlFreePartsCache` | frees the cached parts block of the entity (sub-block + records) | semantic-provisional |
| `FUN_1003a450` | `MvlRefreshWheelCache` | per-frame wheel-part cache refresh for MvlWheelCacheEntity (rebuild on generation mismatch) | semantic-provisional |
| `FUN_1003a810` | `MvlTeardownPartsCache` | IsBadReadPtr-guarded teardown of the cached parts block of MvlPartsTeardownEntity (EAX 1/0) | semantic-provisional |
| `FUN_1003a900` | `MvlSpawnFireEffect` | spawns a fire effect on a free VC class slot | semantic-provisional |
| `FUN_1003ab00` | `MvlEmitProjectileEffect` | projectile/fire-effect emitter, heavy FPU + lost-ECX chain (unported stub) | semantic-provisional |
| `FUN_1003af20` | `MvlStepPartStateMachine` | per-frame door/window/extra part state machine (key polling, anim stepping, matrix writes) | semantic-provisional |
| `FUN_1003b580` | `MvlPatchPartFlagByte` | writes the single-byte 0xC3/0xD9 patch at VC 0x571820 per model-directory flag | semantic-provisional |
| `FUN_1003b5d0` | `MvlUpdatePartFade` | part visibility/fade: scales the part matrix by the dir-entry fade value, fires vtable+0x80 under 0.2 | semantic-provisional |
| `FUN_1003b840` | `MvlApplyRepaintColors` | repaint: pushes resolved colours from the 0x939228 table / dir-entry override into the 4 part groups | semantic-provisional |
| `FUN_1003b9d0` | `MvlRandomizePaint` | randomizes the two paint bytes of MvlPaintRandomEntity (8 attempts, dir override aware) | semantic-provisional |
| `FUN_1003bae0` | `MvlAdvancePaintVariant` | advances the paint-variant index of MvlPaintVariantEntity (round-robin over dir-entry paint arrays) | semantic-provisional |
| `FUN_1003cfc0` | `MvlModelDirEntryExists` | "directory entry exists for model index" | semantic-provisional |
| `FUN_1003de20` | `MvlFormatVcName` | formats "vc%u" (GetTickCount) into MvlGeneratedVcNameBuffer | semantic-provisional |
| `FUN_1004f380` | `MvlVehicleRegGet` | directory-slot / vehicle-registry indexer (unported stub, NULL); name reused from the archive lane | recovered (reused: archive lane) |
| `FUN_1004f400` | `MvlGetPartsBlock` | parts-block accessor: *(entity + 0x1D4) | semantic-provisional |
| `FUN_1004f7e0` | `MvlSetAiClass` | set AI class: flags the slot-table entry or appends to the per-slot 0x61 list | semantic-provisional |
| `FUN_10059d7f` | `MvlPluginOperatorNew` | plugin-CRT operator new(size); falls back to malloc + OOM log | semantic-provisional |
| `FUN_10059f55` | `MvlPluginRand` | MSVC CRT rand() LCG (per-thread _holdrand in the original, process-wide here) | semantic-provisional |
| `FUN_1006cc5a` | `MvlCrtFloat10Trig` | plugin CRT float10 trig helper, returns 0.0 (unported stub) | semantic-provisional |
| `FUN_1006ccc0` | `MvlCrtTrigHelper` | plugin CRT trig helper (unported stub) | semantic-provisional |
| `FUN_1006d730` | `MvlCrtWidenFloatToDouble` | plugin CRT float->double widen | semantic-provisional |
| `FUN_1006e460` | `MvlCrtNarrowDoubleToFloat` | plugin CRT double->float narrow | semantic-provisional |
