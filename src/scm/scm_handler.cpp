// scm_handler.cpp — CScmHandler opcode-registration scaffolding (see scm_handler.h).
//
// The table below is DATA transcribed from the four FUN_100238a0 registration
// sites found in the dumps (cites in scm_handler.h). Undumped engine bodies
// (IgnoreOpcode, the registration primitive itself, opcodes 0801/0802) are
// HOOK POINTs: MvlErrorLog with the original VA + a documented default —
// behaviour is never faked.
#include "scm_handler.h"

#include "vehicle_scm.h"      // CVehicleSCM::ProcessNextTextOpcode (0800 binding)
#include "../loader/log.h"

// ---------------------------------------------------------------------------
// Opcode table (data). handler/origHandlerVa facts:
//   0800 -> 0x51eb0, registered @ 0x51fb0 (range-decomp.txt:1188)
//   0801 -> 0x23920, registered @ 0x238d0 (decomp-chain.txt:1668)
//   0802 -> 0x23940, registered @ 0x238d0 (decomp-chain.txt:1669)
//   0810 -> 0x584f0, registered @ 0x58270 (range-decomp.txt:6029)
// ---------------------------------------------------------------------------
CScmHandler::SOpcodeEntry CScmHandler::s_opcodes[4] = {
	{ 0x0800, &CVehicleSCM::ProcessNextTextOpcode, 0x51eb0,
	  "name-draw; registered by FUN_10051fb0 @ 0x51fb0 via FUN_100238a0 @ 0x238a0" },
	{ 0x0801, NULL, 0x23920,
	  "python-loaded; handler body NOT in dumps (needed dump); registered by FUN_100238d0 @ 0x238d0 (decomp-chain.txt:1668)" },
	{ 0x0802, NULL, 0x23940,
	  "version=9850; handler body NOT in dumps (needed dump); registered by FUN_100238d0 @ 0x238d0 (decomp-chain.txt:1669)" },
	{ 0x0810, NULL, 0x584f0,
	  "weapon pickup; body in src/weapons/ (sibling lane, extern seam in scm_handler.h); registered by FUN_10058270 @ 0x58270 (range-decomp.txt:6029)" },
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

	// 0801 / 0802 / 0810 — data present, bodies not yet in this tree.
	static const unsigned short pending[3] = { 0x0801, 0x0802, 0x0810 };
	for(int i = 0; i < 3; i++){
		SOpcodeEntry *e = FindOpcode(pending[i]);
		if(e != NULL && e->handler == NULL){
			MvlErrorLog("CScmHandler::RegisterMvlOpcodes: opcode %04X handler not ported yet (original @ RVA 0x%X) — %s",
			            e->opcode, e->origHandlerVa, e->note);
		}
	}
}

// ---------------------------------------------------------------------------
// CScmHandler::IgnoreOpcode — 0.98e @ RVA 0x234c0 — STUB (body not dumped).
// ---------------------------------------------------------------------------
bool
CScmHandler::IgnoreOpcode(unsigned short opcode)
{
	// HOOK POINT: FUN_100234c0 is referenced by name in re-098e-map.md §3 but
	// its body is NOT in decomp-chain.txt (81 fns) nor savewpn/range-decomp.txt
	// (151 fns) — a Ghidra dump of RVA 0x234c0 is required (port-notes-scm.md
	// "Needed dumps"). Param width and return value are UNVERIFIED guesses.
	MvlErrorLog("CScmHandler::IgnoreOpcode(%04X): hook point — body not dumped (0.98e @ RVA 0x234c0)", opcode);
	return false;   // UNVERIFIED DEFAULT: false = "do not ignore", so the
	                // dispatcher keeps processing until the real body lands.
}
