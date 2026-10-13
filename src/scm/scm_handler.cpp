// scm_handler.cpp — CScmHandler opcode-registration scaffolding (see scm_handler.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (limout/scmop/
// decomp-chain.txt): CScmHandler::IgnoreOpcode @ 0x234c0 and the opcode
// 0801/0802 handler bodies @ 0x23920/0x23940. Error strings are verbatim from
// the decomp. Every engine/game access (absolute-VA call into gta-vc.exe,
// store into game memory) is a HOOK POINT: MvlErrorLog line with the exact VA
// (+ intended value for stores) and NO real call/write — game behaviour is
// never faked. The registration primitive FUN_100238a0 @ 0x238a0 is still
// undumped; its clone-side scaffold below stores into the table.
#include "scm_handler.h"

#include "vehicle_scm.h"      // CVehicleSCM::ProcessNextTextOpcode (0800 binding)
#include "../loader/log.h"

#include <stdarg.h>
#include <stdio.h>

// ---------------------------------------------------------------------------
// Log helper.
// ---------------------------------------------------------------------------
// FUN_1001cce0 @ RVA 0x1cce0 — original warning logger (name, fmt, ...):
// _vsprintf's the vararg message into a 2048-byte stack buffer, then appends
// "Warning in %s: %s\n" to the log file (DAT_100b0db0). Clone maps it onto
// MvlErrorLog with the same strings; vsnprintf replaces the unbounded
// _vsprintf per the buffer-safety rules (the \n is supplied by MvlErrorLog).
static void
MvlScmWarn(const char *fn, const char *fmt, ...)
{
	char msg[2048];                        // decomp: local_808[2048]
	va_list ap;
	va_start(ap, fmt);
	vsnprintf(msg, sizeof(msg), fmt, ap);  // decomp: _vsprintf (unbounded)
	va_end(ap);
	// BUGFIX(stable-1.0): force termination (MSVC vsnprintf does not
	// NUL-terminate on truncation) — every sibling log wrapper in the port
	// does this; msg feeds the "%s" operand below.
	msg[sizeof(msg) - 1] = '\0';
	MvlErrorLog("Warning in %s: %s", fn, msg);   // decomp: "Warning in %s: %s\n"
}

// ---------------------------------------------------------------------------
// Undumped engine/game seams — HOOK POINTs: log the exact VA + intended
// effect, perform NO engine call or write.
// ---------------------------------------------------------------------------

// FUN_1002e4c0 @ RVA 0x2e4c0 — thunk body (scmop dump:79-86): a single
// tail-call to gta-vc.exe!0x451010, no arguments forwarded. Game-fn identity
// NOT established — TODO(scm): identify gta-vc.exe @ 0x451010. vehicle_scm.cpp
// meets the same thunk as its "opcode-argument context prep" seam
// (ScmScriptArgPrep); the name there is provisional too.
static void
MvlScmHook_GameCall451010(void)
{
	MvlErrorLog("SCM hook point (not implemented): call gta-vc.exe!0x451010 [0.98e @ RVA 0x2e4c0 MvlScmHook_GameCall451010 tail-call thunk]");
}

// FUN_1002e4d0 @ RVA 0x2e4d0 — thunk body (scmop dump:111-118): a single
// tail-call to gta-vc.exe!0x463f00. Identity NOT established — TODO(scm):
// identify gta-vc.exe @ 0x463f00.
static void
MvlScmHook_GameCall463f00(void)
{
	MvlErrorLog("SCM hook point (not implemented): call gta-vc.exe!0x463f00 [0.98e @ RVA 0x2e4d0 MvlScmHook_GameCall463f00 tail-call thunk]");
}

// FUN_1002e4b0 @ RVA 0x2e4b0 — thunk body (scmop dump:133-140): a single
// tail-call to gta-vc.exe!0x450e50. Identity NOT established — TODO(scm):
// identify gta-vc.exe @ 0x450e50.
static void
MvlScmHook_GameCall450e50(void)
{
	MvlErrorLog("SCM hook point (not implemented): call gta-vc.exe!0x450e50 [0.98e @ RVA 0x2e4b0 MvlScmHook_GameCall450e50 tail-call thunk]");
}

// FUN_1002e560 @ RVA 0x2e560 — __fastcall(slot, value) (scmop dump:121-130):
// bounds-checked dword store into a 12-slot game array
// (decomp: if (param_1 < 0xc) *(param_1*4 + 0x7d7438) = param_2;).
// The bounds check is portable; the STORE is an engine write => HOOK POINT:
// exact VA + intended value logged, NO write performed. Array identity NOT
// established — TODO(scm): identify the 12-dword array at gta-vc.exe @
// 0x7d7438.
static void __fastcall
MvlScmHook_SetGameDwordSlot(int slot, unsigned int value)
{
	if(slot < 12){                                                // decomp: if (param_1 < 0xc)
		unsigned int addr = 0x7d7438u + (unsigned int)slot * 4u;  // decomp: param_1 * 4 + 0x7d7438
		MvlErrorLog("SCM hook point (not implemented): write gta-vc.exe 0x%08X = 0x%08X [0.98e @ RVA 0x2e560 MvlScmHook_SetGameDwordSlot slot %d of 12]",
		            addr, value, slot);
	}
}

// ---------------------------------------------------------------------------
// Opcode 0801/0802 handler bodies (defined below, after IgnoreOpcode, in RVA
// order) — file-local like the original FUN_ functions (no this pointer).
// ---------------------------------------------------------------------------
static void MvlScmHandlePythonLoadedOpcode(void);   // FUN_10023920 @ 0x23920
static void MvlScmHandleVersionOpcode(void);        // FUN_10023940 @ 0x23940

// ---------------------------------------------------------------------------
// Opcode table (data). handler/origHandlerVa facts:
//   0800 -> 0x51eb0, registered @ 0x51fb0 (range-decomp.txt:1188)
//   0801 -> 0x23920, registered @ 0x238d0 (decomp-chain.txt:1668)
//   0802 -> 0x23940, registered @ 0x238d0 (decomp-chain.txt:1669)
//   0810 -> 0x584f0, registered @ 0x58270 (range-decomp.txt:6029)
// ---------------------------------------------------------------------------
CScmHandler::SOpcodeEntry CScmHandler::s_opcodes[4] = {
	{ 0x0800, &CVehicleSCM::ProcessNextTextOpcode, 0x51eb0,
	  "name-draw; registered by CVehicleSCM::RegisterNextTextOpcode @ 0x51fb0 via CScmHandler::RegisterOpcode @ 0x238a0" },  // FUN_10051fb0 @ 0x51fb0, FUN_100238a0 @ 0x238a0
	{ 0x0801, &MvlScmHandlePythonLoadedOpcode, 0x23920,
	  "python-loaded; body ported (scmop/decomp-chain.txt:54-63); registered @ 0x238d0 (decomp-chain.txt:1668)" },  // FUN_100238d0 @ 0x238d0
	{ 0x0802, &MvlScmHandleVersionOpcode, 0x23940,
	  "version=9850; body ported (scmop/decomp-chain.txt:66-76); registered @ 0x238d0 (decomp-chain.txt:1669)" },  // FUN_100238d0 @ 0x238d0
	{ 0x0810, NULL, 0x584f0,
	  "weapon pickup; body in src/weapons/ (sibling lane, extern seam in scm_handler.h); registered @ 0x58270 (range-decomp.txt:6029)" },  // FUN_10058270 @ 0x58270
};

// ---------------------------------------------------------------------------
// Lookup / registration scaffolding (clone-side; mirrors FUN_100238a0 @
// RVA 0x238a0 which is not dumped).
// ---------------------------------------------------------------------------
CScmHandler::SOpcodeEntry *
CScmHandler::FindOpcode(unsigned short opcode)
{
	for(int i = 0; i < 4; i++){
		if(s_opcodes[i].opcode == opcode)
			return &s_opcodes[i];
	}
	return NULL;
}

bool
CScmHandler::RegisterOpcode(unsigned short opcode, TOpcodeFn fn)
{
	SOpcodeEntry *e = FindOpcode(opcode);
	if(!e){
		MvlErrorLog("CScmHandler::RegisterOpcode(%04X): opcode not in the clone table (holds the 4 dumped MVL opcodes 0800/0801/0802/0810).", opcode);
		return false;
	}
	e->handler = fn;
	return true;
}

void
CScmHandler::RegisterMvlOpcodes(void)
{
	// 0800 — bound (statically pre-bound as well; this exercises the seam).
	RegisterOpcode(0x0800, &CVehicleSCM::ProcessNextTextOpcode);

	// 0801 / 0802 — bodies now ported (FUN_10023920 @ 0x23920 /
	// FUN_10023940 @ 0x23940, scmop/decomp-chain.txt) and statically bound;
	// these calls exercise the seam.
	RegisterOpcode(0x0801, &MvlScmHandlePythonLoadedOpcode);
	RegisterOpcode(0x0802, &MvlScmHandleVersionOpcode);

	// 0810 — data present, body still pending: it lives in src/weapons/
	// (sibling lane; extern seam in scm_handler.h) and is intentionally NOT
	// linked from this translation unit.
	static const unsigned short pending[1] = { 0x0810 };
	for(int i = 0; i < 1; i++){
		SOpcodeEntry *e = FindOpcode(pending[i]);
		if(e != NULL && e->handler == NULL){
			MvlErrorLog("CScmHandler::RegisterMvlOpcodes: opcode %04X handler not ported yet (original @ RVA 0x%X) — %s",
			            e->opcode, e->origHandlerVa, e->note);
		}
	}
}

// ===========================================================================
// CScmHandler::IgnoreOpcode — 0.98e @ RVA 0x234c0 (FUN_100234c0)
// ===========================================================================
// decomp (scmop/decomp-chain.txt:1-51): __fastcall(this, opcode) — a compare
// tree over 11 ignorable game opcodes; every match tail-calls FUN_1002e4c0
// (game thunk @ gta-vc.exe!0x451010) and returns 1 (CONCAT31(...,1) — the
// return word's low byte is the bool). Anything else logs the verbatim
// warning "Cannot ignore unknown opcode %04X." via FUN_1001cce0 and returns 0
// (decomp: uVar2 & 0xffffff00).
bool
CScmHandler::IgnoreOpcode(unsigned short opcode)
{
	static const char *const fn = "CScmHandler::IgnoreOpcode";

	// decomp compare tree: 0x55/0x16a/0x171/0x1b4 -> LAB_100234e1,
	// 0x1eb/0x4bb/0x4fa -> LAB_1002350b, 0x22a/0x22b -> LAB_10023519 — all
	// three labels do the same (FUN_1002e4c0(); return 1;), so the clone
	// flattens them into one case set.
	switch(opcode){
	case 0x0055:
	case 0x016a:
	case 0x0171:
	case 0x01b4:
	case 0x01eb:
	case 0x022a:
	case 0x022b:
	case 0x03cb:
	case 0x04bb:
	case 0x04f9:
	case 0x04fa:
		MvlScmHook_GameCall451010();   // FUN_1002e4c0 @ 0x2e4c0 (called before the success return)
		return true;                   // decomp: CONCAT31(..., 1)
	default:
		MvlScmWarn(fn, "Cannot ignore unknown opcode %04X.", opcode);   // FUN_1001cce0 @ 0x1cce0
		return false;                  // decomp: FUN_1001cce0(...) & 0xffffff00
	}
}

// ===========================================================================
// Opcode 0801 "python-loaded" handler — 0.98e @ RVA 0x23920 (FUN_10023920)
// ===========================================================================
static void
MvlScmHandlePythonLoadedOpcode(void)
{
	MvlScmHook_GameCall463f00();       // FUN_1002e4d0 @ 0x2e4d0 -> gta-vc.exe!0x463f00
	// decomp: return FUN_1002e4d0() & 0xffffff00 — the handler's return word,
	// low byte always 0. TOpcodeFn is void(void) in the clone (the 0800
	// binding's signature), so the return word is documented, not modelled.
	// TODO(scm): handler return-word consumer (original dispatcher) —
	// VA not in any dump.
}

// ===========================================================================
// Opcode 0802 "version=9850" handler — 0.98e @ RVA 0x23940 (FUN_10023940)
// ===========================================================================
static void
MvlScmHandleVersionOpcode(void)
{
	// decomp: FUN_1002e560(0, 0x267a) — game slot 0 := 0x267a (9850 decimal,
	// the "version=9850" of the registration census).
	MvlScmHook_SetGameDwordSlot(0, 0x267a);   // FUN_1002e560 @ 0x2e560
	MvlScmHook_GameCall450e50();              // FUN_1002e4b0 @ 0x2e4b0 -> gta-vc.exe!0x450e50
	// decomp: return FUN_1002e4b0() & 0xffffff00 — return word low byte 0;
	// see the 0801 note on TOpcodeFn.
}
