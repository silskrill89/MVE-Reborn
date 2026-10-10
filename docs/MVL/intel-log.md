
## 2026-10-10 — Tracker screenshot (user-supplied)
Source: Bitbucket UI screenshot of bitbucket.org/ThirteenAG/mvl/issues (matches Wayback 2022/2023 captures).
Confirmations beyond Wayback text: updated-dates per issue — #2 Lags during jumping (upd 2016-11-17), #6 Wheel Scale (upd 2016-10-29, assignee Sergey P./ThirteenAG), #1 SkyGFX compat (upd 2016-05-30), #4 sharptrails+dynamic-shadows crash (upd 2016-05-01), #5 GTA:Liberty City compat (upd 2016-03-08, assignee ThirteenAG), #3 v0.99 download RESOLVED (task, 2 votes, upd 2016-03-26). Status: all NEW except #3.

## 2026-10-10 — Recyclable source registered (exp-1)
- **E:\SDKs\III.VC.SA.WindowedMode** (MIT, ThirteenAG; originally maxorator's MVL windowed-mode code, v2.2): full windowed/resizable/menu/autopause implementation; pattern-based per-exe binding DB (GTAVC.ixx byte patterns: RsGlobalType, GameState, DisplayMode**, menuOpen/menuClose bools, RwCamera**); D3D8 reset flow docs (Bindings.h ResetResult); packed-exe deferral via GetSystemTimeAsFileTime hook. Include nearly as-is: Window.cpp/Renderer.cpp/Menu.cpp/Settings.cpp/common.h + binding patterns (strip .ixx modules). Submodules (hooking/injector/IniReader) NOT initialized — pull upstream. MVL globalm.xml mapping: windowed/resizable/usemenus/menuonrestore confirmed; fixframewait moved to WidescreenFix in v2.2.
- **E:\dev(dave)\tools\io_scene_dff-master** (MIT, PerikiyoXD 2018; co-author **maxorator** — the MVL author's own DFF plugin): complete DFF chunk-ID table (RwTypes :22-48), version-stamp decode (III 0x0800FFFF/VC 0x1003FFFF/SA 0x1803FFFF), full binary layouts for GEOMETRY/FRAMELIST/MATERIAL/BINMESHPLG/MATEFFECTS/etc. = spec source for our C++ DFF parser. Gaps: COL internals opaque (use reVC/librw/RW36 SDK), no TXD raster support.

## 2026-10-10 — User intel: Open Limit Adjuster + WindowedMode doctrine
- **E:\SDKs\III.VC.SA.LimitAdjuster** (Open Limit Adjuster / OLA, ThirteenAG; maxorator credited contributor): contains the memory addresses + patch logic for the hardcoded game limits MVL breaks (vehicle IDs, handling entries, model indices — VehicleStructs, HandlingData). Treat as the "engine of MVL's limits" — shared research showing where limits live in VC 1.0 memory. Verified on disk (src/, doc/, premake5.lua, CHANGELOG, LICENSE).
- **III.VC.SA.WindowedMode doctrine (user-confirmed)**: MVL included a built-in windowed-mode hack (Alt+Enter) for testing; ThirteenAG's standalone ASI preserves maxorator's logic — D3D device-creation hooks + CreateWindow hooking = proof maxorator used standard memory-injection techniques (same family as injector.hpp), not obscure hacks. Core files: source/Window.cpp, source/Renderer.cpp (D3D present-params forcing), source/Bindings.h.
- Both repos = the two confirmed derivative-source leads: WindowedMode = direct extraction of MVL display management; OLA = shared limit-patch research.

## 2026-10-10 — Hunt find: Almost610's MVL Resources site (LIVE)
- URL: https://almost610.ucoz.com/blog/welcome/2012-12-23-4 (run by ALMOST610, not maxorator; site live as of 2026-10)
- Sections: Vehicle Downloads (/blog/vehicle_downloads/2012-12-23-2), Weapon Downloads (/blog/weapon_downloads/2012-12-23-3), XML Builder (/blog/xml_builder/2012-12-23-1), Video Tutorials (/blog/video_tutorials/2013-04-16-9)
- **XML Builder 0.60** (Apr 6 2013): online MVL XML generator, supports vehicle XMLs + globalm.xml; changelog on XML Builder page. Schema drift data point: "dimensions field not needed in MVL 0.98C but required in all previous versions".
- **Vehicle + Weapon download pages = ready-made parity test corpus** (community 7z archives incl. Frank.S "Pike" — first MVL weapon, made during weapon-feature testing with maxorator).
- Timeline cross-check: MVL 0.98 Nov 12 2012, 0.98a Dec 31 2012, 0.98b Jan 7 2013, 0.98c still pending Apr 2013; Dec 13 2012 test build added towing + weapons + random path generator.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.

## 2026-10-10 — Hunt find: XML Builder dotNet Beta 1.1 (in original_files/, READ-ONLY)
- Path: original_files/XML Builder dotnet Beta 1.1 - Release/ (2.7MB; Almost610's .NET GUI builder, beta 1.1)
- Contents: XML Builder dotNet.exe + AxInterop/Interop.WMPLib.dll; Data/{Main,Car,Bike,cstmcols}.mvl (builder project files — likely XML templates per vehicle type); Data/7zr.exe (7-Zip CLI — confirms builder packs 7z via 7zr); XmlSamples/ (~130 vehicle XMLs, same set as 098e help/); AudioSamples/{Engines,Horns,Sirens,Doors}/*.mp3
- **AudioSamples naming = VC audio sample-index map**: Engines "far_268_near_4_SABRE.mp3" … far_287_near_23_PCJ — encodes VC engine sound far/near sample index pairs per vehicle; Horns horn_0..7, Sirens, Doors door_0..6 — useful spec data for our audio module (mvl/audio) sample-index tables.
