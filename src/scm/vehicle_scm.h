// vehicle_scm.h — CVehicleSCM: SCM opcode 0800 ("name draw") cluster.
//
// Ported from decompiled vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/savewpn/range-decomp.txt, the SCM gap 0x51cb0-0x51ff0
// between the save cluster 0x50a30-0x51900 (src/vehicle/, other lane) and the
// weapon cluster 0x52150+ (src/weapons/, sibling lane — NOT touched here)).
// Original symbol names are reused 1:1 where RTTI recovered them: both named
// functions embed their own names in the 0.98e DLL's error strings
// ("CVehicleSCM::SetNextTextForVehicleId" / "CVehicleSCM::ProcessNextTextOpcode"
// logged via FUN_1001cb80 @ RVA 0x1cb80). Names for the unnamed callees are
// PROVISIONAL and marked; confidence is tabulated in
// .harness-memory/limout/port-notes-scm.md.
//
// Opcode 0800 semantics (SCM census, re-098e-map.md §3): draws the vehicle's
// name. The first script argument is either an immediate model index (arg
// type 1) or a script global holding it (arg type 2). Error strings below are
// verbatim from the decomp.
//
// Engine/game internals that are NOT in the dump are HOOK POINTS in
// vehicle_scm.cpp: each logs MvlErrorLog with the original VA and returns a
// documented default — game behaviour is never faked. The VA in every comment
// is the address of truth.
#pragma once

#include "../core/stdafx.h"

class CVehicleSCM
{
public:
	// === RTTI-recovered names (error strings embedded in the 0.98e DLL) ===

	// CVehicleSCM::SetNextTextForVehicleId — 0.98e @ RVA 0x51e20 (opcode 0800).
	// Validates a vehicle model index (pool lookup FUN_1004f3a0 @ RVA 0x4f3a0;
	// slot range-checked against the "TXD base index" limit gta-vc.exe
	// DAT_00a06320) and formats the display name into the next-text buffer
	// (DAT_100b77d8). Failures log the verbatim "Opcode 800h - ..." messages
	// via FUN_1001cb80 @ RVA 0x1cb80, then fall through to L"Invalid".
	// Ghidra prototype: uint __fastcall FUN_10051e20(int param_1) — ONE ECX
	// argument and NO 'this' (the error path calls it with literal -1), i.e.
	// a static member: ported as static __fastcall (MSVC: no __thiscall on
	// free/static fns — C3865).
	static unsigned int __fastcall SetNextTextForVehicleId(int vehicleModelId);

	// CVehicleSCM::ProcessNextTextOpcode — 0.98e @ RVA 0x51eb0 (opcode 0800
	// body; registered by FUN_10051fb0 @ RVA 0x51fb0 via FUN_100238a0
	// @ RVA 0x238a0). Reads the opcode's arguments (prep FUN_1002e4c0 @
	// 0x2e4c0, per-arg read FUN_1002e530 @ 0x2e530: EAX = value, EDX = type):
	// type 2 = script global (var index = value >> 8, validated against the
	// gta-vc.exe block DAT_00a0fde4), type 1 = immediate index; anything else
	// logs "Opcode 800h - invalid value for first argument.". Every failure
	// path ends with SetNextTextForVehicleId(-1) -> L"Invalid".
	static void ProcessNextTextOpcode(void);

	// === PROVISIONAL names (confidence + evidence in port-notes-scm.md) ===

	// FUN_10051fd0 @ RVA 0x51fd0 — (provisional, MEDIUM) FormatNextTextBuffer:
	// cdecl varargs wrapper over _vswprintf_s(dest, 200, fmt, ...) writing the
	// next-text buffer (200 wchar_t capacity per the decomp). Only caller in
	// the dump: SetNextTextForVehicleId @ 0x51e20.
	static int __cdecl FormatNextTextBuffer(wchar_t *dest, const wchar_t *fmt, ...);

	// FUN_10051fb0 @ RVA 0x51fb0 — (provisional, MEDIUM) RegisterNextTextOpcode:
	// walks the hook-stub table (FUN_10051f90) then registers opcode 0800 ->
	// ProcessNextTextOpcode through the engine registration primitive
	// FUN_100238a0(0x800, fn, 0) @ RVA 0x238a0 (not in any dump; clone-side
	// seam = CScmHandler::RegisterOpcode).
	static void RegisterNextTextOpcode(void);

	// FUN_10051f90 @ RVA 0x51f90 — (provisional, LOW) ProcessHookStubTable:
	// loops a 19-entry pointer table (DLL .data DAT_100af590, 0x4c bytes)
	// calling FUN_10022570(entry - 5) @ RVA 0x22570 — install-vs-restore
	// direction is NOT recoverable from the dump (HOOK POINT).
	static void ProcessHookStubTable(void);

	// FUN_10051cb0 @ RVA 0x51cb0 — (provisional, MEDIUM) InstallScmHooks:
	// writes 14 hooks into gta-vc.exe (FUN_10022560 @ RVA 0x22560) + 4 byte
	// patches, then calls RegisterNextTextOpcode. NOTE: the save lane
	// (vehicle_handler.h) also cites this installer ("the original patches at
	// 0x51cb0"), so the 14 sites likely mix SCM and save-flow hooks — per-site
	// attribution is an open question (port-notes-scm.md §Open questions).
	static void InstallScmHooks(void);

	// CLONE-SIDE (not an original symbol): read accessor for the next-text
	// buffer (original global DAT_100b77d8, 200 wchar_t) — for the future
	// game-hook wiring lane that displays the text.
	static const wchar_t *NextText(void);
};
