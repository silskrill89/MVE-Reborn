// xmlgen.h — GenerateDefaultXML: default vehicle-XML generator (xmlgen feature).
//
// Port of vehmod_retail.dll 0.98e @ RVA 0x40df0 (FUN_10040df0, 4682 bytes,
// free function). Original name kept 1:1 (RTTI/error-string recovered name
// "GenerateDefaultXML"; see .serena/memories/re-098e-map.md §3 and
// docs/MVL/version-matrix.md §0.9). The generator dumps a default XML per
// vehicle model id 130..236 (0x82..0xEC) into "mvl\vlist\%s.xml" (the same
// "vlist\%s.xml" sprintf the 0.9 build used).
//
// Invocation: NOT from DllMain/PrepareMvlForLoad — the original is triggered
// from the companion xmlgen_launcher (Alt+4 hot-key; see
// original_files/xmlgen_launcher/readme.txt). No wiring added in
// src/core/main.cpp (caller evidence absent from this decomp range).
//
// Data-flow note: the decompiler dropped every stack argument of the many
// fprintf calls in this function; all argument bindings below were recovered
// from the 0.98e binary disassembly (capstone pass over vehmod_retail.dll)
// and cross-checked against the shipped sample XMLs
// (original_files/maxovehmod/samplexmls/*.xml) and plugin-sdk struct layouts.
#pragma once

#include "../core/stdafx.h"

// GenerateDefaultXML — vehmod_retail.dll 0.98e @ RVA 0x40df0.
// Returns void; logs errors through the original's error-log channel
// (FUN_1001cb80 @ 0x1cb80 -> clone MvlErrorLog).
void GenerateDefaultXML(void);

// Early-load "data fetch" counter — original guards GenerateDefaultXML with
// `DAT_100b2454 >= 100` ("DLL was loaded too late to fetch all data - use a
// launcher."). Zero until the (not-yet-ported) early-load fetch path runs, so
// the generator fails honestly instead of emitting empty XML. The fetch lane
// owns this global; declared here because xmlgen.cpp defines it.
extern int MvlFetchedDataCount;   // DAT_100b2454 @ RVA 0xb2454

// Game model-info pointer table: the original reads
//   mov eax, [DAT_100a6314] ; model info = [eax + id*4]
// i.e. DAT_100a6314 holds the address of the pointer array (its DLL .data
// initializer is 0x92D4C8 = gta-vc.exe ms_modelInfoPtrs; the clone must have
// the engine lane point this at a live table before the generator can run).
extern void **MvlModelInfoPtrs;   // DAT_100a6314 @ RVA 0xa6314 (init 0x92D4C8, gta-vc.exe)

// "Use <rgbcol> instead of <carcol>" switch (colour loop in GenerateDefaultXML
// tests this byte; original DAT_100b1a1f). False (carcol) matches all shipped
// sample XMLs; the config lane owns it.
extern unsigned char MvlRgbColoursEnabled;  // DAT_100b1a1f @ RVA 0xb1a1f
