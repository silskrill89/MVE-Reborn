// vehicle_handler.cpp — CVehicleHandler save-system cluster (see vehicle_handler.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (savewpn/range-decomp.txt,
// save cluster 0x50a30-0x51900). Element names come from the DLL's own parser
// tables (RVA 0xaf3c8-0xaf548, recovered by PE walk — see port-notes-veh.md
// "Save system"); log strings are verbatim from the decomp. The original's
// table engine (FUN_1001bb40 find / FUN_1001bc10 parse-entry / FUN_1001bbb0
// required-check) is reimplemented as explicit dispatch with identical
// semantics; helper-internal messages that are NOT in the decomp text are
// marked RECONSTRUCTED.
#include "vehicle_handler.h"

#include "../loader/log.h"
#include "../engine/engine_globals.h"   // MvlModelIndexBaseOffset / MvlModelIdBoundary /
                                        // MvlModelLookupRegistry / AI-class + livery tables

#include "tinyxml.h"   // vendored classic TinyXML (external/tinyxml)

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>     // malloc/free/strtol (archive scan cluster)
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrappers (same mapping as the XML lane): FUN_1001cc00 msg,
// FUN_1001cce0 warn, FUN_1001cb80 error; function name prefixed as context.
// ---------------------------------------------------------------------------
static void
SvMsg(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[sizeof(buf) - 1] = '\0';
	MvlLog("%s", buf);
}

static void
SvWarn(const char *fn, const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "%s: %s", fn, msg);
	buf[sizeof(buf) - 1] = '\0';
	MvlLog("%s", buf);
}

static void
SvErr(const char *fn, const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "%s: %s", fn, msg);
	buf[sizeof(buf) - 1] = '\0';
	MvlErrorLog("%s", buf);
}

// ---------------------------------------------------------------------------
// .data scratch globals mirrored from the original (table-engine destinations
// and parsed-value scratch; see vehicle_handler.h):
//   DAT_101247b4 archivehash value, DAT_101247b8 archivename pointer,
//   DAT_101247c4 slot value. (The bind sentinels DAT_101247ac/101247b0/
//   101247bc/101247c0/101247c8/101247cc/101247d0/101247d8 only route the
//   original's table engine and need no clone counterpart.)
// ---------------------------------------------------------------------------
static unsigned    s_nLastArchiveHash;      // @ DAT_101247b4
static const char *s_szLastArchiveName;     // @ DAT_101247b8 (GetText ptr; doc-owned)
static int         s_nLastSlotIndex;        // @ DAT_101247c4

CVehicleHandler::TSaveSlotMap CVehicleHandler::s_poolSlots;     // @ DAT_101244d0
CVehicleHandler::TSaveSlotMap CVehicleHandler::s_garageSlots;   // @ DAT_101244c4

// FUN_1001b970 @ 0x1b970 — case-insensitive "ends with" (used for the ".b"
// save-extension test in LoadExtraSaveFile; same helper as the archive lane).
static bool
MvlNameEndsWithIgnoreCase(const char *name, const char *needle)
{
	size_t nl, ml;
	if(!name || !needle)
		return false;
	nl = strlen(name);
	ml = strlen(needle);
	if(ml > nl)
		return false;
	return _stricmp(name + nl - ml, needle) == 0;
}

// RECONSTRUCTED sibling-pattern messages (the table helper's own strings live
// outside this decomp range; pattern verified in the LoadXML* family):
static bool
SvErrDup(const char *fn, const char *sector, const char *name)
{
	SvErr(fn, "'%s' sector contains multiple '%s' elements.", sector, name);
	return false;
}

static bool
SvErrMissing(const char *fn, const char *sector, const char *name)
{
	SvErr(fn, "'%s' sector is missing '%s' element.", sector, name);
	return false;
}

// ===========================================================================
// CVehicleHandler::LoadSaveXmlVehicleInfoParts — 0.98e @ RVA 0x50a30
// Repeatable <partindex> ints into a 20-slot array; overflow warns once and
// keeps counting. Unknown elements are ignored (table miss -> skip).
// ===========================================================================
bool
CVehicleHandler::LoadSaveXmlVehicleInfoParts(TiXmlElement *sector, int *partsOut)
{
	static const char *const fn = "CVehicleHandler::LoadSaveXmlVehicleInfoParts";
	int count = 0;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		if(_stricmp(e->Value(), "partindex") != 0)
			continue;   // unknown parts element: ignored (original)
		if(count == 20){
			SvWarn(fn, "More than 20 part selections specified, ignoring overflow.");
		}
		else if(count < 20){
			const char *t = e->GetText();
			int v = 0;
			// table type 0x69 ('i') -> "%i"; failure text RECONSTRUCTED (the
			// table helper's own strings are outside this decomp range).
			if(!t || sscanf(t, "%i", &v) != 1){
				SvErr(fn, "Failed to read data from '%s' element.", "partindex");
				return false;
			}
			partsOut[count] = v;   // via the DAT_101247c0 scratch in the original
		}
		count++;
	}
	return true;
}

// ===========================================================================
// FUN_10051480 @ RVA 0x51480 — (provisional) FindLoadedVehicleForIdentity
// Two registry passes: match by hash (= archive total size, vehicle @ 0x330)
// then by archive path stricmp (vehicle @ 0x22c). Uses the archive lane's
// registry hooks (MvlVehicleRegCount / MvlVehicleRegGet, FUN_1004f3f0 /
// FUN_1004f380).
// ===========================================================================
CMvlVehicle *
CVehicleHandler::FindLoadedVehicleForIdentity(void)
{
	int count = MvlVehicleRegCount();
	for(int i = 0; i < count; i++){
		CMvlVehicle *v = MvlVehicleRegGet(i);
		if(v != 0 && (unsigned)v->m_nArchiveTotalSize == s_nLastArchiveHash)
			return v;
	}
	// The original stricmp's DAT_101247b8 unconditionally; archivename is
	// required so it is always set here, but keep the null guard (safety).
	if(s_szLastArchiveName){
		for(int i = 0; i < count; i++){
			CMvlVehicle *v = MvlVehicleRegGet(i);
			if(v && _stricmp(s_szLastArchiveName, v->m_szArchivePath) == 0)
				return v;
		}
	}
	return 0;
}

// ===========================================================================
// CVehicleHandler::LoadSaveXmlVehicleInfoIdentity — 0.98e @ RVA 0x50af0
// ===========================================================================
CMvlVehicle *
CVehicleHandler::LoadSaveXmlVehicleInfoIdentity(TiXmlElement *sector)
{
	static const char *const fn = "CVehicleHandler::LoadSaveXmlVehicleInfoIdentity";
	bool bHash = false, bName = false;

	s_nLastArchiveHash = 0;
	s_szLastArchiveName = 0;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "archivehash") == 0){
			if(bHash)
				return SvErrDup(fn, "identity", name), (CMvlVehicle *)0;
			const char *t = e->GetText();
			// table type 0x58 ('X') -> "%x"; helper failure aborts (return 0);
			// message RECONSTRUCTED (table-helper strings out of range).
			if(!t || sscanf(t, "%x", &s_nLastArchiveHash) != 1){
				SvErr(fn, "Failed to read data from '%s' element.", name);
				return 0;
			}
			bHash = true;
		}
		else if(_stricmp(name, "archivename") == 0){
			if(bName)
				return SvErrDup(fn, "identity", name), (CMvlVehicle *)0;
			// Original stores the GetText() pointer directly (DAT_101247b8).
			s_szLastArchiveName = e->GetText();
			bName = true;
		}
		// unknown identity element: ignored (table miss -> skip)
	}

	// Required: archivehash + archivename (table +12 required flags).
	if(!bHash)
		return SvErrMissing(fn, "identity", "archivehash"), (CMvlVehicle *)0;
	if(!bName)
		return SvErrMissing(fn, "identity", "archivename"), (CMvlVehicle *)0;

	CMvlVehicle *v = FindLoadedVehicleForIdentity();
	if(!v){
		SvErr(fn, "No vehicle match for hash %08X or archive '%s'.",
		      s_nLastArchiveHash, s_szLastArchiveName ? s_szLastArchiveName : "");
		return 0;
	}
	return v;
}

// ===========================================================================
// CVehicleHandler::LoadSaveXmlVehicleInfo — 0.98e @ RVA 0x50bd0
// <slot>/<identity>/<tuningparts>; allocates the 0x58-byte record. The
// decomp types the return void, but VehiclesList reads EAX as the record
// pointer (NULL = failure) — ported accordingly.
// QUIRK: the pool range message says "garage vehicles" (original copy/paste).
// ===========================================================================
SMvlSaveVehicleRecord *
CVehicleHandler::LoadSaveXmlVehicleInfo(TiXmlElement *sector, bool garage)
{
	static const char *const fn = "CVehicleHandler::LoadSaveXmlVehicleInfo";
	int parts[20];
	CMvlVehicle *veh = 0;
	bool bSlot = false;

	memset(parts, 0, sizeof(parts));

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "slot") == 0){
			const char *t = e->GetText();
			// table type 0x69 ('i') -> "%i"; message RECONSTRUCTED.
			if(!t || sscanf(t, "%i", &s_nLastSlotIndex) != 1){
				SvErr(fn, "Failed to read data from '%s' element.", name);
				return 0;
			}
			if(!garage){
				int limit = MvlGamePoolLimit();
				if(s_nLastSlotIndex < 0 || s_nLastSlotIndex >= limit){
					SvErr(fn, "Slot index for garage vehicles must be between 0 and %d.",
					      limit);   // QUIRK: pool path uses the garage wording
					return 0;
				}
				if(s_poolSlots.find(s_nLastSlotIndex) != s_poolSlots.end()){
					SvErr(fn, "Pool slot index %d is already loaded.", s_nLastSlotIndex);
					return 0;
				}
			}else{
				if(s_nLastSlotIndex < 0 || s_nLastSlotIndex > 0x2f){
					SvErr(fn, "Slot index for garage vehicles must be between 0 and 47.");
					return 0;
				}
				if(s_garageSlots.find(s_nLastSlotIndex) != s_garageSlots.end()){
					SvErr(fn, "Garage slot index %d is already loaded.", s_nLastSlotIndex);
					return 0;
				}
			}
			bSlot = true;
		}
		else if(_stricmp(name, "identity") == 0){
			veh = LoadSaveXmlVehicleInfoIdentity(e);
			if(!veh){
				SvErr(fn, "Could not load '%s' sector.", name);
				return 0;
			}
		}
		else if(_stricmp(name, "tuningparts") == 0){
			if(!LoadSaveXmlVehicleInfoParts(e, parts)){
				SvErr(fn, "Could not load '%s' sector.", name);
				return 0;
			}
		}
		// unknown vehicle-info element: ignored (original LAB_10050db9)
	}

	// Required: slot + identity (tuningparts optional; table flags).
	if(!bSlot)
		return SvErrMissing(fn, "vehicle", "slot"), (SMvlSaveVehicleRecord *)0;
	if(!veh)
		return SvErrMissing(fn, "vehicle", "identity"), (SMvlSaveVehicleRecord *)0;

	// FUN_10059d7f(0x58) allocation; [0] slot, [1] model id (= *identity
	// result = CMvlVehicle::m_nId), [2..21] the 20 parts.
	SMvlSaveVehicleRecord *rec = new SMvlSaveVehicleRecord;
	rec->nSlotIndex  = s_nLastSlotIndex;
	rec->nModelIndex = veh->m_nId;
	memcpy(rec->anParts, parts, sizeof(rec->anParts));
	return rec;
}

// ===========================================================================
// CVehicleHandler::LoadSaveXmlVehiclesList — 0.98e @ RVA 0x50f20
// Repeatable <vehicle>; insert-or-assign into the pool/garage slot map
// (original FUN_100061f0 find + FUN_10006220 insert). A failing vehicle is
// logged and SKIPPED (the original continues the loop).
// ===========================================================================
bool
CVehicleHandler::LoadSaveXmlVehiclesList(TiXmlElement *sector, bool garage)
{
	static const char *const fn = "CVehicleHandler::LoadSaveXmlVehiclesList";

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		if(_stricmp(e->Value(), "vehicle") != 0)
			continue;   // unknown list element: ignored (table miss)
		SMvlSaveVehicleRecord *rec = LoadSaveXmlVehicleInfo(e, garage);
		if(!rec){
			SvErr(fn, "Could not load '%s' sector.", "vehicle");
			continue;
		}
		TSaveSlotMap &map = garage ? s_garageSlots : s_poolSlots;
		map[rec->nSlotIndex] = rec;
	}
	// 'vehicle' is the only table entry and is not required -> always true.
	return true;
}

// ===========================================================================
// CVehicleHandler::LoadSaveXmlVehicles — 0.98e @ RVA 0x51060
// ===========================================================================
bool
CVehicleHandler::LoadSaveXmlVehicles(TiXmlElement *sector)
{
	static const char *const fn = "CVehicleHandler::LoadSaveXmlVehicles";

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		bool garage;
		if(_stricmp(name, "pool") == 0)
			garage = false;
		else if(_stricmp(name, "garage") == 0)
			garage = true;
		else
			continue;   // unknown vehicles element: ignored (table miss)
		if(!LoadSaveXmlVehiclesList(e, garage))
			SvErr(fn, "Could not load '%s' sector.", name);
		// original continues on failure
	}
	return true;
}

// ===========================================================================
// CVehicleHandler::LoadSaveXmlRoot — 0.98e @ RVA 0x51150
// ===========================================================================
bool
CVehicleHandler::LoadSaveXmlRoot(TiXmlElement *sector)
{
	static const char *const fn = "CVehicleHandler::LoadSaveXmlRoot";

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		bool ok;
		if(_stricmp(name, "vehicles") == 0)
			ok = LoadSaveXmlVehicles(e);
		else if(_stricmp(name, "playerweps") == 0)
			ok = CWeaponHandlerLoadSaveXmlPlayerWeapons(e);  // TODO(save): 0x567a0 next lane
		else
			continue;   // unknown root element: ignored (table miss)
		if(!ok)
			SvErr(fn, "Could not load '%s' sector.", name);
		// original continues on failure
	}
	return true;
}

// ===========================================================================
// CVehicleHandler::LoadSaveXmlDocument — 0.98e @ RVA 0x51240
// TinyXML load (FUN_100351b0 ctor / FUN_10035280 LoadFile / FUN_10034a80
// dtor); root must be <mvlsave>. Missing file (TinyXML error id 2 ==
// TIXML_ERROR_OPENING_FILE) is NOT an error — the sidecar is optional.
// ===========================================================================
bool
CVehicleHandler::LoadSaveXmlDocument(const char *path)
{
	static const char *const fn = "CVehicleHandler::LoadSaveXmlDocument";

	TiXmlDocument doc(path);
	doc.LoadFile();

	if(doc.Error()){
		if(doc.ErrorId() == TiXmlBase::TIXML_ERROR_OPENING_FILE)   // == 2 in the original
			return true;
		SvErr(fn, "Save XML file could not be parsed: %s.", doc.ErrorDesc());
		return false;
	}

	TiXmlElement *root = doc.FirstChildElement();
	if(!root){
		SvErr(fn, "No root element in XML.");
		return false;
	}
	if(root->Type() != TiXmlNode::TINYXML_ELEMENT ||
	   _stricmp(root->Value(), "mvlsave") != 0){
		SvErr(fn, "Root element has the wrong name.");
		return false;
	}
	return LoadSaveXmlRoot(root);
}

// ===========================================================================
// FUN_10051720 @ RVA 0x51720 — (provisional) ClearSaveXmlRecords
// Frees every record and clears both slot maps (original walks both MSVC
// map trees, freeing node value [5] then the nodes).
// ===========================================================================
void
CVehicleHandler::ClearSaveXmlRecords(void)
{
	for(TSaveSlotMap::iterator it = s_poolSlots.begin();
	    it != s_poolSlots.end(); ++it)
		delete it->second;
	s_poolSlots.clear();
	for(TSaveSlotMap::iterator it = s_garageSlots.begin();
	    it != s_garageSlots.end(); ++it)
		delete it->second;
	s_garageSlots.clear();
}

// ===========================================================================
// CVehicleHandler::LoadExtraSaveFile — 0.98e @ RVA 0x51340
// Sidecar derivation: VC saves end in ".b"; the sidecar is the path minus
// those 2 chars + "_mvl.xml". Staleness: skip when the sidecar timestamp is
// more than 20000000 units OLDER than the main save. Both timestamps must
// be non-zero (both files present).
// ===========================================================================
void
CVehicleHandler::LoadExtraSaveFile(void)
{
	ClearSaveXmlRecords();              // FUN_10051720 @ 0x51720
	CWeaponHandlerClearSaveData();      // TODO(save): 0x56d40 (next lane)

	const char *savePath = MvlGameSaveFilePath();   // TODO(save): 0xa0ce9c (gta-vc.exe)
	if(!savePath)
		return;
	size_t len = strlen(savePath);
	if((int)len <= 2 || !MvlNameEndsWithIgnoreCase(savePath, ".b"))
		return;

	char sidecar[0x118 + 4];   // decomp local_128 [284] with 0x118 capacity
	// BUGFIX(stable-1.0): reserve room for the "_mvl.xml" suffix (8 chars +
	// NUL). The old check only bounded the memcpy, so a save path of
	// 276..283 chars passed the check and then aborted inside strcat_s
	// (invalid-parameter crash).
	if(len - 2 + sizeof("_mvl.xml") > sizeof(sidecar))
		return;   // range-check equivalent (___report_rangecheckfailure path)
	memcpy(sidecar, savePath, len - 2);
	sidecar[len - 2] = '\0';
	strcat_s(sidecar, sizeof(sidecar), "_mvl.xml");

	unsigned long long tMain = MvlFileTimestamp64(savePath);     // TODO(save): 0x1c040
	unsigned long long tSide = MvlFileTimestamp64(sidecar);
	if(tMain != 0 && tSide != 0){
		if(tSide + 20000000ULL < tMain){
			SvMsg("Ignoring extra save XML, it is older than main save file.");
		}
		else if(!LoadSaveXmlDocument(sidecar)){
			SvErr("CVehicleHandler::LoadExtraSaveFile", "Failed to load save extra XML.");
		}
	}
}

// ===========================================================================
// CVehicleHandler::FetchRealModelIndexForPoolSlot — 0.98e @ RVA 0x51500
// Pool-slot lookup for the save-apply flow. Returns the record's MVL model
// index; on lookup/type failure the fallback (DAT_1013dcf8) is returned.
// ===========================================================================
int
CVehicleHandler::FetchRealModelIndexForPoolSlot(void)
{
	static const char *const fn = "CVehicleHandler::FetchRealModelIndexForPoolSlot";
	int slot = MvlSaveApplyCurrentPoolSlot();          // DAT_1013dd20
	int fallback = MvlSaveApplyPoolFallbackIndex();    // DAT_1013dcf8

	TSaveSlotMap::iterator it = s_poolSlots.find(slot);
	if(it == s_poolSlots.end())
		return fallback;

	int mvlIndex = it->second->nModelIndex;            // record[1]
	void *info = MvlLookupModelInfoByModelIndex(mvlIndex);  // TODO(save): 0x4f3a0
	if(!info){
		SvErr(fn, "Failed to find MVL model index %d at pool slot %d.",
		      mvlIndex, slot);
		return fallback;
	}
	if(MvlModelInfoGetType(info) != MvlSaveApplyExpectedModelType()){   // DAT_1013dcd0
		SvErr(fn, "Type mismatch for model index %d at pool slot %d.",
		      mvlIndex, slot);
		return fallback;
	}
	return mvlIndex;
}

// ===========================================================================
// CVehicleHandler::FetchRealModelIndexForGarageSlot — 0x98e @ RVA 0x51620
// Garage-slot lookup; copies the record's 20-part block (0x50 bytes from
// record+8) into the garage record store (DAT_100b3008 + slot*0x50) and
// returns the MVL model index or the fallback (DAT_1013dce4). No type check
// here (unlike the pool path). QUIRK: its failure message says "pool slot".
// ===========================================================================
int
CVehicleHandler::FetchRealModelIndexForGarageSlot(void)
{
	static const char *const fn = "CVehicleHandler::FetchRealModelIndexForGarageSlot";
	int slot = MvlSaveApplyCurrentGarageSlot();        // DAT_1013dd0c
	int fallback = MvlSaveApplyGarageFallbackIndex();  // DAT_1013dce4

	TSaveSlotMap::iterator it = s_garageSlots.find(slot);
	if(it == s_garageSlots.end())
		return fallback;

	SMvlSaveVehicleRecord *rec = it->second;
	int mvlIndex = rec->nModelIndex;                   // *(record + 4)
	void *info = MvlLookupModelInfoByModelIndex(mvlIndex);
	if(!info){
		SvErr(fn, "Failed to find MVL model index %d at pool slot %d.",
		      mvlIndex, slot);   // QUIRK: garage fn says "pool slot"
		return fallback;
	}
	// 20 dwords (0x50) of parts -> garage slot store. The original indexes
	// the store by a driver EDX lost by the decompiler; the current garage
	// slot is the only consistent index.
	void *store = MvlGarageSlotRecord(slot);            // TODO(save): 0x100b3008
	if(store)
		memcpy(store, rec->anParts, 0x50);
	return mvlIndex;
}

// ===========================================================================
// Hook stubs. HARNESS STUBS: log + benign return; the engine / weapons lanes
// replace them. Nothing is faked.
// ===========================================================================
static char s_szEmptyPath[1] = { 0 };

const char *MvlGameSaveFilePath(void)
{
	MvlErrorLog("TODO(save): 0xa0ce9c MvlGameSaveFilePath (gta-vc.exe save-path buffer)");
	return s_szEmptyPath;   // no save path -> LoadExtraSaveFile no-ops
}

int MvlGamePoolLimit(void)
{
	MvlErrorLog("TODO(save): 0xa0fde4+8 MvlGamePoolLimit (gta-vc.exe pool size)");
	return 0x30;            // VC vehicle pool default; keeps range checks sane
}

unsigned long long MvlFileTimestamp64(const char *)
{
	MvlErrorLog("TODO(save): 0x1c040 MvlFileTimestamp64");
	return 0;               // 0 = file not found -> sidecar load skipped
}

void *MvlLookupModelInfoByModelIndex(int)
{
	MvlErrorLog("TODO(save): 0x4f3a0 MvlLookupModelInfoByModelIndex");
	return 0;
}

int MvlModelInfoGetType(void *info)
{
	// model-info object field [1] (offset +0x4) = vehicle type.
	return info ? *(int *)((char *)info + 4) : -1;
}

int MvlSaveApplyCurrentPoolSlot(void)
{
	MvlErrorLog("TODO(save): DAT_1013dd20 MvlSaveApplyCurrentPoolSlot");
	return 0;
}

int MvlSaveApplyCurrentGarageSlot(void)
{
	MvlErrorLog("TODO(save): DAT_1013dd0c MvlSaveApplyCurrentGarageSlot");
	return 0;
}

int MvlSaveApplyPoolFallbackIndex(void)
{
	MvlErrorLog("TODO(save): DAT_1013dcf8 MvlSaveApplyPoolFallbackIndex");
	return 0;
}

int MvlSaveApplyGarageFallbackIndex(void)
{
	MvlErrorLog("TODO(save): DAT_1013dce4 MvlSaveApplyGarageFallbackIndex");
	return 0;
}

int MvlSaveApplyExpectedModelType(void)
{
	MvlErrorLog("TODO(save): DAT_1013dcd0 MvlSaveApplyExpectedModelType");
	return -1;
}

void *MvlGarageSlotRecord(int)
{
	MvlErrorLog("TODO(save): 0x100b3008 MvlGarageSlotRecord (garage slot store)");
	return 0;
}

// CWeaponHandler save-XML entry points: the stubs that used to live here were
// replaced by the real implementations in src/weapons/weapon_handler.cpp
// (tranche-4 weapons lane; signatures unchanged — decls remain in
// vehicle_handler.h).
#include "../weapons/weapon_handler.h"

// ===========================================================================
// CVehicleHandler EM-light sector parsers — vehmod_retail.dll 0.98e
// (emlights/range-decomp.txt). Dest = the 0x44-byte block (SMvlEmLightData /
// CMvlVehicle::m_abEmLightData @ 0x27f4). Log strings verbatim; error-log
// identities are the original function names (strings.txt @ 0x1008b88c /
// 0x1008bcb4 / 0x1008bea8). Vector elements go through MvlParseVector
// (FUN_1001bdb0 — the exact original callee; see port-notes-media.md).
// ===========================================================================

// CVehicleHandler::LoadXMLEmLightData — 0.98e @ RVA 0x4e8f0.
bool
CVehicleHandler::LoadXMLEmLightData(TiXmlElement *e, void *dest)
{
	static const char *const fn = "CVehicleHandler::LoadXMLEmLightData";
	unsigned char *d = (unsigned char *)dest;
	bool bColourOne = false, bColourTwo = false;
	bool bPosLeft = false, bPosRight = false, bAlwaysOn = false;

	if(!e || !dest)
		return false;

	d[0] = 1;

	for(TiXmlElement *c = e->FirstChildElement(); c;
	    c = c->NextSiblingElement()){
		const char *name = c->Value();
		if(_stricmp(name, "colourone") == 0){
			unsigned r, g, b;
			if(bColourOne){
				SvErr(fn, "'emlights' sector contains multiple 'colourone' elements.");
				return false;
			}
			const char *t = c->GetText();
			if(!t){
				SvErr(fn, "Failed to read data from 'colourone' element.");
				return false;
			}
			if(_stricmp(t, "primary") == 0){
				r = 0x3c; g = 0xff; b = 0;
			}else if(_stricmp(t, "secondary") == 0){
				r = 0xff; g = 0; b = 0xaf;
			}else{
				if(sscanf(t, "%u,%u,%u", &r, &g, &b) != 3){
					SvErr(fn, "Failed to parse the contents of 'colourone' element (format 'r,g,b').");
					return false;
				}
				if(0xff < r) r = 0xff;
				if(0xff < g) g = 0xff;
				if(0xff < b) b = 0xff;
			}
			d[0x34] = (unsigned char)r;
			d[0x35] = (unsigned char)g;
			d[0x36] = (unsigned char)b;
			bColourOne = true;
		}
		else if(_stricmp(name, "colourtwo") == 0){
			unsigned r, g, b;
			if(bColourTwo){
				SvErr(fn, "'emlights' sector contains multiple 'colourtwo' elements.");
				return false;
			}
			const char *t = c->GetText();
			if(!t){
				SvErr(fn, "Failed to read data from 'colourtwo' element.");
				return false;
			}
			if(sscanf(t, "%u,%u,%u", &r, &g, &b) != 3){
				SvErr(fn, "Failed to parse the contents of 'colourtwo' element (format 'r,g,b').");
				return false;
			}
			if(0xff < r) r = 0xff;
			if(0xff < g) g = 0xff;
			if(0xff < b) b = 0xff;
			d[0x38] = (unsigned char)r;
			d[0x39] = (unsigned char)g;
			d[0x3a] = (unsigned char)b;
			bColourTwo = true;
		}
		else if(_stricmp(name, "posleft") == 0){
			if(bPosLeft){
				SvErr(fn, "'emlights' sector contains multiple 'posleft' vectors.");
				return false;
			}
			if(!MvlParseVector(c, (float *)(d + 0x04))){
				SvErr(fn, "Could not load 'posleft' vector.");
				return false;
			}
			bPosLeft = true;
		}
		else if(_stricmp(name, "posright") == 0){
			if(bPosRight){
				SvErr(fn, "'emlights' sector contains multiple 'posright' vectors.");
				return false;
			}
			if(!MvlParseVector(c, (float *)(d + 0x10))){
				SvErr(fn, "Could not load 'posright' vector.");
				return false;
			}
			bPosRight = true;
		}
		else if(_stricmp(name, "alwayson") == 0){
			if(bAlwaysOn){
				SvErr(fn, "'emlights' sector contains multiple 'alwayson' vectors.");
				return false;
			}
			const char *t = c->GetText();
			if(!t){
				SvErr(fn, "Failed to read data from 'alwayson' element.");
				return false;
			}
			if(_stricmp(t, "true") == 0){
				d[3] = 1;
			}else if(_stricmp(t, "false") != 0){
				SvErr(fn, "Element 'alwayson' may only have values 'true' or 'false'.");
				return false;
			}
			bAlwaysOn = true;
		}
		else{
			SvErr(fn, "Unknown element '%s' in 'emlights' sector.", name);
			return false;
		}
	}

	if(!bColourOne){
		SvErr(fn, "'emlights' sector is missing 'colourone' element.");
		return false;
	}
	if(!bColourTwo){
		SvErr(fn, "'emlights' sector is missing 'colourtwo' element.");
		return false;
	}
	if(!bPosLeft){
		SvErr(fn, "'emlights' sector is missing 'posleft' vector.");
		return false;
	}
	if(!bPosRight){
		SvErr(fn, "'emlights' sector is missing 'posright' vector.");
		return false;
	}
	return true;
}

// CVehicleHandler::LoadXMLSingleEmLightData — 0.98e @ RVA 0x4edc0.
bool
CVehicleHandler::LoadXMLSingleEmLightData(TiXmlElement *e, void *dest)
{
	static const char *const fn = "CVehicleHandler::LoadXMLSingleEmLightData";
	unsigned char *d = (unsigned char *)dest;
	bool bColour = false, bPosition = false, bAlwaysOn = false;

	if(!e || !dest)
		return false;

	d[1] = 1;

	for(TiXmlElement *c = e->FirstChildElement(); c;
	    c = c->NextSiblingElement()){
		const char *name = c->Value();
		if(_stricmp(name, "colour") == 0){
			unsigned r, g, b;
			if(bColour){
				SvErr(fn, "'emlightsingle' sector contains multiple 'colour' elements.");
				return false;
			}
			const char *t = c->GetText();
			if(!t){
				SvErr(fn, "Failed to read data from 'colour' element.");
				return false;
			}
			if(sscanf(t, "%u,%u,%u", &r, &g, &b) != 3){
				SvErr(fn, "Failed to parse the contents of 'colour' element (format 'r,g,b').");
				return false;
			}
			if(0xff < r) r = 0xff;
			if(0xff < g) g = 0xff;
			if(0xff < b) b = 0xff;
			d[0x3c] = (unsigned char)r;
			d[0x3d] = (unsigned char)g;
			d[0x3e] = (unsigned char)b;
			bColour = true;
		}
		else if(_stricmp(name, "position") == 0){
			if(bPosition){
				SvErr(fn, "'emlightsingle' sector contains multiple 'position' vectors.");
				return false;
			}
			if(!MvlParseVector(c, (float *)(d + 0x1c))){
				SvErr(fn, "Could not load 'position' vector.");
				return false;
			}
			bPosition = true;
		}
		else if(_stricmp(name, "alwayson") == 0){
			if(bAlwaysOn){
				SvErr(fn, "'emlightsingle' sector contains multiple 'alwayson' vectors.");
				return false;
			}
			const char *t = c->GetText();
			if(!t){
				SvErr(fn, "Failed to read data from 'alwayson' element.");
				return false;
			}
			if(_stricmp(t, "true") == 0){
				d[3] = 1;
			}else if(_stricmp(t, "false") != 0){
				SvErr(fn, "Element 'alwayson' may only have values 'true' or 'false'.");
				return false;
			}
			bAlwaysOn = true;
		}
		else{
			SvErr(fn, "Unknown element '%s' in 'emlightsingle' sector.", name);
			return false;
		}
	}

	if(!bColour){
		SvErr(fn, "'emlightsingle' sector is missing 'colour' element.");
		return false;
	}
	if(!bPosition){
		SvErr(fn, "'emlightsingle' sector is missing 'position' vector.");
		return false;
	}
	return true;
}

// CVehicleHandler::LoadXMLTaxiEmLightData — 0.98e @ RVA 0x4f090.
bool
CVehicleHandler::LoadXMLTaxiEmLightData(TiXmlElement *e, void *dest)
{
	static const char *const fn = "CVehicleHandler::LoadXMLTaxiEmLightData";
	unsigned char *d = (unsigned char *)dest;
	bool bColour = false, bPosition = false, bAlwaysOn = false;

	if(!e || !dest)
		return false;

	d[2] = 1;

	for(TiXmlElement *c = e->FirstChildElement(); c;
	    c = c->NextSiblingElement()){
		const char *name = c->Value();
		if(_stricmp(name, "colour") == 0){
			unsigned r, g, b;
			if(bColour){
				SvErr(fn, "'taxilight' sector contains multiple 'colour' elements.");
				return false;
			}
			const char *t = c->GetText();
			if(!t){
				SvErr(fn, "Failed to read data from 'colour' element.");
				return false;
			}
			if(sscanf(t, "%u,%u,%u", &r, &g, &b) != 3){
				SvErr(fn, "Failed to parse the contents of 'colour' element (format 'r,g,b').");
				return false;
			}
			if(0xff < r) r = 0xff;
			if(0xff < g) g = 0xff;
			if(0xff < b) b = 0xff;
			d[0x40] = (unsigned char)r;
			d[0x41] = (unsigned char)g;
			d[0x42] = (unsigned char)b;
			bColour = true;
		}
		else if(_stricmp(name, "position") == 0){
			if(bPosition){
				SvErr(fn, "'taxilight' sector contains multiple 'position' vectors.");
				return false;
			}
			if(!MvlParseVector(c, (float *)(d + 0x28))){
				SvErr(fn, "Could not load 'position' vector.");
				return false;
			}
			bPosition = true;
		}
		else if(_stricmp(name, "alwayson") == 0){
			if(bAlwaysOn){
				SvErr(fn, "'taxilight' sector contains multiple 'alwayson' elements.");
				return false;
			}
			const char *t = c->GetText();
			if(!t){
				SvErr(fn, "Failed to read data from 'alwayson' element.");
				return false;
			}
			if(_stricmp(t, "true") == 0){
				d[3] = 1;
			}else if(_stricmp(t, "false") != 0){
				SvErr(fn, "Element 'alwayson' may only have values 'true' or 'false'.");
				return false;
			}
			bAlwaysOn = true;
		}
		else{
			SvErr(fn, "Unknown element '%s' in 'taxilight' sector.", name);
			return false;
		}
	}

	if(!bColour){
		SvErr(fn, "'taxilight' sector is missing 'colour' element.");
		return false;
	}
	if(!bPosition){
		SvErr(fn, "'taxilight' sector is missing 'position' vector.");
		return false;
	}
	return true;
}

// ===========================================================================
// CVehicleHandler vehicle-archive scan cluster — vehmod_retail.dll 0.98e
// (missing33/decomp-chain.txt, depth-0 roots 0x4f4d0/0x4f5a0/0x4f680/
// 0x4f9a0/0x4fc90 + dumped depth-1 callees 0x4f380/0x4f440/0x4f4a0/0x449e0/
// 0x44a10/0x44c90/0x45110/0x0cac0/0x4f870). Log strings verbatim; engine
// internals reuse the archive lane's vehicle_archive.h seams. Shared .data
// lives in engine_globals (AI-class / livery / model-lookup tables); the
// scan-owned globals are class statics (engine_globals.h §7 pattern).
// ===========================================================================

// Lane-replaced .data (see vehicle_handler.h for the Ghidra-name census).
CMvlVehicle *CVehicleHandler::s_vehRegistry[kVehRegistryCapacity] = { 0 };
                                                          // DAT_100b2838 @ RVA 0xb2838
int  CVehicleHandler::s_nNextFreeCursor     = 0;           // DAT_100b2834 @ RVA 0xb2834
int  CVehicleHandler::s_nAvailableSlots     = 0;           // DAT_100a6d50 @ RVA 0xa6d50
int  CVehicleHandler::s_nSlotLimit          = 0;           // DAT_101244cc @ RVA 0x1244cc
int  CVehicleHandler::s_nLoadedArchiveCount = 0;           // DAT_100b51d4 @ RVA 0xb51d4
unsigned char CVehicleHandler::s_bSaTxdAttempted = 0;      // DAT_101247d4 @ RVA 0x1247d4
void *CVehicleHandler::s_pSaTexDict         = 0;           // DAT_100b51d0 @ RVA 0xb51d0

// ===========================================================================
// FUN_1004f440 @ RVA 0x4f440 — (provisional) ParseSlotIdFromFilename
// "x<id>_" prefix after the last '\' -> id (>= 1); -1 when absent/invalid.
// FUN_1005abd0 @ RVA 0x5abd0 = strrchr (same helper the path code uses).
// ===========================================================================
static long
ParseSlotIdFromFilename(const char *path)
{
	const char *start = path;
	long id;
	char *end = 0;

	if(!path)
		return -1;
	{
		const char *slash = strrchr(path, '\\');   // FUN_1005abd0 @ 0x5abd0
		if(slash)
			start = slash + 1;
	}
	if(start[0] != 'x')
		return -1;
	id = strtol(start + 1, &end, 10);              // base 10 (decomp)
	if(id < 1 || end == 0 || end[0] != '_')
		return -1;
	return id;
}

// ===========================================================================
// CVehicleHandler::RegistryVehicle — 0.98e @ RVA 0x4f380
// Dumped body: if (0 <= slot < DAT_100a6d50) return table[slot]; else NULL.
// ===========================================================================
CMvlVehicle *
CVehicleHandler::RegistryVehicle(int relSlot)
{
	// BUGFIX(stable-1.0): bound by BOTH the configured count and the real
	// table capacity — the config lane seeds s_nAvailableSlots and could in
	// principle exceed kVehRegistryCapacity, which would turn this read into
	// an out-of-bounds fetch.
	if(relSlot >= 0 && relSlot < s_nAvailableSlots &&          // DAT_100a6d50
	   relSlot < kVehRegistryCapacity)
		return s_vehRegistry[relSlot];                // DAT_100b2838
	return 0;
}

// ===========================================================================
// FUN_1004f4a0 @ RVA 0x4f4a0 — (provisional) FindFirstFreeSlot
// Scan [s_nNextFreeCursor, s_nAvailableSlots) for an empty registry slot;
// -1 when full. (Ghidra lost the loop-index moves as extraout_ECX/EDX; the
// intent — return the first free index — is unambiguous.)
// ===========================================================================
static int
FindFirstFreeSlot(void)
{
	int i = CVehicleHandler::s_nNextFreeCursor;      // DAT_100b2834
	while(i < CVehicleHandler::s_nAvailableSlots){   // DAT_100a6d50
		if(CVehicleHandler::RegistryVehicle(i) == 0)
			return i;
		i++;
	}
	return -1;
}

// ===========================================================================
// FUN_100449e0 @ RVA 0x449e0 + FUN_10044a10 @ RVA 0x44a10 — (provisional)
// ApplyArchivedVehicleDefaults. The original mallocs 0x2d2c + memzero
// (FUN_100449e0) then runs the deep CMvlVehicle ctor (FUN_10044a10: game
// info-object alloc via FUN_100449b0 @ 0x449b0 — body NOT in dump; default
// colours; rotor values; +0x2d20 = slot). The clone's CMvlVehicle::
// CMvlVehicle() (archive lane) already memzeros + sets m_nCompRules/-1 and
// m_nAnimsIndex/-1; this helper adds the FUN_10044a10 stores that map onto
// declared CMvlVehicle members. NOT ported here (see comments inline):
//   - m_pInfo allocation + its ctor defaults (info @ 0x174 bytes, vtable;
//     FUN_100449b0 body not in the dump) -> m_pInfo stays NULL (the archive
//     pipeline guards every m_pInfo deref),
//   - the slot store at +0x2d20 (thread via GetNewSlotForArchive's slotOut
//     until the field is modelled on CMvlVehicle),
//   - the 0x40000000 (2.0f) dword at +0x2d28 (the clone models +0x2d28 as
//     m_pColModelDerived — float-vs-pointer mismatch flagged, left zeroed).
// ===========================================================================
static void
ApplyArchivedVehicleDefaults(CMvlVehicle *veh)
{
	unsigned white = MvlPackRgb(255, 255, 255);     // FUN_10032f10 @ 0x32f10

	veh->m_nMaxPassengers   = 0;                    // +0x2bec
	veh->m_fRotorDamageMult = 1.0f;                 // +0x1e4 = 0x3f800000
	veh->m_fRotorRadius     = 6.2f;                 // +0x1e0 = 0x40c4cccd
	veh->m_nHeliWeapons     = 0;                    // +0x1dc
	veh->m_nHeliType        = 0;                    // +0x1d8
	// +0x27f4 = 0 (emlights dword) — covered by the ctor memset.
	veh->m_szArchivePath[0] = 0;                    // +0x22c
	veh->m_audio.bHasLightBox = 1;                  // +0x2bcc
	veh->m_nColourCount     = 1;                    // +0x2cf0
	veh->m_anColours[0][0]  = white;                // +0x2bf0..0x2bfc = 4 white dwords
	veh->m_anColours[0][1]  = white;
	veh->m_anColours[0][2]  = white;
	veh->m_anColours[0][3]  = white;
	veh->m_nGang            = -1;                   // +0x2cf4
}

// ===========================================================================
// CVehicleHandler::GetNewSlotForArchive — 0.98e @ RVA 0x4f4d0
// ===========================================================================
CMvlVehicle *
CVehicleHandler::GetNewSlotForArchive(const char *archivePath, int *slotOut)
{
	static const char *const fn = "CVehicleHandler::GetNewSlotForArchive";
	long parsed = ParseSlotIdFromFilename(archivePath);   // FUN_1004f440 @ 0x4f440
	int slot = (int)parsed - MvlModelIndexBaseOffset;     // DAT_100a6d48

	if(parsed != -1){
		if(parsed < MvlModelIndexBaseOffset || s_nSlotLimit <= parsed){   // DAT_101244cc
			SvWarn(fn, "Slot out of range, taking first free ID.");
			slot = FindFirstFreeSlot();               // FUN_1004f4a0 @ 0x4f4a0
		}
		else if(RegistryVehicle(slot) != 0){          // FUN_1004f380 @ 0x4f380
			SvWarn(fn, "Slot requested by file already in use, taking first free ID.");
			slot = FindFirstFreeSlot();
		}
		// parsed in range + free: keep the requested relative slot (original
		// goto LAB_1004f53f skips the first-free call).
	}
	else{
		slot = FindFirstFreeSlot();                   // no "x<id>_" -> first free
	}

	if(slot == -1)
		return 0;
	// BUGFIX(stable-1.0): the write below indexes s_vehRegistry directly —
	// clamp against the real table capacity, not just the configured count
	// (slot can only arrive in-range via the paths above, but this is the
	// one store that corrupts memory if that ever regresses).
	if(slot < 0 || slot >= kVehRegistryCapacity)
		return 0;
	if(slot == s_nNextFreeCursor)                    // DAT_100b2834
		s_nNextFreeCursor++;

	CMvlVehicle *veh = new CMvlVehicle;              // FUN_100449e0 @ 0x449e0 (malloc+zero; ctor above)
	ApplyArchivedVehicleDefaults(veh);               // FUN_10044a10 @ 0x44a10 subset (see above)
	s_vehRegistry[slot] = veh;                       // *(DAT_100b2838 + slot*4) = veh
	if(slotOut)
		*slotOut = slot;                             // original stores the slot at veh+0x2d20
	return veh;
}

// Public wrapper (original takes the single path argument).
CMvlVehicle *
CVehicleHandler::GetNewSlotForArchive(const char *archivePath)
{
	return GetNewSlotForArchive(archivePath, 0);
}

// ===========================================================================
// FUN_1000cac0 @ RVA 0x0cac0 — (provisional) MenuAddVehicleEntry
// Adds "<id>: <name>" to MVL's own Win32 menu: one submenu per 20 consecutive
// slots titled "<first>-<last>", item ids = slot + 10000. Menu handles are
// the original's .data DAT_100b0bc4 (main) / DAT_100b0bc8 (group array; the
// original's array bound is unrecovered — 25 groups cover slots 0..499).
// ===========================================================================
static HMENU s_hMainVehicleMenu;                  // DAT_100b0bc4 @ RVA 0xb0bc4
static HMENU s_ahVehicleMenuGroup[25];            // DAT_100b0bc8 @ RVA 0xb0bc8

static void
MenuAddVehicleEntry(unsigned slot, const char *label)
{
	int group, base, g;

	if(slot >= 500)
		return;                                    // original range guard
	group = (int)slot / 0x14;
	if(s_hMainVehicleMenu == 0)
		s_hMainVehicleMenu = CreateMenu();
	if(s_ahVehicleMenuGroup[group] == 0 && group >= 0){
		base = 0;
		for(g = 0; g <= group; g++){
			if(s_ahVehicleMenuGroup[g] == 0){
				char title[64];
				int first = MvlModelIndexBaseOffset + base;   // DAT_100a6d48
				s_ahVehicleMenuGroup[g] = CreateMenu();
				sprintf_s(title, "%d-%d", first, first + 0x13);
				AppendMenuA(s_hMainVehicleMenu, 0x10 /*MF_POPUP*/,
				            (UINT_PTR)s_ahVehicleMenuGroup[g], title);
			}
			base += 0x14;
		}
	}
	AppendMenuA(s_ahVehicleMenuGroup[group], 0 /*MF_STRING*/, slot + 10000, label);
}

// ===========================================================================
// CVehicleHandler::LoadVehicleArchive — 0.98e @ RVA 0x4f5a0
// ===========================================================================
bool
CVehicleHandler::LoadVehicleArchive(const char *archivePath)
{
	static const char *const fn = "CVehicleHandler::LoadVehicleArchive";
	int slot = 0;
	char label[268];   // decomp local_10c [268]

	CMvlVehicle *veh = GetNewSlotForArchive(archivePath, &slot);   // FUN_1004f4d0 @ 0x4f4d0
	if(veh == 0){
		// '%s' operand dropped by Ghidra; reconstructed as the archive path.
		SvErr(fn, "Could not find a slot for vehicle file '%s', all slots full.",
		      archivePath ? archivePath : "");
		return false;
	}

	int id = slot + MvlModelIndexBaseOffset;   // this[0xb48] + DAT_100a6d48
	veh->LoadFromArchive(archivePath ? archivePath : "", id);   // FUN_10045110 @ 0x45110 (archive lane)

	// ORIGINAL: cVar1 = FUN_10045110(...); if (cVar1 == '\0') { log "Could
	// not load vehicle archive '%s'."; FUN_10044c90 dtor; FUN_100073c0 free;
	// return }. The clone's CMvlVehicle::LoadFromArchive returns void with an
	// internal catch (archive lane), so the AL status cannot be sensed yet.
	// TODO(archive-lane): give 0x45110 a status return (the original's AL)
	// and rewire the branch below; until then a load is assumed successful
	// (the failure structure is ported, not faked).
	bool loadOk = true;
	if(!loadOk){
		SvErr(fn, "Could not load vehicle archive '%s'.",
		      archivePath ? archivePath : "");
		delete veh;                       // FUN_10044c90 @ 0x44c90 dtor + FUN_100073c0 @ 0x73c0 free
		s_vehRegistry[slot] = 0;          // FUN_1004f410 @ 0x4f410 unregister (body NOT in dump;
		                                  // observable effect on this path = registry clear)
		return false;
	}

	// decomp: _sprintf(buf, "%d: %ls", *this, this + 0x7b) — id + wide name
	// (CMvlVehicle::m_wsName @ 0x1ec = +0x7b dwords), then the menu append.
	sprintf_s(label, "%d: %ls", veh->m_nId, veh->m_wsName);
	MenuAddVehicleEntry((unsigned)slot, label);   // FUN_1000cac0 @ 0x0cac0
	s_nLoadedArchiveCount++;                      // DAT_100b51d4++
	return true;
}

// ===========================================================================
// gta-vc.exe @ 0x9785a8 — per-AI-class "custom cars specified" counters
// (DAT_009785a8 @ gta-vc.exe 0x9785a8; read + incremented by the InitVehicles
// tail). GAME address — accessed via an address constant (engine_globals.h
// header rule); the clone reads the DLL-side AI table FIRST (short-circuit)
// so harness runs never touch game memory until that table is live. Same
// boolean result as the decomp's operand order (both reads are pure).
// ===========================================================================
static int *const MvlGameClassCustomCars = (int *)0x9785a8;   // [16], stride 4

// ===========================================================================
// CVehicleHandler::LoadVehicleArchives — 0.98e @ RVA 0x4f680
// ===========================================================================
void
CVehicleHandler::LoadVehicleArchives(void)
{
	static const char *const fn = "CVehicleHandler::LoadVehicleArchives";
	int pass;

	for(pass = 0; pass < 2; pass++){
		WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA("mvl\\vehicles\\*.7z", &fd);
		if(h != INVALID_HANDLE_VALUE){
			do{
				if(fd.dwFileAttributes & 0x10 /*FILE_ATTRIBUTE_DIRECTORY*/)
					continue;   // skip directories (the decomp renders the
				                // original's `continue` as an empty do-while)
				long slotId = ParseSlotIdFromFilename(fd.cFileName);   // FUN_1004f440
				// pass 0 loads explicit "x<id>_" archives first, pass 1 the rest
				// (decomp: (lVar2 != -1) == (iVar4 == 0)).
				if((slotId != -1) == (pass == 0)){
					char path[0x104 + 4];
					// BUGFIX(stable-1.0): cFileName can be up to MAX_PATH-1
					// (259) chars; "mvl\vehicles\" + that overflows the 0x108
					// buffer and aborts inside sprintf_s (the original used
					// unbounded sprintf — a stack smash). Truncate instead:
					// a too-long path simply won't exist and the load fails
					// with the normal error below.
					// MSVC secure-CRT name is _snprintf_s (snprintf_s is a
					// C99 Annex-K alias not exposed in this TU's CRT mode).
					_snprintf_s(path, sizeof(path), _TRUNCATE,
					            "mvl\\vehicles\\%s", fd.cFileName);
					if(!LoadVehicleArchive(path))
						// verbatim (the original message ends with '\n')
						SvErr(fn, "Could not load vehicle from '%s'.\n", path);
				}
			}while(FindNextFileA(h, &fd));
			FindClose(h);
		}
	}

	// "InitVehicles" tail (verbatim context string, NOT the function name).
	// 11 AI classes (iVar5 0..0x426 stride 0x61 over DAT_100b51d8 records;
	// class index i = 0..10).
	for(int i = 0; i < 11; i++){
		int rec = i * 0x61;                              // DAT_100b51d8 record base
		int defId = MvlAiClassSlotTable[i];              // DAT_100b2804 (engine_globals)
		// decomp order: game counter == 0 && DLL default != 0; the clone
		// tests the DLL side first (see MvlGameClassCustomCars note above).
		if(defId == 0 || MvlGameClassCustomCars[i] != 0) // DAT_009785a8 @ gta-vc.exe 0x9785a8
			continue;
		MvlAiClassSlotList[rec] = defId;                 // DAT_100b51d8[rec]
		MvlGameClassCustomCars[i] += 1;
		if((unsigned)(defId - 0x82) > 0x6d){             // decomp range check [0x82, 0xEF]
			// ORIGINAL: ___report_rangecheckfailure() (CRT abort). The clone
			// logs + skips the out-of-bounds table write instead of killing
			// the process — RECONSTRUCTED deviation.
			SvErr("InitVehicles", "Default car ID %d for class %d out of range [0x82..0xEF].",
			      defId, i);
			continue;
		}
		MvlKeyClassBypassTable[defId] = 0;               // DAT_100b641e (clear NOSPAWN bypass;
		                                                 // engine_globals declares [0xED],
		                                                 // the original writes up to +0xEF —
		                                                 // declared size is provisional)
		{
			// '%s'/'%d' operands dropped by Ghidra: '%d' = defId (verbatim
			// "using default car with ID %d"); '%s' RECONSTRUCTED as the
			// class index string (no class-name table recovered).
			char cls[16];
			sprintf_s(cls, "%d", i);
			SvWarn("InitVehicles",
			       "Class '%s' has all default vehicles with NOSPAWN, yet no custom cars specified for this class - using default car with ID %d",
			       cls, defId);
		}
	}
}

// ===========================================================================
// FUN_1004f870 @ RVA 0x4f870 — (provisional) BuildSaVehicleTxdPath
// "mvl/vehicles/sa_vehicle.txd"; when missing, derive the SA install's
// models\generic\vehicle.txd from HKLM "…\\GTA San Andreas\\Installation"
// ExePath (truncate after the last '\', skip a leading quote). Empty string
// = no candidate (LoadSanAndreasVehiclesTxd no-ops).
// ===========================================================================
static void
BuildSaVehicleTxdPath(char *path)
{
	HKEY hKey = 0;

	strncpy_s(path, 0x104, "mvl/vehicles/sa_vehicle.txd", _TRUNCATE);
	if(GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES)
		return;

	path[0] = '\0';
	if(RegOpenKeyExA(HKEY_LOCAL_MACHINE,
	                 "Software\\Rockstar Games\\GTA San Andreas\\Installation",
	                 0, 0x20019 /*KEY_QUERY_VALUE|KEY_READ*/, &hKey) == 0){
		char buf[0x104];
		DWORD len = 0x103;
		buf[0] = '\0';
		if(RegQueryValueExA(hKey, "ExePath", 0, 0, (LPBYTE)buf, &len) == 0){
			// decomp writes the NUL via param_1 (Ghidra mixup); the buffer
			// being terminated is the RegQueryValueEx target.
			if(len < 0x104)
				buf[len] = '\0';
			else
				buf[0x103] = '\0';
			{
				char *slash = strrchr(buf, '\\');   // FUN_1005abd0 @ 0x5abd0
				if(slash)
					slash[1] = '\0';               // keep the trailing '\'
			}
			strncat_s(buf, 0x104, "models\\generic\\vehicle.txd", _TRUNCATE);
			{
				const char *src = (buf[0] == '\"') ? buf + 1 : buf;
				strncpy_s(path, 0x104, src, _TRUNCATE);
			}
			GetFileAttributesA(path);   // existence probe (result unused, original)
		}
		RegCloseKey(hKey);
	}
}

// ===========================================================================
// CVehicleHandler::LoadSanAndreasVehiclesTxd — 0.98e @ RVA 0x4f9a0
// ===========================================================================
void
CVehicleHandler::LoadSanAndreasVehiclesTxd(void)
{
	static const char *const fn = "CVehicleHandler::LoadSanAndreasVehiclesTxd";

	if(s_bSaTxdAttempted)                     // DAT_101247d4 one-shot latch
		return;
	s_bSaTxdAttempted = 1;

	char path[0x104];
	BuildSaVehicleTxdPath(path);              // FUN_1004f870 @ 0x4f870
	if(path[0] == '\0')
		return;

	HANDLE h = CreateFileA(path, 0x80000000 /*GENERIC_READ*/, 1 /*SHARE_READ*/,
	                       0, 3 /*OPEN_EXISTING*/, 0, 0);
	if(h == INVALID_HANDLE_VALUE){
		GetLastError();   // decomp calls and discards
		// '%s' operand dropped by Ghidra; reconstructed as the resolved path.
		SvErr(fn, "Could not open texture file '%s'", path);
		return;
	}

	DWORD sizeHi = 0;
	DWORD size = GetFileSize(h, &sizeHi);
	if(size < 0x400001 && sizeHi == 0){       // decomp: size <= 4MB and high dword == 0
		void *buf = malloc(size);
		// BUGFIX(stable-1.0): malloc can fail (fragmented 32-bit VA space
		// with the 4MB cap still reachable); ReadFile into NULL would fail
		// or crash. Bail with the read-error message.
		if(!buf){
			CloseHandle(h);
			SvErr(fn, "Could not read texture file.");
			return;
		}
		DWORD read = 0;
		if(ReadFile(h, buf, size, &read, 0)){
			CloseHandle(h);                   // closed before the stream work (original)
			if(!MvlRecognizeTextureData(buf, size)){   // FUN_1000d340 @ 0xd340
				SvErr(fn, "Failed to recognize texture.");
				free(buf);
				return;
			}
			void *stream = MvlRwMemoryStreamOpen(buf, size);   // FUN_100320a0 @ 0x320a0
			if(!stream){
				SvErr(fn, "Could not open memory stream for texture file.");
				free(buf);
				return;
			}
			if(!MvlRwStreamFindChunkHeader(stream)){          // FUN_10032770 @ 0x32770
				SvErr(fn, "Could not find texture dictionary chunk header from .txd file.");
				MvlRwMemoryStreamClose(stream);               // FUN_10032090 @ 0x32090
				free(buf);
				return;
			}
			s_pSaTexDict = MvlRwReadTextureDictionary(stream);   // DAT_100b51d0 = FUN_10031290 @ 0x31290
			MvlRwMemoryStreamClose(stream);
			free(buf);
			if(s_pSaTexDict == 0){
				SvErr(fn, "Loading texture dictionary from .txd file failed.");
				return;
			}
			MvlPrimaryLiveryColor    = MvlRwTexDictCurrentColor();   // DAT_100b6498 = FUN_10032740 @ 0x32740
			MvlAlternateLiveryColor  = MvlRwTexDictCurrentColor();   // DAT_100b649c = FUN_10032740
			SvMsg("Successfully loaded San Andreas vehicle texture dictionary.");
			return;
		}
		free(buf);
		SvErr(fn, "Could not read texture file.");
	}
	else{
		SvErr(fn, "Too big texture file (over 4MB).");
	}
	CloseHandle(h);   // failure paths only (success closed early, original)
}

// ===========================================================================
// CVehicleHandler::AnalyzeStartIndex — 0.98e @ RVA 0x4fc90
// ===========================================================================

// Model-slot occupancy probe: *(DAT_100a6314 + idx*4) — the model-lookup
// registry head (engine_globals MvlModelLookupRegistry). A NULL registry
// (harness / before RegisterModelForLookup wires it) = every slot reads as
// unoccupied, which keeps AnalyzeStartIndex's arithmetic well-defined.
static bool
ModelSlotOccupied(int idx)
{
	return MvlModelLookupRegistry != 0 && MvlModelLookupRegistry[idx] != 0;
}

void
CVehicleHandler::AnalyzeStartIndex(void)
{
	static const char *const fn = "CVehicleHandler::AnalyzeStartIndex";
	int start = MvlModelIndexBaseOffset;   // DAT_100a6d48
	int limit = MvlModelIdBoundary;        // DAT_100a6320 ("TXD base index" bound)
	int i = start;
	int leadingFree = 0;

	// Contiguous free run from the current base (max 500 samples).
	if(start < limit){
		for(leadingFree = 0; i < limit && leadingFree <= 499; i++, leadingFree++){
			if(ModelSlotOccupied(i))
				break;
		}
	}
	leadingFree = i - start;

	// Biggest free gap in [0x82, limit] (limit treated as an occupied
	// sentinel closing any open run — decomp bVar5).
	int bestStart = -1, bestGap = 0;
	if(leadingFree < 500 && limit > 0x81){
		int runStart = -1;
		for(int idx = 0x82; idx <= limit; idx++){
			if(idx == limit || ModelSlotOccupied(idx)){
				if(runStart >= 0 && bestGap < idx - runStart){
					bestGap = idx - runStart;
					bestStart = runStart;
				}
				runStart = -1;
			}
			else if(runStart == -1){
				runStart = idx;
			}
		}
	}

	if(leadingFree < 100){
		if(leadingFree < bestGap){
			SvWarn(fn, "Current start ID gives only %d free slots, switching base to %d which gives %d slots.",
			       leadingFree, bestStart, bestGap);
			MvlModelIndexBaseOffset = bestStart;   // DAT_100a6d48
			s_nAvailableSlots = bestGap;          // DAT_100a6d50
			if(bestGap > 500)
				s_nAvailableSlots = 500;
		}
		else{
			SvWarn(fn, "Current start ID gives only %d free slots and there aren't any better ID ranges available.",
			       leadingFree);
			s_nAvailableSlots = leadingFree;
		}
	}
	else if(leadingFree < 500){
		// warn-only branch (the original adjusts NOTHING here — DAT_100a6d50
		// keeps whatever the config lane seeded)
		if(leadingFree < bestGap)
			SvWarn(fn, "Current start ID gives only %d free slots, suggesting start index %d which gives %d.",
			       leadingFree, bestStart, bestGap);
		else
			SvWarn(fn, "Current start ID gives only %d free slots and there aren't any better ID ranges available.",
			       leadingFree);
	}
	s_nSlotLimit = s_nAvailableSlots + MvlModelIndexBaseOffset;   // DAT_101244cc
}

// ===========================================================================
// MvlRwTexDictCurrentColor — hook-point stub (vehicle_handler.h decl).
// FUN_10032740 @ 0x32740 -> game thunk gta-vc.exe 0x64e060; the engine lane
// binds the real call (returns the next livery colour of the loaded dict).
// ===========================================================================
int MvlRwTexDictCurrentColor(void)
{
	MvlErrorLog("HOOK POINT 0x32740 MvlRwTexDictCurrentColor (game 0x64e060)");
	return 0;
}
