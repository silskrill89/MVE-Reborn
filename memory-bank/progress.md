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

### Phase 3: Recomp Sprint tranches 1–4 (Oct 10–12) — BUILD GREEN, PUSHED
- **Tranche-1** (5a3e2a5): vendored TinyXML 2.6.2 + LZMA SDK 25.01; loader (log, chooser); config (CMvlSettings, global_veh_xml); archive (CSevenWalker 7z, CMvlVehicle ParseArchive).
- **Tranche-2** (07be648): engine — CMvlSettings limits processors; CLongModelId + CModelIndexLimit (2288 patch sites); hook machinery + 22 handlers.
- **Tranche-3** (4af9792): model_helpers (1474 ln, 12 roots + 19 depth-1); dirmgr; CVehicleHandler save cluster.
- **Tranche-4** (7a0fc6e): weapons (CMvlWeapon, CWeaponHandler, CWeaponPickups, 2733 LOC); SCM (CVehicleSCM, CScmHandler, 680 LOC). PUSHED to GitHub origin/master.
- Metrics: recompiled 63% (105/167 census fns); accuracy ~100% of ported = 1:1 (332 RVA sites, 287 TODO + 84 HOOK stubs).

## In Progress

### Phase 4: Recomp sprint tranches 5+ (~37% remaining)
- Next: Python host, missing hook-stub bodies (0x51940–0x51c90, IgnoreOpcode 0x234c0, 0801/0802), engine_globals consolidation, audio/PNG/IFP/EM-lights, init-chain completion.

## Not Started
- SA port; skygfx_plus/reVC integration; O3DE JSON config layer; installer; xmlgen clone.
