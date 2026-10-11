// scm_handler.h — CScmHandler: MVL custom-SCM-opcode registration scaffolding
// + the IgnoreOpcode seam. Data-driven opcode table for the four MVL opcodes
// whose registration sites are on disk.
//
// Provenance (vehmod_retail.dll 0.98e, every claim cited):
//   0x0800 name-draw        -> CVehicleSCM::ProcessNextTextOpcode 0x51eb0
//                              registered by FUN_10051fb0 @ 0x51fb0
//                              (savewpn/range-decomp.txt:1188)
//   0x0801 python-loaded    -> FUN_10023920 @ 0x23920 (body NOT dumped)
//                              registered by FUN_100238d0 @ 0x238d0
//                              (limout/decomp-chain.txt:1668)
//   0x0802 version=9850     -> FUN_10023940 @ 0x23940 (body NOT dumped)
//                              registered by FUN_100238d0 @ 0x238d0
//                              (limout/decomp-chain.txt:1669)
//   0x0810 weapon pickup    -> CWeaponPickups::ProcessNewWeaponPickupOpcode
//                              0x584f0 (body lives in src/weapons/ — sibling
//                              lane; EXTERN SEAM below, not implemented here)
//                              registered by FUN_10058270 @ 0x58270
//                              (savewpn/range-decomp.txt:6029)
//   registration primitive   -> FUN_100238a0(opcode, handler, flags) @ 0x238a0
//                              (referenced by all four sites; body NOT dumped)
//   CScmHandler::IgnoreOpcode -> 0x234c0 (NOT in any dump on disk — STUB,
//                              see .harness-memory/limout/port-notes-scm.md)
//
// Clone policy: the table is DATA (opcode + original handler RVA + status);
// handlers bind through RegisterOpcode() as their lanes land. Undumped
// engine internals are HOOK POINTs with MvlErrorLog — behaviour never faked.
#pragma once

#include "../core/stdafx.h"

class CScmHandler
{
public:
	typedef void (*TOpcodeFn)(void);

	struct SOpcodeEntry
	{
		unsigned short opcode;        // SCM opcode (0x0800..0x0810)
		TOpcodeFn      handler;       // clone handler; NULL = body not yet ported
		unsigned int   origHandlerVa; // original handler RVA (0.98e)
		const char    *note;          // provenance / status (verbatim cites)
	};

	// Opcode table (data) — the four dumped MVL opcodes. 0800 is bound at
	// static init; 0801/0802/0810 stay NULL until their bodies exist.
	// (Mutable: RegisterOpcode() rebinds entries.)
	static SOpcodeEntry s_opcodes[4];

	// Linear lookup in s_opcodes; NULL when the opcode is not one of ours.
	static SOpcodeEntry *FindOpcode(unsigned short opcode);

	// Clone-side scaffolding mirroring the original registration primitive
	// FUN_100238a0 @ RVA 0x238a0 (opcode, handler, flags=0 — not dumped, so
	// the flags channel has no clone equivalent yet). Returns false and logs
	// when the opcode is not in the table.
	static bool RegisterOpcode(unsigned short opcode, TOpcodeFn fn);

	// Clone-side scaffold: binds 0800 (already statically bound) and logs the
	// three opcodes whose handler bodies are not ported yet. Intended entry
	// point for the future plugin-init wiring lane.
	static void RegisterMvlOpcodes(void);

	// CScmHandler::IgnoreOpcode — 0.98e @ RVA 0x234c0.
	// STUB: body NOT present in any dump on disk (checked
	// .harness-memory/limout/decomp-chain.txt (81 fns) and
	// savewpn/range-decomp.txt (151 fns): no FUN_100234c0 anywhere). Parameter
	// width (unsigned short) and return semantics are UNVERIFIED guesses
	// pending that dump — see port-notes-scm.md "Needed dumps".
	static bool IgnoreOpcode(unsigned short opcode);
};

// EXTERN SEAM — opcode 0810's body lives in src/weapons/ (sibling lane; NOT
// implemented here):
//   CWeaponPickups::ProcessNewWeaponPickupOpcode — 0.98e @ RVA 0x584f0
//   (FUN_100584f0), registered by FUN_10058270 @ RVA 0x58270.
// Declaration only — intentionally UNREFERENCED by this translation unit
// (zero link dependency on the sibling lane; a free-function alias of the
// original member name). Bind through CScmHandler::RegisterOpcode() once the
// weapons lane has landed its definition.
extern void ProcessNewWeaponPickupOpcode(void);
