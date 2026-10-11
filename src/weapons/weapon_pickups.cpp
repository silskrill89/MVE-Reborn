// weapon_pickups.cpp — CWeaponPickups implementation (see weapon_pickups.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (savewpn/range-decomp.txt
// 0x57d80-0x58990). Log strings verbatim; game pool / SCM stream internals are
// HOOK POINT stubs (MvlErrorLog + target VA).
#include "weapon_pickups.h"

#include "../archive/vehicle_archive.h"   // MvlParseVector / MvlVectorZero
#include "../loader/log.h"
#include "weapon_handler.h"               // CWeaponHandler::IsValidWeaponId

#include "tinyxml.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrappers (same mapping as the other lanes).
// ---------------------------------------------------------------------------
static void
PMsg(const char *fmt, ...)
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
PErr(const char *fn, const char *fmt, ...)
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

// Spawn list (original MSVC vector @ DAT_10124500..0x10124504).
std::vector<SMvlPickupSpawn> CWeaponPickups::s_spawnList;

// ===========================================================================
// FUN_10058ac0 @ RVA 0x58ac0 — (reconstructed from the consumers; the body
// sits just past the dump boundary 0x58a00) append a spawn record.
// ===========================================================================
void
CWeaponPickups::AppendSpawn(const SMvlPickupSpawn &spawn)
{
	s_spawnList.push_back(spawn);
}

// ===========================================================================
// CWeaponPickups::LoadXMLSectionSpawn — 0x98e @ RVA 0x586d0
// ===========================================================================
bool
CWeaponPickups::LoadXMLSectionSpawn(TiXmlElement *sector, int weaponId)
{
	static const char *const fn = "CWeaponPickups::LoadXMLSectionSpawn";
	int wepid = 0, ammo = 0;
	float pos[3] = { 0.0f, 0.0f, 0.0f };
	bool bWepid = false, bPos = false, bAmmo = false;

	MvlVectorZero(pos);   // FUN_10008c30 @ 0x8c30

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t = e->GetText();
		if(_stricmp(name, "wepid") == 0){
			if(!t || sscanf(t, "%i", &wepid) != 1)
				return PErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
			bWepid = true;
		}
		else if(_stricmp(name, "position") == 0){
			if(!MvlParseVector(e, pos))
				return PErr(fn, "Could not load '%s' vector.", "position"), false;
			bPos = true;
		}
		else if(_stricmp(name, "ammo") == 0){
			if(!t || sscanf(t, "%i", &ammo) != 1)
				return PErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
			bAmmo = true;
		}
		// unknown spawn element: ignored (table miss)
	}
	// required-check (FUN_1001bbb0): all three entries required.
	if(!bWepid)
		return PErr(fn, "'%s' sector is missing '%s' element.", "spawn", "wepid"),
		       false;
	if(!bPos)
		return PErr(fn, "'%s' sector is missing '%s' vector.", "spawn", "position"),
		       false;
	if(!bAmmo)
		return PErr(fn, "'%s' sector is missing '%s' element.", "spawn", "ammo"),
		       false;

	// decomp tail: weapon-embedded lists (weaponId > 0) force the id;
	// pickups.xml (weaponId < 0) uses the parsed wepid.
	int id = wepid;
	if(weaponId > 0)
		id = weaponId;
	else{
		if(id < 0 || (id > MVL_STOCK_WEAPON_ID_MAX &&
		              CWeaponHandler::GetWeaponById(id) == 0)){
			PErr(fn, "Invalid weapon ID %d in spawn info", id);
			return false;
		}
		if(ammo <= 0){
			PErr(fn, "Ammo for weapon ID %d is not positive.", id);
			return false;
		}
	}
	SMvlPickupSpawn s;
	s.nWeaponId = id;
	s.fPosition[0] = pos[0];
	s.fPosition[1] = pos[1];
	s.fPosition[2] = pos[2];
	s.nAmmo = ammo;
	AppendSpawn(s);
	return true;
}

// ===========================================================================
// CWeaponPickups::LoadXMLSectionSpawnList — 0x98e @ RVA 0x58610
// ===========================================================================
bool
CWeaponPickups::LoadXMLSectionSpawnList(TiXmlElement *sector, int weaponId)
{
	static const char *const fn = "CWeaponPickups::LoadXMLSectionSpawnList";

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		if(_stricmp(e->Value(), "spawn") != 0)
			continue;   // unknown spawnlist element: ignored (table miss)
		if(!LoadXMLSectionSpawn(e, weaponId))
			PErr(fn, "Could not load %s section.", "spawn");
		// original logs and continues
	}
	return true;
}

// ===========================================================================
// CWeaponPickups::LoadXMLSectionPickups — 0x98e @ RVA 0x58880
// ===========================================================================
bool
CWeaponPickups::LoadXMLSectionPickups(TiXmlElement *sector)
{
	static const char *const fn = "CWeaponPickups::LoadXMLSectionPickups";

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		if(_stricmp(e->Value(), "spawnlist") != 0)
			continue;   // unknown pickups element: ignored (table miss)
		if(!LoadXMLSectionSpawnList(e, -1))
			PErr(fn, "Could not load '%s' sector.", "spawnlist");
		// original logs and continues
	}
	// required-check: spawnlist entry present (FUN_1001bbb0).
	return true;
}

// ===========================================================================
// CWeaponPickups::LoadXMLPickupFile — 0x98e @ RVA 0x58990
// ===========================================================================
bool
CWeaponPickups::LoadXMLPickupFile(void)
{
	static const char *const fn = "CWeaponPickups::LoadXMLPickupFile";

	if(GetFileAttributesA("mvl\\weapons\\pickups.xml") == INVALID_FILE_ATTRIBUTES)
		return false;   // optional file: silently absent (original)

	TiXmlDocument doc("mvl\\weapons\\pickups.xml");
	doc.LoadFile();
	if(doc.Error()){
		PErr(fn, "XML file could not be parsed: %s.", doc.ErrorDesc());
		return false;
	}
	TiXmlElement *root = doc.FirstChildElement();
	if(!root){
		PErr(fn, "No root element in XML.");
		return false;
	}
	if(root->Type() != TiXmlNode::TINYXML_ELEMENT ||
	   _stricmp(root->Value(), "pickups") != 0){
		PErr(fn, "Root element has the wrong name.");
		return false;
	}
	bool ok = LoadXMLSectionPickups(root);
	if(ok)
		PMsg("Successfully loaded weapon pickup XML file.");
	return ok;
}

// ===========================================================================
// FUN_10058330 @ RVA 0x58330 — (provisional) CreateAllRegisteredPickups.
// ===========================================================================
void
CWeaponPickups::CreateAllRegisteredPickups(void)
{
	PMsg("%d pickups registered, now creating them.", (int)s_spawnList.size());
	for(size_t i = 0; i < s_spawnList.size(); i++){
		const SMvlPickupSpawn &s = s_spawnList[i];
		// position is passed on the stack in the original (FUN_10057d80
		// copies it into the record via FUN_10008c90); hook carries it.
		MvlPickupSetNextPosition(s.fPosition);
		CreateWeaponPickup(s.nWeaponId, s.nAmmo);
	}
}

// ===========================================================================
// CWeaponPickups::CreateWeaponPickup — 0x98e @ RVA 0x57d80
// ===========================================================================
int
CWeaponPickups::CreateWeaponPickup(int weaponId, int ammo)
{
	static const char *const fn = "CWeaponPickups::CreateWeaponPickup";

	int total = 0, used = 0;
	MvlPickupPoolStats(&total, &used);   // TODO(weap): 0x94dbe0 pool walk
	if(total - used <= 1){
		PErr(fn, "Game object pool %d/%d full, not creating pickup.", used, total);
		return -1;
	}

	int slot = MvlPickupFindSlot();      // FUN_10057bb0 @ 0x57bb0 lineage
	if(slot < 0){
		PErr(fn, "Didn't find an empty slot for pickup.");
		return -1;
	}

	MvlPickupAudioPrepare();             // FUN_10033730 @ 0x33730
	// validation: id >= 0 and (stock id or registered custom)
	if(!(weaponId >= 0 &&
	     (weaponId <= MVL_STOCK_WEAPON_ID_MAX ||
	      CWeaponHandler::GetWeaponById(weaponId) != 0))){
		PErr(fn, "Invalid weapon ID for pickup.");
		return -1;
	}

	void *rec = MvlPickupRecord(slot);   // TODO(weap): 0xa6b98 + slot*0x34
	if(rec)
		MvlPickupSetupRecord(rec, weaponId, ammo);   // field fills @ 0x57d80
	MvlPickupSetupObject(rec, weaponId); // FUN_10057c30 @ 0x57c30
	// destroy placeholder objects at +0x10/+0x14 when present (decomp) —
	// part of the 0x57c30 object setup; HOOK.
	int counter = MvlPickupBumpCounter(slot);   // TODO(weap): +0x24 counter
	return (counter << 10) | slot;
}

// ===========================================================================
// CWeaponPickups::ProcessNewWeaponPickupOpcode — 0x98e @ RVA 0x584f0
// Opcode 0810: arg0 = weapon id, arg4 = ammo, then args 3,2,1 skipped; the
// result handle is written back to arg0.
// ===========================================================================
void
CWeaponPickups::ProcessNewWeaponPickupOpcode(void)
{
	static const char *const fn = "CWeaponPickups::ProcessNewWeaponPickupOpcode";

	MvlScmOpcodeBegin();                          // FUN_1002e4c0
	int weaponId = MvlScmOpcodeReadInt(0);        // FUN_1002e530(0)
	int ammo     = MvlScmOpcodeReadInt(4);        // FUN_1002e530(4)
	MvlScmOpcodeSkip(3);                          // FUN_1002e540 x3
	MvlScmOpcodeSkip(2);
	MvlScmOpcodeSkip(1);

	float pos[3] = { 0.0f, 0.0f, 0.0f };
	MvlVectorZero(pos);                           // FUN_10008c50 @ 0x8c50
	MvlPlayerGetPosition(pos);                    // FUN_1000b110 — ground z
	if(pos[2] <= MVL_PICKUP_MIN_Z)                // DAT_1008dfc0
		pos[2] += 1.0f;                           // DAT_1008de00 (spawn height)

	if(!CWeaponHandler::IsValidWeaponId(weaponId)){
		PErr(fn, "Invalid weapon ID in new weapon pickup opcode.");
		return;
	}
	if(ammo < 1)
		ammo = 1;
	else if(ammo > 1000000)
		ammo = 1000000;

	MvlPickupSetNextPosition(pos);
	int handle = CreateWeaponPickup(weaponId, ammo);
	MvlScmOpcodeWriteInt(0, handle);              // FUN_1002e560(0, handle)
	MvlScmOpcodeEnd();                            // FUN_1002e4b0
}

// ===========================================================================
// Hook stubs. HARNESS STUBS: log + benign values; the engine lane replaces
// them (target VAs in weapon_pickups.h comments). Nothing is faked.
// ===========================================================================
static float s_fNextPosition[3] = { 0.0f, 0.0f, 0.0f };
static int   s_nPickupCounter = 0;

void MvlPickupSetNextPosition(const float p[3])
{
	if(p)
		memcpy(s_fNextPosition, p, sizeof(s_fNextPosition));
}

const float *MvlPickupGetNextPosition(void)
{
	return s_fNextPosition;
}

int MvlPickupBumpCounter(int slot)
{
	// the original increments the record's +0x24 counter and returns it;
	// stubbed here as a local counter (records are engine memory).
	(void)slot;
	return ++s_nPickupCounter;
}

void MvlPickupPoolStats(int *total, int *used)
{
	MvlErrorLog("TODO(weap): 0x94dbe0 MvlPickupPoolStats (gta-vc.exe object pool)");
	if(total) *total = 0;
	if(used)  *used = 0;
}

int MvlPickupFindSlot(void)
{
	MvlErrorLog("TODO(weap): 0x57b30/0x57b60 MvlPickupFindSlot");
	return -1;
}

void *MvlPickupRecord(int slot)
{
	MvlErrorLog("TODO(weap): 0xa6b98 MvlPickupRecord (slot %d)", slot);
	return 0;
}

void MvlPickupSetupRecord(void *record, int weaponId, int ammo)
{
	// field fills from FUN_10057d80: +0x18 ammo, +0x20=0, +0x30=0, +0xc=0,
	// +0x2e=2, +0x1c=-1, +0x26=0, +0x22=-weaponId (short), +0x10=+0x14=0,
	// position copied from the next-position scratch.
	MvlErrorLog("TODO(weap): 0x57d80 MvlPickupSetupRecord (record fields)");
	(void)record; (void)weaponId; (void)ammo;
}

void MvlPickupSetupObject(void *record, int weaponId)
{
	MvlErrorLog("TODO(weap): 0x57c30 MvlPickupSetupObject");
	(void)record; (void)weaponId;
}

void MvlPickupAudioPrepare(void)
{
	MvlErrorLog("TODO(weap): 0x33730 MvlPickupAudioPrepare");
}

void MvlScmOpcodeBegin(void)
{
	MvlErrorLog("TODO(weap): 0x2e4c0 MvlScmOpcodeBegin");
}

int MvlScmOpcodeReadInt(int)
{
	MvlErrorLog("TODO(weap): 0x2e530 MvlScmOpcodeReadInt");
	return 0;
}

void MvlScmOpcodeSkip(int)
{
	MvlErrorLog("TODO(weap): 0x2e540 MvlScmOpcodeSkip");
}

void MvlScmOpcodeWriteInt(int, int)
{
	MvlErrorLog("TODO(weap): 0x2e560 MvlScmOpcodeWriteInt");
}

void MvlScmOpcodeEnd(void)
{
	MvlErrorLog("TODO(weap): 0x2e4b0 MvlScmOpcodeEnd");
}

void MvlPlayerGetPosition(float out[3])
{
	MvlErrorLog("TODO(weap): 0xb110 MvlPlayerGetPosition");
	if(out)
		out[0] = out[1] = out[2] = 0.0f;
}

void MvlScmRegisterOpcode(int opcode, void (*handler)(void))
{
	MvlErrorLog("TODO(weap): 0x238a0 MvlScmRegisterOpcode (opcode %d)", opcode);
	(void)handler;
}
