// tuning.cpp — LoadTuningFile (see tuning.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e
// (.harness-memory/limout/missing33/decomp-chain.txt: depth-0 root
// 0x10039ec0 + dumped depth-1 callees 0x10038db0 / 0x10039df0 / 0x10031ca0).
// Log strings verbatim; RW/stream internals reuse the archive lane's
// vehicle_archive.h seams (FUN_100320a0/32090/32770/32760/32750/31670/
// 312c0/31f10/31450). Engine calls whose bodies are NOT in the dump are
// file-static HOOK POINT seams (below) that log + return a benign default —
// nothing is faked.
#include "tuning.h"

#include "../archive/vehicle_archive.h"   // CMvlVehicle + RW/col seams
#include "../loader/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>     // malloc/free
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrappers (lane convention: FUN_1001cce0 warn / FUN_1001cb80 error; the
// original passes "LoadTuningFile" as the context).
// ---------------------------------------------------------------------------
static void
TnWarn(const char *fn, const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "Warning in %s: %s", fn, msg);   // FUN_1001cce0 format
	buf[sizeof(buf) - 1] = '\0';
	MvlLog("%s", buf);
}

static void
TnErr(const char *fn, const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "%s: %s", fn, msg);              // FUN_1001cb80 format
	buf[sizeof(buf) - 1] = '\0';
	MvlErrorLog("%s", buf);
}

// ---------------------------------------------------------------------------
// Tuning part record — 0x15C bytes at CMvlVehicle + 0x7E4 + n*0x15C; the
// block memset in LoadTuningFile is 0x1B30 bytes = 20 records (the used
// count lives at CMvlVehicle + 0x7E0). Field offsets recovered 1:1 from
// LoadTuningFile / FUN_10039df0 (absolute addrs = veh + 0x7E4 + off).
// ---------------------------------------------------------------------------
struct SMvlTuningRecord
{
	char   szName[0x18];         // @ 0x00 part category name ("Wheels", "Front bump", ...)
	char   szResolvedDummy[0x18];// @ 0x18 resolved dummy-frame name ("chassis_dummy" fallback)
	int    nRefCount;            // @ 0x30 (+0x814 abs) sub-part refcount
	int    nState;               // @ 0x34 (+0x818 abs) 0 = nothing, 1 = clump
	                             //        loaded, 2 = dummy attached, 3 = chassis fallback
	void  *apSubSlotSrc[4];      // @ 0x38 (+0x81c abs) per-sub-slot source pointers
	unsigned char pad48[8];      // @ 0x48..0x4F unrecovered
	unsigned char bRegistered;   // @ 0x50 (+0x834 abs) pass-1 "part registered" flag
	unsigned char pad51[3];      // @ 0x51..0x53
	void  *apSubSlotOut[4];      // @ 0x54 (+0x838 abs) per-sub-slot part objects
	unsigned char pad64[0xE0];   // @ 0x64..0x143 unrecovered
	void  *apGeometry[4];        // @ 0x144 (+0x928 abs) per-sub-slot geometry handles
	unsigned char pad154[8];     // @ 0x154..0x15B (record size 0x15C)
};
// Layout check: offsets above must match the decomp's absolute addresses.
static_assert(sizeof(SMvlTuningRecord) == 0x15C, "SMvlTuningRecord must be 0x15C bytes");

static int  &TuningCount(CMvlVehicle *veh)
{
	return *(int *)((unsigned char *)veh + 0x7E0);   // veh+0x7E0
}

static SMvlTuningRecord *TuningRecords(CMvlVehicle *veh)
{
	return (SMvlTuningRecord *)((unsigned char *)veh + 0x7E4);   // veh+0x7E4
}

// ---------------------------------------------------------------------------
// Engine seams (bodies NOT in the dump): log once per call, benign defaults.
// ---------------------------------------------------------------------------

// FUN_10031ca0 @ 0x31ca0 — DUMPED body: `return *(int *)(clump + 4)` (the
// clump's frame/child pointer; used as the dummy-frame search root). The
// archive lane's MvlRwClumpSetup() decl drops the return value, so the
// tuning path re-ports the getter here (same VA — orchestrator may unify).
static void *
ClumpFrameAt4(void *clump)
{
	return clump ? *(void **)((unsigned char *)clump + 4) : 0;
}

// FUN_10031410 @ 0x31410 — game thunk (gta-vc.exe 0x640fd0); creates the
// attached part object (semantics not recovered; return stored per sub-slot).
static void *
MvlRwTuningCreatePart(void)
{
	MvlErrorLog("HOOK POINT 0x31410 MvlRwTuningCreatePart (game 0x640fd0)");
	return 0;
}

// FUN_10031280 @ 0x31280 — game thunk (gta-vc.exe 0x580600); returns the
// name-string pointer copied into the Wheels dummy buffer (semantics not
// recovered).
static const char *
MvlRwCurrentName(void)
{
	MvlErrorLog("HOOK POINT 0x31280 MvlRwCurrentName (game 0x580600)");
	return 0;
}

// FUN_100322b0 @ 0x322b0 — game thunk (gta-vc.exe 0x64cc90); post-attach
// step after a part clump is created (semantics not recovered).
static void
MvlRwTuningAttach(void)
{
	MvlErrorLog("HOOK POINT 0x322b0 MvlRwTuningAttach (game 0x64cc90)");
}

// FUN_10031380 @ 0x31380 — undumped; destroys one part object (per sub-slot,
// from the cleanup path). Semantics not recovered — neutral name.
static void
MvlRwPartDestroyAtomic(void *p)
{
	MvlErrorLog("HOOK POINT 0x31380 MvlRwPartDestroyAtomic (semantics not recovered)");
	(void)p;
}

// FUN_10031da0 @ 0x31da0 — undumped; no-arg per-sub-slot geometry release
// (from the cleanup path). Semantics not recovered — neutral name.
static void
MvlRwPartDestroyGeometry(void)
{
	MvlErrorLog("HOOK POINT 0x31da0 MvlRwPartDestroyGeometry (semantics not recovered)");
}

// ---------------------------------------------------------------------------
// FUN_10038db0 @ RVA 0x38db0 — (provisional) BuildPartDummyName
// Category name + sub-slot index -> dummy-frame name + side code (1/2) +
// isCustom flag (unknown categories: index 0 copies the name verbatim,
// higher indices append 'Q'). out is the 260-byte decomp local (aiStack_110).
// ===========================================================================
static char *
BuildPartDummyName(const char *name, int index, char *out, size_t outSize,
                   int *side, unsigned char *isCustom)
{
	*side = 0;
	out[0] = '\0';
	if(isCustom)
		*isCustom = 0;

	if(_stricmp(name, "Wheels") == 0){
		static const char *const kWheel[6] = {
			"wheel_rb_dummy", "wheel_lb_dummy", "wheel_rf_dummy",
			"wheel_lf_dummy", "wheel_rm_dummy", "wheel_lm_dummy" };
		// decomp strides 0xf per name with an explicit index chain; index >= 5
		// falls through to "wheel_lm_dummy".
		strncpy_s(out, outSize, kWheel[index < 6 ? index : 5], _TRUNCATE);
		return out;
	}
	if(_stricmp(name, "Wings") == 0){
		if(index == 0 || index == 1){
			strncpy_s(out, outSize, "wing_rf_", _TRUNCATE);
			*side = (index == 0) ? 1 : 2;
			strncat_s(out, outSize, "dummy", _TRUNCATE);
			return out;
		}
		strncpy_s(out, outSize, "wing_lf_", _TRUNCATE);
		*side = (index == 2) ? 1 : 2;
		strncat_s(out, outSize, "dummy", _TRUNCATE);
		return out;
	}
	if(_stricmp(name, "Windscreen") == 0){
		strncpy_s(out, outSize, "windscreen_dummy", _TRUNCATE);
		*side = (index == 0) ? 1 : 2;
		return out;
	}
	if(_stricmp(name, "Boot") == 0){
		strncpy_s(out, outSize, "boot_dum", _TRUNCATE);
		*side = (index == 0) ? 1 : 2;
		strncat_s(out, outSize, "my", _TRUNCATE);   // "boot_dummy"
		return out;
	}
	if(_stricmp(name, "Bonnet") == 0){
		strncpy_s(out, outSize, "bonnet_d", _TRUNCATE);
		*side = (index == 0) ? 1 : 2;
		strncat_s(out, outSize, "ummy", _TRUNCATE); // "bonnet_dummy"
		return out;
	}
	if(_stricmp(name, "Front bump") == 0){
		strncpy_s(out, outSize, "bump_front_dummy", _TRUNCATE);
		*side = (index == 0) ? 1 : 2;
		return out;
	}
	if(_stricmp(name, "Rear bump") == 0){
		strncpy_s(out, outSize, "bump_rear_dummy", _TRUNCATE);
		*side = (index == 0) ? 1 : 2;
		return out;
	}
	if(_stricmp(name, "Front doors") == 0){
		if(index == 0 || index == 1){
			strncpy_s(out, outSize, "door_lf_", _TRUNCATE);
			*side = (index == 0) ? 1 : 2;
			strncat_s(out, outSize, "dummy", _TRUNCATE);
			return out;
		}
		strncpy_s(out, outSize, "door_rf_", _TRUNCATE);
		*side = (index == 2) ? 1 : 2;
		strncat_s(out, outSize, "dummy", _TRUNCATE);
		return out;
	}
	if(_stricmp(name, "Rear doors") == 0){
		if(index == 0 || index == 1){
			strncpy_s(out, outSize, "door_lr_", _TRUNCATE);
			*side = (index == 0) ? 1 : 2;
			strncat_s(out, outSize, "dummy", _TRUNCATE);
			return out;
		}
		strncpy_s(out, outSize, "door_rr_", _TRUNCATE);
		*side = (index == 2) ? 1 : 2;
		strncat_s(out, outSize, "dummy", _TRUNCATE);
		return out;
	}
	// Unknown category: mark custom; index 0 copies the name verbatim (the
	// decomp's overlapping byte copy = strcpy), higher indices append 'Q'.
	if(isCustom)
		*isCustom = 1;
	if(index == 0)
		strncpy_s(out, outSize, name, _TRUNCATE);
	else
		sprintf_s(out, outSize, "%sQ", name);
	return out;
}

// ---------------------------------------------------------------------------
// FUN_10039df0 @ RVA 0x39df0 — (provisional) FreeTuningParts
// For each used record: destroy the 4 sub-slot part objects
// (FUN_10031380) + geometry handles (FUN_10031da0), zero the record's
// refcount, then zero the part count. Called on the wheel-dummy failure
// path (the original's only caller in this dump).
// ===========================================================================
static void
FreeTuningParts(CMvlVehicle *veh)
{
	int count = TuningCount(veh);
	if(count < 1){
		TuningCount(veh) = 0;
		return;
	}
	int r = 0;
	do{
		SMvlTuningRecord *rec = &TuningRecords(veh)[r];
		if(r < rec->nRefCount){                    // decomp: r < *(veh + r*0x15c + 0x814)
			for(int k = 0; k < 4; k++){
				if(rec->apSubSlotOut[k] != 0){      // piVar3[-0x3c] = record+0x54
					MvlRwPartDestroyAtomic(rec->apSubSlotOut[k]);   // FUN_10031380
					rec->apSubSlotOut[k] = 0;
				}
				if(rec->apGeometry[k] != 0){        // *piVar3 = record+0x144
					MvlRwPartDestroyGeometry();                     // FUN_10031da0 (no-arg)
					rec->apGeometry[k] = 0;
				}
			}
		}
		rec->nRefCount = 0;                         // *(veh + r*0x15c + 0x814) = 0
		r++;
	}while(r < count);
	TuningCount(veh) = 0;
}

// ===========================================================================
// LoadTuningFile — 0.98e @ RVA 0x39ec0
// ===========================================================================
void
LoadTuningFile(CMvlVehicle *veh)
{
	static const char *const fn = "LoadTuningFile";   // verbatim log identity

	if(veh == 0)
		return;

	TuningCount(veh) = 0;                             // *(veh+0x7e0) = 0
	memset(TuningRecords(veh), 0, 0x1B30);            // FUN_10063900 @ 0x63900 (memset)

	// --- Pass 1: the nine tuning DFF slots (m_apFiles[MVL_SLOT_TUNING1..9];
	// the decomp walks veh+0x344 dwords = m_apFiles[4], sizes at +0x384).
	// Only cars (*(veh+4) == 0 == m_nType) load tuning.
	for(int i = 0; i < 9; i++){
		void *buf = veh->m_apFiles[MVL_SLOT_TUNING1 + i];
		unsigned size = veh->m_anFileSizes[MVL_SLOT_TUNING1 + i];
		if(buf == 0 || veh->m_nType != 0)
			continue;

		void *stream = MvlRwMemoryStreamOpen(buf, size);   // FUN_100320a0 @ 0x320a0
		if(!stream){
			// '%s' operand dropped by Ghidra; reconstructed as m_szArchivePath.
			TnErr(fn, "Could not open memory stream for tuning model file in archive '%s'.",
			      veh->m_szArchivePath);
			continue;
		}
		if(!MvlRwStreamFindChunkHeader(stream)){           // FUN_10032770 @ 0x32770
			TnErr(fn, "Could not find clump chunk header from tuning file in archive '%s'.",
			      veh->m_szArchivePath);
			MvlRwMemoryStreamClose(stream);                // FUN_10032090 @ 0x32090
			continue;
		}
		MvlRwClumpReadPrep();                              // FUN_10032760 @ 0x32760
		MvlRwTexDictReadPrep();                            // FUN_10032750 @ 0x32750
		void *clump = MvlRwReadClump(stream);              // FUN_10031670 @ 0x31670
		MvlRwMemoryStreamClose(stream);
		free(veh->m_apFiles[MVL_SLOT_TUNING1 + i]);        // free + null the buffer (original)
		veh->m_apFiles[MVL_SLOT_TUNING1 + i] = 0;
		MvlRwTexDictReadPrep();                            // FUN_10032750 (again)
		if(!clump){
			TnErr(fn, "Loading clump from tuning file failed in archive '%s'.",
			      veh->m_szArchivePath);
			continue;
		}
		MvlRwClumpPostRead(clump);                          // FUN_100312c0 @ 0x312c0
		MvlWeaponClumpRegister(clump);                      // FUN_10031450 @ 0x31450
		// "part registered" pass: bump the per-record refcount of every
		// record flagged in a previous vehicle pass, then clear the flag.
		for(int r = 0; r < TuningCount(veh); r++){
			SMvlTuningRecord *rec = &TuningRecords(veh)[r];
			if(rec->bRegistered != 0){
				rec->nRefCount += 1;                       // *(rec+0x30) += 1
				rec->bRegistered = 0;
			}
		}
	}

	// Pass 1b: free every tuning buffer the pipeline did not consume.
	for(int i = 0; i < 9; i++){
		if(veh->m_apFiles[MVL_SLOT_TUNING1 + i]){
			free(veh->m_apFiles[MVL_SLOT_TUNING1 + i]);
			veh->m_apFiles[MVL_SLOT_TUNING1 + i] = 0;
		}
	}

	// --- Pass 2: resolve every recorded part (4 sub-slots each).
	// The decomp reads the "main clump" from *(m_pInfo + 0x28) (the clone
	// models that offset as SMvlVehicleInfo::nFlags028; the tuning path uses
	// it as a clump POINTER — engine lane may retype).
	void *mainClump = veh->m_pInfo ? (void *)(intptr_t)veh->m_pInfo->nFlags028 : 0;

	int r = 0;
	while(r < TuningCount(veh)){                 // count re-read: the original loop bound is live
		SMvlTuningRecord *rec = &TuningRecords(veh)[r];
		rec->nRefCount += 1;                     // piVar6[-1] += 1 (+0x814 abs)
		rec->nState = 0;                         // *piVar6 = 0 (+0x818 abs)

		char lastDummy[260];                     // decomp aiStack_110 (260 B)
		char lastCustom = 0;                     // decomp cStack_149
		lastDummy[0] = '\0';                     // uninitialised in the decomp when no
		                                         // sub-slot has a source; zeroed here
		for(int k = 0; k < 4; k++){
			if(rec->apSubSlotSrc[k] == 0)        // piVar5[-7] = record+0x38+k*4
				continue;
			int side = 0;                        // decomp uStack_130
			unsigned char isCustom = 0;          // unsigned: matches FUN_10038db0's out-param
			lastDummy[0] = '\0';
			BuildPartDummyName(rec->szName, k, lastDummy, sizeof(lastDummy),
			                   &side, &isCustom);  // FUN_10038db0 @ 0x38db0
			lastCustom = isCustom;

			if(_stricmp(rec->szName, "Wheels") == 0){
				// decomp iStack_134 correlates with FUN_10031ca0's RETURN (its
				// dumped body yields *(clump+4)) — RECONSTRUCTED linkage; the
				// +0x98 field read on the frame is "has a custom wheel"
				// (semantics not recovered).
				void *frame = ClumpFrameAt4(mainClump);   // FUN_10031ca0 @ 0x31ca0
				if(frame == 0 || *(int *)((unsigned char *)frame + 0x98) == 0){
					TnErr(fn, "Main model is missing a wheel dummy OR doesn't have a custom wheel.");
					FreeTuningParts(veh);        // FUN_10039df0 @ 0x39df0
					return;                      // aborts the whole function (original)
				}
				// decomp copies the string at FUN_10031280()'s result into the
				// local dummy buffer (bounded here — safety rule).
				{
					const char *src = MvlRwCurrentName();   // FUN_10031280 @ 0x31280
					if(src)
						strncpy_s(lastDummy, sizeof(lastDummy), src, _TRUNCATE);
				}
			}

			// FUN_100312c0's search RESULT (decomp iStack_118) is a fastcall
			// out-param the decompiler lost — it stays 0 in the clone until
			// the engine lane recovers the thunk's true signature, so the
			// "loaded" path is only reachable once wired.
			void *foundClump = 0;
			MvlRwClumpPostRead(foundClump);        // FUN_100312c0 @ 0x312c0 (arg = lost out-param)
			if(foundClump == 0){
				if(isCustom != 0){
					// decomp shadow piStack_138 = record+0x18 implies the
					// resolved dummy name is the search operand — RECONSTRUCTED
					// copy into the record buffer before the search.
					strncpy_s(rec->szResolvedDummy, sizeof(rec->szResolvedDummy),
					          lastDummy, _TRUNCATE);
					void *frame2 = ClumpFrameAt4(mainClump);   // FUN_10031ca0 @ 0x31ca0
					MvlRwClumpFinalise();                       // FUN_10031f10 @ 0x31f10 (game 0x645060; name arg lost)
					rec->apSubSlotOut[k] = 0;                   // *piVar5 = 0
					if(frame2 == 0){
						if(rec->nState == 0)
							rec->nState = 3;                    // -> chassis_dummy fallback
					}
					else if(rec->nState != 1){
						rec->nState = 2;                        // dummy attached
					}
				}
			}
			else{
				rec->apSubSlotOut[k] = MvlRwTuningCreatePart(); // FUN_10031410 @ 0x31410
				MvlRwTuningAttach();                            // FUN_100322b0 @ 0x322b0
				rec->nState = 1;
			}
		}

		// decomp: if (cStack_149 != 0 && *piVar6 == 3) write "chassis_dummy"
		// (14 bytes incl. NUL) at record+0x18, state = 2.
		if(lastCustom != 0 && rec->nState == 3){
			strncpy_s(rec->szResolvedDummy, sizeof(rec->szResolvedDummy),
			          "chassis_dummy", _TRUNCATE);
			rec->nState = 2;
		}
		if(rec->nState == 0)
			// '%s' = the part name (verbatim source); the second '%s' was
			// dropped by Ghidra — RECONSTRUCTED as the last built dummy name.
			TnWarn(fn, "Nothing to do with part '%s' (%s)", rec->szName, lastDummy);
		r++;
	}
}
