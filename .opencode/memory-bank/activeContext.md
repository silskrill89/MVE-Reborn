# Active Context — MVL-clone

## Current Date: 2026-10-10

## Status: Scaffolding / harness setup

## Current State
- Harness cloned from skygfx_plus_expIV and adapted for MVL domain (memory-bank, .serena, .sadie, .opencode, .harness-memory, .obsidian, docs/).
- sadie.db initialized fresh; vault sync pointed at `docs/`.
- RE targets present in `original_files/`: maxovehmod 0.9 → 0.98e (+ xmlgen_launcher).
- No RE work started yet; no clone code in src/ yet (orchestrator-owned).

## Open Questions
- `MVL_GAME_DIR` (VC 1.0 game dir for deploy) — unknown, placeholder.
- Which binary to start RE on: 0.98e `vehmod_retail.dll` (primary) vs `.flt` variants.
- Tooling for RE: Ghidra vs IDA import workflow (see `.opencode/skills/mvl-re/SKILL.md`).

## Next Steps
1. Baseline RE pass on 0.98e: identify DllMain, XML parser entry, 7z loader, Python 3.3 embedding.
2. Version-diff adjacent versions (0.9 → 0.98e) to isolate feature code.
3. Log findings into `.serena/memories/re-*.md` and docs/ vault.
