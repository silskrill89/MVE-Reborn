# Active Context — MVL-clone

## Current Date: 2026-10-13

## Status: STABLE-1.0 PREP — commit `4bb2ea7` pushed to origin/continued; test package built; oracle review owed

## Current State
- Git: branch **'continued'** active (user directive m0637 — all new work here, NOT master).
  master = 0b58d62 (tranche-7b). continued: 4bb2ea7 "stable-1.0 prep - 42 bug-audit fixes,
  rcengine1 name table, full FUN_/decompiler ident renames, pad044 layout fix, 0.99d
  acquisition, full test package" (36 files, +2017/−926) → PUSHED to origin/continued.
  master history: 1916452 scaffold → 5a3e2a5 recomp → 07be648 engine → 4af9792 t3 →
  7a0fc6e t4 → 272770b t5-prep → 9577e6f t5 → a19783e t6 → 69ef529 t7 → 0b58d62 t7b.
- Build GREEN: compile+link OK (`cmd //c premake5.cmd` + MSBuild Release|Win32 →
  bin/Release/vehmod.dll 195,584 B). Postbuild deploy copy FAILS (MSB3073 to
  H:\games\VC) = H: not mounted this session — NOT a code error.
- Metrics: **100%** (165/165 census fns) at 0b58d62; accuracy ~100% of ported = 1:1
  (HOOK POINT/TODO stubs log-never-fake). Census = re-098e-map.md after prune.
- Stable-1.0 hardening on disk (in 4bb2ea7): 42 bug-audit fixes across all modules
  (2 CRITICAL crash classes fixed: weapon .wav literal-string-as-pointer → NULL-term
  name table; vehicle rcengine1 same pattern; ANP3 16→24-byte buffer; CMvlVehicle
  flat-layout padding 0xBB0→0x2D2C + pad044; sa_col bounds; 7z walker index map).
  All marked `// BUGFIX(stable-1.0):` with RVA cites.
- Naming hygiene: **0 FUN_ code identifiers** remain (was 425; 182 code + 243 comment).
  All renamed to semantic Mvl* names with `// FUN_<va> @ RVA 0x…` trailing comments
  (mapping tables in .harness-memory/limout/renames-{engine,script,scm,extras-core-render}.md).
  Decomp-style param_N/local_XX/iVarN also eliminated (fix-9: 192 replacements).
- Test package: **dist/MVE-Reborn-test.zip** (1,158,348 B, 190 files) — FULL original
  0.98e mvl/ data set (help/, img/, scripts/, vehicles/, weapons/, bullethole.png,
  globalm.xml, libpng15.dll, readme.txt) + our vehmod_retail.dll + original
  vehmod_steam.dll fallback + mss/vehmod_chooser.flt + README-TEST.txt.
- 0.99d ACQUIRED: original_files/maxovehmod099d{,.zip} MD5-verified (proprietary,
  gitignored). Deltas vs 0.98e: python33→python37.dll, spdlog debug.log, SkyGfx
  hook (AttachCarPipeToRwObject), rtshadowonpeds key. Banner still says "0.98e".

## RE Tooling (all verified working)
- Ghidra 12.1.4 headless: subst M: "E:\dev(dave)" then analyzeHeadless MVL_clone
  -process vehmod_retail.dll -noanalysis -scriptPath scripts -postScript
  DumpRange/DecompChainD3 (JAVA_HOME=M:\tools\jdk-21).
- Ghidra MCP (bethington fork v7.0.0), IDA Free 9.3 (GUI cross-check only).

## Board
- fix-9 = reusable (engine + rename context, ses_ed2e96afcffecAW3RggQVDieeQ).
- fix-10 = reusable (weapons/media context, ses_ecbc43178ffeB6ebfgp7gx1TeZ).
- lib-1 = reconciled (0.99d acquisition).

## Open Next Steps
1. **Oracle review** (user m0690: "disapatch oracle for that and areasercher after") —
   independent @oracle review of clone correctness/quality vs original.
2. Memory sync completion (this pass): progress.md, .serena project.md, sadie re-init.
3. stable-1.0 commit/tag on 'continued' after oracle verdict.
4. 0.99d follow-up: 0.98e→0.99d binary diff lane (python37 swap, spdlog, SkyGfx hook,
   rtshadowonpeds) to decide whether parity target moves from 0.98e-port to 0.99d.

## Process Rules (learned)
- ALWAYS regen premake AFTER new source dirs/files before build.
- Build gotchas: __thiscall-free-fn → __fastcall+edx-dummy; naked on definitions only;
  NO Precomp.c in lzma list; class/struct fwd-decl must match (PAU/PAV mangling).
- Paths: bash → /e/dev(dave)/; read/write tools → E:\dev(dave)\; NEVER E:\dev\dave\.
- deploy: H: not mounted — MSB3073 copy failure is NOT a code error.
- Naming: real names from evidence (RTTI/PyMethodDef/logger literals); mbase not
  mvlbase; FUN_+RVA only as trailing comments.
