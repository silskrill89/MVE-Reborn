# MVL-clone — Expansion Questionnaire (v2, anchored to original PLAN.md)

> How this works: the **original PLAN.md is untouched** (git commit `1916452`,
> restored verbatim). Each stage below quotes what the plan already says, then
> asks questions whose answers **expand that section** into 10 numbered major
> points (complex points get a–e sub-points). After you answer, I merge your
> answers into the plan — the original points stay, the padding is yours.
>
> Answer style: anything goes — one word, a URL, a paragraph. Skip anything
> (defaults noted as *[default: …]*). Answers keyed `Q<stage>.<n>`.

---

## Stage 1 → expands §1 Mission *(currently 3 points → pad to 10)*

Existing: reverse MVL (VC 1.0) → rebuild as C/C++ clone at 0.98e parity → port to SA → integrate into skygfx_plus / community reVC.

- **Q1.1** What else belongs in the mission? Any deliverable I missed (docs, tools, source release, installer)? *[default: none]*
- **Q1.2** Rebuild the `vehmod_chooser.flt` shim too, or only the main `mvl/vehmod_retail.dll`? *[default: rebuild both]*
- **Q1.3** Target parity version: 0.98e only, or also 0.99d once fetched? *[default: 0.98e first, 0.99d after]*
- **Q1.4** Non-goals to declare (things we will NOT do)? *[default: byte-identical binary, obfuscation, anti-cheat]*
- **Q1.5** Must the clone keep the same file/dir names (`mvl/`, `vehmod_log.txt`, hotkey set) for user-facing compatibility? *[default: yes]*
- **Q1.6** Is "clonable RE methodology" (like the hybrid Ghidra/IDA skill) part of the mission deliverables? *[default: yes, as docs/ skill]*
- **Q1.7** First demo definition: what must the very first in-game success look like? *[default: one 7z vehicle archive spawns with wheels+audio]*
- **Q1.8** Priority order if features conflict: vehicle archives / engine limits / extras / scripting? *[default: archive→limits→extras→script]*
- **Q1.9** Who is the end user of the clone: you, the community, both? *[default: both]*
- **Q1.10** Anything about the mission statement itself you want reworded/added? *[default: keep as-is]*

## Stage 2 → expands §2 Hard constraints *(C1–C5 → pad to 10)*

- **Q2.1** Compiler pin: MSVC 2022 only, or also allow clang-cl? *[default: MSVC 2022]*
- **Q2.2** C++ standard ceiling? *[default: C++17 (VS2022 default-safe)]*
- **Q2.3** Third-party policy: vendor sources into `external/` vs system/SDK paths? *[default: vendor everything needed to build]*
- **Q2.4** Must the clone stay dependency-light like the original (kernel32/user32 + libpng15 + python33 only)? *[default: yes, same import surface]*
- **Q2.5** Exceptions you'd allow to "pure C/C++"? *[default: none]*
- **Q2.6** Perf acceptance: "same as original" — any concrete metric (FPS, load time)? *[default: no measurable regression in-game]*
- **Q2.7** Warning policy: /W4 clean? warnings-as-errors? *[default: /W3 clean-ish, /W4 aspirational]*
- **Q2.8** Testing constraint: automated unit tests allowed/required, or in-game probes only? *[default: in-game probes + doc tests]*
- **Q2.9** Git policy: commit granularity you want? *[default: per green batch]*
- **Q2.10** Any constraint from your O3DE/open-source goals to bake in (licensing, headers)? *[default: MIT-style for our code]*

## Stage 3 → expands §3 RE targets & materials *(your internet hunt lives here)*

- **Q3.1** Hunt finds — earlier versions? Path/URL → *(add to ladder)*
- **Q3.2** Hunt finds — 0.99a/b/c binaries or mirrors? *(ModDB 0.99d already known: MD5 778918…)*
- **Q3.3** Hunt finds — lost trackers (Gitorious/Google Code/GitHub mirrors)?
- **Q3.4** Hunt finds — community tools (XML builder,Almost610's site, wikis)?
- **Q3.5** Hunt finds — VC 1.0 exe bases / existing .idb/.i64 databases?
- **Q3.6** Do you have the VC 1.0 retail exe anywhere I can hash/anchor against (H:\games\VC\gta-vc.exe confirmed — use it)? *[default: use H:\games\VC]*
- **Q3.7** Any MVL-related files outside `original_files/` you want registered (saves, logs from old runs)?
- **Q3.8** Contact maxorator on GTAForums (profile 146289, last active 2018)? *[default: not yet]*
- **Q3.9** Format for intel log entries — file+URL+date+finding OK? *[default: yes → docs/MVL/intel-log.md]*
- **Q3.10** Priority if hunt yields lots: old versions first, trackers first, or tools first? *[default: old versions first]*

## Stage 4 → expands §4 RE methodology *(currently 5 steps → pad to 10)*

- **Q4.1** Name-anchoring depth: auto-rename Ghidra functions from plugin_vc/reVC symbol tables — aggressive, or conservative (only high-confidence)? *[default: conservative first pass]*
- **Q4.2** Stop-rule per function: when do we call it "understood"? *[default: inputs/outputs/side-effects/callers mapped + cross-checked vs spec]*
- **Q4.3** Patch-site logging format OK (`version @ RVA` + original bytes + purpose in patch-sites.md)? *[default: yes]*
- **Q4.4** Decompile-first or assembly-first for logging claims? *[default: decompile-first, assembly to disambiguate]*
- **Q4.5** Diff granularity: binary-level (bytes) or function-level (Ghidra hash-match)? *[default: function-level via Ghidra version tracking]*
- **Q4.6** Python33/libpng15 internals: RE them too, or treat as black-box deps? *[default: black-box]*
- **Q4.7** Steam-vs-retail diff: one unified clone with runtime exe detection (like chooser.flt), or two builds? *[default: unified with runtime detection]*
- **Q4.8** Glossary artifact wanted (terms: store, pool, flag names) in docs/MVL? *[default: yes]*
- **Q4.9** Do we log *rejected* hypotheses too (helps reconstruction history)? *[default: yes, one line each]*
- **Q4.10** Weekly RE summary artifact for you to review? *[default: docs/MVL/re-weekly.md]*

## Stage 5 → NEW section: Version-diff categorization & documentation *(grows from §4 step 2)*

- **Q5.1** Delta category taxonomy OK: config schema / engine patch / rendering / audio / scripting / loader / extras? *[default: yes]*
- **Q5.2** Master artifact location/name: `docs/MVL/version-matrix.md`? *[default: yes]*
- **Q5.3** Row granularity: per version or per growth step? *[default: per version, grouped by step]*
- **Q5.4** Want a chronological "how the program was made" narrative doc (`history-reconstruction.md`)? *[default: yes]*
- **Q5.5** Attribution policy when changelog and binary disagree — which wins? *[default: binary is truth]*
- **Q5.6** Should each matrix row link to its re-*.md evidence files? *[default: yes]*
- **Q5.7** XML schema drift documented as its own matrix (global.xml→globalv.xml→globalm.xml)? *[default: yes]*
- **Q5.8** Python API drift (`script_functions.txt` per version) tracked too? *[default: yes]*
- **Q5.9** flags.html flag-set diff per version included? *[default: yes]*
- **Q5.10** Who curates the matrix: me (orchestrator) per R-lane, or a dedicated doc agent? *[default: me, per lane]*

## Stage 6 → expands §5 Clone architecture *(13 modules — decisions needed)*

- **Q6.1** Module granularity final: keep all 13, or merge some? *[default: keep 13]*
- **Q6.2** 7z implementation: vendored 7z SDK (LZMA SDK, C) vs extract-only minimal reader? *[default: LZMA SDK, extract-only]*
- **Q6.3** Python host: keep embedded Python 3.3, replace with native C++ script API, or both? *[default: replace with native API, keep bridge]*
- **Q6.4** PNG: keep libpng15 (match original import surface) or ministb/stb_image? *[default: match original → libpng15]*
- **Q6.5** Hot-reload watcher: replicate exactly (poll mvl/vehicles) or improve (ReadDirectoryChangesW)? *[default: replicate exact]*
- **Q6.6** Log format: keep `vehmod_log.txt` format byte-compatible? *[default: yes]*
- **Q6.7** Hook style: injector lib (C++, like skygfx) vs hand-rolled jmp patches (like original)? *[default: hand-rolled to match original]*
- **Q6.8** Namespace: `mvl::` C++ vs flat `MVL_` C-style? *[default: flat C-style at ABI boundaries, mvl:: inside]*
- **Q6.9** Retail+steam from one DLL at runtime? *[default: yes]*
- **Q6.10** Config: XML parser choice (vendored tiny parser vs full lib) for Phase A? *[default: vendored minimal XML reader like original's]*

## Stage 7 → expands §6 Agent deployment *(S1/S2 + R1–R6 + phases)*

- **Q7.1** GREENLIGHT Phase 1 now (S1 data-spec mining + S2 forum deep-scan)? *[default: awaiting you]*
- **Q7.2** Any lane ordering preference in Phase 2 (R1–R5) or all parallel? *[default: parallel, write-scopes disjoint]*
- **Q7.3** Max concurrent agents you want running? *[default: 5–6]*
- **Q7.4** Oracle gate timing: after Phase 2 (as planned) or also mid-Phase 3? *[default: after Phase 2 + per-batch reviews]*
- **Q7.5** Do you want per-lane progress reports posted here, or only at milestones? *[default: milestones + on failure]*
- **Q7.6** Implementation batches A–E sequential or overlap where modules are disjoint? *[default: A→B→C→D sequential; E last]*
- **Q7.7** Review lane: @oracle code review per batch, or your eyes only? *[default: oracle per batch + your eyes at M3/M4]*
- **Q7.8** Sadie/vault indexing after each lane or daily? *[default: per lane]*
- **Q7.9** Should R-lanes also draft clone code stubs, or pure RE notes first? *[default: RE notes first]*
- **Q7.10** Escalation rule: when a lane fails twice, escalate to oracle automatically? *[default: yes]*

## Stage 8 → NEW section: Implementation pipeline & build doctrine *(grows from §6 Phase 3)*

- **Q8.1** Batch order OK (A core/archive/config → B models/vehicle/engine → C audio/ai/weapons/scm → D extras/save → E script)? *[default: yes]*
- **Q8.2** Definition of Done per batch: compiles + in-game probe + docs updated — enough? *[default: yes]*
- **Q8.3** Feature-toggle approach: config flags to enable/disable half-built features during development? *[default: yes, via globalm.xml]*
- **Q8.4** Branch model: main + per-batch branches, or straight commits to main? *[default: straight to main, per green batch]*
- **Q8.5** Deploy ritual: b.bat deploy to H:\games\VC automatic after each build? *[default: yes, guarded]*
- **Q8.6** Crash handling during dev: minidump+log on unhandled exception like original? *[default: yes]*
- **Q8.7** Debug menu/test hotkeys ported from MVL's testmenus? *[default: later, Phase 3D]*
- **Q8.8** CI-ish checks: premake+build+size-report script per commit? *[default: simple b.bat check]*
- **Q8.9** Keep clone versioning scheme (0.0.x dev → 0.98e parity)? *[default: yes]*
- **Q8.10** Anything to forbid during implementation (patterns, libs)? *[default: no exceptions to CRT-safety rules]*

## Stage 9 → expands §7 Milestones *(M0–M7 → pad to 10 with acceptance criteria)*

- **Q9.1** Milestone ordering OK (M1 spec → M2 RE db → M3 minimal clone → M4 parity → M5 0.99d → M6 JSON → M7 port)? *[default: yes]*
- **Q9.2** Parity test-case set: tracker issues + forum crashes enough, or add your own scenarios? *[default: tracker+forum]*
- **Q9.3** M3 demo sign-off: you watch it in-game? *[default: yes]*
- **Q9.4** A/B test method: original and clone both installed, swap via MVL_GAME_DIR? *[default: yes]*
- **Q9.5** Save compatibility: must MVL-created saves load in the clone unchanged? *[default: yes at M4]*
- **Q9.6** Regression checklist owner: me auto-generated or you manual? *[default: auto-generated + your eyes]*
- **Q9.7** Any milestone you want inserted (e.g. "M3.5 hotkeys work")? *[default: none]*
- **Q9.8** JSON layer (M6) timing: after parity or parallel? *[default: after M4]*
- **Q9.9** Definition of M4 "full parity" — feature matrix 100% green, or known-gaps list acceptable? *[default: matrix green + documented gaps]*
- **Q9.10** Time-box expectations per milestone (days/weeks) you care about? *[default: no hard dates]*

## Stage 10 → expands §8 Open questions + standing items

- **Q10.1** 7z lib final answer (from Q6.2 — repeat here only if you want to override)? *[default: LZMA SDK extract-only]*
- **Q10.2** Python host final answer (from Q6.3)? *[default: native C++ API]*
- **Q10.3** Intel log file OK at docs/MVL/intel-log.md? *[default: yes, created]*
- **Q10.4** Lost-tracker shortlist to add? *[default: Google Code archive, Gitorious mirrors, GitHub issue mirrors]*
- **Q10.5** Any standing instruction for me (like the Notepad++ auto-open rule) to record? *[default: none new]*
- **Q10.6** Review ritual: auto-open changed docs in Notepad++ every time, or on request? *[default: every time]*
- **Q10.7** Author PM (from Q3.8 — escalate here if you want it as a plan item)? *[default: not yet]*
- **Q10.8** Should the 10-padded structure become the permanent format for ALL future project plans? *[default: yes — cloneable methodology]*
- **Q10.9** After padding, do you want a diff view (v1 → padded) in Notepad++ for review? *[default: yes]*
- **Q10.10** Return style: paste answers inline here, or edit QUESTIONNAIRE.md directly and say "done"? *[default: paste inline]*

---

*Version: 2026-10-10 v2 — rebuilt anchored to original PLAN.md (v1, git 1916452).*
