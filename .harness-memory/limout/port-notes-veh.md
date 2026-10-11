# port-notes-veh.md — CMvlVehicle archive pipeline + vehicle-XML cluster (Lane V, tranche-2)

Source of truth: `.harness-memory/limout/vehpipe/range-decomp.txt` (54 fns,
0x44f20-0x4e900) + `range-functions.txt`; RTTI/name table in
`.serena/memories/re-098e-map.md`; schema grounding from
`original_files/maxovehmod098e/mvl/help/samplexmls/` (+ `commented/`, `flags.html`).

All names follow the binding naming policy: recovered original names 1:1
(error-string-confirmed names are authoritative). Functions with no recovered
original name get a provisional name and are flagged below.

## 1. Function inventory

### Archive pipeline — implemented in `src/archive/vehicle_archive.cpp`

| Original name | RVA | Status | Notes |
|---|---|---|---|
| CMvlVehicle::InitialiseLoaded | 0x44f20 | DONE | final registration; SA-txd warning latch (DAT_100b3f09), AI class push, comprules resolve, sound/engine hooks |
| ResolveAnimIndex (PROVISIONAL) | 0x44fc0 | DONE | resolves `<anims>` name vs anim-group table (DAT_0x6857b0, ~60 rows x 24B); "null" -> -1 |
| SetDimensionsFromCollision (PROVISIONAL) | 0x45030 | DONE | dims fallback from col bbox; "Dimensions set to %f %f %f." |
| ResolveComprules (PROVISIONAL) | 0x450d0 | DONE | comprules default from info+0x48 byte (cars) or type==5 |
| CMvlVehicle::LoadFromArchive | 0x45110 (catch @ 0x45318) | DONE | stage sequencing verbatim incl. pre-XML 0x2838 default and registry writes (hooks) |
| CMvlVehicle::ParseArchive | 0x45360 | DONE | CSevenWalker 7z; dup-archive check by totalSize @ 0x330 (registry hooks 0x4f3f0/0x4f380) |
| CMvlVehicle::RegisterArchiveFile | 0x455d0 | DONE | ends-with chain tuning1-9.dff / weapons.txd/dff / .xml .dff .txd .col .ifp .wav; 12 MB cap; slots 0-3 keep names |
| CMvlVehicle::VerifyRequiredFiles | 0x45870 | DONE | XML+DFF+TXD mandatory; COL unless SA DFF |
| CMvlVehicle::LoadTextureFile | 0x45900 (catch @ 0x45a5a) | DONE | main .txd -> RwTexDictionary @ 0x1e8; cleanup tail = FUN_10045a98 |
| (unnamed cleanup) | 0x45a98 | DONE (inlined in tail) | stream close + free slot 2 |
| CMvlVehicle::LoadModelFile | 0x45af0 (catch @ 0x45ccb) | DONE | clump load; info vtable +0x24 install; cleanup tail = FUN_10045d00 |
| (unnamed cleanup) | 0x45d00 | DONE (inlined in tail) | |
| CMvlVehicle::LoadCollisionFile | 0x45d60 (catch @ 0x45ed7) | DONE | "COLL" check, size 0x20/0x43 bounds, col id stamp +0x31/+0x32, cleanup = FUN_10045f0f |
| (unnamed cleanup) | 0x45f0f | DONE (inlined in tail) | |
| CMvlVehicle::LoadAnimationFile | 0x45f50 | DONE | CCustomAnimFile load; "Loaded %d anims from IFP file for vehicle." |
| CMvlVehicle::LoadWeaponFiles | 0x46010 (catch @ 0x462fe) | DONE | weapons.txd + weapons.dff; catch destroys half-loaded txd (FUN_10032710) |
| IsSaDff (PROVISIONAL) | 0x46370 | DONE | `(chunkVersion - 0x34004U) < 0x2000U` on the clump chunk version |
| GetWeaponSoundSlot (PROVISIONAL) | 0x463f0 | DONE | 2 slots x 20B @ 0x2cf8; returns NULL unless first dword set |
| (SEH epilogue fns) | 0x45318/0x45341 | N/A | catch filters / security-cookie epilogues — ported as C++ try/catch |

### Vehicle XML cluster — implemented in `src/vehicle/vehicle_xml.cpp`

| Original name | RVA | Status | Notes |
|---|---|---|---|
| CMvlVehicle::LoadXMLBasic | 0x46420 | DONE | 12-entry table; unknown elements ignored; first-child rule; car/plane/bike gates |
| CMvlVehicle::LoadXMLAIData | 0x46960 | DONE | class(0-10,0xff)/freq/level/gang; 'gamg' typo kept |
| CMvlVehicle::LoadXMLCarCol | 0x46e90 | DONE | carcol/carcol4/rgbcol/rgbcol4; 16 entries; palette 0x939228 + 0xff->0xfe clamp |
| CMvlVehicle::LoadXMLAudioBoatEngine | 0x474d0 | DONE | type/basevolume/basefrequency/volumeincrease/frequencyincrease; range msgs |
| CMvlVehicle::LoadXMLAudio | 0x478a0 | DONE | 7 scalars w/ ranges + boatengine sub-sector; boat defaults msg |
| CMvlVehicle::LoadXMLHandlingTraction | 0x47c40 | DONE | multiplier/loss/bias -> 0x48/0xbc/0xc0 |
| CMvlVehicle::LoadXMLHandlingTransmission | 0x47f10 | DONE | missing-msg quirk ('multiplier'/'loss'/'bias') kept; acceleration x DAT_1008ddfc |
| CMvlVehicle::LoadXMLHandlingBrakes | 0x483b0 | DONE | deceleration/bias/abs(0/1) |
| CMvlVehicle::LoadXMLHandlingBoatBrakes | 0x486c0 | DONE | shares brake slots; pre-clears abs byte |
| CMvlVehicle::LoadXMLHandlingSuspension | 0x488d0 | DONE | 6 floats -> 0xc8..0xdc |
| CMvlVehicle::LoadXMLHandlingBoatSuspension | 0x48e20 | DONE | same slots, boat names |
| CMvlVehicle::LoadXMLHandlingBoatSteering | 0x49370 | DONE | shares traction slots (sectors exclusive) |
| CMvlVehicle::LoadXMLHandlingLights | 0x49640 | DONE | front/rear + hasboxes ('lightbox' quirk in all its error strings) |
| CMvlVehicle::LoadXMLHandling | 0x49930 | DONE | scalars + dimensions/centreofmass + sub-sector dispatch w/ boat gates |
| CMvlVehicle::LoadXMLAeroHandling | 0x4a610 | DONE | 12 scalars + turnres/speedres vecs; 'pitch' has NO dupe check (quirk) |
| CMvlVehicle::LoadXMLBikeHandling | 0x4b1a0 | DONE | 15 scalars; 4 stored deg->rad; 'speedsteer' NO dupe check (quirk) |
| CMvlVehicle::LoadXMLBoatHandling | 0x4bee0 | DONE | 8 scalars + moveres/turnres vecs |
| CMvlVehicle::LoadXMLHeliData | 0x4c700 | DONE | helitype 0-3, weapons 0-2, rotorradius/rotordammult |
| CMvlVehicle::LoadXMLHeliWepsData | 0x4cb80 | DONE | machinegunpos/missilepos vecs + present flags |
| CMvlVehicle::LoadXMLSpecialData | 0x4ccd0 | DONE (dispatch) | emlights/emlightsingle/taxilight -> CVehicleHandler HOOKs (0x4e8f0/0x4edc0/0x4f090); heliweps |
| CMvlVehicle::LoadXMLWeaponKeyData | 0x4cec0 | DONE | 5 x "%u,%u,%u" -> 3 bytes each @ w+0xac |
| CMvlVehicle::LoadXMLWeaponMissileInfo | 0x4d0d0 | DONE | gravity on/off, timeout, addspeedmul |
| CMvlVehicle::LoadXMLWeaponMachineGunInfo | 0x4d380 | DONE | range/accuracy(clamp 0..45 deg->rad)/timeout(0->25)/sound; 'missileinfo' quirk |
| CMvlVehicle::LoadXMLWeaponDockedInfo | 0x4d610 | DONE | dockspeed/position/rotation(euler->quat); short parse-msg quirk |
| CMvlVehicle::LoadXMLWeaponArmedInfo | 0x4d890 | DONE | minpos/minrot required; max* default to min*; mid-rot + matrix hooks |
| CMvlVehicle::LoadXMLExtraWeaponData | 0x4dd20 | DONE | weapon slot defaults; type/model enums; custom1-4 loaded-flag checks |
| CMvlVehicle::LoadXMLWeaponList | 0x4e430 | DONE | max 4 weapons |
| CMvlVehicle::LoadXMLSectionVehicle | 0x4e4f0 | DONE | 11 sections; unknown sections ignored; boat/plane-heli/bike required sectors |
| CMvlVehicle::LoadXMLFile | 0x4e790 | DONE | TinyXML parse from memory; root must be `vehicle` |
| (shared helpers) | 0x1baf0/0x1bb40/0x1bc10/0x1bbb0 | DONE (as explicit dispatch) | tag-table engine reimplemented per-function with identical messages |
| (vector helper) | 0x1bdb0 | DONE | MvlParseVector |
| (type-name lookup) | 0x1ba00 | DONE (names PROVISIONAL) | MvlLookupVehicleType |

### Out of scope (this lane) — left as HOOK POINTs

| Symbol | RVA | Reason |
|---|---|---|
| CVehicleHandler::LoadXMLEmLightData | 0x4e8f0 | CVehicleHandler lane (range 0x4e8f0-0x4e900 is 1:1 the EmLights trio); called from LoadXMLSpecialData |
| (emlightsingle) | 0x4edc0 | same |
| (taxilight) | 0x4f090 | same |
| FUN_1004e8f0 tail (rest of EmLights) | 0x4e900+ | outside provided decomp range |

## 2. Struct layout assumptions (x86, absolute offsets in original CMvlVehicle)

Full map in `src/archive/vehicle_archive.h`. Key assumptions:

- **CMvlVehicle header** @ 0x00-0x18: id(0), eVehicleType(4), isSA(8),
  comprules(9 -> actually 0x0c), extraflags(0x10), SMvlVehicleInfo*(0x14),
  id copy(0x18). Type values pinned to GTA eVehicleType
  (0 car/1 boat/2 train/3 heli/4 plane/5 bike) by the decomp gates
  (type==4 plane lodmodel, type==5 bike wheelrotangle, type==1 boat).
- **SMvlVehicleHandling** @ 0x1c-0x1bc: single block parsed by the
  LoadXMLHandling* family. Gaps at 0x20-0x27, 0x4c-0x93, 0xa0-0xab unrecovered.
  Traction slots (0x48/0xbc/0xc0) are SHARED with boatsteering; brake slots
  (0xac-0xb4) SHARED with boatbrakes; suspension slots (0xc8-0xdc) SHARED
  with boatsuspension — sectors are mutually exclusive in the schema so the
  original overwrites are safe (and mirrored here).
- **handling/flags** is written twice (dword @ 0xe4 + low byte @ 0x97).
- **handling/percentsubmerged** stored as parsed INT dword @ 0x40 (the float
  conversion happens at handling-apply time, outside this cluster).
- **SMvlVehicleWeaponData** = 0xC0 bytes, 4 entries @ 0x2864, count @ 0x2860.
  Fully recovered (see struct comment): type/model @ 0/4, missile info
  0x8-0x14, machinegun info 0x14-0x24, docked block 0x24-0x44, armed block
  0x44-0x94 (+derived 0x84/0x94), fireoffset 0xA0, hotkeys 0xAC (5 x 4B).
- **SMvlVehicleAudioData**: 6 sample-id ints. Exact base NOT recoverable
  from this decomp (decompiler left the dest base as an opaque EDX value).
  Placement assumed immediately before the known lightbox byte @ 0x2bcc and
  boatengine block @ 0x2bd0 (only gap consistent with neighbours). TODO:
  confirm against a wider decomp of FUN_100478a0's caller frame.
- **doorsounds** is a byte @ 0x1d4 (outside the audio block — decomp copies
  it via a separate byte store).
- **EM light data** block @ 0x27f4 (size >= 0x44; the following field
  @ 0x2838 is outside it) — parsed by the out-of-scope CVehicleHandler
  functions; kept as raw bytes.
- **basic table slot i=10** -> float @ 0x2838, PRE-DEFAULTED before XML
  parse to DAT_1008de28 (assumed 1.0f) or DAT_1008de40 for SA models.
  Element name NOT recoverable (table is .rdata). Left unhandled (unknown
  basic elements are ignored by the original parser anyway).
- **basic table slot i=3** -> info+0x8c: assumed **immunity** (bitfield,
  flags.html: 1 bullet/2 fire/4 explosion/8 collision/16 melee/64 tiredamp/
  128 flipdamp; sample value 5 seen).
- **basic table slot i=11** -> @ 0x2bec: assumed **maxpassengers** (int;
  samples: baggage 0, coach 8, firetrk 2, hunter 1, pizzabo 0).
  These two assignments are PROVISIONAL (bit indices only pinned for
  i=4..9 = wheelmodel/wheelscale/lodmodel/wheelrotangle/comprules/extraflags).
- **SMvlVehicleInfo** (game model-info wrapper, ptr @ 0x14): only the fields
  touched by this lane are typed (0x1b col-from-file flag, 0x1c colmodel,
  0x28 has-clump, 0x3c type, 0x40 wheelscale, 0x44 wheelmodel, 0x46 id,
  0x48 comprules byte, 0x49 aiclass, 0x4a level, 0x4c freq, 0x8c immunity,
  0x90 wheelrotangle, 0x144/0x14c/0x154 carcol-init flags, 0x170 special
  model id). Virtual slots used: +0x18 set-anims-name, +0x1c post hook,
  +0x24 install clump.
- **col model** direct-offset reads: bbox at +0x10..0x24, id stamp +0x31/32,
  field +0x0c mirrored to CMvlVehicle @ 0x2d28, col-from-file flag info+0x1b.

## 3. Verbatim quirks kept 1:1

- LoadXMLAIData: `'aidata' sector contains multiple 'gamg' elements.` (typo).
- LoadXMLAIData 'gang' parse-failure message names `'level'` (shared label
  in the decomp — kept).
- LoadXMLHandlingLights: element `hasboxes` but all messages say `lightbox`.
- LoadXMLHandlingTransmission missing-messages name `multiplier`/`loss`/`bias`.
- LoadXMLWeaponMachineGunInfo unknown-element message says `'missileinfo'`.
- LoadXMLExtraWeaponData fireoffset dupe-message says `'heliweps'`; model
  error lists `'rlauncher'` while the accepted value is `rocketlauncher`.
- LoadXMLWeaponDockedInfo/ArmedInfo parse messages drop the trailing
  ` element` ("Failed to parse the contents of 'dockspeed'.").
- AeroHandling `pitch` and BikeHandling `speedsteer` have no duplicate
  checks (last occurrence wins); `dimensions` likewise (only `centreofmass`
  has a dupe check).
- LoadXMLAudioBoatEngine unknown-element message names `'audio'`.
- `custom1..4` model error strings use the `'model' set to 'customN', which
  is not loaded.` pattern (checked against flags @ 0x2b68-0x2b74).

## 4. Unportable / deferred items

1. **EmLights trio (0x4e8f0/0x4edc0/0x4f090)** — CVehicleHandler methods,
   outside the assigned 0x46420-0x4e790 cluster. Dispatch ported; parsers
   are HOOK POINT stubs writing into m_abEmLightData.
2. **Engine internals** (RW stream/clump/txd, col pool, anim files, sound
   handler, vehicle registry, palette table, quat/matrix math) — HOOK POINT
   stubs with target VAs in `vehicle_archive.cpp`; real behaviour belongs to
   the engine lane. Stubs log and return benign values; nothing faked.
3. **Thrown-exception literals in LoadFromArchive** — the decomp shows
   `FUN_1001b8d0(local,"%s")` with the literal passed in a dropped register.
   Stage-name strings are used instead ("ParseArchive failed" etc.).
4. **FUN_1001bc10 range-error message (LoadXMLAudio ranges)** — the shared
   table-helper strings live outside this decomp range. Reconstructed as
   `The value of '%s' element is out of range %u-%u.` (closest recovered
   sibling: the AudioBoatEngine range messages). Marked in code.
5. **DAT_1008ddfc (acceleration scale), DAT_1008de40 (SA default for 0x2838),
   DAT_1008de28/DAT_1008ded4 constants** — float immediates in .data, not in
   the decomp text. Assumed: 1008ddfc = 1.0f (identity, TODO), 1008de28 =
   1.0f, 1008ded4 = 45.0f (accuracy clamp; matches "0-45" in the sample XML
   comment). 1008de40 unknown (assumed 2.0f). Confirm from binary .data.
6. **Slot i=10 element name** (basic) — unknown; unhandled.

## 5. Open questions

1. Exact base offset of the 6 audio sample-id ints (see §2). Neighbour
   constraints give a 0x2bb4-0x2bc8 window; needs a wider decomp frame.
2. i=3/i=11 basic-table name assignment (immunity vs maxpassengers) — bit
   positions for i=4..9 are pinned, 0-3/10-11 are not. Verify against the
   PTR_DAT_100af138 table data in the binary.
3. i=10's element name and the exact semantics of the 0x2838 float
   (pre-defaulted 1.0f / SA variant before XML parse).
4. comprules parse radix: samples contain `12272` (decimal or hex 0x12272?).
   Currently %i (decimal). extraflags is hex (%x) — proven by flags.html bit
   mapping (`20000` = AI_TAXI).
5. Type-name strings for MvlLookupVehicleType (FUN_1001ba00 table). Values
   are pinned (eVehicleType numbering); text labels assumed
   car/boat/train/heli/plane/bike.
6. Anim-group table (0x6857b0) row layout beyond the name pointer (+0) and
   stride 24 — only the index matters here, table is a hook.
7. Whether `MvlHandlerApplyGang` (FUN_1004f820) really takes the gang value
   (decomp passes this+0x2cf4 = m_nGang) — plausible but odd; confirm in the
   CVehicleHandler lane.
 8. The LoadFromArchive registry writes (DAT_100a6314[id*4] = info,
    DAT_100a631c + 8 + id*0x14 = 1) — model-id/flags tables owned by the
    engine lane; shapes taken from the decomp index arithmetic.

---

# SAVE SYSTEM (Lane V follow-up) — `src/vehicle/vehicle_handler.{h,cpp}`

Sources: `.harness-memory/limout/savewpn/range-decomp.txt` (151 fns,
0x50a30–0x58a00) — this lane extracted the save subset only
(0x50a30–0x51900); the SCM (0x51e20+) and weapons (0x5235a+) ranges in the
dump belong to the next lane. **Schema recovery**: the save-XML element
names live in the DLL's own parser tables, which were read directly from
`original_files/maxovehmod098e/mvl/vehmod_retail.dll` (PE walk, READ-ONLY):
tables at RVA 0xaf3c8 (identity), 0xaf410 (parts), 0xaf440 (vehicleslist),
0xaf470 (vehicles), 0xaf4b8 (root), 0xaf530 (vehicleinfo). Each row is
6 dwords: {nameptr, typecode, dest, required, ?, repeatable}; typecodes
recovered: 0x69 'i' = %i int, 0x58 'X' = %x hex, 0x6d 'm' = sector,
0x4d 'M' = string (0x75 'u' = %u was seen in the XML lane).

## Function inventory

| Original name | RVA | Status | Notes |
|---|---|---|---|
| CVehicleHandler::LoadSaveXmlVehicleInfoParts | 0x50a30 | DONE | `<partindex>` repeatable int, max 20, overflow warn verbatim |
| CVehicleHandler::LoadSaveXmlVehicleInfoIdentity | 0x50af0 | DONE | `<archivehash>` hex + `<archivename>` string (both required) -> vehicle match |
| (provisional) FindLoadedVehicleForIdentity | 0x51480 | DONE | registry pass 1 by hash == totalSize @ vehicle+0x330, pass 2 by path stricmp @ +0x22c |
| CVehicleHandler::LoadSaveXmlVehicleInfo | 0x50bd0 | DONE | slot/identity/tuningparts; pool vs garage slot validation + map dup checks; 0x58 record alloc |
| CVehicleHandler::LoadSaveXmlVehiclesList | 0x50f20 | DONE | `<vehicle>` -> insert-or-assign into slot map (pool/garage) |
| CVehicleHandler::LoadSaveXmlVehicles | 0x51060 | DONE | `<pool>` / `<garage>` sub-lists; failures logged, loop continues |
| CVehicleHandler::LoadSaveXmlRoot | 0x51150 | DONE | `<vehicles>` -> Vehicles; `<playerweps>` -> CWeaponHandler (stub, next lane) |
| CVehicleHandler::LoadSaveXmlDocument | 0x51240 | DONE | TinyXML load; root `mvlsave`; TIXML_ERROR_OPENING_FILE (2) = optional-file success |
| CVehicleHandler::LoadExtraSaveFile | 0x51340 | DONE | clear lanes, sidecar = save path minus ".b" + "_mvl.xml", 20000000-unit staleness check |
| (provisional) ClearSaveXmlRecords | 0x51720 | DONE | free all records + clear both slot maps |
| CVehicleHandler::FetchRealModelIndexForPoolSlot | 0x51500 | DONE | pool map lookup, model-type check, fallback DAT_1013dcf8 |
| CVehicleHandler::FetchRealModelIndexForGarageSlot | 0x51620 | DONE | garage map lookup, 0x50-byte part copy to DAT_100b3008, fallback DAT_1013dce4 |

### Depth callees of the above that are NOT in this lane (stubbed with VA)

| Symbol | RVA | Reason |
|---|---|---|
| CWeaponHandler::LoadSaveXmlPlayerWeapons | 0x567a0 | weapons lane (next); called from Root `<playerweps>` |
| CWeaponHandler save-clear (zeros DAT_10117960..0x101179a8) | 0x56d40 | weapons lane; called from LoadExtraSaveFile |
| FUN_100515b0 / 0x516e0 / 0x518b0 / 0x51900 | 0x515b0–0x51900 | save-APPLY helpers (garage copy / type map), called from OUTSIDE this dump; observed but not reached by the load chain — next lane or apply-flow lane |
| FUN_10051cb0 (code patcher) / 0x51db0 (vsprintf helper) / 0x51dd0 (str compare) | 0x51cb0–0x51dd0 | engine-patch / weapons-save helpers; no load-chain callers |
| thunk @ 0x51470 | 0x51470 | tail jump to CWeaponHandler::ApplySaveData @ 0x56db0 |

## Schema recovered (verbatim table names)

```xml
<mvlsave>                                  <!-- LoadSaveXmlDocument 0x51240 -->
  <vehicles>                               <!-- LoadSaveXmlRoot     0x51150 -->
    <pool>                                 <!-- LoadSaveXmlVehicles 0x51060 (garage=0) -->
      <vehicle>                            <!-- VehiclesList        0x50f20 -->
        <slot>N</slot>                     <!-- VehicleInfo         0x50bd0, int %i -->
        <identity>                         <!-- required -->
          <archivehash>HEX</archivehash>   <!-- %x = archive total size (same key as ParseArchive dup check) -->
          <archivename>file.7z</archivename>
        </identity>
        <tuningparts>                      <!-- optional -->
          <partindex>N</partindex> ×20     <!-- Parts               0x50a30 -->
        </tuningparts>
      </vehicle>
    </pool>
    <garage> …same… </garage>              <!-- garage slots 0..47 -->
  </vehicles>
  <playerweps>…</playerweps>               <!-- CWeaponHandler, NEXT LANE -->
</mvlsave>
```

Sidecar file: `"<save path minus trailing '.b'>_mvl.xml"` (VC saves end in
".b"); staleness rule ported literally: skip when
`sidecarTime + 20000000 < mainTime`.

## Save record layout (0x58 bytes, FUN_10059d7f alloc)

| Offset | Field |
|---|---|
| 0x00 | slot index (pool slot or garage 0..47) |
| 0x04 | MVL model index (CMvlVehicle::m_nId of the identity-matched vehicle; decomp derefs the identity result before store) |
| 0x08..0x57 | 20 tuning-part ints (garage fetch memcpy's this 0x50-byte block to DAT_100b3008 + slot*0x50) |

Slot maps: original globals DAT_101244d0 (pool) / DAT_101244c4 (garage),
MSVC std::map (node[4]=key, node[5]=record) — modelled as
`std::map<int, SMvlSaveVehicleRecord*>` class statics.

## Verbatim quirks kept 1:1

- LoadSaveXmlVehicleInfo pool range failure says "Slot index for **garage**
  vehicles must be between 0 and %d." (original copy/paste bug).
- FetchRealModelIndexForGarageSlot failure message says "**pool** slot %d"
  (original copy/paste bug).

## Save open questions

1. The pool range limit `%d` arg (from gta-vc.exe `*(DAT_00a0fde4+8)`) — the
   hook returns 0x30 as a benign stand-in; real value is the game's vehicle
   pool size at runtime.
2. The garage-store index in FetchRealModelIndexForGarageSlot: the decomp's
   EDX is lost (`&DAT_100b3008 + EDX*0x50`); assumed the current garage slot
   (DAT_1013dd0c). FUN_10051900 (out of scope) uses the same store, so the
   apply-flow lane should confirm.
3. Timestamp unit of FUN_1001c040 (64-bit): 100ns vs seconds changes the
   20000000 tolerance meaning (2 s vs ~231 days). Constant ported literally.
4. FindLoadedVehicleForIdentity null-guards the archive-name stricmp (the
   original would pass DAT_101247b8 straight to stricmp; required-checks make
   NULL unreachable in practice — guard added per safety rules).
5. The `<playerweps>` sub-tree shape (weapon records + `identity` strings at
   FUN_10056880) is weapons-lane scope.
6. Required/dup messages for the table-driven checks (e.g. missing slot /
   duplicate archivehash) are RECONSTRUCTED from the recovered sibling
   pattern — the table helper's own strings are outside the decomp range.
