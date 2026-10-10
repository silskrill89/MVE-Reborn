# RE Daily Log

## 2026-10-10 — Lane D2: static version-ladder diff (resumed after PC crash)

Completed the full 23-version static ladder diff (0.9 → 0.98e) with
`tools/ladder_diff.py` (stdlib PE parser: sections/imports/strings; no
Ghidra). 34 binaries scanned, 0 errors. Outputs:
`docs/MVL/version-matrix.md` (core artifact), `.serena/memories/re-ladder-static.md`
(analytical narrative + top-10 diff anchors), this log.

Highlights:
- Subsystem timeline pinned by string appearances: global.xml/nospawn 0.91,
  tuning 0.93, TXD parser 0.94, SA TXD+registry 0.94a, SA COL3 0.95, vehicle
  weapons 0.96, static-CRT toolchain switch 0.96d, boat/RC audio 0.96e, IFP
  anims 0.97, Python 3.3 host 0.98 (dynamic GetProcAddress, no import entry),
  windowed mode + custom weapons + libpng15 0.98a, pool-limit engine patch
  0.98b, WAV sounds + save XML + SCM opcodes + .flt→.dll 0.98c,
  autointeriors 0.98d, testmenus/streamingmem 0.98e.
- Retail vs steam DLLs (0.98c/d/e): identical size/imports/strings except PDB
  path and one dump filename embedding an exe address (0x640FE5 retail vs
  0x63FF95 steam at 0.98e) — steam = retail with retargeted exe addresses.
- libpng15.dll and vehmod_chooser.flt byte-identical across all their versions.
- Rejected hypotheses logged (0.96d jump = CRT switch, not features; chooser
  = 532-byte stub; python33 not statically imported).

## 2026-10-10 (later) — D3 synthesis (orchestrator, from d3out extraction)
- **BREAKTHROUGH**: 0.98e retains RTTI/class-name strings → 136 functions directly named (CMvlSettings/CMvlVehicle/CVehicleHandler/CMvlWeapon/CWeaponHandler/CWeaponPickups/CModelIndexLimit/CLongModelId/CCustomAnimFile/CSoundHandler/CSevenWalker/CPngFuncs/CScmHandler/CDxHandler/CHelpers/CVehicleSCM/CWeaponLogicHat). Full table in .serena/memories/re-098e-map.md.
- Init chain decompiled (81 fns): entry 0x10005c1d1 → FUN_10043c00 → PrepareMvlForLoad 0x2baf0 + LoadGlobalVXMLFile 0x38160 + hook installers 0x22560/0x22590/0x225a0 (VirtualProtect, 0x401000 region).
- Python host confirmed dynamic @ 0x287c0 (python33.dll + Py* GetProcAddress); mveh/manim/mvlbase module functions located.
- patch-sites.md skeleton written (10 seeded sites, hook stubs 0x3dff0–0x3e320 pending decompile).
- vcconsts.txt = 2,755 VC-range constant hits = retail address table; steam diff pending.

## 2026-10-10 (evening) — chooser shim solved + Ghidra MCP live
- vehmod_chooser.flt FULLY understood (capstone disasm, 64-byte .text): reads dword @ gta-vc.exe VA 0x61A7F8, fingerprint 0x9753C405→retail / 0xE8535004→steam, LoadLibraryA("mvl\vehmod_retail.dll"/"mvl\vehmod_steam.dll"), silent no-op on unknown exe. Note: .serena/memories/re-loader-shim.md.
- Ghidra MCP installed (bethington fork v7.0.0, bridge verified) — interactive agent-driven RE now possible once Ghidra GUI is open.

## 2026-10-10 (recomp pivot) — Lanes V/C/A landed + FULL BUILD GREEN
- Pivot per user directive: questionnaire cancelled, focus = working recomp ASAP.
- **Lane V (vendoring)**: external/tinyxml (2.6.2 classic, matches 0.9 TiXml* RTTI) + external/lzma (SDK 25.01 C layout); premake wired (explicit lzma C file list, no Precomp.c — SDK 25.01 has only Precomp.h; vendored-file warning suppression /wd4100 etc.).
- **Lane L (loader)**: src/loader/log.{h,cpp} (MvlLog/MvlErrorLog), chooser.cpp shim clone.
- **Lane C (config)**: src/config/xml_helpers, global_veh_xml (LoadGlobalVXMLFile), mvl_settings (CMvlSettings singleton + free wrapper LoadGlobalMXMLFile; mvl\globalm.xml: windowed/usemenus/antialias/log).
- **Lane A (archive)**: src/archive/seven_walker (CSevenWalker, SzArEx_Open + solid-block cache + UTF-16→UTF-8), vehicle_archive (CMvlVehicle::ParseArchive, RegisterArchiveFile tuning1-9.dff/weapons, VerifyRequiredFiles xml-mandatory).
- **BUILD GREEN (Release|Win32)**: single error fixed (seven_walker.cpp missing `#include "7zCrc.h"` for CrcGenerateTable — 7zCrc.h:14, must precede SzArEx_Open); vehmod.dll linked → bin\Release\; postbuild deploy ran → H:\games\VC\vehmod_retail.dll (deploy-subdir alignment with b.bat's mvl\ path noted as TODO).
- Next big block: Ghidra MCP decomp port of remaining ~136+ RTTI-named functions (model/limit engine, vehicle XML parsers, weapons, SCM, Python host).
