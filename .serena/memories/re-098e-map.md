# RE: vehmod_retail.dll 0.98e — Architectural Map

**Binary**: `original_files/maxovehmod098e/mvl/vehmod_retail.dll` (760,832 B, MD5 3db3d4b83617eab761293f6c5dbd35ea)
**Ghidra**: project `MVL_clone` @ `E:\dev(dave)\tools\ghidra-projects`, image base 0x10000000, entry 0x10005c1d1, 2922 functions, x86:LE:32.
**Extraction artifacts**: `.harness-memory/d3out/` (strings.txt, named-strings.txt, functions.txt, imports.txt, decomp-chain.txt, vcconsts.txt, mkfuncs-xrefs.txt, thunks.txt).
**Citation format**: `vehmod_retail.dll 0.98e @ RVA 0xXXXX` (RVA = Ghidra addr − 0x10000000).

## 1. HEADLINE FINDING — RTTI class names give us real symbols

The DLL retains **RTTI/exception type-descriptor strings** naming 136 functions directly
(`named-strings.txt`; string addr 1007xxxx → referencing function RVA in last column).
The original was built with RTTI and error messages embed `Class::Method` names.
**This collapses the biggest RE risk** — no blind FUN_xxxx guessing for the core.

Named classes (string addr → function RVA):
- `CMvlSettings` — global config + limits engine
- `CMvlVehicle` — per-vehicle archive/XML loader
- `CVehicleHandler` — vehicle archive manager + save system
- `CMvlWeapon`, `CWeaponHandler`, `CWeaponPickups`, `CWeaponLogicHat` — weapons
- `CVehicleSCM` — SCM opcode integration
- `CCustomAnimFile` — IFP loader (LoadVersionOne/Two)
- `CLongModelId`, `CModelIndexLimit` — model-ID/limit patch engine
- `CHelpers` (XmlParseVector, PngFileToTexture), `CPngFuncs` (libpng15 host)
- `CScmHandler`, `CDxHandler` (DX9TextureFix), `CSevenWalker` (7z), `CSoundHandler`

## 2. Init chain (decomp-chain.txt, 81 functions decompiled)

```
entry 0x10005c1d1 → ___DllMainCRTStartup (0x1005c1f4) → security_init_cookie
  → FUN_10043c00 (main init, depth-0) — the orchestrator
      ├→ FUN_1002baf0 = CMvlSettings::PrepareMvlForLoad (banner "----- MVL version 0.98e, compiled on %s -----\n" @ 0x1007f34c)
      ├→ FUN_10038160 = LoadGlobalVXMLFile (mvl\vehicles\globalv.xml)
      ├→ FUN_10033740 (xref via mkfuncs-xrefs)
      └→ hook installer FUN_10022560 (write hook at VC address) / FUN_10022590 (VirtualProtect variant)
           FUN_100225a0 = VirtualProtect wrapper (PUSH 401000h seen @ 100225afh — patches VC code region)
```
Hook stub cluster: 19 functions created at 0x1003dff0–0x1003e320 + 0x10040d40/0x10043010/0x10043730–0x10043780
(`mkfuncs-xrefs.txt` F-CREATE rows) — these are the MVL-side hook bodies referenced by installed patches.

## 3. Subsystem census (named functions → RVA)

### Config / settings (CMvlSettings)
| Function | RVA |
|---|---|
| LoadMXMLSectionInternal | 0x2a7f0 (also parses `windowed`, `autointeriors` tags) |
| LoadMXMLSectionLimitsInstances | 0x2ac20 (also `vehicles`, `colmodel` tags) |
| LoadMXMLSectionLimitsModelStore | 0x2ae60 |
| LoadMXMLSectionLimits | 0x2b030 |
| LoadMXMLSectionMiscData | 0x2b230 |
| LoadGlobalMXMLFile | 0x2b390 (mvl\globalm.xml) |
| ProcessPoolLimits | 0x2b4d0 |
| ProcessModelStoreLimits | 0x2b5a0 |
| ProcessIdLimits | 0x2b8f0 |
| ProcessStreamingMemLimit | 0x2ba50 |
| PrepareMvlForLoad | 0x2baf0 |
| HookPoolNewVehicle / HookPoolNewColModel | strings @ 0x1007ef64/0x1007f10c ("RAN OUT OF VEHICLE/COLMODEL SLOTS!!!") |

Global vehicle XML (free functions, not CMvl*): LoadGlobalXMLNoSpawn 0x37a90, LoadGlobalAISpecials 0x37b80,
LoadGlobalVehExtraFlags 0x37e70, LoadGlobalStockAISpecials 0x380b0, LoadGlobalVXMLFile 0x38160.
`idstart` validation: "has to be between 250 and MIN(65000, modelLimit - 100)" @ 0x10081260.

### Vehicle archive pipeline (CMvlVehicle / CVehicleHandler)
LoadFromArchive 0x45318 → ParseArchive 0x45360 → RegisterArchiveFile 0x455d0 (tuning1-9.dff, weapons.txd/dff)
→ VerifyRequiredFiles 0x45870 → LoadTextureFile 0x45a5a / LoadModelFile 0x45ccb / LoadCollisionFile 0x45ed7 /
LoadAnimationFile 0x45f50 / LoadWeaponFiles 0x462fe → InitialiseLoaded 0x44f20.
CVehicleHandler: GetNewSlotForArchive 0x4f4d0, LoadVehicleArchive 0x4f5a0, LoadVehicleArchives 0x4f680
(mvl\vehicles\*.7z), LoadSanAndreasVehiclesTxd 0x4f9a0 (mvl/vehicles/sa_vehicle.txd fallback models\generic\vehicle.txd),
AnalyzeStartIndex 0x4fc90.

### Vehicle XML parsers (CMvlVehicle::LoadXML*)
Basic 0x46420, AIData 0x46960, CarCol 0x46e90, AudioBoatEngine 0x474d0, Audio 0x478a0,
HandlingTraction 0x47c40, HandlingTransmission 0x47f10, HandlingBrakes 0x483b0, HandlingBoatBrakes 0x486c0,
HandlingSuspension 0x488d0, HandlingBoatSuspension 0x48e20, HandlingBoatSteering 0x49370, HandlingLights 0x49640,
Handling 0x49930, AeroHandling 0x4a610, BikeHandling 0x4b1a0, BoatHandling 0x4bee0, HeliData 0x4c700,
HeliWepsData 0x4cb80, SpecialData 0x4ccd0, WeaponKeyData 0x4cec0, WeaponMissileInfo 0x4d0d0,
WeaponMachineGunInfo 0x4d380, WeaponDockedInfo 0x4d610, WeaponArmedInfo 0x4d890, ExtraWeaponData 0x4dd20,
WeaponList 0x4e430, SectionVehicle 0x4e4f0, LoadXMLFile 0x4e790.
EmLights (CVehicleHandler): 0x4e8f0 / 0x4edc0 / 0x4f090.

### Save system (CVehicleHandler::LoadSaveXml*)
VehicleInfoParts 0x50a30, VehicleInfoIdentity 0x50af0, VehicleInfo 0x50bd0, VehiclesList 0x50f20,
Vehicles 0x51060, Root 0x51150, Document 0x51240, LoadExtraSaveFile 0x51340 (mvlsave/_mvl.xml sidecar,
"older than main save file" staleness check), FetchRealModelIndexForPoolSlot 0x51500 / ForGarageSlot 0x51620.

### Weapons (CMvlWeapon / CWeaponHandler / CWeaponPickups)
CMvlWeapon: LoadFromArchive 0x5235a, ParseArchive 0x523b0, RegisterArchiveFile 0x52540 (icon.png, scope.png,
full_scope.png, stretch_scope.png), LoadTextureFile 0x528d1, LoadModelFile 0x52b2b, LoadImageTexture 0x52d0e,
LoadIconFile 0x52dc0, LoadScopeFile 0x52e00, LoadAnimFile 0x52e40, LoadXMLFile 0x52f20,
SectionWeapon 0x53060, SectionBasic 0x53390, SectionProperties 0x53620, SectionShotgun 0x53cd0.
CWeaponLogicHat::ParseXmlBlock 0x543a0. CWeaponHandler: WeaponAudio 0x54910, ShotAudio 0x54ae0,
GetNewSlotForArchive 0x550c0, LoadWeaponArchives 0x55140 (mvl\weapons\*.7z), save-XML 0x56560–0x567a0,
ApplySaveData 0x56db0, DumpWeaponXML 0x57320 (mvl\wlist\%s.xml).
CWeaponPickups: CreateWeaponPickup 0x57d80, ProcessNewWeaponPickupOpcode 0x584f0,
spawn-list XML 0x58610–0x58990 (mvl\weapons\pickups.xml).

### SCM (CVehicleSCM / CScmHandler)
SetNextTextForVehicleId 0x51e20 (opcode 0800), ProcessNextTextOpcode 0x51eb0, CScmHandler::IgnoreOpcode 0x234c0.

### Python host (dynamic, FUN_100287c0)
python33.dll loaded via LoadLibraryA + GetProcAddress @ **0x287c0** (strings: PyImport_Import,
PyObject_IsTrue/CallObject/GetAttrString, PyImport_ImportModule, PyImport_AppendInittab @ 0x1007e3c8–0x1007e798;
"Couldn't load python interpreter." / "Python interpreter loaded.").
Scripts dir `mvl\scripts\` @ 0x1007e85c; module `mvlbase` (logPrint 0x25280, setHudMessage 0x254b0);
`mveh` module: createVehicle 0x27a70, getVehicleHealth 0x28000, setVehicleHealth 0x28080, blowVehicleUp 0x28110,
getPedVehicle 0x26990; `manim.initAnimType` 0x24110. Script error files `%s.err.txt` 0x2a170/0x2a1d0.

### 7z / PNG / audio / anims
CSevenWalker::LoadSeven 0x2bf00 (static LZMA SzArEx lineage per version-matrix).
CPngFuncs::LoadPngFuncs 0x1c3f0 (mvl/libpng15.dll LoadLibrary + "%d functions are missing"),
RtPNGImageStreamRead 0x1c690 (catch block ported in src/render/png_funcs.cpp), CHelpers::PngFileToTexture 0x1bfb3.
CSoundHandler: FillInfoFromName 0x338a0 (filename-encoded freq/loop), FillSampleBufferFromData 0x33a00,
LoadSoundFile 0x33e10. CCustomAnimFile::Load 0x1472/0x1595/0x16ed, LoadVersionOne 0x1ea4, LoadVersionTwo 0x25fb.

### Limits / model-ID engine (CModelIndexLimit / CLongModelId)
CLongModelId: RelocateEntityLevel 0x17170, RelocateEntityZone 0x171a0, ExtendModelIdMovsx 0x171d0,
ExtendModelIdOr 0x17220, ExtendTxdIndexMovsx 0x17250.
CModelIndexLimit: ReplaceStreamingBufferRefs 0x17410, ReplaceModelPointerBufferRefs 0x17510,
ReplaceTexStartIndexRefs 0x175f0, ReplaceColStartIndexRefs 0x17720, ReplaceAnimStartIndexRefs 0x177c0,
RegisterModelForLookup 0x179e0, VerifyModelHasColModel 0x17f60, VerifyDirectoryCanAdd 0x18030.
LoadSAColSection 0x388a0, LoadTuningFile 0x39ec0, GenerateDefaultXML 0x40df0 (mvl\vlist\%s.xml).

### Misc
CDxHandler::DX9TextureFix 0xd340. CHelpers::XmlParseVector 0x1bdb0. mvl\pathcache.bin 18b80/18c60.
mvl\errorlog.txt 1c180, mvl\messagelog.txt 1cad0, exdbg.txt 42a60/42b10, write.txt 2df00.
MvlSecWnd / "MVL camera view" 0x12170 (window class — testmenus/trashbin UI). mvl\bullethole.png f4b0,
mvl\img\seledge.png + dot.png 10ed0.

## 4. Retail vs steam divergence
Markers `mvl/dump640FE5h.txt` (retail) vs `mvl/dump63FF95h.txt` (steam) @ 0x1007bba4 — debug dump filenames
embed the differing VC address. Steam build = same code with retargeted exe addresses (per version-matrix.md).
→ diff the two DLLs' embedded 0x4xxxxx/0x5xxxxx/0x6xxxxx constants (vcconsts.txt has retail's 2,755 hits)
to enumerate the full game-address table.

## 5. Rejected hypotheses
- "RTTI strings are exception type descriptors only" — REJECTED: they appear in error/log message strings
  ("%s sector is missing…", function-name logging), giving direct string→function mapping.
- "python33.dll statically imported" — REJECTED: no import entry; dynamic LoadLibraryA @ 0x287c0 (matches D2's 0.98 finding).
- Guessed symbol greps (InitZipMod etc.) — REJECTED: decomp output has no symbols; use string refs.

## 6. Open items
- Enumerate ALL FUN_10022560/10022590 call sites in FUN_10043c00 + depth-1 callees → complete patch-site table (patch-sites.md).
- Map remaining ~2786 unnamed functions via string-xref clustering (strings.txt has xref columns).
- Decompile the 19 hook stubs (0x1003dff0–0x1003e320) to identify what each hooked VC site does.


## Census addenda (tranches 5-6, 2026-10-12)

Function identities recovered after the initial census; counted in the progress metric.

| RVA | Role | Status |
|---|---|---|
| 0x28cd0 | event registry (eventGameStart/eventFrame/eventBulletFired) | ported |
| 0x29310 | script scan (mvl\scripts) | ported |
| 0x29580 | CWD guard | ported |
| 0x23920 | SCM opcode 0801 handler | ported |
| 0x23940 | SCM opcode 0802 handler | ported |
| 0x1cce0 | MvlScmWarn warning logger | ported |
| 0x56880 | WriteSaveXmlWeaponInfoIdentity | ported |
| 0x56aa0 | WriteSaveXmlPlayerWeaponInfo | ported |
| 0x56c00 | WriteSaveXmlPlayerWeapons | ported |
| 0x51db0 | SaveSprintf helper | ported |
| 0x25a70 | mveh pool getter | ported |
| 0x25b00 | mveh pool-slot lookup | stub |
| 0x290d0 | event-handler map insert | stub |
| 0x27b90 | mveh.getTuningPartCount | ported |
| 0x27bf0 | mveh.getTuningPartName | ported |
| 0x27c50 | mveh.getTuningPartVarCount | ported |
| 0x27cb0 | mveh.getVehUsedTuningPart | ported |
| 0x27d50 | mveh.setVehUsedTuningPart | ported |
| 0x27de0 | mveh.getVehModelAIType | ported |
| 0x27f60 | mveh.setCarTaxiLightState | ported |
