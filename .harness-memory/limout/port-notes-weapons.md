# port-notes-weapons.md — TRANCHE-4 weapons port (Lane V follow-up 2)

Source of truth: `.harness-memory/limout/savewpn/range-decomp.txt` (151 fns,
VA 0x50a30–0x58a00). Save subset (0x50a30–0x51720) was ported by the
vehicle_handler lane; this tranche covers the WEAPONS subset (0x52150–0x58990
plus its depth callees). Element names cross-checked against
`original_files/maxovehmod098e/mvl/help/samplewepxmls/` (35 samples) and
`hats/` (hat.xml + readme.txt). Names follow the binding 1:1 policy; the
RTTI census names (re-098e-map.md) were upgraded by the decomp's own error
strings where they differ (e.g. SectionWeapon → LoadXMLSectionWeapon).

## 1. Function inventory

### CMvlWeapon — `src/weapons/mvl_weapon.{h,cpp}`

| Original name | RVA | Status | Notes |
|---|---|---|---|
| CMvlWeapon::LoadFromArchive | 0x52150 (catch @ 0x5235a) | DONE | stage chain ParseArchive→XML→icon→scope→txd→dff→anim→stock-audio fill; catch logs "CMvlWeapon::LoadFromArchive" "%s" |
| CMvlWeapon::ParseArchive | 0x523b0 | DONE | CSevenWalker; totalSize @ 0x190 = save hash |
| CMvlWeapon::RegisterArchiveFile | 0x52540 | DONE | .xml/.dff/.txd (single each), .wav→sound slot @0x30, icon.png, scope.png family (flavor flags 0xf8/0xf9), .ifp; "Another … file is already registered." verbatim |
| CMvlWeapon::LoadTextureFile | 0x52780 (catch @ 0x528d1) | DONE | .txd → texdict @ 0x108; frees buffer (tail 0x5290f) |
| CMvlWeapon::LoadModelFile | 0x52960 (catch @ 0x52b2b) | DONE | clump → first atomic @ 0x10c ("Clump contains no atomics."); modelidx/-2 reset -1; tail 0x52b75 |
| MvlWeaponLoadImageTexture (free) | 0x52be0 (catch @ 0x52d0e) | DONE | PNG stream → RwImage → texture; frees buffer (tail 0x52d68) |
| CMvlWeapon::LoadIconFile | 0x52dc0 | DONE | icon.png → texture @ 0xf0 |
| CMvlWeapon::LoadScopeFile | 0x52e00 | DONE | scope.png → texture @ 0xf4 |
| CMvlWeapon::LoadAnimFile | 0x52e40 | DONE | CCustomAnimFile @ 0x110; "Loaded %d anims from IFP file for weapon." |
| (unnamed) anim lookup | 0x52f00 | OBSERVED | animation lookup via CCustomAnimFile; not called by the load chain in this dump — not ported |
| CMvlWeapon::LoadXMLFile | 0x52f20 | DONE | TinyXML; root "weapon"; frees xml buffer |
| CMvlWeapon::LoadXMLSectionWeapon | 0x53060 | DONE | basic(req)/properties(req)/spawnlist/audio/shotgun/logic-block; "Could not load '%s' sector." |
| CMvlWeapon::LoadXMLSectionBasic | 0x53390 | DONE | name→0x150 ("Weapon has no name."→"Unknown"); logicalid→0x88; 80→Spike, 81→Hat, >35→0 |
| CMvlWeapon::LoadXMLSectionProperties | 0x53620 | DONE | all 23 elements; weaponslot clamp 0..9; 7 anim fields ÷ kAnimFrameScale (TODO: DAT_1008dec0) |
| CMvlWeapon::LoadXMLSectionShotgun | 0x53cd0 | DONE | fragments clamp 1..100, spread; flag @ 0xfa |
| CMvlWeapon::ResetPropertiesDefaults | 0x53e10 | DONE | type=instant_hit, animgroup 7, modelidx -1/-1, flags 0x3b |
| CMvlWeapon::Release | 0x53f10 | DONE | buffers→txd/atomic/textures→logic dtor→name→defaults |
| MvlWeaponGetModelAtomic (free) | 0x54110 | HOOK | model store DAT_100a6314[id*4]->+0x28 |
| accessors Get*/GetSoundSlot/GetShotAudio | 0x54010–0x541b0 | DONE | offsets 0x30/0x80/0x84/0x88/0x150/0x190/0x194/0x114 |

### CWeaponLogic* (logic objects) — `mvl_weapon.*`

| Original name | RVA | Status | Notes |
|---|---|---|---|
| CWeaponLogic ctor/base vtable | 0x541c0–0x54250 | DONE | GetLogicalId 0x54220, block-name "" 0x54240, ParseXmlBlock no-op 0x54250 |
| CWeaponLogicSpike ctor | 0x545c0 | DONE | logicalid 80; vtable-only (8-byte) object |
| CWeaponLogicHat ctor | 0x54260 | DONE | logicalid 81; 0x24: scale@8=1.0, offset@0xc, rotation@0x18 |
| CWeaponLogicHat::GetXmlBlockName | 0x54390 | DONE | returns "hatinfo" |
| CWeaponLogicHat::ParseXmlBlock | 0x543a0 | DONE | isrelative/offset/rotation/scale; relative → frame transform (HOOK 0x8ee0), absolute → zero |

### CWeaponHandler — `src/weapons/weapon_handler.{h,cpp}`

| Original name | RVA | Status | Notes |
|---|---|---|---|
| CWeaponHandler::LoadXMLSectionWeaponAudio | 0x54910 | DONE | gunshot(req)/reload(req)/echo → out[0..2] |
| CWeaponHandler::LoadXMLSectionWeaponShotAudio | 0x54ae0 | DONE | 8 elements → SMvlShotAudio (0x14); vol/voldisp clamp 127; loopcount forced 1 if sample 0 |
| (unnamed) GetWeaponById | 0x54890 | DONE (as GetWeaponById) | id 100..1099 + self-id validation |
| (unnamed) IsValidWeaponId | 0x54850 | DONE | 0..36 stock or registered |
| (unnamed) GetWeaponBySlot | 0x54870 | DONE | slot view <1000 |
| (unnamed) GetWeaponDisplayName | 0x548c0 | DONE | custom name else stock table (HOOK) |
| (unnamed) FindFirstFreeSlot | 0x550a0 | DONE | 0..999, -1 full |
| (unnamed) GetSlotFromFileName | 0x55040 | DONE | "w<N>_" → N; else -1 |
| CWeaponHandler::GetNewSlotForArchive | 0x550c0 | DONE | named slot/free fallback; both warnings verbatim |
| CWeaponHandler::LoadWeaponArchives | 0x55140 | DONE | 2-pass FindFirstFileA mvl\weapons\*.7z; frees failed weapons; ends with pickups.xml load |
| CWeaponHandler::LoadSaveXmlWeaponInfoIdentity | 0x56560 | DONE | archivehash+archivename → FindWeaponForIdentity |
| CWeaponHandler::LoadSaveXmlPlayerWeaponInfo | 0x56640 | DONE | identity(req)+ammo(1..100000) → save slot by weaponslot; collision warning verbatim |
| CWeaponHandler::LoadSaveXmlPlayerWeapons | 0x567a0 | DONE | <weapon> list; free-fn entry matches vehicle_handler decls |
| (provisional) FindWeaponForIdentity | 0x56cb0 | DONE | hash pass then path pass |
| CWeaponHandlerClearSaveData | 0x56d40 | DONE | zeros the 10 player-save slots |
| CWeaponHandler::DumpWeaponXML | 0x57320 | DONE (stock table HOOKED) | dir create, shift-key compression, verbatim format strings; stock iteration via hooks |
| CWeaponHandler::ApplySaveDataHooked | 0x56db0 | TODO(weap) | engine save-apply; out of scope (patch flow) |

### CWeaponPickups — `src/weapons/weapon_pickups.{h,cpp}`

| Original name | RVA | Status | Notes |
|---|---|---|---|
| CWeaponPickups::CreateWeaponPickup | 0x57d80 | PARTIAL (pool HOOKED) | validations + 3 error strings verbatim; pool/slot/object via hooks |
| CWeaponPickups::ProcessNewWeaponPickupOpcode | 0x584f0 | PARTIAL (SCM stream HOOKED) | opcode 0810; id validation + ammo clamp + result write verbatim |
| CWeaponPickups::LoadXMLSectionSpawnList | 0x58610 | DONE | <spawn> repeating; "Could not load %s section." |
| CWeaponPickups::LoadXMLSectionSpawn | 0x586d0 | DONE | wepid/position/ammo; weaponId>0 overrides wepid; both tail errors verbatim |
| CWeaponPickups::LoadXMLSectionPickups | 0x58880 | DONE | <spawnlist> sub-sectors |
| CWeaponPickups::LoadXMLPickupFile | 0x58990 | DONE | mvl\weapons\pickups.xml; root "pickups"; success message |
| (provisional) CreateAllRegisteredPickups | 0x58330 | DONE | "%d pickups registered, now creating them." |
| (reconstructed) AppendSpawn | 0x58ac0 | DONE | body is 0xc0 past the dump boundary; semantics fixed by consumers (0x58330/0x586d0) |
| opcode registration thunk | 0x58270 | OBSERVED | code-patcher; opcode hookup FUN_100238a0 @ 0x238a0 → hook |
| debug spawn-template printer | 0x583e0/0x58490 | OBSERVED | Ctrl+Shift+F12 dev helper; out of scope (strings captured) |
| slot scans / object build | 0x57b30/0x57b60/0x57c30 | HOOKED | game pickup pool internals (DAT_100a6b98 0x34-byte records) |

### Out-of-dump depth callees (stubbed with VA)

| Symbol | RVA | Reason |
|---|---|---|
| stock weapon audio table | 0x54e10 | engine data; fills empty audio slots in LoadFromArchive |
| stock weapon info table | 0x782a14 (rec 100B) + DAT_0069d99c names | DumpWeaponXML iteration |
| MvlGameRegisterWeapon | 0xcbb0 | game weapon-store insert after archive load |
| anim-group lookup | 0x3090 | same DAT_006857b0 table as the vehicle lane |
| hat frame transform | 0x8ee0 | relative hat fit |
| model store access | 0x8bc0 / DAT_100a6314 | MvlWeaponGetModelAtomic |
| weapon SCM patcher init | 0x57060 | engine code patch; not ported |

## 2. Layout assumptions (x86, absolute offsets)

### CMvlWeapon (0x1fb-byte object)
Buffers @0x00–0x14 (xml/dff/txd/icon/ifp/scope) + sizes @0x18–0x2c;
sound slots @0x30 (4×20B); slotIndex @0x80; weaponId @0x84; logicalid @0x88;
properties block @0x8c–0xec (type, 9 floats, offset[3], animgroup, 7 anim
floats, modelidx/-2, flags, weaponslot); icon/scope textures @0xf0/0xf4;
scope flags @0xf8/0xf9; shotgun flag/spread/fragments @0xfa/0xfc/0x100;
logic @0x104; txd @0x108; atomic @0x10c; animfile @0x110; audio[3] @0x114
(0x14 each); name[0x40] @0x150; totalSize @0x190; path[100] @0x194.

### SMvlShotAudio (0x14) — recovered from BOTH sides
Producer (0x54ae0 stores) and consumer (0x57320 field reads) agree:
+0x00 sample, +0x04 frequency, +0x08 displacement (default 1),
+0x0c distance (default 120.0), +0x10 loopcount (default 1),
+0x11 volume, +0x12 voldisplacement, +0x13 nospam (default true).

### CWeaponLogicHat (0x24): +0 vtable, +4 owner, +8 scale (1.0f),
+0xc offset[3], +0x18 rotation[3].

### Registry
One 1100-entry pointer array indexed by ABSOLUTE id (weapons 100..1099);
DAT_101179b0 is its slot-view base (id-100). Player save: 10 × (id, ammo)
pairs (DAT_10117960+i*8).

### Spawn record (0x14): id @0, position @+4..0xc, ammo @+0x10.

## 3. Verbatim quirks kept 1:1

- SectionWeapon: spawnlist/audio/shotgun failures log and CONTINUE;
  basic/properties/logic failures return early.
- LoadXMLSectionBasic: "Weapon has no name." (warn) → name "Unknown";
  <2-char names warn.
- SectionProperties: all 23 elements REQUIRED (table flags); unknown
  elements silently ignored.
- DumpWeaponXML compression: shift NOT down → omit audio values equal to
  the defaults (frequency 0 / displacement 1 / voldisplacement 0 /
  distance 120 / loopcount 1 / nospam 0).
- LoadWeaponArchives: pass 0 = "w<N>_" named archives (id N, slot N-100),
  pass 1 = unnamed (first free slot); both GetNewSlotForArchive warnings
  verbatim.
- Opcode 0810 ammo clamp 1..1000000; save-XML ammo clamp 1..100000.
- CreateWeaponPickup: "Game object pool %d/%d full, not creating pickup."
  fires when pool has ≤1 free slot; result = (counter<<10)|slot.

## 4. Open questions

1. **Logic-block table row (SectionWeapon 0x53060)**: the decomp writes the
   logic block name ("hatinfo") into the SHOTGUN row's name slot
   (`local_58 = vtable+0x18()`), which would make <hatinfo> parse via
   SectionShotgun and the hat ParseXmlBlock unreachable. The shipped
   feature (hats/readme.txt: `<hatinfo>` inside `<weapon>`) requires the
   dedicated logic row (dest → vtable+0x1c), so the clone binds the logic
   name to the logic row. Most likely a Ghidra stack-slot mixup between the
   two adjacent name fields. Needs binary confirmation.
2. **kAnimFrameScale (DAT_1008dec0)**: assumed 30.0 (30fps frame →
   seconds). Verify from .data.
3. **MVL_PICKUP_MIN_Z / spawn-height bump (DAT_1008dfc0 / DAT_1008de00)**:
   opcode-0810 ground clamp constants — assumed 0.0f / 1.0f. Verify.
4. **Stock weapon table** (0x782a14, 100-byte records): DumpWeaponXML and
   GetWeaponDisplayName iterate it; names via DAT_0069d99c, logicalid at
   rec+0x34, animgroup name via DAT_006857b0. All HOOKED.
5. **FUN_10057b30/0x57b60** slot scans (reuse vs new pickup slot) — engine
   pool internals; hooked as one MvlPickupFindSlot.
6. **Weapon archive failure detection**: LoadFromArchive's catch returns
   false (decomp caller reads cVar1); the clone matches via the bool
   return. The original's individual stage bools (LoadTextureFile etc.)
   always fall through to true in the decomp (void + throw/catch), so the
   clone returns true after logging inside the stage and lets the outer
   catch (in LoadFromArchive) fail the chain — same net behavior.
7. **FUN_10052f00** (0x52f00, CCustomAnimFile lookup) and **0x52392**
   (EH epilogue) observed but not part of the load chain — not ported.
8. **Weapon save-write side** (0x56880 identity/archivehash writer,
   0x56aa0 weapon+ammo, 0x56c00 playerweps root): the inverse XML dump for
   mvlsave files; not in this lane's scope list (load-only) — future lane.
9. **ApplySaveData (0x56db0)** and its patched call sites (0x57060
   weapon patcher) — engine save-apply; stubbed.

## 5. File inventory (this tranche)

| File | Purpose |
|---|---|
| src/weapons/mvl_weapon.h | CMvlWeapon + CWeaponLogic/Spike/Hat + SMvlShotAudio + weapon-lane hooks |
| src/weapons/mvl_weapon.cpp | CMvlWeapon pipeline + XML sections + logic classes + hook stubs |
| src/weapons/weapon_handler.h | CWeaponHandler + registry state + free-fn decls |
| src/weapons/weapon_handler.cpp | audio sections, registry, archives, save-XML load, DumpWeaponXML |
| src/weapons/weapon_pickups.h | CWeaponPickups + SMvlPickupSpawn + pool/SCM hooks |
| src/weapons/weapon_pickups.cpp | spawn XML chain, opcode 0810, CreateWeaponPickup |
| src/vehicle/vehicle_handler.{h,cpp} | STUB SYNC ONLY: two CWeaponHandler stub defs replaced by the real ones (decls kept; comments updated) |

## 6. Save-XML write side (ported — supersedes §1 row `ApplySaveDataHooked` and open question 8/9)

Counterpart of the load cluster (0x56560/0x56640/0x567a0/0x56cb0). The three
writers are **unnamed** in the binary (no strings, not in the RTTI census
row) — provisional names mirror the load family. `ApplySaveData` IS named
(string `"CWeaponHandler::ApplySaveData"` in the warn call). Everything below
was verified against the pristine DLL bytes (vehmod_retail.dll 0.98e) because
Ghidra's 2-param prototype for `FUN_10051db0` hides ALL vsprintf varargs in
the decomp.

| Original name | RVA | Status | Notes |
|---|---|---|---|
| FUN_10056880 (prov. WriteSaveXmlWeaponInfoIdentity) | 0x56880 | DONE | `<identity>` = `<archivehash>%08X</archivehash>` (FUN_10054170 = GetArchiveTotalSize) + `<archivename>%s</archivename>` (FUN_10054160 = GetArchivePath); NULL for unknown ids (FUN_10054890 miss) |
| FUN_10056aa0 (prov. WriteSaveXmlPlayerWeaponInfo) | 0x56aa0 | DONE | `<weapon>` = `<identity>` (calls 0x56880 with the same id) + `<ammo>%d</ammo>`; NULL for unknown ids |
| FUN_10056c00 (prov. WriteSaveXmlPlayerWeapons) | 0x56c00 | DONE (ped HOOKED) | `<playerweps>` root; 10 ped weapon slots; returns the root element |
| CWeaponHandler::ApplySaveData | 0x56db0 | DONE (engine HOOKED) | pending (id, ammo) pairs → ped give-weapon / collision warn; name string-confirmed |
| FUN_10051db0 (SaveSprintf helper) | 0x51db0 | DONE (static helper) | `_vsprintf_s(buf, 0x104, fmt, ...)` — the 260-byte `local_118` buffer |
| thunk_FUN_10056db0 | 0x51470 | OBSERVED | 5-byte `jmp` thunk — the game-side hook entry for ApplySaveData; not code |

### Binary-verified signature/layout corrections (bytes, not decomp)

1. **FUN_10056aa0 has TWO __fastcall params** — `id` in ECX, `ammo` in EDX.
   Prologue is `mov edi,edx` (fn+0x2e) and EDI is pushed as the `"%d"`
   vsprintf vararg before `call FUN_10051db0` (fn+0x4c). The decomp shows one
   param because FUN_10051db0's prototype is `(char*, char*)` and the vararg
   is invisible at every call site.
2. **FUN_10056c00's slot walk** (fn+0x70): `mov ecx,[esi]` = weapon id @
   slot+0x00, `mov edx,[esi+0x0c]` = ammo @ slot+0x0c (CWeapon.m_nAmmoTotal),
   `call FUN_10056aa0` — then `add esi,0x18` (CWeapon stride 0x18) × 10,
   `jle` skip when id ≤ 0. The ped base is `FUN_1002dfd0(param_1)` and the
   array starts at +0x408.
3. **FUN_10056880 sprintf args**: `call FUN_10054170` → `push eax` → `"%08X"`;
   `call FUN_10054160` → `push eax` → `"%s"` (fn+0x79/0x8b and fn+0x13e/0x150).
4. **FUN_10056db0 give call** (fn+0x41): `push 1 / push 0 / push 0 /
   push [esi+4] / push ecx(id) / mov ecx,ebx(ped) / call FUN_10030560` —
   give-weapon (ped, id, ammo, 0, 0, 1). Loop end compares the pending-pair
   pointer against `0x101179b0` (10 pairs × 8 bytes from DAT_10117960).

### Record shape (mvlsave sidecar — inverse of LoadSaveXmlPlayerWeapons)

```xml
<playerweps>
  <weapon>
    <identity>
      <archivehash>08X of GetArchiveTotalSize</archivehash>
      <archivename>GetArchivePath</archivename>
    </identity>
    <ammo>int</ammo>
  </weapon>
</playerweps>
```

Only slots whose type resolves through `GetWeaponById` (MVL ids 100..1099)
produce records — stock weapons stay in the vanilla save; the sidecar carries
MVL weapons only. `<ammo>` round-trips the load-side clamp (1..100000).

### ApplySaveData (0x56db0) — verified role

Applies the 10 pending `(weaponId, ammo)` pairs (DAT_10117960..0x101179b0,
written by `LoadSaveXmlPlayerWeaponInfo`, cleared by `ClearSaveData` 0x56d40)
to the player ped. Per slot i: empty ped slot (`ped+0x408+i*0x18` type < 1) →
`FUN_10030560(ped, id, ammo, 0, 0, 1)`; occupied → warn verbatim `"Cannot
apply weapon %s, slot collision."` under `"CWeaponHandler::ApplySaveData"`
with `FUN_10054150(FUN_10054890(id))` (the weapon name). Exactly the same
message the LOAD side warns with (0x56640). The old `ApplySaveDataHooked`
TODO is replaced by the real port.

### Engine seams added (weapon_handler.{h,cpp}) — HOOK POINT, perform NOTHING

| Seam | Original | Logs |
|---|---|---|
| `MvlGameGetPlayerPed` | FUN_1002dfd0 @ 0x2dfd0 | intended VA + ctx |
| `MvlGameGetPedWeaponSlot` | ped+0x408+slot*0x18 reads | intended VA + slot/ped |
| `MvlGameGiveWeapon` | FUN_10030560 @ 0x30560 | intended VA + id/ammo/flags |

With the seams unhooked the writers emit an empty `<playerweps/>` and
ApplySaveData gives nothing — no engine behavior is ever fabricated.

### Adjacent, out of this port's scope

- **FUN_10056e20 @ 0x56e20** (slot sanitizer: clamps ped weapon types > 0x24
  to 0 at DAT_1013dd90+0x408, stride 0x18 × 10) — save-boundary companion,
  not requested; not ported.
- **The save-write root** that calls FUN_10056c00 (vehicle-side `<vehicles>`
  writer of the `_mvl.xml` sidecar, cf. `LoadExtraSaveFile` 0x51340) lives
  outside this dump — vehicle lane's future write side.
