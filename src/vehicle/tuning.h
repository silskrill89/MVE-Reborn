// tuning.h — LoadTuningFile: the CMvlVehicle tuning-part load stage.
//
// vehmod_retail.dll 0.98e, free function (name from the DLL's own error-log
// identity "LoadTuningFile"; re-098e-map §3). Decomp source:
// .harness-memory/limout/missing33/decomp-chain.txt depth-0 root
// 0x10039ec0 (1071 B) + dumped depth-1 callees 0x38db0 (dummy-name builder),
// 0x39df0 (part cleanup), 0x31ca0 (clump+4 getter).
//
// Called by CMvlVehicle::LoadFromArchive after LoadCollisionFile (the archive
// lane's forwarding decl MvlPostColModelSetup @ vehicle_archive.h:69 is the
// same original — 0.98e @ RVA 0x39ec0; the orchestrator routes the call to
// LoadTuningFile once the seams are wired).
//
// The tuning block lives at CMvlVehicle+0x7e0 (count) / +0x7e4 (20 records
// of 0x15c; the archive lane's CMvlVehicle struct does not model this region,
// so the port accesses it via byte offsets). See tuning.cpp for the record
// layout recovered 1:1 from this dump.
#pragma once

#include "../core/stdafx.h"

class CMvlVehicle;

// LoadTuningFile — 0.98e @ RVA 0x39ec0. Loads the nine tuning DFF slots
// (CMvlVehicle::m_apFiles[MVL_SLOT_TUNING1..9]) as RW clumps, resolves each
// recorded part's dummy frame (category dispatch via FUN_10038db0), and
// attaches/frees parts accordingly. Cars only (m_nType == 0).
// Ghidra: __fastcall (vehicle arg in ECX); clone callers pass normally.
void LoadTuningFile(CMvlVehicle *veh);
