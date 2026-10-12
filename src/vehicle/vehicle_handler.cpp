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

#include "tinyxml.h"   // vendored classic TinyXML (external/tinyxml)

#include <stdarg.h>
#include <stdio.h>
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
	if(len - 2 >= sizeof(sidecar))
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
