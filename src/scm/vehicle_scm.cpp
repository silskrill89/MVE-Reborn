// vehicle_scm.cpp — CVehicleSCM opcode-0800 cluster (see vehicle_scm.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (savewpn/range-decomp.txt,
// SCM gap 0x51cb0-0x51ff0). Error strings are verbatim from the decomp. Every
// engine/game call that is NOT in the dump is a HOOK POINT: MvlErrorLog line
// with the original VA + a documented default return — game behaviour is never
// faked. Log mapping matches the other lanes: FUN_1001cb80 error -> MvlErrorLog
// with the function name as context (vehicle_handler.cpp SvErr convention).
#include "vehicle_scm.h"

#include "scm_handler.h"      // CScmHandler::RegisterOpcode (opcode-registration seam)
#include "../loader/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <wchar.h>

// ---------------------------------------------------------------------------
// Next-text buffer — original global DAT_100b77d8 (0.98e .data), 200 wchar_t
// (capacity used by _vswprintf_s in FUN_10051fd0). Zero-initialised.
// ---------------------------------------------------------------------------
static wchar_t s_nextText[200];

// ---------------------------------------------------------------------------
// Log + hook-point helpers.
// ---------------------------------------------------------------------------
// FUN_1001cb80 @ RVA 0x1cb80 — original error logger (function-name, message).
static void
ScmErr(const char *fn, const char *msg)
{
	MvlErrorLog("%s: %s", fn, msg);
}

// HOOK POINT marker: undumped engine/game code. `cite` is the address of truth.
static void
ScmHookPoint(const char *what, const char *cite)
{
	MvlErrorLog("SCM hook point (not implemented): %s [%s]", what, cite);
}

// ---------------------------------------------------------------------------
// Undumped engine/game seams (all HOOK POINTs; defaults documented).
// ---------------------------------------------------------------------------

// MvlGetModelDirRecord @ RVA 0x4f3a0 — pool/model lookup: decomp reads EAX (entry
// pointer, NULL = none) and EDX (pool slot index left as a side channel,
// Ghidra "extraout_EDX"). NOTE: the vehicle lane exposes the EAX half as
// MvlLookupModelInfoByModelIndex (vehicle_handler.h, TODO(save): 0x4f3a0);
// this file needs the EDX half too, hence the private seam (both stubbed).
static void
ScmLookupModelSlot(int modelIdx, void **entryOut, int *slotOut)
{
	ScmHookPoint("MvlGetModelDirRecord model/pool lookup (EAX entry + EDX slot)", "0.98e @ RVA 0x4f3a0");
	*entryOut = NULL;
	*slotOut  = 0;
	(void)modelIdx;
}

// gta-vc.exe DAT_00a06320 — "TXD base index" model-index ceiling (verbatim
// message: "... exceeds TXD base index.").
static int
ScmTxdBaseIndex(void)
{
	ScmHookPoint("gta-vc.exe DAT_00a06320 (TXD base index limit)", "gta-vc.exe @ 0xa06320");
	return 0;
}

// gta-vc.exe DAT_00a06314 — model-info pointer table, indexed by pool slot
// (*(int*)(base + slot*4) in the decomp).
static int
ScmModelInfoForSlot(int slot)
{
	ScmHookPoint("gta-vc.exe DAT_00a06314[slot] model-info table", "gta-vc.exe @ 0xa06314");
	(void)slot;
	return 0;
}

// MvlTextGet @ RVA 0x2ebb0 — resolves the model display name before the
// type-6 success path formats it.
static void
ScmResolveModelName(void)
{
	ScmHookPoint("MvlTextGet model-name resolve", "0.98e @ RVA 0x2ebb0");
}

// FUN_1002e4c0 @ RVA 0x2e4c0 — opcode-argument context prep (first call in
// ProcessNextTextOpcode, no arguments).
static void
ScmScriptArgPrep(void)
{
	ScmHookPoint("ScmScriptArgPrep opcode-argument context prep", "0.98e @ RVA 0x2e4c0");  // FUN_1002e4c0 @ 0x2e4c0
}

// FUN_1002e530 @ RVA 0x2e530 — read script opcode argument N: EAX = value,
// EDX = argument type (the decomp's extraout_EDX). Both channels captured
// from ONE read, matching the original's single call.
static int s_scriptArgValue;
static int s_scriptArgType;
static void
ScmReadScriptArg(int index)
{
	ScmHookPoint("ScmReadScriptArg script opcode argument read (EAX value / EDX type)", "0.98e @ RVA 0x2e530");  // FUN_1002e530 @ 0x2e530
	(void)index;
	s_scriptArgValue = 0;
	s_scriptArgType  = 0;
}

// MvlGetActiveModelIndex @ RVA 0x2d880 — resolve the vehicle id held in the script
// variable (type-2 path). Default -1 => "Invalid" text, never a fabricated id.
static int
ScmScriptVarVehicleId(void)
{
	ScmHookPoint("MvlGetActiveModelIndex vehicle id from script variable", "0.98e @ RVA 0x2d880");
	return -1;
}

// gta-vc.exe DAT_00a0fde4 — 3-dword script-global block used by the type-2
// check: [0] = space base (varIdx*0x5dc + [0] != 0), [1] = type-byte base,
// [2] = global count. NULL until wired => the check fails honestly (no
// deref, no fabricated success).
static const int *
ScmScriptGlobals(void)
{
	ScmHookPoint("gta-vc.exe DAT_00a0fde4 script-global block (3 dwords)", "gta-vc.exe @ 0xa0fde4");
	return NULL;
}

// FUN_10022560 @ RVA 0x22560 — engine patch writer (installs a hook at a
// gta-vc.exe address, original args: site, MVL stub address).
static void
ScmWriteHook(unsigned int site, unsigned int stub)
{
	MvlErrorLog("SCM hook point (not implemented): install game hook gta-vc.exe 0x%06X -> MVL stub 0x%08X [0.98e @ RVA 0x22560; stub body NOT in dump]",
	            site, stub);
}

// Direct byte stores into gta-vc.exe performed by FUN_10051cb0 itself.
static void
ScmPatchGameBytes(unsigned int addr, unsigned int value, unsigned int size)
{
	MvlErrorLog("SCM hook point (not implemented): patch gta-vc.exe 0x%06X (%u bytes = 0x%08X) [0.98e @ RVA 0x51cb0 direct store]",
	            addr, size, value);
}

// Success-path formatting (decomp LAB_10051e63): FUN_10051fd0(&DAT_100b77d8,
// L"%s", <name>) — Ghidra's 2-parameter prototype for FUN_10051fd0 HIDES the
// %s vararg operand, so the name source is not recoverable from this dump.
// HOOK POINT: the buffer is left untouched (no fabricated text).
static unsigned int
ScmFormatResolvedName(void)
{
	ScmHookPoint("success-path name format into DAT_100b77d8 (hidden %s operand of CVehicleSCM::FormatNextTextBuffer)",
	             "0.98e @ RVA 0x51e20 / name via 0x2ebb0");  // FUN_10051fd0 @ 0x51fd0
	return 0;   // == (vswprintf_s count & 0xffffff00) for any count < 256
}

// ===========================================================================
// CVehicleSCM::SetNextTextForVehicleId — 0.98e @ RVA 0x51e20
// ===========================================================================
unsigned int __fastcall
CVehicleSCM::SetNextTextForVehicleId(int vehicleModelId)
{
	static const char *const fn = "CVehicleSCM::SetNextTextForVehicleId";

	if(vehicleModelId >= 0){                    // decomp: if (-1 < param_1)
		void *entry = NULL;
		int slot = 0;
		ScmLookupModelSlot(vehicleModelId, &entry, &slot);   // MvlGetModelDirRecord @ 0x4f3a0 (EAX/EDX)
		int txdBase = ScmTxdBaseIndex();                     // gta-vc.exe DAT_00a06320
		if(slot < txdBase){                                  // decomp: extraout_EDX < DAT_100a6320
			if(entry){
				return ScmFormatResolvedName();              // LAB_10051e63
			}
			int info = ScmModelInfoForSlot(slot);            // gta-vc.exe DAT_00a06314[slot]
			const char *msg;
			if(info == 0){
				msg = "Opcode 800h - specified vehicle model index is not in use.";
			}
			else if(*(const char *)(info + 0x19) == 6){      // model-info type field @ +0x19 == 6
				ScmResolveModelName();                       // MvlTextGet @ 0x2ebb0
				return ScmFormatResolvedName();              // LAB_10051e63 (same path)
			}
			else{
				msg = "Opcode 800h - specified vehicle model index is not actually a vehicle.";
			}
			ScmErr(fn, msg);                                 // FUN_1001cb80 @ 0x1cb80
		}
		else{
			ScmErr(fn, "Opcode 800h - specified vehicle model index is out of range - exceeds TXD base index.");
		}
	}
	// Failure fall-through (decomp verbatim): format L"Invalid" into the
	// next-text buffer, returning the same (count & 0xffffff00) word.
	return FormatNextTextBuffer(s_nextText, L"Invalid") & 0xffffff00u;
}

// ===========================================================================
// CVehicleSCM::ProcessNextTextOpcode — 0.98e @ RVA 0x51eb0
// ===========================================================================
void
CVehicleSCM::ProcessNextTextOpcode(void)
{
	static const char *const fn = "CVehicleSCM::ProcessNextTextOpcode";

	ScmScriptArgPrep();          // FUN_1002e4c0 @ 0x2e4c0
	ScmReadScriptArg(0);         // FUN_1002e530(0) @ 0x2e530 — decomp discards EAX; side effects kept
	ScmReadScriptArg(1);         // FUN_1002e530(1) @ 0x2e530
	int argValue = s_scriptArgValue;
	int argType  = s_scriptArgType;              // EDX channel of the last read

	if(argType == 2){                            // script-global argument
		unsigned int varIdx = (unsigned int)argValue >> 8;   // decomp: iVar1 >> 8
		const int *globals = ScmScriptGlobals();  // gta-vc.exe DAT_00a0fde4
		bool ok = false;
		if(globals != NULL){
			// decomp: iVar2 < globals[2] && (char)iVar1 == *(char*)(iVar2 + globals[1])
			//         && iVar2 * 0x5dc + globals[0] != 0
			if((int)varIdx < globals[2] &&
			   (char)argValue == *(const char *)(varIdx + (unsigned int)globals[1]) &&
			   (int)(varIdx * 0x5dcu + (unsigned int)globals[0]) != 0){
				ok = true;
			}
		}
		if(ok){
			int vehId = ScmScriptVarVehicleId();  // MvlGetActiveModelIndex @ 0x2d880
			SetNextTextForVehicleId(vehId);
			return;
		}
		ScmErr(fn, "Opcode 800h - specified vehicle does not exist in vehicle pool.");
	}
	else if(argType == 1){                       // immediate integer argument
		SetNextTextForVehicleId(argValue);
		return;
	}
	else{
		ScmErr(fn, "Opcode 800h - invalid value for first argument.");
	}
	SetNextTextForVehicleId(-1);                 // decomp: error path always formats L"Invalid"
}

// ===========================================================================
// FUN_10051fd0 @ RVA 0x51fd0 — (provisional) FormatNextTextBuffer
// ===========================================================================
int __cdecl
CVehicleSCM::FormatNextTextBuffer(wchar_t *dest, const wchar_t *fmt, ...)
{
	// decomp: _vswprintf_s(param_1, 200, param_2, &stack0xc) — cdecl varargs,
	// 200-wchar_t capacity. Ghidra typed the return void but both call sites
	// consume EAX (vswprintf_s's char count), so it is ported as int.
	va_list ap;
	va_start(ap, fmt);
	int n = vswprintf_s(dest, 200, fmt, ap);
	va_end(ap);
	return n;
}

// ===========================================================================
// FUN_10051f90 @ RVA 0x51f90 — (provisional) ProcessHookStubTable
// ===========================================================================
void
CVehicleSCM::ProcessHookStubTable(void)
{
	// decomp: u = 0; do { FUN_10022570(*(int*)(DAT_100af590 + u) - 5); u += 4;
	// } while (u < 0x4c);  — 19-entry pointer table in the DLL .data
	// (0x4c / 4 = 19, matching the 19-function hook-stub cluster
	// re-098e-map.md §2). Table content and FUN_10022570's
	// install-vs-restore direction are NOT in the dump => HOOK POINT.
	ScmHookPoint("hook-stub table walk: 19 x hook-stub callee @ RVA 0x22570 over DAT_100af590",
	             "0.98e @ RVA 0x51f90");  // FUN_10022570 @ 0x22570 (install-vs-restore direction unknown)
}

// ===========================================================================
// FUN_10051fb0 @ RVA 0x51fb0 — (provisional) RegisterNextTextOpcode
// ===========================================================================
void
CVehicleSCM::RegisterNextTextOpcode(void)
{
	ProcessHookStubTable();      // FUN_10051f90 @ 0x51f90 (first in original order)
	// decomp: FUN_100238a0(0x800, FUN_10051eb0, 0) — engine registration
	// primitive @ RVA 0x238a0 (not in any dump; 3rd arg = 0, semantics
	// unverified). Clone-side seam stores into CScmHandler's opcode table.
	CScmHandler::RegisterOpcode(0x0800, &CVehicleSCM::ProcessNextTextOpcode);
}

// ===========================================================================
// FUN_10051cb0 @ RVA 0x51cb0 — (provisional) InstallScmHooks
// ===========================================================================

// Original hook sites (verbatim from the decomp): {gta-vc.exe address,
// MVL-side stub address (absolute, image base 0x10000000)}.
// NOTE: the 14 stub bodies (RVAs 0x51940..0x51c90) fall in the dump gap
// 0x5193c-0x51cb0 and are NOT enumerated in range-functions.txt — see
// port-notes-scm.md "Needed dumps".
static const struct { unsigned int site; unsigned int stub; } kScmHookSites[14] = {
	{ 0x42dc9d, 0x10051940 },
	{ 0x5f3345, 0x100519a0 },
	{ 0x5f36a2, 0x10051a10 },
	{ 0x5f36b9, 0x10051a70 },
	{ 0x61d925, 0x10051ad0 },
	{ 0x42e23a, 0x10051ae0 },
	{ 0x42e13a, 0x10051b00 },
	{ 0x42cb49, 0x10051b20 },
	{ 0x4bf902, 0x10051b60 },
	{ 0x4bfa28, 0x10051b90 },
	{ 0x4bfc1c, 0x10051bf0 },
	{ 0x42c72c, 0x10051c30 },
	{ 0x61a803, 0x10051c70 },
	{ 0x4a4723, 0x10051c90 },
};

// Original byte stores (verbatim values; store WIDTHS are Ghidra's inference
// and marked UNVERIFIED — see port-notes-scm.md).
static const struct { unsigned int addr; unsigned int value; unsigned int size; } kScmBytePatches[4] = {
	{ 0x4bfc6c, 0x90909090, 4 },   // decomp: _DAT_004bfc6c = 0x90909090
	{ 0x4bfc70, 0x00009090, 2 },   // decomp: _DAT_004bfc70 = 0x9090 (width UNVERIFIED: word vs dword)
	{ 0x5f337a, 0x901c75ff, 4 },   // decomp: _DAT_005f337a = 0x901c75ff
	{ 0x5f337e, 0x00000090, 1 },   // decomp: DAT_005f337e = 0x90
};

void
CVehicleSCM::InstallScmHooks(void)
{
	// Original order: 14 hooks (FUN_10022560 @ 0x22560) -> 4 byte stores ->
	// FUN_10051fb0() (opcode 0800 registration).
	for(int i = 0; i < 14; i++)
		ScmWriteHook(kScmHookSites[i].site, kScmHookSites[i].stub);
	for(int i = 0; i < 4; i++)
		ScmPatchGameBytes(kScmBytePatches[i].addr, kScmBytePatches[i].value, kScmBytePatches[i].size);
	RegisterNextTextOpcode();    // FUN_10051fb0 @ 0x51fb0
}

// ===========================================================================
// CLONE-SIDE accessor (not an original symbol)
// ===========================================================================
const wchar_t *
CVehicleSCM::NextText(void)
{
	return s_nextText;           // original global DAT_100b77d8
}
