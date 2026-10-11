// vehicle_handler.h — CVehicleHandler: MVL save-system cluster (save-XML load).
//
// Faithful recompilation of the save-system subset of CVehicleHandler from
// decompiled vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/savewpn/range-decomp.txt, save cluster 0x50a30-0x51900;
// the SCM 0x51e20+ and weapons 0x5235a+ ranges in that dump belong to other
// lanes). Every function carries its origin VA. Original symbol names are
// reused 1:1 (error-string-confirmed). Names for the two unnamed depth
// callees are provisional and marked.
//
// Save-XML schema (recovered 1:1 from the 0.98e DLL's own parser tables at
// RVA 0xaf3c8-0xaf548 — see .harness-memory/limout/port-notes-veh.md
// "Save system" section):
//
//   <mvlsave>                                ; LoadSaveXmlDocument @ 0x51240
//     <vehicles>                             ; LoadSaveXmlRoot      @ 0x51150
//       <pool>                               ; LoadSaveXmlVehicles  @ 0x51060
//         <vehicle>                          ; LoadSaveXmlVehiclesList @ 0x50f20
//           <slot>N</slot>                   ; LoadSaveXmlVehicleInfo @ 0x50bd0
//           <identity>                       ;  required
//             <archivehash>0xXXXXXXXX</archivehash>  ; hex = archive total size
//             <archivename>name.7z</archivename>
//           </identity>
//           <tuningparts>                    ; optional
//             <partindex>N</partindex>       ; up to 20 (LoadSaveXmlVehicleInfoParts @ 0x50a30)
//           </tuningparts>
//         </vehicle>
//       </pool>
//       <garage> ... same vehicle list ... </garage>
//     </vehicles>
//     <playerweps>                           ; CWeaponHandler (NEXT LANE, stubbed)
//   </mvlsave>
//
// The sidecar file is "<savefile-without-.b>_mvl.xml" next to the game save
// (LoadExtraSaveFile @ 0x51340; staleness check "Ignoring extra save XML, it
// is older than main save file.").
#pragma once

#include "../core/stdafx.h"

#include "../archive/vehicle_archive.h"   // CMvlVehicle + MvlVehicleReg* hooks

#include <map>

class TiXmlElement;

// ---------------------------------------------------------------------------
// One saved-vehicle record — FUN_10059d7f(0x58) allocation in
// LoadSaveXmlVehicleInfo (0x50bd0). Layout recovered from the decomp:
//   [0]  slot index (pool slot, or garage slot 0..47)
//   [1]  MVL model index (CMvlVehicle::m_nId of the identity-matched vehicle;
//        the decomp dereferences the identity result before storing)
//   [2..21] 20 tuning-part selections (<partindex> values)
// Total 0x58 bytes = 2 + 20 dwords.
// ---------------------------------------------------------------------------
struct SMvlSaveVehicleRecord
{
	int nSlotIndex;          // @ 0x00
	int nModelIndex;         // @ 0x04 (CMvlVehicle::m_nId)
	int anParts[20];         // @ 0x08 .. 0x57
};

// ---------------------------------------------------------------------------
// Engine / game hooks for the save lane (stubs in vehicle_handler.cpp log
// MvlErrorLog + carry `// TODO(save): <VA>` comments; game behaviour is not
// faked). Address of truth is the VA in the comment.
// ---------------------------------------------------------------------------

// Current save-file path buffer (gta-vc.exe DAT_00a0ce9c, 260-byte path of
// the save being loaded):
const char *MvlGameSaveFilePath(void);          // TODO(save): 0xa0ce9c (gta-vc.exe)
// Vehicle-pool slot count (gta-vc.exe *(DAT_00a0fde4 + 8)):
int  MvlGamePoolLimit(void);                    // TODO(save): 0xa0fde4+8 (gta-vc.exe)
// 64-bit file timestamp (0 = not found) used by the sidecar staleness check:
unsigned long long MvlFileTimestamp64(const char *path); // TODO(save): 0x1c040
// Model info by MVL model index (FUN_1004f3a0; NULL when unknown):
void *MvlLookupModelInfoByModelIndex(int mvlModelId);  // TODO(save): 0x4f3a0
// Model-info vehicle type (object field [1] @ +0x4 — the piVar5[1] type
// check in FetchRealModelIndexForPoolSlot):
int  MvlModelInfoGetType(void *info);
// Save-apply driver globals (set by the game's save-load flow which the
// original patches at 0x51cb0; outside this dump):
int  MvlSaveApplyCurrentPoolSlot(void);         // TODO(save): DAT_1013dd20
int  MvlSaveApplyCurrentGarageSlot(void);       // TODO(save): DAT_1013dd0c
int  MvlSaveApplyPoolFallbackIndex(void);       // TODO(save): DAT_1013dcf8
int  MvlSaveApplyGarageFallbackIndex(void);     // TODO(save): DAT_1013dce4
int  MvlSaveApplyExpectedModelType(void);       // TODO(save): DAT_1013dcd0
// Garage record store slot: &DAT_100b3008 + garageSlot * 0x50 (0x50-byte records):
void *MvlGarageSlotRecord(int garageSlot);      // TODO(save): 0x100b3008
// CWeaponHandler save-data: REAL implementations now live in
// src/weapons/weapon_handler.cpp (tranche-4 weapons lane); decls kept here
// because LoadExtraSaveFile/LoadSaveXmlRoot call them by these signatures.
void CWeaponHandlerClearSaveData(void);         // 0x98e @ RVA 0x56d40
bool CWeaponHandlerLoadSaveXmlPlayerWeapons(TiXmlElement *sector); // 0x98e @ RVA 0x567a0

// ---------------------------------------------------------------------------
// CVehicleHandler — save-system cluster.
// The original's save functions are STATIC members (no thiscall; Ghidra shows
// __fastcall = register-passed static member funcs); the two slot maps are
// globals in the original (DAT_101244d0 pool / DAT_101244c4 garage, counts at
// +4) and are modelled here as class statics.
// ---------------------------------------------------------------------------
class CVehicleHandler
{
public:
	// Slot maps (original globals: DAT_101244d0 + count DAT_101244d4 pool;
	// DAT_101244c4 + count DAT_101244c4[2]/DAT_101244c8 garage). The original
	// is an MSVC std::map (node[4] = key, node[5] = record ptr).
	typedef std::map<int, SMvlSaveVehicleRecord *> TSaveSlotMap;
	static TSaveSlotMap s_poolSlots;            // @ DAT_101244d0
	static TSaveSlotMap s_garageSlots;          // @ DAT_101244c4

	// === Save-XML load chain (call order: LoadExtraSaveFile -> Document
	//     -> Root -> Vehicles -> VehiclesList -> VehicleInfo(-Parts/-Identity))

	// CVehicleHandler::LoadSaveXmlVehicleInfoParts — 0.98e @ RVA 0x50a30.
	// Collects repeatable <partindex> ints (max 20; overflow warns
	// "More than 20 part selections specified, ignoring overflow.").
	static bool LoadSaveXmlVehicleInfoParts(TiXmlElement *sector, int *partsOut);

	// CVehicleHandler::LoadSaveXmlVehicleInfoIdentity — 0.98e @ RVA 0x50af0.
	// <identity>: <archivehash> (hex, DAT_101247b4) + <archivename> (string,
	// DAT_101247b8); both required; then matches a loaded vehicle by
	// hash (= archive total size) or archive path. Returns the matched
	// vehicle (non-NULL) or 0 with "No vehicle match for hash %08X or
	// archive '%s'.".
	static CMvlVehicle *LoadSaveXmlVehicleInfoIdentity(TiXmlElement *sector);

	// CVehicleHandler::LoadSaveXmlVehicleInfo — 0.98e @ RVA 0x50bd0.
	// Parses <slot>/<identity>/<tuningparts> and allocates the 0x58-byte
	// record. garage=false: pool slot (range check + "Pool slot index %d is
	// already loaded."); garage=true: garage slot 0..47 ("Garage slot index
	// %d is already loaded."). QUIRK: the pool range failure logs the
	// garage-worded message "Slot index for garage vehicles must be between
	// 0 and %d." Returns the record (decomp typed it void; the caller reads
	// EAX as the record pointer and checks NULL).
	static SMvlSaveVehicleRecord *LoadSaveXmlVehicleInfo(TiXmlElement *sector, bool garage);

	// CVehicleHandler::LoadSaveXmlVehiclesList — 0.98e @ RVA 0x50f20.
	// Repeating <vehicle> sectors -> LoadSaveXmlVehicleInfo -> slot-map
	// insert (pool or garage per 'garage').
	static bool LoadSaveXmlVehiclesList(TiXmlElement *sector, bool garage);

	// CVehicleHandler::LoadSaveXmlVehicles — 0.98e @ RVA 0x51060.
	// <pool> (garage=0) and <garage> (garage=1) sub-lists; failures log
	// "Could not load '%s' sector." and the loop continues (original).
	static bool LoadSaveXmlVehicles(TiXmlElement *sector);

	// CVehicleHandler::LoadSaveXmlRoot — 0.98e @ RVA 0x51150.
	// <vehicles> -> LoadSaveXmlVehicles; <playerweps> -> CWeaponHandler
	// (next lane; stub).
	static bool LoadSaveXmlRoot(TiXmlElement *sector);

	// CVehicleHandler::LoadSaveXmlDocument — 0.98e @ RVA 0x51240.
	// TinyXML load of the save-XML file; root must be <mvlsave>. A missing
	// file (TinyXML TIXML_ERROR_OPENING_FILE == 2) is NOT an error — returns
	// true (the sidecar is optional).
	static bool LoadSaveXmlDocument(const char *path);

	// CVehicleHandler::LoadExtraSaveFile — 0.98e @ RVA 0x51340.
	// Clears both save lanes, then derives the sidecar path from the current
	// save path ("<path minus '.b'>_mvl.xml", VC saves end in ".b") and
	// loads it when both files exist and the sidecar is not stale (>20000000
	// timestamp units older than the main save).
	static void LoadExtraSaveFile(void);

	// === Save-apply helpers =============================================

	// CVehicleHandler::FetchRealModelIndexForPoolSlot — 0.98e @ RVA 0x51500.
	// Pool slot lookup for the save-apply flow; validates the record's model
	// type ("Type mismatch for model index %d at pool slot %d.") and returns
	// the MVL model index or the fallback (DAT_1013dcf8).
	static int FetchRealModelIndexForPoolSlot(void);

	// CVehicleHandler::FetchRealModelIndexForGarageSlot — 0.98e @ RVA 0x51620.
	// Garage-slot lookup; copies the record's 20-part block (0x50 bytes) into
	// the garage record store and returns the MVL model index or the fallback
	// (DAT_1013dce4). QUIRK: its failure message says "pool slot".
	static int FetchRealModelIndexForGarageSlot(void);

	// FUN_10051480 @ RVA 0x51480 — (provisional) FindLoadedVehicleForIdentity:
	// scan the loaded-vehicle registry first by hash (archive total size ==
	// vehicle+0x330), then by archive-path stricmp (vehicle+0x22c).
	// Called from LoadSaveXmlVehicleInfoIdentity.
	static CMvlVehicle *FindLoadedVehicleForIdentity(void);

	// FUN_10051720 @ RVA 0x51720 — (provisional) ClearSaveXmlRecords:
	// free every record and clear both slot maps. Called from
	// LoadExtraSaveFile before the sidecar reload.
	static void ClearSaveXmlRecords(void);
};
