# MVL-clone — Master Plan (Maxo's Vehicle Loader reverse → rebuild → port)

> Status: infrastructure done (M0). Plan owner: orchestrator. This file is the
> authoritative deployment + RE roadmap. Session rules live in `AGENTS.md`.

## 1. Mission

1. **Reverse** Maxo's Vehicle Loader (MVL) — GTA **Vice City 1.0** game-code-patching
   DLL plugin by maxorator — using the full version ladder in `original_files/`.
2. **Rebuild** it as a clean, compilable C/C++ clone (this repo, `vehmod.dll`),
   functionally at parity with MVL 0.98e.
3. **Port** the vehicle-extension layer to higher engine versions
   (GTA SA first), for later integration into skygfx_plus (vehicle-limit expansion)
   and/or a community reVC/re3 build that expands and fixes the engine.

## 2. Hard constraints (user directives)

| # | Constraint |
|---|------------|
| C1 | **Pure C/C++ only** — no Rust. Write fast, lean C code in the RenderWare-era idiom; match original performance. |
| C2 | The original is a **language mix** (C++ core + embedded Python 3.3 + XML configs). Our C++ core replaces all of it; the Python-hosting subsystem is a late phase, behind a C ABI bridge. |
| C3 | Config format progression: game uses **INI** → MVL uses **XML** → our open end-version uses **O3DE-style JSON** (settings-registry / layered-JSON conventions) for parity with the O3DE community's config quality. Phase A keeps 1:1 MVL XML compatibility; the JSON layer is additive, not a breaking rename. |
| C4 | x86 only (VC 1.0 is 32-bit), static CRT (/MT), MBCS. |
| C5 | `original_files/` is read-only RE material. Every RE claim cites `binary + version + RVA` (e.g. `vehmod_retail.dll 0.98e @ RVA 0x1234`). |

## 3. RE targets & materials (from recon, 2026-10-10)

- **Primary**: `original_files/maxovehmod098e/mvl/vehmod_retail.dll` (760,832 B,
  PE32 x86, built 2013-12-08) + `vehmod_steam.dll` (same size, different bytes).
- **Loader shims**: 0.9–0.98b = single `mss/vehmod.flt` (game auto-loads .flt from
  mss/); 0.98c+ = 532 B `vehmod_chooser.flt` that selects `mvl/vehmod_retail.dll`
  or `vehmod_steam.dll` by host exe.
- **Aux**: `xmlgen_launcher.exe` (47,616 B; Alt+4 dumps vlist/ XMLs),
  `libpng15.dll` (identical 098a–098e), bundled `python33.dll`.
- **Version ladder**: 0.9 (146,944 B, 2011) → 0.98e (760,832 B, 2013). Every
  version's main binary is unique — ideal for incremental diffing.
- **Free behavioral spec** (no RE needed): readmes per version, `flags.html`
  bitmask calculator, ~130 sample vehicle XMLs (incl. commented/), 35 weapon
  XMLs, `script_functions.txt` (Python API), 17 sample scripts,
  `sample_globalm.xml`.
- **Readable derivative code**: `ThirteenAG/III.VC.SA.WindowedMode` (MIT,
  "originally created by maxorator as part of MVL") — real hooking code;
  maxorator also contributed to `limit_adjuster_gta3vcsa`.
  - Cloned to `E:\SDKs\III.VC.SA.WindowedMode` (2026-10-10): full
    windowed/resizable/menu/autopause implementation + pattern-based per-exe
    binding DB (GTAVC.ixx byte patterns) — include nearly as-is (see
    docs/MVL/intel-log.md reusable-parts table).
  - `E:\SDKs\III.VC.SA.LimitAdjuster` (Open Limit Adjuster, maxorator credited
    contributor): memory addresses + patch logic for the hardcoded limits MVL
    breaks (vehicle IDs, handling, model indices) — the "engine of MVL's
    limits"; cross-check against our engine-limits module.
  - `E:\dev(dave)\tools\io_scene_dff-master` (MIT, co-author maxorator): DFF
    chunk-ID table + binary layouts = spec source for our C++ DFF parser
    (COL internals/TXD rasters come from reVC/librw/RW36 SDK).
- **Archived tracker**: Wayback has `bitbucket.org/ThirteenAG/mvl/issues`
  (2022/2023 snapshots): 6 issues — #1 SkyGFX compat, #2 jump lags, #3 v0.99
  download, #4 sharptrails+dynamic-shadows crash, #5 GTA:Liberty City compat,
  #6 wheel scale. 0.99 binaries NOT recoverable from tracker; **0.99d survives on
  ModDB** (1.78 MB, MD5 7789189644c47c75663faa6165afa720) — fetch live and add
  as `original_files/maxovehmod099d/` to extend the ladder.
- **Thread**: gtaforums.com/topic/477801-maxos-vehicle-loader/ (76 pages) —
  author-confirmed engine-patch list + bug reports usable as parity test cases.

## 4. RE methodology (version-diff first)

1. Tool check: Ghidra (preferred; scriptable diffing) / IDA. Import 0.98e retail.
2. Diff adjacent versions across the ladder to isolate each feature's code
   growth (see size table in §3 recon record / b1):
   0.9→0.91→…→0.98e (23 versions, ~14 growth steps). Growth bursts at 0.96d,
   0.97, 0.98, 0.98a, 0.98c correlate with changelog features.
3. Per-function logging: `.serena/memories/re-<subsystem>.md`, one claim per
   line, cited as `binary version @ RVA`. Patch sites also go into
   `docs/MVL/patch-sites.md` (version, RVA, original bytes, purpose).
4. Behavior-first: validate inferred features against sample XMLs/flags.html
   before deep-diving assembly.
5. Extract strings, XML tag names, file paths (e.g. `mvl/vehicles`,
   `vehmod_log.txt`) as anchors — cheap, high-signal.

## 5. Clone architecture (target src/ modules)

| Module | Replaces (original) | Notes |
|---|---|---|
| `mvl/core` | DllMain, logging, hotkey dispatcher, config dispatch | entry lives in `src/core/main.cpp` (already scaffolded) |
| `mvl/config` | XML readers (globalv/globalm/vehicle/weapon) | Phase A: MVL-XML compat reader; Phase B: O3DE-style layered JSON |
| `mvl/archive` | 7z + PNG consumption | 7z lib decision: bundled 7z SDK vs minizip-class; libpng15 already a known dep of the original |
| `mvl/models` | DFF/TXD/COL loaders, model store + store-limit overrides | SA asset loading (COL2/COLL/COL3 in DFF), TXD/COL file limits |
| `mvl/vehicle` | vehicle slots 50→127/200, spawning, wheels, tuning, colors, EM lights | hot-reload watcher on mvl/vehicles |
| `mvl/audio` | engine/horn/siren/boat/RC samples, sfx.sdt-style filename encoding | custom sounds need 0-compression 7z packing — replicate exact read path |
| `mvl/ai` | extraflags bitmask, gang/class/freq/level assignment | 27 flags from flags.html |
| `mvl/weapons` | 4 vehicle weapons, custom weapons (mvl/weapons, pickups.xml, logicalid 80/81) | |
| `mvl/scm` | built-in opcodes 0800/0801/0802/0810 + user-registered opcodes (typed i/f/V/P/O) | |
| `mvl/script` | Python 3.3 host (mvl/scripts, script_functions.txt API) | LAST phase; C ABI bridge so core never depends on python33 |
| `mvl/engine` | pool limits (ptrnode…colmodel), streaming memory, IMG/IDE fix, heavy-collision fix, sound overflow fix, color-material limit, RC entry fix | the "limit adjuster" side; cross-check limit_adjuster_gta3vcsa |
| `mvl/extras` | windowed mode+menus, real-time shadows, bullet holes, scope, autointeriors, decapitations, trashbin, roadthingy, antialias | cross-check III.VC.SA.WindowedMode source |
| `mvl/save` | save integration (proxies in main save + sidecar XML keyed by archive checksum+name) | |

## 6. Agent deployment plan (5–10 lanes)

### Phase 1 — Spec extraction (2 agents, parallel, ~half-day)
- **S1 @explorer**: mine all data files → `docs/MVL/spec/{xml-schema,flags,hotkeys,audio-naming,python-api}.md`. Output: normalized feature matrix.
- **S2 @librarian**: deep-scan forum thread pages era-by-era (2011–2018) for
  author-stated patch sites + crash reports → `docs/MVL/known-bugs.md`.

### Phase 2 — Binary RE (5–6 agents, parallel per version pair)
- **R1**: 0.9→0.93 — core loader, 7z reading, model-slot expansion (50→127).
- **R2**: 0.94→0.96e — SA asset loading, colors, xmlgen, MSVCR100 drop.
- **R3**: 0.97→0.98b — tuning, Python embed, globalm model/pool limits.
- **R4**: 0.98c→0.98e retail vs steam — save system, weapons, windowed mode, final targets.
- **R5**: `vehmod_chooser.flt` (532 B — trivial) + `xmlgen_launcher.exe` — quick wins, confirms loader/exe-detection mechanism.
- **R6** (@oracle review gate): classify all discovered patch sites into engine subsystems → finalize hook architecture + injection order for `mvl/core`.

Each R-agent logs to `.serena/memories/re-<subsystem>.md` + `docs/MVL/patch-sites.md`.

### Phase 3 — Implementation (3–4 @fixer lanes, sequential batches)
Batch A: `mvl/core` + `mvl/archive` + `mvl/config` (XML compat).
Batch B: `mvl/models` + `mvl/vehicle` + `mvl/engine` (limits).
Batch C: `mvl/audio` + `mvl/ai` + `mvl/weapons` + `mvl/scm`.
Batch D: `mvl/extras` + `mvl/save`.
Batch E: `mvl/script` (Python host) — only if still wanted after core parity
(consider replacing with C++ script API; keep C ABI bridge either way).

### Phase 4 — Parity verification
- Build feature matrix per MVL version (readme/changelog as checklist).
- Test cases from archived Bitbucket issues (#1 SkyGFX compat, #4 shadows crash,
  #6 wheel scale, …) + forum crash reports (replays+weapons, misc_a/misc_b dummies,
  hidden packages, 5-star wanted, quicksave >32k models).
- Deploy into VC 1.0 via `MVL_GAME_DIR`, run alongside original for A/B.

### Phase 5 — Port & integration (separate plan later)
- SA port of the vehicle-extension layer; skygfx_plus integration for expanded
  vehicle limits; optional reVC community-build contribution.

## 7. Milestones

| M | Definition of done |
|---|--------------------|
| M0 | Infra: scaffold compiles (`bin/Release/vehmod.dll`), harness cloned ✅ 2026-10-10 |
| M1 | Spec docs complete (Phase 1 output) |
| M2 | RE database: 0.98e function inventory + full patch-site table |
| M3 | Minimal clone: loads one 7z vehicle, spawns it (read-path only) |
| M4 | Full 0.98e parity matrix green |
| M5 | 0.99d added to ladder + parity (0.99d fetched from ModDB) |
| M6 | O3DE-style JSON config layer shipped alongside XML compat |
| M7 | SA port merged into skygfx_plus or reVC build |

## 8. Open questions

- RE tooling availability on this machine (Ghidra install path?) — blocks Phase 2 dispatch.
- `MVL_GAME_DIR` real value (VC 1.0 install path) — ask user before Phase 4.
- 7z implementation choice for `mvl/archive` (vendored 7z SDK vs static extract-only).
- Whether `mvl/script` (Python host) survives to the end-version or gets replaced
  by a native C++ script API.
