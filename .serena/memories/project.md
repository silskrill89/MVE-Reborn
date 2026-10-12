# Project: MVL-clone (Maxos_vehicle_revnres)

Reverse-engineering rebuild of **Maxo's Vehicle Loader (MVL)** — a GTA Vice City 1.0 DLL plugin (`vehmod_retail.dll`, also distributed as `.flt`). MVL implements an XML + 7z vehicle-archive configuration system with Python 3.3 scripting hooks and direct engine patching. Goal: clone the functionality from binary RE, then later port to GTA San Andreas.

## Status (2026-10-12): recomp sprint tranche-4 pushed
- 63% recompiled (105/167 census fns 1:1), accuracy ~100% of ported code; build GREEN; pushed to https://github.com/silskrill89/MVE-Reborn.git (5 commits, HEAD 7a0fc6e).
- src/ modules: loader, config, archive, engine, vehicle, weapons, scm. Full inventory + metrics in README.md + docs/MVL/re-daily.md.
- Questionnaire Stages 1–10 answered → PLAN.md §9 decision record. Parity baseline 0.99d.
- Board: fix-2/fix-3/fix-4 reusable; fix-1 dead (2 empty results).
- Next tranches: Python host (FUN_100287c0), missing hook stubs (0x51940–0x51c90, 0x234c0, 0801/0802), engine_globals consolidation, audio/PNG/IFP/EM-lights.

## RE Targets

| Target | Path | Notes |
|--------|------|-------|
| MVL 0.98e | `original_files/maxovehmod098e/` | Primary RE target (latest version) |
| MVL 0.9–0.98d | `original_files/maxovehmod*/` | Adjacent versions for diffing |
| xmlgen_launcher | `original_files/xmlgen_launcher/` | XML generator launcher |

## Key Domain Concepts

- **XML config system**: vehicle definitions in XML files next to the plugin
- **7z vehicle archives**: vehicle assets packed in 7z archives, loaded by the plugin
- **Python 3.3 scripting**: MVL embeds/hosts Python 3.3 for per-vehicle scripts
- **Engine patching**: direct code patches into the VC 1.0 exe (D3D8-era, x86)
- **extraflags bitmask**: per-vehicle feature flags (RE pending — log findings)

## Repo Layout

| Path | Purpose |
|------|---------|
| `original_files/` | READ-ONLY RE targets by version — never modify |
| `src/` | Clone implementation (orchestrator-owned) |
| `docs/` | Obsidian vault: RE notes |
| `.serena/memories/` | Curated RE findings (`re-*.md` per version) |
| `.sadie/` | SQLite knowledge index |

## Rules

- Cite binary + version for every RE claim (e.g., "vehmod_retail.dll 0.98e @ RVA 0x1234").
- Log patch sites per version into `.serena/memories/re-*.md`.
- Never modify anything under `original_files/`.
