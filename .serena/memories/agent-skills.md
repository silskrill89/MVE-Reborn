# Agent Skills Seed — MVL-clone

> NOTE: Sections below marked [skygfx-historical] are evidence carried over from the source project (skygfx_plus_expIV). Replace with MVL RE evidence as it accumulates.

_Evidence date: 2026-09-14. Items marked **inferred** are routing boundaries derived from the supplied evidence, not observed failures._

## Cross-wave routing

- Every successful write lane began with an idempotence check, named a single file or explicit file list, and said **do not build**.
- Read-only verifier lanes produced the wave's highest-value findings: one use-after-free and three HIGH shader findings.
- The failed lanes were model-availability failures, not task-complexity failures; check route health and re-dispatch failed work on a different model.
- Exact file:line, register names, and expected values produced precise work; vague prompts produced summaries.

## fixer-principal

**Model:** `opencode-go/deepseek-v4-pro`

**Use it for:** Naked-hook implementation, specifically the verified `0x7618B0` `PSSetCameraConstantD_Guard` injection.

**Do not use it for:** **Inferred:** vague hook work without verified disassembly, replay bytes, resume offsets, and a narrow write scope.

**Prompt shape that worked:** Step 0 idempotence check; full verified disassembly; exact prologue-replay bytes; three legal resume offsets; exact writable file; **do not build**; reuse the existing `rxD3D9DefaultRenderCallback_Hook` naked-hook idiom.

**Failure modes observed:** None in the supplied run. **Inferred risk:** omitting the byte-level details or idempotence step removes the conditions that made this run precise.

## oracle

**Model:** `opencode-go/deepseek-v4-pro`

**Use it for:** Read-only independent review of buffer fixes, including leak, double-release, stage-collision, and leftover-render-target regression hunts.

**Do not use it for:** **Inferred:** mixing implementation into this lane; the successful framing was explicitly read-only.

**Prompt shape that worked:** Require a table with `defect | status | file:line | evidence | bounded fix` and an explicit regression hunt.

**Failure modes observed:** None for oracle. The contrasting observed failure was that the implementing lane missed the dangling `g_ssaoDepthTex` alias after `DepthHook_ReleaseResources`; oracle found it as a new HIGH use-after-free.

## shader-verifier-a-go

**Model:** `opencode-go/deepseek-v4.1-flash`

**Use it for:** The six named shaders' expected register declarations and bindings-vs-C++ consistency, with a doc-vs-code contradiction check.

**Do not use it for:** **Inferred:** verifier B's disjoint registers/samplers/resources/multi-entry remit; B was explicitly told not to duplicate A's bindings-vs-C++ audit.

**Prompt shape that worked:** Name all six shaders; list each expected register declaration up front; name exact C++ binding functions; require severity plus doc-vs-code contradictions.

**Failure modes observed:** None in the supplied run; it delivered three HIGH findings and no BLOCKER. **Inferred risk:** vague shader/register expectations produce summaries rather than precise contradictions.

## shader-verifier-b-go

**Model:** `opencode-go/deepseek-v4.1-flash`

**Use it for:** Independent registers/samplers/resources/multi-entry verification.

**Do not use it for:** A bindings-vs-C++ audit or any work duplicating verifier A.

**Prompt shape that worked:** State a clearly disjoint remit and explicitly say **do not duplicate a bindings-vs-C++ audit**.

**Failure modes observed:** None in the supplied run; it delivered three HIGH findings. The observed routing risk is remit overlap, which would make the second pass redundant rather than independent.

## fixer-mid

**Model:** `opencode-go/deepseek-v4-flash`

**Use it for:** Adding the SEH wrapper to `DrawPipeChain`.

**Do not use it for:** Applying the same treatment to sibling function `DrawSMAA`.

**Prompt shape that worked:** Step 0 explicit idempotence grep; warn that `DrawSMAA` must not be changed; require a C2712/destructor risk check before applying.

**Failure modes observed:** The same treatment on `DrawSMAA` triggers MSVC C2712. No failure was observed in the delivered `DrawPipeChain` wrapper.

## fixer-intern

**Model:** `openrouter/poolside/laguna-xs-2.1:free`

**Use it for:** Appending the exact listed content to the exact listed memory files.

**Do not use it for:** Reformatting existing content.

**Prompt shape that worked:** List exact files and content bullets; require **do not reformat existing content** and a duplicate check; note that the files are gitignored, so `git diff` will not show them.

**Failure modes observed:** None in the supplied run. The observed verification limitation is that `git diff` cannot prove these appends occurred.

## free-fixer

**Model:** `openrouter/nvidia/nemotron-3-ultra-550b-a55b:free`

**Use it for:** Adding a NaN-safety guard to a shader.

**Do not use it for:** The broken component pattern `normalize(v + 1e-7)`; guard vector magnitude instead.

**Prompt shape that worked:** Explicitly reject the broken pattern; require before/after lines and a grep proving no unguarded `normalize` remains.

**Failure modes observed:** The componentwise `1e-7` pattern is explicitly identified as broken. The delivered magnitude-guard change had no reported failure.

## free-librarian-or

**Model:** `openrouter/free`

**Use it for:** Producing a full implementation spec.

**Do not use it for:** An unstructured narrative instead of a ranked, bounded task list.

**Prompt shape that worked:** Number the required sections; require `file:line` for every claim; request a ranked bounded task list.

**Failure modes observed:** None in the supplied run. **Inferred risk:** the cross-wave evidence says vague prompts produce summaries rather than precise, citeable work.

## explorer

**Model:** Preset model

**Use it for:** **Inferred:** a read-only scout, but only when dispatched on an explicitly named live model.

**Do not use it for:** The rotating `explorer` preset itself; `explorer`/`librarian` presets have previously resolved to dead or unavailable models.

**Prompt shape that worked:** No successful prompt is established by this evidence; the observed routing fix is to name a live model explicitly.

**Failure modes observed:** `Session error` from model unavailability. Re-dispatch on a different model rather than retrying the same unavailable route.

## pickle-code

**Model:** `opencode/big-pickle`

**Use it for:** **Inferred:** small file-creation tasks such as a new shader include; this evidence contains no successful run.

**Do not use it for:** Retrying the same model after a burnout; the retry succeeded only after switching models.

**Prompt shape that worked:** No successful prompt shape is established by this evidence.

**Failure modes observed:** It crashed mid-task while creating a new shader include. A different model completed the retry.

## Promotion path

- Extraction threshold: **3 repeats** of an agent type.
- Formal promotion: **30 runs** of an agent type.
- Target: a shareable skill under `C:\Users\aaaaaaaaa\.config\opencode\skills\`.
- Raw evidence: `.opencode/agent-logs/<agent>.md`, generated from the `runs` table in `.sadie/sadie.db`.
