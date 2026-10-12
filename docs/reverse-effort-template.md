# Reverse-Effort Workflow Template — forged in MVL-clone

> Status: retrospective + reusable playbook. Source project: MVL-clone (Maxo's Vehicle
> Loader rebuild), 2026-10-10 → 2026-10-12. Any future "binary → clone source" effort
> in a similar vein should start from this document.

## 1. The shift that mattered

**Before (survey-first):** a 10-stage questionnaire × 10 questions, phased
recon → RE → implement plan. Work stalled at planning: the questionnaire itself
became the work, and no code existed.

**After (recomp-ASAP, user directive 2026-10-10):** questionnaire Stages 5–10
cancelled with defaults applied; all effort redirected to producing a *working
recomp* as fast as possible. Planning artifacts already written (PLAN.md v1,
§9 decision record) were kept as constraints, not gates.

**Result in ~2 days:** 4 green tranches, 13.8k LOC ported, 67% of the
MVL-specific function census covered, CI-grade build + deploy loop, pushed to
GitHub as milestone commits. Planning-first would still be in Stage 6.

## 2. The pipeline (the actual reusable loop)

```
 1. SPEC MINING (no disassembler needed)
    version ladder diff → strings/XML tags/changelog → subsystem timeline
    → docs/version-matrix.md  (what features exist, which version added them)
 2. NAME ANCHORING
    RTTI/class names in the binary → hundreds of real symbol names for free
    + game-SDK address tables (plugin-sdk/reVC) → rename FUN_* to game names
 3. EXPORT (batch, headless, cheap)
    Ghidra analyzeHeadless + DumpRange/DecompChainD3 scripts → plain-text C
    dumps into .harness-memory/<lane>out/  (readable by any agent, no GUI)
 4. PORT (parallel fixer lanes, disjoint scopes)
    one lane = one subsystem; naming policy binding; HOOK POINT stubs for
    engine internals — log-and-never-fake; no-build rule (orchestrator builds)
 5. BUILD+VERIFY (single owner)
    premake regen AFTER any new dirs/files → MSBuild → deploy
 6. COMMIT (each green tranche = one milestone commit with % metrics)
 7. SYNC memory layers (activeContext/progress/serena/sadie/re-daily) → repeat 3
```

## 3. Rules that prevented failure (all learned the hard way)

| Rule | Why |
|---|---|
| Regen premake **after** every new source dir/file | silent LNK2019 (stale vcxproj) cost 2 build cycles |
| `cmd //c premake5.cmd`, never `./premake5.cmd` in git-bash | paren path `dev(dave)` breaks arg parsing |
| Read-only `original_files/` — MD5-verify untouched | pristine RE targets |
| Cite `binary + version @ RVA 0x…` on every ported symbol | unsourced addresses are worthless |
| Semantic `Mvl*` names in code, original `DAT_*` only as trailing comment | traceable AND modernizable (AGENTS.md binding rule) |
| Shared globals → single `engine_globals.{h,cpp}` | 71 duplicate per-TU statics found; consolidate early |
| Parallel lanes only with disjoint file scopes | concurrent writers → conflicts |
| Empty/stuck lane results = session is dead, fresh dispatch | 3 wasted retries taught this |
| Never re-run an identical failed command; bash paths as `/e/…` | paren-path flakiness |
| Each green tranche = milestone commit + README % table | cross-checkable progress history |

## 4. Metrics discipline (README milestone table)

- **Recompiled %** = ported ∩ MVL-specific census ÷ census. Never divide by the
  raw Ghidra function count (includes CRT/thunks/libs → misleading).
- **Accuracy %** = share of ported code that is 1:1 vs stubbed. Stub kinds are
  counted honestly: `TODO(` (unported callee) + `HOOK POINT` (engine write seam).
- Every commit appends a row; the previous % is struck through so any commit
  can be cross-checked against the tree state at that point.

## 5. Porting conventions (binding, see AGENTS.md)

1. Original RTTI/RE symbol names 1:1 (`CMvlVehicle::LoadXMLBasic`).
2. Unnamed functions → provisional name, confidence logged in `re-*.md`.
3. Unnamed data → semantic `Mvl*` name in code + `// DAT_x @ RVA 0x…` comment.
4. Everything cites `0.98e @ RVA 0x…`.
5. Engine-side writes: `HOOK POINT` seam with target VA — never fabricate.
6. MSVC x86 gotchas: Ghidra `__thiscall` free-fn → `__fastcall` + dummy EDX;
   `__declspec(naked)` only on definitions, never declarations.

## 6. Adapt-for-next-project checklist

- [ ] Copy harness skeleton (.sadie/.serena/memory-bank/.opencode — see AGENTS.md knowledge system)
- [ ] Set up headless disassembler + range/chain dump scripts (Ghidra 12.x: needs `subst` workaround for paren paths)
- [ ] Run SPEC MINING on data files before touching the binary
- [ ] Extract RTTI/vtable names → seed the symbol census
- [ ] Bind naming rules in AGENTS.md *before* first port lane
- [ ] Stand up green-build loop early (premake + MSBuild + deploy env var)
- [ ] Commit-per-tranche with % metrics from day one
