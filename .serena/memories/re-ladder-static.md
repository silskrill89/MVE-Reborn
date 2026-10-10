# RE: MVL static version-ladder diff (0.9 → 0.98e) — D2 lane

Date: 2026-10-10. Method: pure static PE analysis (no Ghidra) via
`tools/ladder_diff.py` — struct-based PE parsing (sections, imports,
ASCII/UTF-16LE strings ≥5 chars), 23 version dirs / 34 binaries, 0 errors.
Machine-readable scan: run `python tools/ladder_diff.py ladder > scan.json`;
curated deltas: `python tools/ladder_diff.py diff scan.json`.
Core artifact: `docs/MVL/version-matrix.md`.

## Subsystem introduction timeline (string-evidence correlated with readme changelogs)

| Subsystem | Introduced | Binary evidence (cited) |
|-----------|-----------|------------------------|
| XML vehicle schema v1 (vehicle/basic/aidata/colors/audio/handling*/helidata) | 0.9 | tag-name strings, vehmod.flt 0.9 |
| 7z archive loading (statically linked LZMA SDK) | 0.9 | `SzArEx_Open`, `InFile_Open`, `vehicles\*.7z`, vehmod.flt 0.9 |
| XML generator (vlist\*.xml) | 0.9 | `vlist\%s.xml`, vehmod.flt 0.9 |
| Global XML (nospawn) | 0.91 | `vehicles\global.xml`, `vehdata`, `nospawn`, LoadGlobalXMLFile, vehmod.flt 0.91 |
| Tuning parts | 0.93 | `tuning1.dff`…`tuning9.dff`, LoadTuningFile, vehmod.flt 0.93 |
| TXD parser | 0.94 | "Texture dictionary sub-header missing", vehmod.flt 0.94 |
| SA TXD loading + registry SA-path detection | 0.94a | `Software\Rockstar Games\GTA San Andreas\Installation`, LoadSanAndreasTxd, vehmod.flt 0.94a |
| EM lights (specials/emlights) | 0.94a | `emlights`, `colourone`, vehmod.flt 0.94a |
| SA collision from DFF (COL3) | 0.95 | "SA collision section magic code not COL3", LoadSAColSection, vehmod.flt 0.95 |
| taxilight/emlightsingle/heliweps | 0.95 | tag strings, vehmod.flt 0.95 |
| Vehicle weapons (weaponlist/armedinfo/dockedinfo/missileinfo/hotkeys) | 0.96 | tag strings, vehmod.flt 0.96 |
| idstart/usespawner/usetuner | 0.96a | tag strings, vehmod.flt 0.96a |
| xMODELINDEX_ prefix | 0.96c | strrchr/strtol imports, vehmod.flt 0.96c |
| Static CRT (/MT) + C++ EH/RTTI | 0.96d | MSVCR100 imports removed, KERNEL32 38→83, RTTI strings, vehmod.flt 0.96d |
| boatengine audio + RC audio | 0.96e | `boatengine`, LoadXMLAudioBoatEngine, vehmod.flt 0.96e |
| IFP animations | 0.97 | "Loaded %d anims from IFP file", vehmod.flt 0.97 |
| immunity/rgbcol/disableflipdamage | 0.97 | tag strings, vehmod.flt 0.97 |
| machineguninfo + stockvehiclesettings | 0.97c | tag strings, vehmod.flt 0.97c |
| Python 3.3 host (dynamic GetProcAddress) | 0.98 | 60+ `Py*` strings, no python33 import entry, vehmod.flt 0.98 |
| Scripting API v1 (events, entity access, anims) | 0.98 | registerCallback/eventFrame/getEntPosition/addPedAnimation…, vehmod.flt 0.98 |
| mvl\ directory reorg (globalv.xml, mvl\scripts, messagelog.txt) | 0.98 | `mvl\vehicles\globalv.xml`, vehmod.flt 0.98 |
| Windowed mode + in-game menus | 0.98a | USER32 1→25 imports, "MVL camera view", vehmod.flt 0.98a |
| globalm.xml (misc config) | 0.98a | `mvl\globalm.xml`, `miscdata`, `windowed`, `usemenus`, vehmod.flt 0.98a |
| Custom weapons (mvl\weapons) | 0.98a | `mvl\weapons\*.7z`, `logicalid`, weapon XML schema, vehmod.flt 0.98a |
| PNG loading via libpng15.dll (dynamic) | 0.98a | png_* strings, `mvl/libpng15.dll`, vehmod.flt 0.98a |
| PSAPI module scanning | 0.98a | PSAPI.DLL imports, "DLL was loaded too late…", vehmod.flt 0.98a |
| Pool/model-store limit expansion (engine patch) | 0.98b | "RAN OUT OF … SLOTS!!!", `modelstore`, vehmod.flt 0.98b |
| Exe version check | 0.98b | "This is not a Vice City 1.0 executable.", vehmod.flt 0.98b |
| WAV custom sounds | 0.98c | RIFF/fmt/data strings, `gunshot.wav` naming, vehmod_retail.dll 0.98c |
| Extra save XML (mvlsave) | 0.98c | `mvlsave`, `archivehash`, vehmod_retail.dll 0.98c |
| SCM opcode framework (0800/0810 + Python opcodes) | 0.98c | registerHandler, "i:[iOs] Register opcode handler.", vehmod_retail.dll 0.98c |
| VS2013-era toolchain | 0.98c | W-API CRT batch, threadpool APIs, asInvoker manifest, vehmod_retail.dll 0.98c |
| .flt → .dll packaging (mss\vehmod_chooser.flt stub) | 0.98c | 532-byte chooser stub, vehmod_retail.dll 0.98c |
| autointeriors | 0.98d | tag string, vehmod_retail.dll 0.98d |
| testmenus UI + streamingmem + debugidedata | 0.98e | tag strings, `mvl\img\dot.png`, vehmod_retail.dll 0.98e |
| createVehicle/createExplosion/eventBulletFired | 0.98e | ident strings, vehmod_retail.dll 0.98e |

## Retail vs Steam (0.98c/d/e)

Identical sizes, identical import tables, identical notable strings. Only
differences: PDB path (`vehmod_retail.pdb` vs `vehmod_steam.pdb`) and, at
0.98e, one dump filename embedding a hardcoded exe address:
`mvl/dump640FE5h.txt` (retail) vs `mvl/dump63FF95h.txt` (steam).
⇒ steam build = retail code with a small set of exe addresses retargeted.
Diffing retail↔steam is a cheap oracle for locating exe-address-dependent
patch sites (vehmod_retail.dll 0.98e / vehmod_steam.dll 0.98e).

## Constancy checks

- libpng15.dll byte-identical 0.98a–0.98e (md5 e521f5619a49d60e38f34dcbe1402a34).
- vehmod_chooser.flt byte-identical 0.98c–0.98e (md5 ba45b291db889be54607527007cf4a2f).
- Rebuild-only releases: 0.96a→0.96b and 0.97→0.97a (0-byte deltas, no new strings).

## Top 10 diff anchors (string appears at known version → pins feature to code region)

1. `SzArEx_Open` / `InFile_Open - Could not open archive file '%s'` — 7z SDK
   loader code, present from vehmod.flt 0.9 (static LZMA SDK).
2. `vehicles\global.xml` + `nospawn` + `vehdata` — global-config parser,
   appears exactly at vehmod.flt 0.91.
3. `tuning1.dff` + `LoadTuningFile` — tuning-part loader, appears exactly at
   vehmod.flt 0.93.
4. `Software\Rockstar Games\GTA San Andreas\Installation` + `vehicles/sa_vehicle.txd`
   — SA-TXD loader + registry detection, appears exactly at vehmod.flt 0.94a.
5. `SA collision section magic code not COL3` + `LoadSAColSection` — SA
   collision extractor, appears exactly at vehmod.flt 0.95.
6. `weaponlist` / `armedinfo` / `dockedinfo` / `missileinfo` — vehicle-weapon
   XML parser, appears exactly at vehmod.flt 0.96.
7. `Complete Object Locator` (RTTI) + removal of all MSVCR100.dll imports —
   static-CRT toolchain switch, appears exactly at vehmod.flt 0.96d.
8. `PyArg_ParseTuple` / `PyImport_AppendInittab` / `registerCallback` /
   `eventFrame` — Python host + scripting API, appears exactly at vehmod.flt 0.98.
9. `mvl/libpng15.dll` + `png_create_read_struct_2` + PSAPI.DLL imports — PNG
   pipeline + module scan, appears exactly at vehmod.flt 0.98a.
10. `RAN OUT OF VEHICLE SLOTS!!! Increase pool size.` + `modelstore` — pool
    limit engine patch, appears exactly at vehmod.flt 0.98b.
Bonus: `mvl/dump640FE5h.txt` vs `mvl/dump63FF95h.txt` — retail/steam exe
address divergence marker at 0.98e.

## Rejected hypotheses (log)

- 0.96d size jump = new feature subsystem → rejected (static CRT switch).
- python33.dll statically imported → rejected (dynamic GetProcAddress).
- vehmod_chooser.flt = functional older loader → rejected (532-byte stub).
- 0.98c jump entirely = WAV/save features → partially rejected (VS2013 CRT bump).
- 0.92/0.96b/0.97a contain hidden features → rejected (no notable string or
  import deltas; changelog says bugfix only).

## Tool notes

- `tools/ladder_diff.py` commands: `ladder` (scan→JSON), `strings <bin>`,
  `diff <scan.json>` (curated per-version deltas), `fullnew <scan.json> <ver>`
  (all new strings vs previous version).
- String classifier `classify()` buckets: text/xml/path/ident; junk = fp/jump
  tables (digit-punct runs) filtered by letter/vowel/word-structure heuristics.
