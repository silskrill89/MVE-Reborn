# Active Context — MVL-clone

## Current Date: 2026-10-12

## Status: Recomp sprint — tranche-4 pushed to GitHub (63% recompiled)

## Current State
- Git: 5 commits, pushed to origin/master (https://github.com/silskrill89/MVE-Reborn.git).
  History: 1916452 scaffold → 5a3e2a5 recomp milestone → 07be648 engine tranche → 4af9792 tranche-3 → 7a0fc6e tranche-4.
- Build GREEN: `cmd //c premake5.cmd` + MSBuild Release|Win32 → bin/Release/vehmod.dll, postbuild deploys to H:\games\VC\vehmod_retail.dll.
- Modules ported (src/): loader (log, chooser), config (CMvlSettings + limits processors), archive (CSevenWalker 7z, CMvlVehicle pipeline), engine (model_id 2288 patch sites, game_hooks 22 handlers, model_helpers 1474 ln, dirmgr), vehicle (vehicle_xml 29 LoadXML* parsers, vehicle_handler save cluster), weapons (mvl_weapon, weapon_handler, weapon_pickups 2733 LOC), scm (vehicle_scm, scm_handler 680 LOC).
- Metrics: **recompiled 63%** (105/167 MVL-specific census fns from re-098e-map.md), **accuracy ~100% of ported code is 1:1** (332 RVA-cited sites; 287 TODO + 84 HOOK POINT stubs log-and-skip). Both posted in README.md.
- Questionnaire Stages 1–10 ALL ANSWERED → merged into PLAN.md §9 decision record (2026-10-11). 0.99d = parity baseline; MSVC+clang-cl; C++11/14; vendored deps; full TDD; MIT-style; dual-loading legacy+modern.

## RE Tooling (all verified working)
- Ghidra 12.1.4 headless: subst M: "E:\dev(dave)" then analyzeHeadless MVL_clone -process vehmod_retail.dll -noanalysis -scriptPath scripts -postScript DumpRange/DecompChainD3 (JAVA_HOME=M:\tools\jdk-21). Scripts in M:\tools\ghidra-projects\scripts\.
- Ghidra MCP (bethington fork v7.0.0) installed, needs GUI on :8089 for live use.
- IDA Free 9.3 (GUI cross-check only, no MCP possible).

## Board
- fix-1 = DEAD (2 empty results, never reuse for helpers).
- fix-2 = reusable (vehicle/save/weapons context).
- fix-3 = reusable (helpers context, ses_ed3135eabffehKHxB11fMOphED).
- fix-4 = reusable (SCM context, ses_ed2e96afcffecAW3RggQVDieeQ).

## Open Next-Tranche Candidates (recomp-ASAP, ~37% remaining)
1. Python 3.3 host: FUN_100287c0 (LoadLibraryA python33.dll + GetProcAddress), mvlbase/mveh/manim modules (0x25280–0x28110).
2. Missing hook-stub bodies 0x51940–0x51c90 (14 fns, Ghidra discovery gap) + IgnoreOpcode 0x234c0 + 0801/0802 bodies 0x23920/0x23940.
3. engine_globals consolidation (port-notes-helpers Q1: shared DAT_* globals fragmented per-TU).
4. Audio (CSoundHandler 0x338a0–0x33e10), PNG (CPngFuncs 0x1bfb3–0x1c9aa), IFP anims (CCustomAnimFile 0x1472–0x25fb), EM lights (CVehicleHandler 0x4e8f0–0x4f090).
5. Init chain completion: FUN_10043c00 remaining callees (0x43810/0x43a10/0x43900/0x43870/0x43b30).

## Process Rules (learned)
- ALWAYS regen premake AFTER new source dirs/files before build (stale vcxproj caused phantom LNK2019 in tranche-4).
- Build gotchas: __thiscall-free-fn → __fastcall+edx-dummy; naked on definitions only; NO Precomp.c in lzma list.
- Paths: bash → /e/dev(dave)/; read/write tools → E:\dev(dave)\; NEVER E:\dev\dave\ (wrong tree). subst M: lost on reboot.
