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
// EM-light destination block — the 0x44-byte array CMvlVehicle::
// m_abEmLightData @ 0x27f4 (written by the CVehicleHandler::LoadXML*EmLight*
// trio below). Field offsets recovered 1:1 from the FUN_1004e8f0/4edc0/4f090
// dest writes.
// ---------------------------------------------------------------------------
struct SMvlEmLightData
{
	unsigned char abEnabled[3];     // @ 0x00 [0] emlights [1] emlightsingle
	                                //        [2] taxilight (sector present)
	unsigned char bAlwaysOn;        // @ 0x03 shared 'alwayson' flag
	float afPosLeft[3];             // @ 0x04 emlights 'posleft'
	float afPosRight[3];            // @ 0x10 emlights 'posright'
	float afPosSingle[3];           // @ 0x1c emlightsingle 'position'
	float afPosTaxi[3];             // @ 0x28 taxilight 'position'
	unsigned char abColourOne[3];   // @ 0x34 emlights 'colourone' RGB
	unsigned char pad37;            // @ 0x37
	unsigned char abColourTwo[3];   // @ 0x38 emlights 'colourtwo' RGB
	unsigned char pad3b;            // @ 0x3b
	unsigned char abColourSingle[3];// @ 0x3c emlightsingle 'colour' RGB
	unsigned char pad3f;            // @ 0x3f
	unsigned char abColourTaxi[3];  // @ 0x40 taxilight 'colour' RGB
	unsigned char pad43;            // @ 0x43 (total 0x44)
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
// Vehicle-archive scan cluster seams (missing33 dump 0x4f380-0x4fe80).
// ---------------------------------------------------------------------------

// FUN_10032740 @ 0x32740 — game thunk (gta-vc.exe 0x64e060) called twice by
// LoadSanAndreasVehiclesTxd after the SA tex dict loads; results land in the
// primary/alternate livery colours (DAT_100b6498/100b649c = engine_globals
// MvlPrimaryLiveryColor/MvlAlternateLiveryColor). PROVISIONAL semantic (the
// thunk returns a colour-ish value; exact RW call not recovered).
int  MvlRwTexDictCurrentColor(void);

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

	// === EM-light sector parsers (emlights dump 0x4e8f0-0x4f230) =========
	// Names 1:1 from the functions' own error-log identities (strings.txt
	// @ 0x1008b88c/0x1008bcb4/0x1008bea8). All three write into the 0x44-
	// byte block (SMvlEmLightData / CMvlVehicle::m_abEmLightData @ 0x27f4)
	// handed over as dest by CMvlVehicle::LoadXMLSpecialData. The
	// forwarding decls LoadXMLEmLightData/LoadXMLEmLightSingle/
	// LoadXMLTaxiLight in src/archive/vehicle_archive.h keep their names
	// and route here (src/archive/vehicle_archive.cpp).

	// CVehicleHandler::LoadXMLEmLightData — 0.98e @ RVA 0x4e8f0.
	// <emlights>: 'colourone' ('primary' (60,255,0) / 'secondary'
	// (255,0,175) / 'r,g,b' with 255-clamp), 'colourtwo' ('r,g,b'),
	// 'posleft'/'posright' vectors, optional 'alwayson' ('true'/'false').
	// colourone/colourtwo/posleft/posright are required (verbatim errors).
	static bool LoadXMLEmLightData(TiXmlElement *e, void *dest);

	// CVehicleHandler::LoadXMLSingleEmLightData — 0.98e @ RVA 0x4edc0.
	// <emlightsingle>: 'colour' ('r,g,b'), 'position' vector, optional
	// 'alwayson'. Sets dest[1]=1 at entry; writes RGB @ +0x3c, pos @ +0x1c.
	static bool LoadXMLSingleEmLightData(TiXmlElement *e, void *dest);

	// CVehicleHandler::LoadXMLTaxiEmLightData — 0.98e @ RVA 0x4f090.
	// <taxilight>: 'colour' ('r,g,b'), 'position' vector, optional
	// 'alwayson'. Sets dest[2]=1 at entry; writes RGB @ +0x40, pos @ +0x28.
	static bool LoadXMLTaxiEmLightData(TiXmlElement *e, void *dest);

	// === Vehicle-archive scan cluster (missing33 dump 0x4f380-0x4fe80) ====
	// mvl\vehicles\*.7z discovery + per-archive slot allocation + SA-vehicle
	// txd fallback + idstart auto-adjust. Originals are register-passed
	// static member funcs (Ghidra __fastcall); log identities verbatim.

	// Capacity of the archive registry below (DAT_100b2838): the decomp's
	// hard bounds are all "< 500" (AnalyzeStartIndex clamps the count to 500,
	// MenuAddVehicleEntry ignores slots >= 500), so 500 entries cover every
	// reachable index.
	enum { kVehRegistryCapacity = 500 };

	// Slot table (original .data DAT_100b2838 @ RVA 0xb2838, one CMvlVehicle*
	// per relative slot; the scan cluster's registry). NOTE: the archive
	// lane's MvlVehicleRegCount/MvlVehicleRegGet (vehicle_archive.cpp) are
	// still hook-point stubs over the same original (FUN_1004f3f0/0x4f380) —
	// the orchestrator should re-point them at RegistryVehicle below.
	static CMvlVehicle *s_vehRegistry[kVehRegistryCapacity];  // DAT_100b2838 @ RVA 0xb2838
	static int  s_nNextFreeCursor;        // DAT_100b2834 @ RVA 0xb2834 (FirstFree scan start)
	static int  s_nAvailableSlots;        // DAT_100a6d50 @ RVA 0xa6d50 (slot count; 0 until the config lane / AnalyzeStartIndex seeds it)
	static int  s_nSlotLimit;             // DAT_101244cc @ RVA 0x1244cc (= base + count; AnalyzeStartIndex tail)
	static int  s_nLoadedArchiveCount;    // DAT_100b51d4 @ RVA 0xb51d4 (successful-archive counter)
	static unsigned char s_bSaTxdAttempted; // DAT_101247d4 @ RVA 0x1247d4 (one-shot latch)
	static void *s_pSaTexDict;            // DAT_100b51d0 @ RVA 0xb51d0 (loaded SA tex dict; MvlSaTexDictAvailable @ 0x44f20 reads this — archive lane to re-point)

	// CVehicleHandler::RegistryVehicle — 0.98e @ RVA 0x4f380.
	// Direct port of the dumped body: bounds-checked slot-table read
	// ([-1, s_nAvailableSlots) -> table[slot], else NULL). Same original as
	// the archive lane's MvlVehicleRegGet hook stub.
	static CMvlVehicle *RegistryVehicle(int relSlot);

	// CVehicleHandler::GetNewSlotForArchive — 0.98e @ RVA 0x4f4d0.
	// Parses the "x<id>_" filename prefix (FUN_1004f440), validates the
	// requested slot against [MvlModelIndexBaseOffset, s_nSlotLimit) and the
	// registry, else takes the first free slot (FUN_1004f4a0); allocates the
	// CMvlVehicle (FUN_100449e0 + FUN_10044a10) and stores it into the
	// registry. Returns NULL when no slot is free ("Slot out of range /
	// already in use, taking first free ID." warns verbatim). The internal
	// overload also reports the assigned slot (the original stores it at
	// CMvlVehicle+0x2d20 and LoadVehicleArchive reads it back as this[0xb48];
	// the clone threads it via the out-param until that field is modelled).
	static CMvlVehicle *GetNewSlotForArchive(const char *archivePath);
	static CMvlVehicle *GetNewSlotForArchive(const char *archivePath, int *slotOut);

	// CVehicleHandler::LoadVehicleArchive — 0.98e @ RVA 0x4f5a0.
	// Slot allocation + CMvlVehicle::LoadFromArchive(path, slot+base) + menu
	// entry ("%d: %ls" -> Win32 menu, FUN_1000cac0) + counter bump
	// (DAT_100b51d4). Errors: "Could not find a slot for vehicle file '%s',
	// all slots full." / "Could not load vehicle archive '%s'.".
	// Ghidra types the body void, but LoadVehicleArchives checks its AL
	// return (0 = failure) — ported as bool.
	static bool LoadVehicleArchive(const char *archivePath);

	// CVehicleHandler::LoadVehicleArchives — 0.98e @ RVA 0x4f680.
	// Two FindFirstFileA("mvl\\vehicles\\*.7z") passes (pass 0: only files
	// with an explicit "x<id>_" prefix, pass 1: the rest), then the
	// "InitVehicles" tail: an AI class (0..10) whose default car
	// (DAT_100b2804) is NOSPAWN-gated with no custom cars gets the default
	// registered into its list (DAT_100b51d8), the gta-vc.exe counter bumped
	// (DAT_009785a8) and the bypass byte cleared (DAT_100b641e).
	static void LoadVehicleArchives(void);

	// CVehicleHandler::LoadSanAndreasVehiclesTxd — 0.98e @ RVA 0x4f9a0.
	// One-shot: loads mvl/vehicles/sa_vehicle.txd, falling back to the SA
	// install's models\generic\vehicle.txd via the registry (ExePath),
	// through the RW tex-dict pipeline; result cached in s_pSaTexDict, the
	// two livery colours read via MvlRwTexDictCurrentColor.
	static void LoadSanAndreasVehiclesTxd(void);

	// CVehicleHandler::AnalyzeStartIndex — 0.98e @ RVA 0x4fc90.
	// idstart auto-adjust: counts the contiguous free model slots from
	// MvlModelIndexBaseOffset (max 500), scans for the biggest free gap in
	// [0x82, MvlModelIdBoundary], and (when fewer than 100 slots are free)
	// switches the base/count — warns verbatim — then publishes
	// s_nSlotLimit = base + count (DAT_101244cc).
	static void AnalyzeStartIndex(void);
};
