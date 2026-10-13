# RE: Real function names recovered from MVL 0.98e loggers + string xrefs

**Binary**: `original_files/maxovehmod098e/mvl/vehmod_retail.dll` (0.98e, 760,832 B).
**Method**: the original MVL loggers take the *calling function's own name* as the first
`char*` argument —
`FUN_1001cb80(name, fmt, …)` error (`"%s: %s"`), `FUN_1001cce0(name, fmt, …)` warn
(`"Warning in %s: %s"`), `FUN_1001cc00(msg)` plain. Every self-logged call site therefore
embeds a **verbatim function-name string literal** in `.rdata`.
Cross-checked against `d3out/strings.txt` (`addr\txrefs\tfuncs\tstring\tref-funcs`) — a name
string referenced by exactly one function is that function passing its own name to the logger.

**Mined**: 851 logger calls with identifier-shaped first arg → 124 distinct name literals,
117 distinct caller RVAs self-identify. Loggers live at error 0x1cb80 / warn 0x1cce0 /
msg 0x1cc00 (RVA). No PE export directory (all-zero; DLL exports nothing). PDB path string
`C:\GamesX\GTA Vice City\mvl\vehmod_retail.pdb` @ file 0x9f438 (RSDS) — **no PDB shipped**,
no source-file path strings besides that.

Counts below. RTTI census (136 names, `re-098e-map.md`) is NOT re-derived; this file adds
the **logger/string-xref recoveries on top of it** and the no-evidence sweep.

---

## 1. HIGH-confidence recovered names
(HIGH = verbatim name string is the first logger argument, or the string's sole xref is the
function it names, sitting adjacent to that function's error message.)

### 1a. NEW free-function names — handling / heli / vehicle-create cluster (0x3c9b0–0x42350)
Each name string has exactly **one xref = the function itself**, and sits immediately after
that function's error message in `.rdata` (contiguous block 0x10081cb0–0x10081e6c):

| RVA | recovered name | evidence | current clone identifier |
|---|---|---|---|
| 0x3c9b0 | SetupBasicHandling | strings.txt `SetupBasicHandling` @ 10081cf8 xref=03c9b0; err "A vehicle with model %d was created, however it has not been loaded." @ 10081cb0 xref=03c9b0 | FUN_1003c9b0 (not in src/) |
| 0x3c9e0 | SetupBikeHandling | strings.txt @ 10081d0c xref=03c9e0 | FUN_1003c9e0 |
| 0x3ca40 | FixBikeAnimGroup | strings.txt @ 10081d20 xref=03ca40 | FUN_1003ca40 |
| 0x3ca90 | InAeroHandlingGetter | strings.txt @ 10081d34 xref=03ca90 | FUN_1003ca90 |
| 0x3cad0 | InBoatHandlingGetter | strings.txt @ 10081d4c xref=03cad0 | FUN_1003cad0 |
| 0x3cb10 | GetHandlingForNewCars | strings.txt @ 10081d94 xref=03cb10; err "Model %d used, however it has not been loaded." @ 10081d64 xref=03cb10 | FUN_1003cb10 |
| 0x3cb40 | ModelRequested | strings.txt @ 10081de4 xref=03cb40; err "Requested name for model %d, which has not been loaded." @ 10081dac xref=03cb40 | FUN_1003cb40 |
| 0x3cda0 | GetNewMaxPassengers | strings.txt @ 10081e24 xref=03cda0 | FUN_1003cda0 |
| 0x3cde0 | DetectFlyableBoats | strings.txt @ 10081e38 xref=03cde0 | FUN_1003cde0 |
| 0x3ce30 | **HeliGetType** | strings.txt @ 10081e4c xref=03ce30 **AND** logger literal `FUN_1003ce30("HeliGetType", …)` in missing33/decomp-chain.txt:2507 | FUN_1003ce30 (named only in xmlgen.cpp comment) |
| 0x3cea0 | **HeliGetWeaponType** | strings.txt @ 10081e58 xref=03cea0 **AND** logger literal missing33/decomp-chain.txt:2828 | FUN_1003cea0 (xmlgen.cpp comment only) |
| 0x3cf00 | **HeliGetRotorSettings** | strings.txt @ 10081e6c xref=03cf00 **AND** logger literal missing33/decomp-chain.txt:2856 | FUN_1003cf00 (xmlgen.cpp comment only) |
| 0x39b00 | SaveTuningAtomics | strings.txt @ 10081a88 xref=039b00 (adjacent to LoadTuningFile 0x39ec0 block) | FUN_10039b00 |
| 0x42350 | CreateVehicleNear | strings.txt @ 10083918 xref=042350 | FUN_10042350 |

Shared error string "Checks for model %d, which has not been loaded." @ 10081de4-region
xref=03ce30,03cea0,03cf00,03cda0,03cde0 — the five "Checks for model" getters, all named above.

### 1b. Class::Method names recovered for functions census only had partially
| RVA | recovered name | evidence | census had |
|---|---|---|---|
| 0x4e8f0 | CVehicleHandler::LoadXMLEmLightData | named-strings.txt @ 10085… + logger literal emlights/range-decomp.txt:103 | "EmLights 0x4e8f0" |
| 0x4edc0 | CVehicleHandler::LoadXMLSingleEmLightData | logger literal emlights/range-decomp.txt:301 | "…0x4edc0" |
| 0x4f090 | CVehicleHandler::LoadXMLTaxiEmLightData | logger literal emlights/range-decomp.txt:384 | "…0x4f090" |
| 0x54910 | CWeaponHandler::LoadXMLSectionWeaponAudio | strings.txt + logger literal savewpn:3850 | "WeaponAudio 0x54910" |
| 0x54ae0 | CWeaponHandler::LoadXMLSectionWeaponShotAudio | strings.txt @ 1008d338 xref=054ae0 | "ShotAudio 0x54ae0" |
| 0x56560 | CWeaponHandler::LoadSaveXmlWeaponInfoIdentity | strings.txt + logger savewpn:4810 | (only Write* 0x56880 known) |
| 0x56640 | CWeaponHandler::LoadSaveXmlPlayerWeaponInfo | strings.txt + logger savewpn:4847 | (only Write* 0x56aa0 known) |
| 0x567a0 | CWeaponHandler::LoadSaveXmlPlayerWeapons | strings.txt @ 1008d4f4 xref=0567a0 + logger savewpn:4920 | (only Write* 0x56c00 known) |
| 0x58610 | CWeaponPickups::LoadXMLSectionSpawnList | strings.txt @ 1008dbe8 xref=058610 + logger savewpn:6198 | "spawn-list 0x58610–0x58990" |
| 0x586d0 | CWeaponPickups::LoadXMLSectionSpawn | strings.txt + logger savewpn:6302 | range only |
| 0x58880 | CWeaponPickups::LoadXMLSectionPickups | strings.txt + logger savewpn:6362 | range only |
| 0x58990 | CWeaponPickups::LoadXMLPickupFile | strings.txt + logger d3out/decomp-chain.txt:3297 + savewpn:6423 | range only |

### 1c. Self-logged confirmations of the existing 136-name census (no change needed)
117 caller RVAs log their own census name verbatim — the logger pattern validates the RTTI
census end-to-end. Examples (all in `attrib.txt` / mined.json): every `CMvlVehicle::LoadXML*`
(0x46420–0x4e790, 27 fns), every `CMvlSettings::Process*/Load*` (0x2a7f0–0x2baf0),
`CLongModelId::*` (0x17170–0x17250), `CModelIndexLimit::Replace*/Verify*/Register*`
(0x17410–0x18030), `CSoundHandler::*` (0x338a0–0x33e10), `CMvlWeapon::*` (0x5235a–0x53cd0),
`CVehicleHandler::LoadSaveXml*` (0x50a30–0x51240), `CSevenWalker::LoadSeven` 0x2bf00,
`CDxHandler::DX9TextureFix` 0xd340, `CPngFuncs::LoadPngFuncs` 0x1c3f0,
`CCustomAnimFile::LoadVersionOne/Two` 0x1ea4/0x25fb, `CScmHandler::IgnoreOpcode` 0x234c0,
`CVehicleSCM::SetNextTextForVehicleId/ProcessNextTextOpcode` 0x51e20/0x51eb0.
Free-function census names also self-log: `LoadGlobalVXMLFile` 0x38160,
`LoadSAColSection` 0x388a0, `LoadTuningFile` 0x39ec0, `GenerateDefaultXML` 0x40df0,
`InitVehicles` 0x4f680 (alias of `CVehicleHandler::LoadVehicleArchives`, same RVA —
d3out/decomp-chain.txt:714+735 shows both literals in one function body).

---

## 2. MEDIUM-confidence recovered names

| RVA | recovered name | evidence | why MEDIUM |
|---|---|---|---|
| 0x42ec0 | InitVehicles (second site) | strings.txt `InitVehicles` @ 10083a38 xref=**042ec0,04f680**; shares the whole string cluster `mvl\vehicles\%s` / "Could not load vehicle from '%s'." with 0x4f680 | two functions reference the same name cluster; 0x42ec0 is likely an **older/duplicate copy** of LoadVehicleArchives (it also imports WaitForSingleObject/ReleaseMutex like 0x42090). Needs a decomp pass to confirm identity vs. mere caller. |
| 0x52be0 | CMvlWeapon::LoadImageTexture (helper) | logger literal `FUN_1001cb80("CMvlWeapon::LoadImageTexture","Buffer is not loaded…")` inside FUN_10052be0 (savewpn/range-decomp.txt:1886); named-strings.txt lists LoadImageTexture xref=**052d0e,052be0** | 0x52be0 is a **sub-function / split** of LoadImageTexture (0x52d0e is the primary); it logs the *parent's* name. Clone already comments it `MvlWeaponLoadImageTexture` (mvl_weapon.cpp:397). |
| 0x51470 | CWeaponHandler::ApplySaveData (game-side thunk) | savewpn/range-decomp.txt:602 `FUN_1001cce0("CWeaponHandler::ApplySaveData",…)` sits in `thunk_FUN_10056db0` | it's the **installed hook thunk** to ApplySaveData 0x56db0, not a distinct function; clone notes it (weapon_handler.cpp:851). |
| 0x1c9aa / 0x1c690 | CPngFuncs::RtPNGImageStreamRead | named-strings.txt xref=**01c9aa,01c690**; clone comment png_funcs.cpp:209 "catch filter 0.98e @ RVA 0x1c9aa" | two entries: 0x1c690 main reader, 0x1c9aa SEH catch-filter tail sharing the name. |
| 0x1bdb0 | CHelpers::XmlParseVector | strings.txt @ 1007ba14 xref=01bdb0; also `Vector` @ 100… xref=01bdb0 | already listed in census §Misc; full signature string `bool CHelpers::XmlParseVector…` variants map to the IFP/anims area, minor ambiguity about overloads. |

**False positives excluded** (identifier-shaped strings whose xref is a *parser*, not a
self-name — XML tag/enum values, do NOT treat as function names):
`Bonnet`/`Wheels`/`Windscreen`/`Wings` (xref 039300,038db0,03be50 = component-section parsers),
`EntryInfoNode`/`PtrNode` (xref 02b4d0 = pool node type names), `Simple` (02b5a0),
`Unknown` (053390,046420), `None` (042c10), `Invalid` (051e20), `ExePath` (04f870 = registry
value name read by the installer), `train`/`plane`/`normal`/`poorfamily`… (046960 etc. = AI
class strings), CRT/API names (`CloseThreadpoolTimer`, `FlsAlloc`, … xref 05fe8b = CRT glue),
`MvlSecWnd` (012170 = window class), `OiOOii`/`StringIO`/`RRRFFFFFFRRRR` (python/CRT internals).

---

## 3. Functions checked — NO name evidence found (stop re-checking)

Swept all 733 decompiled functions across `d3out/decomp-chain.txt`, `limout/decomp-chain.txt`,
and `limout/{vehpipe,savewpn,pyaudio,audio,ifp,png,emlights,stubs,stubs2,stubs3,dirmgr,helpers,initchain,missing33,scmop}/range-*.txt`.
**117** self-identify; **616 have no logger first-arg name literal and no name-string xref** —
they either never log their own name or weren't decompiled at a site that does. Full list in
`%TEMP%\opencode\noevidence.txt`. Representative no-evidence RVAs (all currently `FUN_*`):

- CRT / exception internals: 0x14a7, 0x14e0, 0x15c7, 0x1600, 0x171b, 0x1740, 0x1f11, 0x1f30,
  0x1f90, 0x3a10, 0x4b90, 0x5d30, 0x61f0, 0x73c0, 0x80f0 … 0xd860 (CCustomAnimFile internals +
  MSVC security cookie / RTTI helpers — low value).
- Hook installers / stubs: 0x12170 (MvlSecWnd window), 0x16c20, 0x17290, 0x17390, 0x173d0,
  0x17820, 0x17940, 0x180a0, 0x18180, 0x181c0 … (patch-engine plumbing, no self-log).
- Python-host glue, SCM opcode trampolines, pool hooks — see noevidence.txt.

These are plumbing/stub/utility functions that legitimately carry no developer-supplied name
string; further mining of the same three loggers will not name them. New names for this set
must come from **behavioral decomp + version-diff**, not logger mining.

---

## Tooling (re-runnable, in %TEMP%\opencode)
- `mine_names.py` → `mined.json` (851 calls) + console summary
- `attribute.py` → caller-VA ↔ literal attribution vs census
- `pe_exports.py` → `pe-exports.txt` (no exports; PDB path)
- `multiplicity.py` → multi-xref / alias detection
