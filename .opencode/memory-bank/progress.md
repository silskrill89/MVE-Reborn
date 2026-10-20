# Progress — MVL-clone

## Completed Phases

### Phase 0: Scaffolding (Oct 10)
- Harness cloned from skygfx_plus_expIV: memory-bank, .serena, .sadie, .opencode, .harness-memory, .obsidian, docs/
- sadie.py / sadie_vault_sync.py adapted to MVL domain (vault → docs/)
- .lsai/mjsf.json regenerated with premake5 project entry
- AGENTS.md, README.md written fresh; docs/ vault stub created

### Phase 1: Spec + Recon (Oct 10–11)
- S1 data-file spec mining + S2 forum deep-scan (librarian): full behavioral spec, crash/patch clues
- Version ladder static diff (tools/ladder_diff.py, 23 versions): feature timeline 0.9→0.98e
- 0.98e structural map: 136 RTTI-named functions, init chain, Python host, config clusters
- Reference sources registered: reVC re3-miami, librw, RW 3.6 SDK, plugin-sdk plugin_vc
- Tooling: Ghidra 12.1.4 headless verified, IDA Free 9.3 installed, Ghidra MCP wired

### Phase 2: Questionnaire + Plan (Oct 11)
- 10-stage questionnaire answered (Stages 1–10); answers merged into PLAN.md §9 decision record
- 0.99d = parity baseline; MSVC+clang-cl; C++11/14; vendored; full TDD; MIT-style; dual-loading

### Phase 3: Recomp Sprint tranches 1–4 (Oct 10–12) — BUILD GREEN, PUSHED (origin/master)
- **Tranche-1** (5a3e2a5): vendored TinyXML 2.6.2 + LZMA SDK 25.01; loader (log, chooser); config (CMvlSettings, global_veh_xml); archive (CSevenWalker 7z, CMvlVehicle ParseArchive).
- **Tranche-2** (07be648): engine — CMvlSettings limits processors; CLongModelId + CModelIndexLimit (2288 patch sites); hook machinery + 22 handlers.
- **Tranche-3** (4af9792): model_helpers (1474 ln, 12 roots + 19 depth-1); dirmgr; CVehicleHandler save cluster.
- **Tranche-4** (7a0fc6e): weapons (CMvlWeapon, CWeaponHandler, CWeaponPickups, 2733 LOC); SCM (CVehicleSCM, CScmHandler, 680 LOC). Metrics 63%.

### Phase 4: Recomp sprint tranches 5–7b (Oct 12–13) — 100% census, PUSHED (origin/master)
- **Tranche-5-prep** (272770b): engine_globals consolidation (76 externs, 71 dup statics removed); naming rules bound in AGENTS.md; workflow template docs.
- **Tranche-5** (9577e6f): Python host (MvlPyApi 50-slot, mbase/mveh/mped/manim… 8 modules); init chain (5 roots via patch seams); SCM bodies (0801/0802/IgnoreOpcode); media cluster (audio/png/ifp/emlights). Metrics 81%.
- **Tranche-6** (a19783e): weapon save-write (0x56880/56aa0/56c00) + ApplySaveData; Python tuning methods/pool getter; census addenda (+22 fns). Metrics 81% (144/177).
- **Tranche-7** (69ef529): vehicle archive-scanning cluster (LoadVehicleArchives/AnalyzeStartIndex/LoadSanAndreasVehiclesTxd); tuning parts (LoadTuningFile/BuildPartDummyName); SA COL loader (COL2/COL3/COLL); GenerateDefaultXML (Alt+4 xmlgen); MvlSecWnd; DX9TextureFix. Metrics 92%.
- **Tranche-7b** (0b58d62): last census fns (FUN_10024110 manim.initAnimType, FUN_100290d0 event-handler insert); real Python method names recovered (PyMethodDef PE parse; mbase NOT mvlbase); census prune. **Metrics 100% (165/165 census fns); accuracy ~100% of ported = 1:1.**

### Phase 5: STABLE-1.0 hardening (Oct 13) — commit 4bb2ea7 on 'continued' branch, PUSHED
- User directive: new branch 'continued' (m0637); stable "1.0" goal (m0635).
- 4 bug-audit lanes: 42 fixes across ALL modules — 2 CRITICAL crash classes (weapon .wav + vehicle rcengine1 literal-string-as-pointer → NULL-term name tables; ANP3 16→24-byte buffer; CMvlVehicle flat-layout padding 0xBB0→0x2D2C + pad044; sa_col bounds; 7z walker index map; err.txt PyBytes writer; streaming-buffer heap overflow; .text unlock before patch chain). All `// BUGFIX(stable-1.0):` with RVA cites.
- Naming hygiene: 0 FUN_ code identifiers remain (was 182 code; all renamed semantic Mvl* with trailing `// FUN_<va> @ RVA 0x…` comments; mapping tables in .harness-memory/limout/renames-*.md). fix-9 eliminated 192 decompiler-style param_N/local_XX/iVarN identifiers.
- rcengine1 vehicle-sound crash fixed (reconstructed NULL-term name table @ 0xaf104).
- Build GREEN (compile+link; postbuild deploy copy fails = H: not mounted, not a code error).
- Test package: dist/MVE-Reborn-test.zip (1.16 MB, 190 files) — full original 0.98e mvl/ data + our vehmod_retail.dll + original steam fallback + README-TEST.txt.
- 0.99d ACQUIRED (librarian, MD5-verified): original_files/maxovehmod099d{,.zip}; deltas = python37.dll, spdlog debug.log, SkyGfx hook, rtshadowonpeds.

## In Progress / Owed
- **Oracle review** (user m0690): independent @oracle review of clone correctness/quality vs original.
- stable-1.0 tag after oracle verdict.
- 0.99d follow-up: 0.98e→0.99d binary diff lane (python37 swap, spdlog, SkyGfx hook, rtshadowonpeds).
- Test in-game (H:\games\VC): boots? mvlclone_log.txt banner? (needs H: mounted).

## Not Started
- SA port; skygfx_plus/reVC integration; O3DE JSON config layer; installer; xmlgen clone.
