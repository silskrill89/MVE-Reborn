# RE Daily Log

## 2026-10-10 — Lane D2: static version-ladder diff (resumed after PC crash)

Completed the full 23-version static ladder diff (0.9 → 0.98e) with
`tools/ladder_diff.py` (stdlib PE parser: sections/imports/strings; no
Ghidra). 34 binaries scanned, 0 errors. Outputs:
`docs/MVL/version-matrix.md` (core artifact), `.serena/memories/re-ladder-static.md`
(analytical narrative + top-10 diff anchors), this log.

Highlights:
- Subsystem timeline pinned by string appearances: global.xml/nospawn 0.91,
  tuning 0.93, TXD parser 0.94, SA TXD+registry 0.94a, SA COL3 0.95, vehicle
  weapons 0.96, static-CRT toolchain switch 0.96d, boat/RC audio 0.96e, IFP
  anims 0.97, Python 3.3 host 0.98 (dynamic GetProcAddress, no import entry),
  windowed mode + custom weapons + libpng15 0.98a, pool-limit engine patch
  0.98b, WAV sounds + save XML + SCM opcodes + .flt→.dll 0.98c,
  autointeriors 0.98d, testmenus/streamingmem 0.98e.
- Retail vs steam DLLs (0.98c/d/e): identical size/imports/strings except PDB
  path and one dump filename embedding an exe address (0x640FE5 retail vs
  0x63FF95 steam at 0.98e) — steam = retail with retargeted exe addresses.
- libpng15.dll and vehmod_chooser.flt byte-identical across all their versions.
- Rejected hypotheses logged (0.96d jump = CRT switch, not features; chooser
  = 532-byte stub; python33 not statically imported).

## 2026-10-10 (later) — D3 synthesis (orchestrator, from d3out extraction)
- **BREAKTHROUGH**: 0.98e retains RTTI/class-name strings → 136 functions directly named (CMvlSettings/CMvlVehicle/CVehicleHandler/CMvlWeapon/CWeaponHandler/CWeaponPickups/CModelIndexLimit/CLongModelId/CCustomAnimFile/CSoundHandler/CSevenWalker/CPngFuncs/CScmHandler/CDxHandler/CHelpers/CVehicleSCM/CWeaponLogicHat). Full table in .serena/memories/re-098e-map.md.
- Init chain decompiled (81 fns): entry 0x10005c1d1 → FUN_10043c00 → PrepareMvlForLoad 0x2baf0 + LoadGlobalVXMLFile 0x38160 + hook installers 0x22560/0x22590/0x225a0 (VirtualProtect, 0x401000 region).
- Python host confirmed dynamic @ 0x287c0 (python33.dll + Py* GetProcAddress); mveh/manim/mvlbase module functions located.
- patch-sites.md skeleton written (10 seeded sites, hook stubs 0x3dff0–0x3e320 pending decompile).
- vcconsts.txt = 2,755 VC-range constant hits = retail address table; steam diff pending.

## 2026-10-10 (evening) — chooser shim solved + Ghidra MCP live
- vehmod_chooser.flt FULLY understood (capstone disasm, 64-byte .text): reads dword @ gta-vc.exe VA 0x61A7F8, fingerprint 0x9753C405→retail / 0xE8535004→steam, LoadLibraryA("mvl\vehmod_retail.dll"/"mvl\vehmod_steam.dll"), silent no-op on unknown exe. Note: .serena/memories/re-loader-shim.md.
- Ghidra MCP installed (bethington fork v7.0.0, bridge verified) — interactive agent-driven RE now possible once Ghidra GUI is open.

## 2026-10-10 (recomp pivot) — Lanes V/C/A landed + FULL BUILD GREEN
- Pivot per user directive: questionnaire cancelled, focus = working recomp ASAP.
- **Lane V (vendoring)**: external/tinyxml (2.6.2 classic, matches 0.9 TiXml* RTTI) + external/lzma (SDK 25.01 C layout); premake wired (explicit lzma C file list, no Precomp.c — SDK 25.01 has only Precomp.h; vendored-file warning suppression /wd4100 etc.).
- **Lane L (loader)**: src/loader/log.{h,cpp} (MvlLog/MvlErrorLog), chooser.cpp shim clone.
- **Lane C (config)**: src/config/xml_helpers, global_veh_xml (LoadGlobalVXMLFile), mvl_settings (CMvlSettings singleton + free wrapper LoadGlobalMXMLFile; mvl\globalm.xml: windowed/usemenus/antialias/log).
- **Lane A (archive)**: src/archive/seven_walker (CSevenWalker, SzArEx_Open + solid-block cache + UTF-16→UTF-8), vehicle_archive (CMvlVehicle::ParseArchive, RegisterArchiveFile tuning1-9.dff/weapons, VerifyRequiredFiles xml-mandatory).
- **BUILD GREEN (Release|Win32)**: single error fixed (seven_walker.cpp missing `#include "7zCrc.h"` for CrcGenerateTable — 7zCrc.h:14, must precede SzArEx_Open); vehmod.dll linked → bin\Release\; postbuild deploy ran → H:\games\VC\vehmod_retail.dll (deploy-subdir alignment with b.bat's mvl\ path noted as TODO).
- Next big block: Ghidra MCP decomp port of remaining ~136+ RTTI-named functions (model/limit engine, vehicle XML parsers, weapons, SCM, Python host).

## 2026-10-10 (engine tranche) — limits/model-ID/hooks ported + BUILD GREEN
- fix-1 ported CMvlSettings limit processors (ProcessPoolLimits/ProcessModelStoreLimits/ProcessIdLimits/ProcessStreamingMemLimit + LoadMXMLSectionLimitsInstances/ModelStore) into src/config/mvl_settings.{h,cpp} with typed limit structs, exact log strings, real engine VAs documented (pool value table 0x100b1794, streaming writes 0x0041079f/0x0094dd54).
- fix-2 ported CLongModelId + CModelIndexLimit (13 methods, 2288 extracted patch-site entries with static_asserts) into src/engine/model_id.{h,cpp}; hook trio (MvlWriteJmp/MvlWriteDword/MvlUnlockGameText) + 22 handler stubs + 21-entry patch table into src/engine/game_hooks.{h,cpp}. MvlInstallStubHooks NOT called yet — 11 helper stubs unported.
- Build fixes: __thiscall free-function decl → __fastcall(this,edx-dummy); naked stripped from header decls (kept on definitions). Full build GREEN, deployed to H:\games\VC\vehmod_retail.dll.
- Next: vehicle archive/XML pipeline tranche + hook helper completion; re-daily continues below.

## 2026-10-11 (tranche-2) — vehicle pipeline + XML parsers ported, BUILD GREEN
- **Lane V (fix-2)** ported CMvlVehicle archive pipeline (0x44f20–0x463f0: LoadFromArchive/ParseArchive/RegisterArchiveFile/VerifyRequiredFiles/Load{Texture,Model,Collision,Animation}File/LoadWeaponFiles/InitialiseLoaded) into src/archive/vehicle_archive.{h,cpp} + all 29 LoadXML* parsers (0x46420–0x4e790) into src/vehicle/vehicle_xml.{h,cpp} (~4.9k LOC). Original names 1:1, `0.98e @ RVA` citations, verbatim quirks ('gamg', 'lightbox', transmission copy/paste bug) preserved. 59 engine HOOK POINT stubs with target VAs. Port notes: .harness-memory/limout/port-notes-veh.md (201 lines, 8 open questions).
- **Lane E (fix-1)** = FAILED (empty result, wrote nothing). Export inputs ready in .harness-memory/limout/{helpers,dirmgr}/. Re-dispatched on fresh session.
- Build fix: vehicle_xml.cpp fMachineGunSound → nMachineGunSound (field name per SMvlVehicleWeaponData @ vehicle_archive.h:261).
- BUILD GREEN: bin\Release\vehmod.dll (postbuild deploy ran). Total tranche LOC ~5.6k incl. prior engine tranche.

## 2026-10-11 (tranche-3) — hook helpers + directory manager + save system ported, BUILD GREEN
- **fix-3 (fresh session)**: src/engine/model_helpers.{h,cpp} (1474 lines) — all 12 depth-0 roots (11 helpers + driver FUN_10017820) + 19 depth-1 ports (quat→matrix 0x8990, LCG rand 0x59f55) + ~60 log-once TODO stubs (RW thunks, XMM-loss fns needing disasm); src/engine/dirmgr.{h,cpp} — FUN_10017f60/18030 delegate to CModelIndexLimit, FUN_100180a0 guard ported (9 E9-JMP installs withheld — handlers 0x17db0..0x18060 unported); port-notes-helpers.md (~200 lines, 5 open Qs incl. shared-global fragmentation = engine_globals consolidation candidate).
- **fix-2 (vehicle session reuse)**: src/vehicle/vehicle_handler.{h,cpp} (621 lines) — full CVehicleHandler save cluster 1:1 (LoadSaveXml* 7 methods, LoadExtraSaveFile sidecar _mvl.xml, FetchRealModelIndexForPoolSlot/GarageSlot); save XML schema recovered from DLL parser tables @ RVA 0xaf3c8–0xaf548 (root mvlsave); 13 hook stubs (CWeaponHandler save fns = next lane); port-notes-veh.md save section.
- Build fixes (orchestrator): model_helpers.cpp illegal static_cast<uintptr_t>(ptr) → direct reinterpret_cast (2 sites); vehicle_handler.cpp TIXML_ERROR_OPENING_FILE → TiXmlBase::TIXML_ERROR_OPENING_FILE (class-scoped enum).
- BUILD GREEN: vehmod.dll linked + postbuild deploy ran. Tranche-3 = ~2167 new lines.

## 2026-10-12 (tranche-4) — weapons + SCM clusters ported, BUILD GREEN, metrics posted
- **fix-2 (reuse)**: src/weapons/ (2733 LOC) — CMvlWeapon archive/XML pipeline (0x52150–0x53e10: LoadFromArchive/ParseArchive/RegisterArchiveFile icon-scope-quirks/LoadTexture-Model-Icon-Scope-AnimFile/SectionWeapon-Basic-Properties-Shotgun 1:1 vs sample XMLs), CWeaponLogicHat+Spike (0x54260–0x545c0), CWeaponHandler (WeaponAudio/ShotAudio, LoadWeaponArchives w<N>_ 2-pass slots, save cluster 0x56560–0x56cb0 matching vehicle_handler free-fn decls byte-for-byte, ClearSaveData, DumpWeaponXML), CWeaponPickups (CreateWeaponPickup 0x57d80, opcode 0810 0x584f0, pickups.xml spawn cluster 0x58610–0x58990, PARTIAL). 34 TODO(weap) stubs. port-notes-weapons.md. Open Q: SectionWeapon hatinfo-row Ghidra stack-slot mixup (clone binds dedicated logic row).
- **fix-4 (fresh)**: src/scm/ (680 LOC) — CVehicleSCM (SetNextTextForVehicleId 0x51e20 + ProcessNextTextOpcode 0x51eb0, both RTTI; InstallScmHooks 0x51cb0 with kScmHookSites[14]+kScmBytePatches[4] verbatim), CScmHandler opcode table (0800/0801/0802/0810 registration data; IgnoreOpcode 0x234c0 body not in any dump → logged needed-dump stub). 13 stubs. port-notes-scm.md. Open Q: 14 hook-stub bodies 0x51940–0x51c90 missing from Ghidra range discovery.
- Build root cause: stale premake regen — src/weapons/ + src/scm/ were absent from vehmod.vcxproj (written after last regen) → 2 phantom LNK2019s. Fix: `cmd //c premake5.cmd` regen + 2 compile fixes (CMvlWeapon::GetWeaponSlot accessor added; MvlPickupSetupRecord fwd decl added).
- BUILD GREEN: vehmod.dll linked + postbuild deploy ran (H:\games\VC\vehmod_retail.dll).
- **Metrics (user request)**: recompiled = 63% (105/167 MVL-specific census fns ported 1:1, from re-098e-map.md census; 2922 total fns incl. CRT/thunks excluded); accuracy = ~100% of ported code is 1:1 (332 RVA-cited sites; remaining gaps are 287 TODO + 84 HOOK POINT stubs that log-and-skip). Both numbers posted in README.md Status section.

## 2026-10-12 (questionnaire close + memory sync)
- Questionnaire Stages 5–10 completed (Stage 4 Q4.10 = a-c: weekly + on-new-findings + when-blocked cadence). All 10 stages answered → PLAN.md §9 decision record written (parity baseline 0.99d; MSVC+clang-cl; C++11/14; vendored deps; free-to-differ import surface; /W4 WAE; full TDD; MIT-style; dual-loading legacy+modern; conservative name anchoring; python33/libpng15 black-box; unified clone + runtime exe detection; rejected hypotheses logged).
- Memory layers synced: memory-bank/activeContext.md + progress.md rewritten to tranche-4 state; .serena/memories/project.md status block added; sadie re-indexed; agentmemory saved.
- Process refinements recorded (from tranche lessons): ALWAYS regen premake after new source dirs before build (stale vcxproj → phantom LNK2019); fix-1 session permanently dead.

## 2026-10-12 (tranche-5-prep) — globals consolidation bound as rules, workflow template, metrics 67%
- **AGENTS.md**: added binding Globals/data naming rule (directive 2026-10-12): semantic `Mvl*` name in code, original `DAT_* @ RVA` only as trailing comment; canonical shared-global storage = `src/engine/engine_globals.{h,cpp}` (71 duplicate per-TU statics already removed).
- **docs/reverse-effort-template.md**: NEW retrospective + reusable playbook — how the recomp-ASAP pivot (questionnaire Stages 5–10 cancelled) turned a stalled survey effort into 4 green tranches / 13.8k LOC in ~2 days; pipeline (spec mining → name anchoring → headless export → parallel port lanes → single-owner build → milestone commit → memory sync), the 10 hard-won rules table, metrics discipline, adapt-for-next-project checklist.
- **Metrics refresh**: Recompiled 63% → **67%** (99/148 census; census regex-normalized, prior 105/167 figure cross-checked and struck through), 320 RVA sites, 253 TODO + 77 HOOK POINT markers, 13,784 LOC. README.md now carries a per-commit milestone table with struck-through prior values as cross-checks.
- Pending: run_export5.ps1 (8 dumps) → build-verify globals → tranche-5 lanes.

## 2026-10-12 (tranche-5) — Python host, init chain, SCM bodies, media cluster — BUILD GREEN

- fix-3: src/script/python_host.{h,cpp} (242/904 ln) — MvlPyApi 50-slot proc table, loader FUN_100287c0, event registry 0/1/2/3/5, script scan, .err.txt, mvlbase/mveh/mped (getPedVehicle = mped, 8 modules), 6x AppendInittab; src/core/init.{h,cpp} (83/205 ln) — 5 init-chain roots 0x43810/43870/43900/43a10/43b30 via MvlInitPatchDword/Byte seams; main.cpp wiring MvlInstallInitChainPatches(). port-notes-py.md.
- fix-4 (5A retry): scm_handler.cpp 87->229 ln — IgnoreOpcode 0x234c0 real body (11-opcode set), 0801 MvlScmHandlePythonLoadedOpcode, 0802 MvlScmHandleVersionOpcode (0x267a=9850), MvlScmWarn (FUN_1001cce0), 4 MvlScmHook_* seams.
- fix-5 (5C retry): src/audio/sound_handler (CSoundHandler 0x338a0/33a00/33e10), src/render/png_funcs (CPngFuncs MvlPngApi 16-slot dynamic libpng15, PngFileToTexture 0x1bed0), src/anim/custom_anim (CCustomAnimFile Load overloads + LoadVersionOne/Two structural), EmLight trio as CVehicleHandler statics (0x4e8f0/4edc0/4f090), vehicle_archive.cpp 3 one-line forwards. port-notes-media.md.
- Build fixes: python_host.cpp engine_globals include + reinterpret_cast cap[3]; class/struct MSVC mangling (CCustomAnimFile fwd-decls struct->class in vehicle_archive.h:36 + mvl_weapon.h:21) resolved 5 LNK2019.
- Metrics: 81
## 2026-10-12 (tranche-5) - Python host, init chain, SCM bodies, media cluster - BUILD GREEN

- fix-3: src/script/python_host.{h,cpp} (242/904 ln) - MvlPyApi 50-slot proc table, loader FUN_100287c0, event registry 0/1/2/3/5, script scan, pct-s.err.txt writers, mvlbase/mveh/mped (getPedVehicle = mped, 8 modules), 6x AppendInittab; src/core/init.{h,cpp} (83/205 ln) - 5 init-chain roots 0x43810/43870/43900/43a10/43b30 via MvlInitPatchDword/Byte seams; main.cpp wiring MvlInstallInitChainPatches(). port-notes-py.md.
- fix-4 (5A retry): scm_handler.cpp 87->229 ln - IgnoreOpcode 0x234c0 real body (11-opcode set), 0801 MvlScmHandlePythonLoadedOpcode, 0802 MvlScmHandleVersionOpcode (0x267a=9850), MvlScmWarn (FUN_1001cce0), 4 MvlScmHook seams.
- fix-5 (5C retry): src/audio/sound_handler (CSoundHandler 0x338a0/33a00/33e10), src/render/png_funcs (CPngFuncs MvlPngApi 16-slot dynamic libpng15, PngFileToTexture 0x1bed0), src/anim/custom_anim (CCustomAnimFile Load overloads + LoadVersionOne/Two structural), EmLight trio as CVehicleHandler statics (0x4e8f0/4edc0/4f090), vehicle_archive.cpp 3 one-line forwards. port-notes-media.md.
- Build fixes: python_host.cpp engine_globals include + reinterpret_cast cap[3]; class/struct MSVC mangling (CCustomAnimFile fwd-decls struct->class in vehicle_archive.h:36 + mvl_weapon.h:21) resolved 5 LNK2019.
- Metrics: 81pct recompiled (125/155 census), 513 RVA cites, 322 TODO + 115 HOOK POINT, 17735 LOC. README milestone row added.


## 2026-10-12 — tranche-6: weapon save-write + Python tuning methods, metric census refined

- **fix-6 (weapons save-WRITE)**: ported `WriteSaveXmlWeaponInfoIdentity` (0x56880), `WriteSaveXmlPlayerWeaponInfo` (0x56aa0, 2-param __fastcall id ECX + ammo EDX verified from binary), `WriteSaveXmlPlayerWeapons` (0x56c00, ped+0x408 stride 0x18, 10 slots), `CWeaponHandler::ApplySaveData` (0x56db0, pending pairs at DAT_10117960, verbatim "Cannot apply weapon %s, slot collision."), `SaveSprintf` (FUN_10051db0) -> src/weapons/weapon_handler.{h,cpp} (172/953 ln). 3 new HOOK seams: MvlGameGetPlayerPed (0x2dfd0), MvlGameGetPedWeaponSlot, MvlGameGiveWeapon (0x30560). port-notes-weapons.md section 6 (273 ln).
- **fix-7 (Python dispatch, partial)**: FUN_10025a70 pool getter + 7 mveh tuning methods (0x27b90 getTuningPartCount, 0x27bf0 getTuningPartName, 0x27c50 getTuningPartVarCount, 0x27cb0 getVehUsedTuningPart, 0x27d50 setVehUsedTuningPart, 0x27de0 getVehModelAIType, 0x27f60 setCarTaxiLightState) into src/script/python_host.{h,cpp}. FUN_10025b00 + FUN_100290d0 remain stubs (engine-seam territory).
- **Build**: MSBuild Release|Win32 GREEN after fixes (engine_globals include, reinterpret_cast cap[3], class/struct mangling x2, _vsprintf_s -> vsprintf_s). Postbuild deploy skipped: H: drive not mounted this session (setx MVL_GAME_DIR verified earlier; not a code error).
- **Metrics**: census refined — added 20-row "Census addenda (tranches 5-6)" table to .serena/memories/re-098e-map.md (22 tranche-5/6 function identities were missing from the census, making those ports invisible to the metric). Recompiled 81% = 144/177 census fns (was 125/155 at tranche-5); 926 RVA cites, 387 TODO, 120 HOOK POINT, 18,025 LOC.
- **Next (tranche-7)**: export the 14 undiscovered hook-stub bodies 0x51940-0x51c90 (Ghidra force-create/disasm over 10051940-10051cb0) -> port into src/scm/.
