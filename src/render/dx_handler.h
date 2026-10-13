// dx_handler.h — CDxHandler: Direct3D9-era texture dictionary fix-up.
//
// Port of vehmod_retail.dll 0.98e @ RVA 0xd340 (FUN_1000d340, 650 bytes).
// Original symbol recovered 1:1 from its own error strings —
// "CDxHandler::DX9TextureFix" (@ RVA 0x1007afb4) — which also names the
// class (RTTI-style class::method strings, same convention as
// "CPngFuncs::LoadPngFuncs" in png_funcs.cpp). See re-098e-map.md §3 Misc.
//
// WHAT IT DOES (faithful port): validates a RW texture-dictionary buffer
// (main header id 0x16, struct sub-header id 1/len 4, per-texture id 0x15,
// sub-header id 1), clamps out-of-range library IDs to 0x1003ffff, normalises
// raster flags/format values, derives mask texture names ("<name>a") for
// 4444/RGBA formats, and clears mask names for 1555/565 formats — i.e. the
// classic "make VC's D3D8 txd stream accept modern .txd" repair.
//
// NOTE: vehicle_archive.h declares the provisional seam MvlRecognizeTextureData
// ("FUN_1000d340 @ 0xd340") — same RVA, logged in the dispatch report; the
// real implementation lives here under the recovered name.
#pragma once

#include "../core/stdafx.h"

class CDxHandler
{
public:
	// CDxHandler::DX9TextureFix — 0.98e @ RVA 0xd340.
	// data = texture dictionary buffer, size = buffer length. Returns true
	// when the buffer is accepted (and possibly rewritten in place), false
	// after logging one of the original's error strings. The original returns
	// uint (low byte = success); the clone uses bool.
	static bool DX9TextureFix(int *data, unsigned size);
};
