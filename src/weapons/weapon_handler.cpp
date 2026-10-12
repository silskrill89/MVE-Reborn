// weapon_handler.cpp — CWeaponHandler implementation (see weapon_handler.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (savewpn/range-decomp.txt).
// Log strings verbatim; engine internals are HOOK POINT stubs (MvlErrorLog +
// target VA). Includes tinyxml for the audio/save parsers.
#include "weapon_handler.h"

#include "../archive/vehicle_archive.h"   // shared RW/sound hooks + CSevenWalker deps
#include "../loader/log.h"
#include "weapon_pickups.h"

#include "tinyxml.h"

#include <new>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrappers (same mapping as the other lanes).
// ---------------------------------------------------------------------------
static void
HMsg(const char *fmt, ...)
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
HWarn(const char *fn, const char *fmt, ...)
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
HErr(const char *fn, const char *fmt, ...)
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
// FUN_10051db0 @ RVA 0x51db0 — save-XML sprintf helper. The original is
// _vsprintf_s(buf, 0x104, fmt, ...) into the 260-byte local_118 buffer; the
// varargs are invisible in the decomp (2-param prototype) and were recovered
// from the binary's push sites.
// ---------------------------------------------------------------------------
static void
SaveSprintf(char *buf, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vsprintf_s(buf, 0x104, fmt, ap);   // CRT name fix: MSVC declares vsprintf_s (original _vsprintf_s intent, 260-byte buffer)
	va_end(ap);
}

// ---------------------------------------------------------------------------
// Registry state (original globals; see weapon_handler.h for the map).
// ---------------------------------------------------------------------------
CMvlWeapon *CWeaponHandler::s_weapons[MVL_WEAPON_SLOT_COUNT] = { 0 };
int  CWeaponHandler::s_nMaxWeaponId = 0;
int  CWeaponHandler::s_anPlayerSaveWeapon[MVL_PLAYER_WEP_SAVE_SLOTS] = { 0 };
int  CWeaponHandler::s_anPlayerSaveAmmo[MVL_PLAYER_WEP_SAVE_SLOTS] = { 0 };
const char *CWeaponHandler::s_szSaveArchiveName = 0;
unsigned    CWeaponHandler::s_nSaveArchiveHash = 0;
int         CWeaponHandler::s_nSaveAmmo = 0;

// DumpWeaponXML audio skip-default distance (DAT_1008df04 == 120.0f).
static const float kShotAudioDefaultDistance = 120.0f;   // 0x42f00000

// ===========================================================================
// CWeaponHandler::LoadXMLSectionWeaponShotAudio — 0x98e @ RVA 0x54ae0
// Table rows: sample(u,req), volume(u,req), frequency(u), displacement(u),
// voldisplacement(u), distance(f), loopcount(u), nospam(t). Defaults:
// displacement=1, distance=120.0, loopcount=1, nospam=true. Post: volume
// clamp 0..127; voldisplacement clamp so volume+voldisp <= 127; loopcount
// forced 1 when sample==0.
// ===========================================================================
bool
CWeaponHandler::LoadXMLSectionWeaponShotAudio(TiXmlElement *sector, SMvlShotAudio *out)
{
	static const char *const fn = "CWeaponHandler::LoadXMLSectionWeaponShotAudio";
	unsigned sample = 0, volume = 0, freq = 0, disp = 1, voldisp = 0, loop = 1;
	float dist = kShotAudioDefaultDistance;
	bool nospam = true;
	bool bSample = false, bVolume = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t = e->GetText();
		if(_stricmp(name, "sample") == 0){
			if(bSample)
				return HErr(fn, "'%s' sector contains multiple '%s' elements.",
				            "shotaudio", name), false;
			if(!t || sscanf(t, "%u", &sample) != 1)
				return HErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
			bSample = true;
		}
		else if(_stricmp(name, "volume") == 0){
			if(bVolume)
				return HErr(fn, "'%s' sector contains multiple '%s' elements.",
				            "shotaudio", name), false;
			if(!t || sscanf(t, "%u", &volume) != 1)
				return HErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
			bVolume = true;
		}
		else if(_stricmp(name, "frequency") == 0){
			if(!t || sscanf(t, "%u", &freq) != 1)
				return HErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
		}
		else if(_stricmp(name, "displacement") == 0){
			if(!t || sscanf(t, "%u", &disp) != 1)
				return HErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
		}
		else if(_stricmp(name, "voldisplacement") == 0){
			if(!t || sscanf(t, "%u", &voldisp) != 1)
				return HErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
		}
		else if(_stricmp(name, "distance") == 0){
			if(!t || sscanf(t, "%f", &dist) != 1)
				return HErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
		}
		else if(_stricmp(name, "loopcount") == 0){
			if(!t || sscanf(t, "%u", &loop) != 1)
				return HErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
		}
		else if(_stricmp(name, "nospam") == 0){
			if(!t)
				return HErr(fn, "Failed to read data from '%s' element.", name), false;
			nospam = (_stricmp(t, "true") == 0);
		}
		// unknown shot-audio element: ignored (table miss)
	}
	// required-check: sample + volume (table +12=1).
	if(!bSample || !bVolume)
		return false;   // helper logs its own missing message (out of range)

	// decomp post-pass clamps.
	if(volume > 0x7f)
		volume = 0x7f;
	if(voldisp + volume > 0x7f)
		voldisp = 0x7f - volume;
	if(sample == 0)
		loop = 1;

	// output layout (decomp stores @ 0x54ae0; see SMvlShotAudio).
	out->nSample       = sample;
	out->nFrequency    = freq;
	out->nDisplacement = disp;
	out->fDistance     = dist;
	out->nLoopCount    = (unsigned char)loop;
	out->nVolume       = (unsigned char)volume;
	out->nVolDisplacement = (unsigned char)voldisp;
	out->bNoSpam       = nospam ? 1 : 0;
	return true;
}

// ===========================================================================
// CWeaponHandler::LoadXMLSectionWeaponAudio — 0x98e @ RVA 0x54910
// <audio>: gunshot → out[0], reload → out[1], echo → out[2]. gunshot and
// reload are required (table flags).
// ===========================================================================
bool
CWeaponHandler::LoadXMLSectionWeaponAudio(TiXmlElement *sector, SMvlShotAudio *out)
{
	static const char *const fn = "CWeaponHandler::LoadXMLSectionWeaponAudio";
	bool bGun = false, bReload = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "gunshot") == 0){
			if(!LoadXMLSectionWeaponShotAudio(e, &out[0])){
				HErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bGun = true;
		}
		else if(_stricmp(name, "reload") == 0){
			if(!LoadXMLSectionWeaponShotAudio(e, &out[1])){
				HErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bReload = true;
		}
		else if(_stricmp(name, "echo") == 0){
			// echo (dest &DAT_1008d2a8 name row) → out[2]; optional.
			if(!LoadXMLSectionWeaponShotAudio(e, &out[2])){
				HErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
		}
		// unknown audio element: ignored (table miss)
	}
	// required-check (FUN_1001bbb0): gunshot + reload.
	if(!bGun)
		return HErr(fn, "'%s' sector is missing '%s' element.", "audio", "gunshot"),
		       false;
	if(!bReload)
		return HErr(fn, "'%s' sector is missing '%s' element.", "audio", "reload"),
		       false;
	return true;
}

// ===========================================================================
// FUN_100550a0 @ RVA 0x550a0 — FindFirstFreeSlot (0..999, -1 when full).
// ===========================================================================
int
CWeaponHandler::FindFirstFreeSlot(void)
{
	for(int i = 0; i < MVL_WEAPON_SLOT_COUNT; i++){
		if(s_weapons[i] == 0)
			return i;
	}
	return -1;
}

// ===========================================================================
// FUN_10054870 @ RVA 0x54870 — GetWeaponBySlot (slot view; <1000).
// ===========================================================================
CMvlWeapon *
CWeaponHandler::GetWeaponBySlot(unsigned slot)
{
	if(slot < MVL_WEAPON_SLOT_COUNT)
		return s_weapons[slot];
	return 0;
}

// ===========================================================================
// FUN_10054890 @ RVA 0x54890 — GetWeaponById (absolute id 100..1099).
// ===========================================================================
CMvlWeapon *
CWeaponHandler::GetWeaponById(int id)
{
	if((unsigned)(id - MVL_WEAPON_SLOT_MIN) < MVL_WEAPON_SLOT_COUNT){
		CMvlWeapon *w = s_weapons[id - MVL_WEAPON_SLOT_MIN];
		if(w != 0 && w->GetWeaponId() == id)   // FUN_10054090 validation
			return w;
	}
	return 0;
}

// ===========================================================================
// FUN_10054850 @ RVA 0x54850 — IsValidWeaponId (0..36 stock or registered).
// ===========================================================================
bool
CWeaponHandler::IsValidWeaponId(int id)
{
	if(id < 0)
		return false;
	if(id <= MVL_STOCK_WEAPON_ID_MAX)
		return true;
	return GetWeaponById(id) != 0;
}

// ===========================================================================
// FUN_100548c0 @ RVA 0x548c0 — GetWeaponDisplayName.
// ===========================================================================
const char *
CWeaponHandler::GetWeaponDisplayName(int id)
{
	CMvlWeapon *w = GetWeaponById(id);
	if(w)
		return w->GetName();   // FUN_10054040 → properties name path
	if(id < 0)
		return "";             // DAT_00782a14 sentinel
	return MvlStockWeaponTableName(id > MVL_STOCK_WEAPON_ID_MAX ?
	                               MVL_STOCK_WEAPON_ID_MAX : id);
}

// ===========================================================================
// FUN_10055040 @ RVA 0x55040 — slot number from an archive file name:
// "w<N>_..." → N (strtol after 'w', must be followed by '_'), else -1.
// (Provisional name: GetSlotFromFileName.)
// ===========================================================================
static long
GetSlotFromFileName(const char *fileName)
{
	if(!fileName || fileName[0] != 'w')
		return -1;
	char *end = 0;
	long v = strtol(fileName + 1, &end, 10);
	if(v < 1 || end == 0 || end[0] != '_')
		return -1;
	return v;
}

// ===========================================================================
// CWeaponHandler::GetNewSlotForArchive — 0x98e @ RVA 0x550c0
// ===========================================================================
CMvlWeapon *
CWeaponHandler::GetNewSlotForArchive(const char *fileName)
{
	long slot = GetSlotFromFileName(fileName);
	unsigned use;

	if(slot != -1){
		if((unsigned)(slot - 100) < 1000 && s_weapons[slot - 100] != 0){
			HWarn("CWeaponHandler::GetNewSlotForArchive",
			      "Slot requested by file already in use, taking first free ID.");
		}else if((unsigned)(slot - 100) >= 1000){
			HWarn("CWeaponHandler::GetNewSlotForArchive",
			      "Slot out of range, taking first free ID.");
		}else{
			use = (unsigned)(slot - 100);
			goto got_slot;
		}
	}
	{
		int f = FindFirstFreeSlot();
		if(f < 0)
			return 0;
		use = (unsigned)f;
	}
got_slot:
	{
		CMvlWeapon *w = new CMvlWeapon();   // FUN_10059d7f(0x1fb) + FUN_10051ff0 ctor
		// FUN_10051ff0 @ 0x51ff0 ctor: properties defaults + slot bind.
		w->m_nSlotIndex = (int)use;        // @ 0x80
		s_weapons[use] = w;
		return w;
	}
}

// ===========================================================================
// CWeaponHandler::LoadWeaponArchives — 0x98e @ RVA 0x55140
// Two passes over mvl\weapons\*.7z: pass 0 = files with a "w<N>_" slot name,
// pass 1 = the rest. Per file: build "mvl\weapons\<name>", allocate a slot,
// CMvlWeapon::LoadFromArchive(path, slot+100); on failure free the weapon;
// on success track the high-water id and register with the game. Ends with
// CWeaponPickups::LoadXMLPickupFile (pickups.xml).
// ===========================================================================
void
CWeaponHandler::LoadWeaponArchives(void)
{
	for(int pass = 0; pass < 2; pass++){
		WIN32_FIND_DATAA fd;
		HANDLE h = FindFirstFileA("mvl\\weapons\\*.7z", &fd);
		if(h == INVALID_HANDLE_VALUE)
			continue;
		do{
			if(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				continue;
			long slot = GetSlotFromFileName(fd.cFileName);
			// decomp: (slot-100 < 1000) == (pass == 0)
			bool named = ((unsigned)(slot - 100) < 1000);
			if(named != (pass == 0))
				continue;

			char path[0x104];
			_snprintf(path, sizeof(path), "mvl\\weapons\\%s", fd.cFileName);

			CMvlWeapon *w = GetNewSlotForArchive(fd.cFileName);
			if(!w){
				HErr("CWeaponHandler::LoadWeaponArchives",
				     "Could not find a slot for weapon file '%s', all slots full.",
				     fd.cFileName);
				continue;
			}
			if(!w->LoadFromArchive(path, w->GetSlotIndex() + MVL_WEAPON_SLOT_MIN)){
				HErr("CWeaponHandler::LoadWeaponArchives",
				     "Could not load weapon archive '%s'.", fd.cFileName);
				w->Release();
				s_weapons[w->GetSlotIndex()] = 0;
				delete w;
				continue;
			}
			// high-water counter (DAT_1011795c = max id + 1).
			int id = w->GetSlotIndex() + MVL_WEAPON_SLOT_MIN;
			if(s_nMaxWeaponId <= id)
				s_nMaxWeaponId = id + 1;
			// HOOK POINT @ 0x55140: FUN_10054150 (register name) +
			// FUN_10054090 + FUN_1000cbb0 (game weapon-store insert).
			MvlGameRegisterWeapon(id, w);
		}while(FindNextFileA(h, &fd));
		FindClose(h);
	}
	CWeaponPickups::LoadXMLPickupFile();   // FUN_10058990 @ 0x58990
}

// ===========================================================================
// FUN_10056cb0 @ RVA 0x56cb0 — (provisional) FindWeaponForIdentity
// Pass 1: match archive totalSize (hash) against every loaded weapon;
// pass 2: match the archive path (stricmp). Same two-phase shape as the
// vehicle lane's FUN_10051480.
// ===========================================================================
CMvlWeapon *
CWeaponHandler::FindWeaponForIdentity(void)
{
	for(int i = 0; i < MVL_WEAPON_SLOT_COUNT; i++){
		CMvlWeapon *w = s_weapons[i];
		if(w && w->GetArchiveTotalSize() == s_nSaveArchiveHash)
			return w;
	}
	// The original stricmp's DAT_101247e4 unconditionally; archivename is
	// required so it is always set, but keep the null guard (safety).
	if(s_szSaveArchiveName){
		for(int i = 0; i < MVL_WEAPON_SLOT_COUNT; i++){
			CMvlWeapon *w = s_weapons[i];
			if(w && _stricmp(s_szSaveArchiveName, w->GetArchivePath()) == 0)
				return w;
		}
	}
	return 0;
}

// ===========================================================================
// CWeaponHandler::LoadSaveXmlWeaponInfoIdentity — 0x98e @ RVA 0x56560
// ===========================================================================
CMvlWeapon *
CWeaponHandler::LoadSaveXmlWeaponInfoIdentity(TiXmlElement *sector)
{
	static const char *const fn =
		"CWeaponHandler::LoadSaveXmlWeaponInfoIdentity";
	bool bHash = false, bName = false;

	s_nSaveArchiveHash = 0;
	s_szSaveArchiveName = 0;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "archivehash") == 0){
			if(bHash)
				return HErr(fn, "'%s' sector contains multiple '%s' elements.",
				            "identity", name), (CMvlWeapon *)0;
			const char *t = e->GetText();
			if(!t || sscanf(t, "%x", &s_nSaveArchiveHash) != 1)
				return HErr(fn, "Failed to read data from '%s' element.", name),
				       (CMvlWeapon *)0;
			bHash = true;
		}
		else if(_stricmp(name, "archivename") == 0){
			if(bName)
				return HErr(fn, "'%s' sector contains multiple '%s' elements.",
				            "identity", name), (CMvlWeapon *)0;
			s_szSaveArchiveName = e->GetText();   // doc-owned pointer (DAT_101247e4)
			bName = true;
		}
		// unknown identity element: ignored (table miss)
	}
	// required: archivehash + archivename (same table shape as the vehicle lane).
	if(!bHash || !bName)
		return HErr(fn, "'%s' sector is missing '%s' element.", "identity",
		            bHash ? "archivename" : "archivehash"), (CMvlWeapon *)0;

	CMvlWeapon *w = FindWeaponForIdentity();
	if(!w){
		HErr(fn, "No weapon match for hash %08X or archive '%s'.",
		     s_nSaveArchiveHash, s_szSaveArchiveName ? s_szSaveArchiveName : "");
		return 0;
	}
	return w;
}

// ===========================================================================
// CWeaponHandler::LoadSaveXmlPlayerWeaponInfo — 0x98e @ RVA 0x56640
// <weapon>: <identity>(req) + <ammo> (clamp 1..100000). On success stores
// (weaponId, ammo) into s_anPlayerSave*/[weaponslot]; collision warns
// "Cannot apply weapon %s, slot collision." then overwrites.
// ===========================================================================
bool
CWeaponHandler::LoadSaveXmlPlayerWeaponInfo(TiXmlElement *sector)
{
	static const char *const fn = "CWeaponHandler::LoadSaveXmlPlayerWeaponInfo";
	CMvlWeapon *w = 0;
	s_nSaveAmmo = 0;
	bool bAmmo = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "identity") == 0){
			w = LoadSaveXmlWeaponInfoIdentity(e);
			if(!w){
				HErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
		}
		else if(_stricmp(name, "ammo") == 0){
			const char *t = e->GetText();
			if(!t || sscanf(t, "%i", &s_nSaveAmmo) != 1)   // '%d' in the dump
				return HErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
			if(s_nSaveAmmo < 1)
				s_nSaveAmmo = 1;
			else if(s_nSaveAmmo > 100000)
				s_nSaveAmmo = 100000;
			bAmmo = true;
		}
		// unknown weapon-info element: ignored (table miss)
	}
	// required: identity (ammo optional — table +12 flags; FUN_1001bbb0).
	if(!w)
		return HErr(fn, "'%s' sector is missing '%s' element.", "weapon",
		            "identity"), false;

	// store into the weapon's save slot (weaponslot index 0..9)
	int slot = w->GetWeaponSlot();
	if(slot >= 0 && slot < MVL_PLAYER_WEP_SAVE_SLOTS){
		if(s_anPlayerSaveWeapon[slot] != 0){
			HWarn(fn, "Cannot apply weapon %s, slot collision.", w->GetName());
		}
		s_anPlayerSaveWeapon[slot] = w->GetWeaponId();   // DAT_10117960[i*2]
		s_anPlayerSaveAmmo[slot]   = bAmmo ? s_nSaveAmmo : 0; // DAT_10117964+i*8
	}
	return true;
}

// ===========================================================================
// CWeaponHandler::LoadSaveXmlPlayerWeapons — 0x98e @ RVA 0x567a0
// ===========================================================================
bool
CWeaponHandler::LoadSaveXmlPlayerWeapons(TiXmlElement *sector)
{
	static const char *const fn = "CWeaponHandler::LoadSaveXmlPlayerWeapons";

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		if(_stricmp(e->Value(), "weapon") != 0)
			continue;   // unknown weapon-list element: ignored (table miss)
		if(!LoadSaveXmlPlayerWeaponInfo(e))
			HErr(fn, "Could not load '%s' sector.", "weapon");
		// original logs and continues
	}
	return true;
}

// ===========================================================================
// Free-function entry points referenced by src/vehicle/vehicle_handler.{h,cpp}.
// Same signatures as the stubs that lane declared (0x98e @ RVA 0x567a0 /
// 0x56d40).
// ===========================================================================
bool CWeaponHandlerLoadSaveXmlPlayerWeapons(TiXmlElement *sector)
{
	return CWeaponHandler::LoadSaveXmlPlayerWeapons(sector);
}

// CWeaponHandler::ClearSaveData — 0.98e @ RVA 0x56d40
// (provisional class name; the decomp is a free fn zeroing the 10 save slots.)
void CWeaponHandlerClearSaveData(void)
{
	for(int i = 0; i < MVL_PLAYER_WEP_SAVE_SLOTS; i++){
		CWeaponHandler::s_anPlayerSaveWeapon[i] = 0;
		CWeaponHandler::s_anPlayerSaveAmmo[i] = 0;
	}
}

// ===========================================================================
// Save-XML write cluster — the inverse of the load section above. Record
// shape (mvlsave sidecar, matches LoadSaveXmlPlayerWeapons 0x567a0):
//   <playerweps>
//     <weapon>
//       <identity>
//         <archivehash>%08X</archivehash>   (GetArchiveTotalSize)
//         <archivename>%s</archivename>     (GetArchivePath)
//       </identity>
//       <ammo>%d</ammo>
//     </weapon>                             (one per registered MVL weapon)
//   </playerweps>
// ===========================================================================

// ===========================================================================
// FUN_10056880 @ RVA 0x56880 — (provisional) WriteSaveXmlWeaponInfoIdentity.
// Builds the <identity> element for the weapon id; NULL for unknown ids
// (GetWeaponById miss). sprintf args binary-verified: %08X of FUN_10054170
// (GetArchiveTotalSize), %s of FUN_10054160 (GetArchivePath).
// ===========================================================================
TiXmlElement *
CWeaponHandler::WriteSaveXmlWeaponInfoIdentity(int weaponId)
{
	CMvlWeapon *w = GetWeaponById(weaponId);   // FUN_10054890 @ 0x54890
	if(!w)
		return 0;

	TiXmlElement *identity = new (std::nothrow) TiXmlElement("identity");
	if(!identity)
		return 0;

	char buf[0x104];   // local_118
	SaveSprintf(buf, "%08X", w->GetArchiveTotalSize());   // FUN_10054170 @ 0x54170
	{
		TiXmlElement *hash = new (std::nothrow) TiXmlElement("archivehash");
		TiXmlText *text = new (std::nothrow) TiXmlText(buf);
		if(hash && text)
			hash->LinkEndChild(text);
		else
			delete text;
		if(hash)
			identity->LinkEndChild(hash);
	}

	SaveSprintf(buf, "%s", w->GetArchivePath());          // FUN_10054160 @ 0x54160
	{
		TiXmlElement *name = new (std::nothrow) TiXmlElement("archivename");
		TiXmlText *text = new (std::nothrow) TiXmlText(buf);
		if(name && text)
			name->LinkEndChild(text);
		else
			delete text;
		if(name)
			identity->LinkEndChild(name);
	}
	return identity;
}

// ===========================================================================
// FUN_10056aa0 @ RVA 0x56aa0 — (provisional) WriteSaveXmlPlayerWeaponInfo.
// <weapon> = <identity> (recursive writer above) + <ammo>%d</ammo>; NULL for
// unknown ids. The original is __fastcall with TWO params (id in ecx, ammo in
// edx) — binary-verified (mov edi,edx at fn+0x2e; EDI is pushed as the "%d"
// vsprintf vararg). The decomp shows one param and hides the vararg behind
// FUN_10051db0's 2-param prototype.
// ===========================================================================
TiXmlElement *
CWeaponHandler::WriteSaveXmlPlayerWeaponInfo(int weaponId, int ammo)
{
	CMvlWeapon *w = GetWeaponById(weaponId);   // FUN_10054890 @ 0x54890
	if(!w)
		return 0;

	char buf[0x104];   // local_118
	SaveSprintf(buf, "%d", ammo);   // hidden edx param (binary-verified)

	TiXmlElement *weapon = new (std::nothrow) TiXmlElement("weapon");
	if(!weapon)
		return 0;

	TiXmlElement *identity = WriteSaveXmlWeaponInfoIdentity(weaponId);
	if(identity)
		weapon->LinkEndChild(identity);

	TiXmlElement *ammoEl = new (std::nothrow) TiXmlElement("ammo");
	TiXmlText *text = new (std::nothrow) TiXmlText(buf);
	if(ammoEl && text)
		ammoEl->LinkEndChild(text);
	else
		delete text;
	if(ammoEl)
		weapon->LinkEndChild(ammoEl);
	return weapon;
}

// ===========================================================================
// FUN_10056c00 @ RVA 0x56c00 — (provisional) WriteSaveXmlPlayerWeapons.
// <playerweps> root. Walks the player ped's 10 weapon slots (ped+0x408,
// stride 0x18 = 6 ints); a slot yields a record only when its type field
// (> 0) resolves through GetWeaponById — stock weapons live in the vanilla
// save, the sidecar carries MVL weapons only. Slot reads are binary-verified:
// mov ecx,[esi] (id @ +0x00), mov edx,[esi+0xc] (ammo @ +0x0c) feed
// FUN_10056aa0(ecx,edx). Ped lookup FUN_1002dfd0 @ 0x2dfd0 → HOOK seam.
// ===========================================================================
TiXmlElement *
CWeaponHandler::WriteSaveXmlPlayerWeapons(unsigned playerCtx)
{
	void *ped = MvlGameGetPlayerPed(playerCtx);   // FUN_1002dfd0 @ 0x2dfd0

	TiXmlElement *root = new (std::nothrow) TiXmlElement("playerweps");
	if(!root)
		return 0;

	for(int i = 0; i < MVL_PLAYER_WEP_SAVE_SLOTS; i++){
		int id = 0, ammo = 0;
		if(!MvlGameGetPedWeaponSlot(ped, i, &id, &ammo))
			continue;   // HOOK seam: no engine data — never fabricate
		if(id > 0){
			TiXmlElement *rec = WriteSaveXmlPlayerWeaponInfo(id, ammo);
			if(rec)
				root->LinkEndChild(rec);
		}
	}
	return root;
}

// ===========================================================================
// CWeaponHandler::DumpWeaponXML — 0x98e @ RVA 0x57320
// Creates mvl\wlist (error "Failed to create mvl/wlist directory."), then
// writes one mvl\wlist\<name>.xml per stock weapon (lowercased name). When
// shift is NOT held, audio values equal to the defaults are omitted
// (frequency!=0 / displacement!=1 / voldisplacement!=0 / distance!=120 /
// loopcount!=1 / nospam!=0). The stock table (0x782a14, 100-byte records;
// names via DAT_0069d99c) is engine data → HOOK iteration; the fprintf
// format strings are verbatim from the decomp.
// ===========================================================================
void
CWeaponHandler::DumpWeaponXML(void)
{
	static const char *const fn = "CWeaponHandler::DumpWeaponXML";

	if(GetFileAttributesA("mvl\\wlist") == INVALID_FILE_ATTRIBUTES){
		if(!CreateDirectoryA("mvl\\wlist", NULL)){
			HErr(fn, "Failed to create mvl/wlist directory.");
			return;
		}
	}
	// compress = shift NOT down (decomp: ~GetKeyState(0x10)>>15 & 1)
	bool compress = ((GetKeyState(VK_SHIFT) & 0x8000) == 0);

	const int count = MvlStockWeaponTableCount();   // TODO(weap): 0x782a14
	bool warned = false;
	for(int i = 0; i < count; i++){
		const char *name = MvlStockWeaponTableName(i);   // TODO(weap): DAT_0069d99c
		if(!name || name[0] == '\0')
			continue;
		// the original lowercases the whole path (sprintf "mvl\\wlist\\%s.xml")
		char path[0x104];
		_snprintf(path, sizeof(path), "mvl\\wlist\\%s.xml", name);
		for(char *p = path; *p; p++)
			*p = (char)tolower((unsigned char)*p);

		FILE *f = fopen(path, "w");
		if(!f){
			if(!warned){
				HWarn(fn, "Unable to generate file mvl\\wlist\\%s.xml, does folder "
				      "mvl\\wlist exist?", name);
				warned = true;
			}
			continue;
		}
		// The stock record is engine data; a stub fill leaves the weapon at
		// defaults and the dump still emits the verbatim skeleton.
		CMvlWeapon tmp;
		MvlStockWeaponFillProperties(i, &tmp);   // TODO(weap): 0x782a14 rec

		fputs("<?xml version=\"1.0\" encoding=\"ASCII\"?>\n", f);
		fputs("<weapon>\n", f);
		fputs("\t<basic>\n", f);
		fprintf(f, "\t\t<name>%s</name>\n", tmp.m_szName);
		fprintf(f, "\t\t<logicalid>%d</logicalid>\n", tmp.m_nLogicalId);
		fputs("\t</basic>\n\n", f);
		fputs("\t<properties>\n", f);
		{
			static const char *const kTypes[6] = {
				"melee","instant_hit","projectile","area_effect","camera","other"
			};
			int t = tmp.m_nType;
			if(t < 0 || t > 5) t = MVL_WEP_OTHER;
			fprintf(f, "\t\t<type>%s</type>\n", kTypes[t]);
		}
		fprintf(f, "\t\t<range>%f</range>\n", tmp.fRange);
		fprintf(f, "\t\t<firerate>%u</firerate>\n", tmp.nFireRate);
		fprintf(f, "\t\t<reload>%u</reload>\n", tmp.nReload);
		fprintf(f, "\t\t<clipsize>%d</clipsize>\n", (int)tmp.nClipSize);
		fprintf(f, "\t\t<damage>%d</damage>\n", (int)tmp.nDamage);
		fprintf(f, "\t\t<speed>%f</speed>\n", tmp.fSpeed);
		fprintf(f, "\t\t<radius>%f</radius>\n", tmp.fRadius);
		fprintf(f, "\t\t<lifespan>%f</lifespan>\n", tmp.fLifespan);
		fprintf(f, "\t\t<spread>%f</spread>\n", tmp.fSpread);
		// verbatim offset block (single format string in the original):
		fprintf(f, "\t\t<offset>\n\t\t\t<x>%f</x>\n\t\t\t<y>%f</y>\n\t\t\t<z>%f</z>\n\t\t</offset>\n",
		        tmp.fOffset[0], tmp.fOffset[1], tmp.fOffset[2]);
		fprintf(f, "\t\t<animgroup>%s</animgroup>\n",
		        MvlStockWeaponAnimgroupName(i));   // TODO(weap): DAT_006857b0
		fprintf(f, "\t\t<animstart>%d</animstart>\n", (int)tmp.fAnimStart);
		fprintf(f, "\t\t<animend>%d</animend>\n", (int)tmp.fAnimEnd);
		fprintf(f, "\t\t<animfire>%d</animfire>\n", (int)tmp.fAnimFire);
		fprintf(f, "\t\t<anim2start>%d</anim2start>\n", (int)tmp.fAnim2Start);
		fprintf(f, "\t\t<anim2end>%d</anim2end>\n", (int)tmp.fAnim2End);
		fprintf(f, "\t\t<anim2fire>%d</anim2fire>\n", (int)tmp.fAnim2Fire);
		fprintf(f, "\t\t<animbreak>%d</animbreak>\n", (int)tmp.fAnimBreak);
		fprintf(f, "\t\t<modelindex>%d</modelindex>\n", tmp.m_nModelIndex);
		fprintf(f, "\t\t<model2index>%d</model2index>\n", tmp.m_nModelIndex2);
		fprintf(f, "\t\t<flags>%08X</flags>\n", tmp.m_nFlags);
		fprintf(f, "\t\t<weaponslot>%d</weaponslot>\n", tmp.m_nWeaponSlot);
		fputs("\t</properties>\n", f);
		// audio (only non-default values when compressing)
		const SMvlShotAudio *audio[3] = {
			&tmp.m_aAudio[0], &tmp.m_aAudio[1], &tmp.m_aAudio[2]
		};
		static const char *const kAudioNames[3] = { "gunshot", "reload", "echo" };
		bool any = false;
		for(int a = 0; a < 3; a++)
			if(audio[a]->nSample != 0 || audio[a]->nLoopCount != 0)
				any = true;
		if(any){
			fputs("\n\t<audio>", f);
			for(int a = 0; a < 3; a++){
				const SMvlShotAudio *s = audio[a];
				// the original emits the block when the stock entry exists
				if(!(s->nSample != 0 || s->nLoopCount != 0))
					continue;
				fprintf(f, "\n\t\t<%s>\n", kAudioNames[a]);
				fprintf(f, "\t\t\t<sample>%u</sample>\n", s->nSample);
				fprintf(f, "\t\t\t<volume>%u</volume>\n", (unsigned)s->nVolume);
				if(!compress || s->nFrequency != 0)
					fprintf(f, "\t\t\t<frequency>%u</frequency>\n",
					        (unsigned)s->nFrequency);
				if(!compress || s->nDisplacement != 1)
					fprintf(f, "\t\t\t<displacement>%u</displacement>\n",
					        s->nDisplacement);
				if(!compress || s->nVolDisplacement != 0)
					fprintf(f, "\t\t\t<voldisplacement>%u</voldisplacement>\n",
					        (unsigned)s->nVolDisplacement);
				if(!compress || s->fDistance != kShotAudioDefaultDistance)
					fprintf(f, "\t\t\t<distance>%f</distance>\n", s->fDistance);
				if(!compress || s->nLoopCount != 1)
					fprintf(f, "\t\t\t<loopcount>%u</loopcount>\n",
					        (unsigned)s->nLoopCount);
				if(!compress || s->bNoSpam != 0)
					fprintf(f, "\t\t\t<nospam>%s</nospam>\n",
					        s->bNoSpam ? "true" : "false");
				fprintf(f, "\t\t</%s>\n", kAudioNames[a]);
			}
			fputs("\t</audio>\n", f);
		}
		fputs("</weapon>\n", f);
		fclose(f);
	}
}

// ===========================================================================
// CWeaponHandler::ApplySaveData — 0.98e @ RVA 0x56db0 (name confirmed by its
// warn string "CWeaponHandler::ApplySaveData"). Role verified from the decomp
// + binary: it applies the 10 pending (weaponId, ammo) save pairs
// (DAT_10117960..0x101179b0, filled by LoadSaveXmlPlayerWeaponInfo and
// cleared by ClearSaveData) to the player ped. Per slot: if the ped weapon
// slot (ped+0x408+i*0x18, stride 0x18) is empty (type < 1) it calls
// FUN_10030560(ped, id, ammo, 0, 0, 1) (engine give-weapon); otherwise it
// warns verbatim "Cannot apply weapon %s, slot collision." (FUN_1001cce0)
// with FUN_10054150(GetWeaponById(id)) = the weapon name. Ped lookup
// FUN_1002dfd0 @ 0x2dfd0 and the give call are engine access → HOOK seams
// (perform NOTHING). Game-side entry is thunk FUN_10056db0 @ 0x51470.
// ===========================================================================
void
CWeaponHandler::ApplySaveData(unsigned playerCtx)
{
	static const char *const fn = "CWeaponHandler::ApplySaveData";

	void *ped = MvlGameGetPlayerPed(playerCtx);   // FUN_1002dfd0 @ 0x2dfd0

	for(int i = 0; i < MVL_PLAYER_WEP_SAVE_SLOTS; i++){
		int id = s_anPlayerSaveWeapon[i];   // *piVar2 @ DAT_10117960+i*8
		if(id == 0)
			continue;   // empty pending slot
		int ammo = s_anPlayerSaveAmmo[i];   // piVar2[1] @ +4

		int curId = 0, curAmmo = 0;
		if(!MvlGameGetPedWeaponSlot(ped, i, &curId, &curAmmo))
			continue;   // HOOK seam: no engine data — never fabricate

		if(curId < 1){
			// FUN_10030560(ped, id, ammo, 0, 0, 1) — give the weapon.
			MvlGameGiveWeapon(ped, id, ammo);
		}
		else {
			CMvlWeapon *w = GetWeaponById(id);   // FUN_10054890 @ 0x54890
			HWarn(fn, "Cannot apply weapon %s, slot collision.",
			      w ? w->GetName() : "");   // FUN_10054150 @ 0x54150
		}
	}
}

// ===========================================================================
// Stock-table / game-store hook stubs (see weapon_handler.h).
// ===========================================================================
int MvlStockWeaponTableCount(void)
{
	MvlErrorLog("TODO(weap): 0x782a14 MvlStockWeaponTableCount (stock table 100-byte recs)");
	return 0;
}

const char *MvlStockWeaponTableName(int)
{
	MvlErrorLog("TODO(weap): DAT_0069d99c MvlStockWeaponTableName");
	return 0;
}

int MvlStockWeaponLogicalId(int)
{
	MvlErrorLog("TODO(weap): stock rec+0x34 MvlStockWeaponLogicalId");
	return 0;
}

const char *MvlStockWeaponAnimgroupName(int)
{
	MvlErrorLog("TODO(weap): DAT_006857b0 MvlStockWeaponAnimgroupName");
	return 0;
}

bool MvlStockWeaponFillProperties(int, CMvlWeapon *out)
{
	MvlErrorLog("TODO(weap): 0x782a14 MvlStockWeaponFillProperties");
	(void)out;
	return false;   // leaves the caller's default-initialized weapon
}

void MvlGameRegisterWeapon(int, CMvlWeapon *)
{
	MvlErrorLog("TODO(weap): 0xcbb0 MvlGameRegisterWeapon");
}

// ---------------------------------------------------------------------------
// Save-write engine seams (see weapon_handler.h). HOOK POINT stubs: log the
// intended VA + value via MvlErrorLog and perform NOTHING — never fake
// engine behavior.
// ---------------------------------------------------------------------------
void *
MvlGameGetPlayerPed(unsigned playerCtx)
{
	MvlErrorLog("HOOK POINT @ 0x2dfd0: player ped lookup (ctx=%08X) — "
	            "engine access not hooked", playerCtx);
	return 0;
}

bool
MvlGameGetPedWeaponSlot(void *ped, int slot, int *outWeaponId, int *outAmmo)
{
	MvlErrorLog("HOOK POINT @ ped+0x408+0x%X (slot %d of %p): read weapon "
	            "slot id @ +0x00 / ammo @ +0x0c — engine access not hooked",
	            slot * 0x18, slot, ped);
	if(outWeaponId)
		*outWeaponId = 0;
	if(outAmmo)
		*outAmmo = 0;
	return false;
}

void
MvlGameGiveWeapon(void *ped, int weaponId, int ammo)
{
	MvlErrorLog("HOOK POINT @ 0x30560: give weapon id=%d ammo=%d to ped %p "
	            "(flags 0, 0, 1) — engine access not hooked",
	            weaponId, ammo, ped);
}
