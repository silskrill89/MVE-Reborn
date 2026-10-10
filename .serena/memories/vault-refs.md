# Knowledge Vaults & SDKs

## Obsidian Vault — MVL RE (docs/)

Path: `E:/dev(dave)/projects/Maxos_vehicle_revnres/docs`
REST API: https://127.0.0.1:27124 (Obsidian Local REST API plugin; bearer key in `tools/obsidian-mcp-wrapper.ps1`)
SSL: self-signed (ssl._create_unverified_context())
Content: MVL reverse engineering notes — XML schema, extraflags bitmask, version diffs, engine patch sites. Currently a stub (`index.md` + `MVL/` folder); grows as RE progresses.

## SDKs / References

- **reVC engine source (Rosetta stone)**: `E:/dev(dave)/releases/GTASource/GTA VC RE/source/re3-miami` — full reVC source (390 .cpp; src/{vehicles,modelinfo,core,render,save,...}). Compare values/variables against reversed DLL: struct layouts (CVehicle, CVehicleModelInfo, CStreaming), function semantics, VC 1.0 addresses to anchor FUN_1000xxxx names.
- **librw**: `E:/SDKs/librw` — open-source RenderWare reimplementation; binary-format reference (DFF/TXD/geometry/materials).
- **RenderWare 3.6 SDK (official)**: `E:/SDKs/RW36_031126/RW36_031126` — full SDK: `export/include` (authoritative struct/chunk-format headers), `export/lib`, `tools/` (rwspitxd etc.), `viewers/`, `shared/skel`, `readme_Win.pdf`. Wire formats from RW36 headers + game semantics from reVC.
- **plugin-sdk (VC variant)**: `E:/SDKs/plugin-sdk-master/plugin_vc` — VC class headers (game_vc/*.h) with VC 1.0 addresses/structs pre-matched; hook-declaration source.
- **WindowedMode (MVL derivative, MIT)**: `E:/SDKs/III.VC.SA.WindowedMode` — maxorator's windowed-mode code (v2.2 rewrite); pattern-based per-exe bindings + D3D8 reset flow; include nearly as-is for mvl/extras windowed module.
- **Open Limit Adjuster (maxorator contributor)**: `E:/SDKs/III.VC.SA.LimitAdjuster` — limit patch addresses/logic (vehicle IDs, handling, model indices); cross-check for mvl/engine limits module.
- **io_scene_dff (maxorator co-author, MIT)**: `E:/dev(dave)/tools/io_scene_dff-master` — DFF chunk-ID table + binary layouts; spec source for C++ DFF parser.
- Still to add: VC 1.0 exe bases, 7z SDK, Python 3.3 SDK locations.
