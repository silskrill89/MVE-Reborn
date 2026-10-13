// mvl_weapon.cpp — CMvlWeapon implementation (see mvl_weapon.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (savewpn/range-decomp.txt
// 0x52150-0x53f10 pipeline + 0x53060-0x53cd0 XML sections). Log strings are
// verbatim; engine/RW internals are HOOK POINT stubs (MvlErrorLog + target VA)
// reusing the archive lane's hook set from ../archive/vehicle_archive.h.
#include "mvl_weapon.h"

#include "../archive/vehicle_archive.h"   // CSevenWalker-side RW/anim/sound hooks
#include "../archive/seven_walker.h"
#include "../loader/log.h"
#include "../weapons/weapon_pickups.h"     // LoadXMLSectionSpawnList (weapon <spawnlist>)
#include "../weapons/weapon_handler.h"     // CWeaponHandler::LoadXMLSectionWeaponAudio

#include "tinyxml.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrappers (same mapping as the other lanes): FUN_1001cc00 msg,
// FUN_1001cce0 warn, FUN_1001cb80 error.
// ---------------------------------------------------------------------------
static void
WepMsg(const char *fmt, ...)
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
WepWarn(const char *fn, const char *fmt, ...)
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
WepErr(const char *fn, const char *fmt, ...)
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

// Stage-failure exception (same family as the vehicle pipeline's SMvlLoadError;
// Catch @ RVA 0x5235a etc. log "CMvlWeapon::<fn>" + "%s").
struct SWepLoadError
{
	char msg[512];
};

static void
WepThrow(const char *fmt, ...)
{
	SWepLoadError e;
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(e.msg, sizeof(e.msg), fmt, ap);
	va_end(ap);
	e.msg[sizeof(e.msg) - 1] = '\0';
	throw e;
}

// Frame-scale constant: the properties post-pass divides the 7 anim fields by
// DAT_1008dec0 (unknown .data float; most likely the 30fps frame length).
static const float kAnimFrameScale = 30.0f;   // TODO(weap): DAT_1008dec0 exact value

// FUN_1001b970 @ 0x1b970 — case-insensitive ends-with (RegisterArchiveFile).
static bool
WepNameEndsWith(const char *name, const char *needle)
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

// ---------------------------------------------------------------------------
// Construction / defaults
// ---------------------------------------------------------------------------
CMvlWeapon::CMvlWeapon()
{
	memset(this, 0, sizeof(*this));
	ResetPropertiesDefaults();
}

// FUN_10053e10 @ RVA 0x53e10 — properties defaults.
void
CMvlWeapon::ResetPropertiesDefaults(void)
{
	m_nType = MVL_WEP_INSTANT_HIT;   // 0x8c = 1
	fRange = 0.0f; nFireRate = 0; nReload = 0; nClipSize = 0; nDamage = 0;
	fSpeed = 0.0f; fRadius = 0.0f; fLifespan = 0.0f; fSpread = 0.0f;
	fOffset[0] = fOffset[1] = fOffset[2] = 0.0f;
	nAnimGroup = 7;                  // 0xc0 = "unarmed"
	fAnimStart = fAnimEnd = fAnimFire = 0.0f;
	fAnim2Start = fAnim2End = fAnim2Fire = fAnimBreak = 0.0f;
	m_nModelIndex = -1;              // 0xe0
	m_nModelIndex2 = -1;             // 0xe4
	m_nFlags = 0x3b;                 // 0xe8
	m_nWeaponSlot = 0;               // 0xec
}

// ---------------------------------------------------------------------------
// CWeaponLogic accessors / ctors
// ---------------------------------------------------------------------------
int
CWeaponLogic::GetLogicalId(void)
{
	// FUN_10054220 @ 0x54220 — returns the owner weapon's logicalid.
	return m_pWeapon ? ((CMvlWeapon *)m_pWeapon)->GetLogicalId() : 0;
}

CWeaponLogicHat::CWeaponLogicHat(void *weapon)
	: CWeaponLogic(weapon)
{
	// FUN_10054260 @ 0x54260 — vtable + two zero vectors + scale 1.0.
	fScale = 1.0f;   // +0x08 = 0x3f800000
	fOffset[0] = fOffset[1] = fOffset[2] = 0.0f;   // FUN_10008c50 zero @ +0x0c
	fRotation[0] = fRotation[1] = fRotation[2] = 0.0f;  // @ +0x18
	bIsRelative = true;
}

// ===========================================================================
// CMvlWeapon::LoadFromArchive — 0.98e @ RVA 0x52150 (catch @ RVA 0x5235a)
// ===========================================================================
bool
CMvlWeapon::LoadFromArchive(const char *archivePath, int id)
{
	strncpy(m_szArchivePath, archivePath ? archivePath : "", 100);
	m_szArchivePath[99] = '\0';           // decomp nulls @ 0x1f7
	ResetPropertiesDefaults();
	m_nWeaponId = id;                     // @ 0x84

	try{
		if(!ParseArchive(m_szArchivePath))
			WepThrow("ParseArchive failed");
		if(!LoadXMLFile())
			WepThrow("LoadXMLFile failed");
		if(!LoadIconFile())
			WepThrow("LoadIconFile failed");
		if(!LoadScopeFile())
			WepThrow("LoadScopeFile failed");
		if(!LoadTextureFile())
			WepThrow("LoadTextureFile failed");
		if(!LoadModelFile())
			WepThrow("LoadModelFile failed");
		LoadAnimFile();   // logs its own failure; return not checked (original)

		// Fill empty audio slots from the stock weapon for this logical id
		// (FUN_10054e10 @ 0x54e10 — stock audio table lookup; HOOK).
		for(int i = 0; i < 3; i++){
			SMvlShotAudio *a = &m_aAudio[i];
			if(a->nSample == 0 && a->nLoopCount == 0){
				const SMvlShotAudio *stock = MvlGetStockWeaponAudio(m_nLogicalId, i);
				if(stock)
					*a = *stock;
			}
		}
		WepMsg("Successfully loaded weapon %d from file %s", m_nWeaponId,
		       m_szArchivePath);
		return true;
	}catch(const SWepLoadError &e){
		// Catch @ RVA 0x5235a → "CMvlWeapon::LoadFromArchive" "%s"
		WepErr("CMvlWeapon::LoadFromArchive", "%s", e.msg);
		return false;
	}
}

// ===========================================================================
// CMvlWeapon::ParseArchive — 0.98e @ RVA 0x523b0
// ===========================================================================
bool
CMvlWeapon::ParseArchive(const char *archivePath)
{
	CSevenWalker walker;

	if(!walker.LoadSeven(archivePath)){
		WepErr("CMvlWeapon::ParseArchive", "Could not load 7zip archive '%s'.",
		       archivePath);
		return false;
	}

	const UInt32 num = walker.GetNumEntries();
	m_nArchiveTotalSize = 0;    // @ 0x190
	for(UInt32 i = 0; i < num; i++)
		m_nArchiveTotalSize += (unsigned)walker.GetEntrySize(i);

	for(UInt32 i = 0; i < num; i++){
		const char *name = walker.GetEntryName(i);
		std::vector<unsigned char> tmp;
		if(!walker.ExtractToMemory(i, tmp)){
			WepErr("CMvlWeapon::ParseArchive",
			       "Could not load file '%s' from archive '%s'.", name, archivePath);
			break;
		}
		unsigned size = (unsigned)tmp.size();
		void *data = malloc(size ? size : 1);
		if(!data)
			break;
		if(size)
			memcpy(data, &tmp[0], size);
		if(!RegisterArchiveFile(name, data, size)){
			WepErr("CMvlWeapon::ParseArchive",
			       "Could not register file '%s' from archive '%s'.", name,
			       archivePath);
			break;
		}
	}
	walker.Close();
	return true;
}

// ===========================================================================
// CMvlWeapon::RegisterArchiveFile — 0.98e @ RVA 0x52540
// ===========================================================================
bool
CMvlWeapon::RegisterArchiveFile(const char *name, void *data, unsigned size)
{
	if(WepNameEndsWith(name, ".xml")){
		if(m_pXmlBuf == 0){
			m_pXmlBuf = data;
			m_anBufSizes[0] = size;    // @ 0x18
			return true;
		}
		WepErr("CMvlWeapon::RegisterArchiveFile", "Another XML file is already registered.");
		return false;
	}
	if(WepNameEndsWith(name, ".dff")){
		if(m_pDffBuf == 0){
			m_pDffBuf = data;
			m_anBufSizes[1] = size;    // @ 0x1c
			return true;
		}
		WepErr("CMvlWeapon::RegisterArchiveFile", "Another DFF file is already registered.");
		return false;
	}
	if(WepNameEndsWith(name, ".txd")){
		if(m_pTxdBuf == 0){
			m_pTxdBuf = data;
			m_anBufSizes[2] = size;    // @ 0x20
			return true;
		}
		WepErr("CMvlWeapon::RegisterArchiveFile", "Another TXD file is already registered.");
		return false;
	}
	if(WepNameEndsWith(name, ".wav")){
		// CSoundHandler::LoadSoundFile @ 0x33e10 — name table "gunshot"
		// (PTR_s_gunshot_r_100af5dc), slot block @ 0x30.
		bool ok = MvlSoundLoadFile("gunshot", &m_abSoundSlots[0][0], name, data, size);
		if(!ok)
			WepErr("CMvlWeapon::RegisterArchiveFile", "Loading sound file %s failed.", name);
		free(data);
		return ok;
	}
	if(WepNameEndsWith(name, "icon.png")){
		if(m_pIconBuf == 0){
			m_pIconBuf = data;
			m_anBufSizes[3] = size;    // @ 0x24
			return true;
		}
		WepErr("CMvlWeapon::RegisterArchiveFile", "Another icon file is already registered.");
		return false;
	}
	if(WepNameEndsWith(name, "scope.png")){
		// NOTE (original quirk, decomp order): a full_scope.png /
		// stretch_scope.png name ALSO ends with "scope.png" — the flavor
		// flags are set inside the scope.png branch, exactly as decompiled.
		if(WepNameEndsWith(name, "full_scope.png"))
			m_bFullScope = true;      // @ 0xf8
		else if(WepNameEndsWith(name, "stretch_scope.png"))
			m_bStretchScope = true;   // @ 0xf9
		if(m_pScopeBuf == 0){
			m_pScopeBuf = data;
			m_anBufSizes[5] = size;    // @ 0x2c
			return true;
		}
		WepErr("CMvlWeapon::RegisterArchiveFile",
		       "Another scope texture file is already registered.");
		return false;
	}
	if(WepNameEndsWith(name, ".ifp")){
		if(m_pIfpBuf == 0){
			m_pIfpBuf = data;
			m_anBufSizes[4] = size;    // @ 0x28
			return true;
		}
		WepErr("CMvlWeapon::RegisterArchiveFile", "Another IFP file is already registered.");
		return false;
	}
	free(data);   // unregistered file: freed and ignored (original)
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadTextureFile — 0.98e @ RVA 0x52780 (catch @ RVA 0x528d1)
// ===========================================================================
bool
CMvlWeapon::LoadTextureFile(void)
{
	if(m_pTxdBuf == 0)
		return true;

	try{
		if(!MvlRecognizeTextureData(m_pTxdBuf, m_anBufSizes[2]))
			WepThrow("Failed to recognize texture.");
		void *stream = MvlRwMemoryStreamOpen(m_pTxdBuf, m_anBufSizes[2]);
		if(!stream)
			WepThrow("Could not open memory stream for texture file.");
		if(MvlRwStreamFindChunkHeader(stream) == 0)
			WepThrow("Could not find texture dictionary chunk header from .txd file.");
		m_pTxd = MvlRwReadTextureDictionary(stream);   // @ 0x108
		if(!m_pTxd)
			WepThrow("Loading texture dictionary from .txd file failed.");
		MvlRwMemoryStreamClose(stream);
	}catch(const SWepLoadError &e){
		WepErr("CMvlWeapon::LoadTextureFile", "%s", e.msg);
	}

	// FUN_1005290f @ 0x5290f tail — free the .txd buffer.
	free(m_pTxdBuf);
	m_pTxdBuf = 0;
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadModelFile — 0.98e @ RVA 0x52960 (catch @ RVA 0x52b2b)
// ===========================================================================
bool
CMvlWeapon::LoadModelFile(void)
{
	if(m_pDffBuf == 0)
		return true;

	try{
		MvlRwClumpReadPrep();   // FUN_10032760 @ 0x32760
		void *stream = MvlRwMemoryStreamOpen(m_pDffBuf, m_anBufSizes[1]);
		if(!stream)
			WepThrow("Could not open memory stream for DFF file.");
		if(MvlRwStreamFindChunkHeader(stream) == 0)
			WepThrow("Could not find clump chunk header from DFF file.");
		if(m_pTxd != 0)
			MvlRwTexDictReadPrep();   // FUN_10032750 @ 0x32750
		void *clump = MvlRwReadClump(stream);
		if(!clump)
			WepThrow("Loading clump from DFF file failed.");
		m_pAtomic = MvlRwClumpGetFirstAtomic(clump);   // FUN_10031260 @ 0x31260
		if(!m_pAtomic)
			WepThrow("Clump contains no atomics.");
		MvlRwClumpBindAtomic(clump, m_pAtomic);       // FUN_10031470 @ 0x31470
		MvlRwAtomicFinalise();                        // FUN_10031eb0 @ 0x31eb0
		MvlRwAtomicSetup(m_pAtomic);                  // FUN_10031ce0 @ 0x31ce0
		MvlRwNormalizeModel();                        // FUN_1002eb80 @ 0x2eb80
		m_nModelIndex = -1;                            // @ 0xe0
		m_nModelIndex2 = -1;                           // @ 0xe4
		MvlRwMemoryStreamClose(stream);
	}catch(const SWepLoadError &e){
		WepErr("CMvlWeapon::LoadModelFile", "%s", e.msg);
	}

	// FUN_10052b75 @ 0x52b75 tail — tex-dict mode, weapon clump register,
	// stream close, free the .dff buffer.
	MvlRwTexDictReadPrep();   // FUN_10032750
	if(m_pAtomic)
		MvlWeaponClumpRegister(m_pAtomic);   // FUN_10031450 @ 0x31450
	free(m_pDffBuf);
	m_pDffBuf = 0;
	return true;
}

// ===========================================================================
// FUN_10052be0 @ RVA 0x52be0 — MvlWeaponLoadImageTexture (icon/scope PNG).
// Catch @ RVA 0x52d0e → "CMvlWeapon::LoadImageTexture".
// ===========================================================================
bool
MvlWeaponLoadImageTexture(void **buf, unsigned size, void **outTex)
{
	if(buf == 0 || *buf == 0){
		WepErr("CMvlWeapon::LoadImageTexture",
		       "Buffer is not loaded - file does not exist.");
		return false;
	}
	*outTex = 0;
	try{
		void *stream = MvlRwMemoryStreamOpen(*buf, size);
		if(!stream)
			WepThrow("Could not open memory stream for icon file.");
		void *image = MvlPngReadImage(stream);   // FUN_1001c690 @ 0x1c690
		if(!image)
			WepThrow("Could not read RwImage from PNG file.");
		*outTex = MvlImageToTexture(image);      // FUN_1001ca90 @ 0x1ca90
		if(!*outTex)
			WepThrow("Could not get texture from RwImage.");
		MvlRwMemoryStreamClose(stream);
	}catch(const SWepLoadError &e){
		// Catch @ RVA 0x52d0e destroys the image when the texture failed
		// (FUN_100327c0 @ 0x327c0) — HOOK note for the engine lane.
		WepErr("CMvlWeapon::LoadImageTexture", "%s", e.msg);
		return false;
	}
	// FUN_10052d68 @ 0x52d68 tail — stream close + free the raw buffer.
	free(*buf);
	*buf = 0;
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadIconFile — 0.98e @ RVA 0x52dc0
// ===========================================================================
bool
CMvlWeapon::LoadIconFile(void)
{
	if(!MvlWeaponLoadImageTexture(&m_pIconBuf, m_anBufSizes[3], &m_pIconTex)){
		WepErr("CMvlWeapon::LoadIconFile", "Failed to read icon buffer to texture.");
		return false;
	}
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadScopeFile — 0.98e @ RVA 0x52e00
// ===========================================================================
bool
CMvlWeapon::LoadScopeFile(void)
{
	if(m_pScopeBuf != 0){
		if(!MvlWeaponLoadImageTexture(&m_pScopeBuf, m_anBufSizes[5], &m_pScopeTex)){
			WepErr("CMvlWeapon::LoadScopeFile",
			       "Failed to read scope buffer to texture.");
			return false;
		}
	}
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadAnimFile — 0.98e @ RVA 0x52e40
// ===========================================================================
bool
CMvlWeapon::LoadAnimFile(void)
{
	if(m_pIfpBuf == 0)
		return true;

	m_pAnimFile = MvlAnimFileNew();   // @ 0x110
	if(!MvlAnimFileLoad((CCustomAnimFile *)m_pAnimFile, m_pIfpBuf,
	                    m_anBufSizes[4])){
		WepErr("CMvlWeapon::LoadAnimFile", "Failed to read animation file.");
		MvlAnimFileFree((CCustomAnimFile *)m_pAnimFile);
		m_pAnimFile = 0;
	}else{
		int count = MvlAnimFilePostProcess((CCustomAnimFile *)m_pAnimFile);
		WepMsg("Loaded %d anims from IFP file for weapon.", count);
	}
	free(m_pIfpBuf);   // tail frees the .ifp buffer
	m_pIfpBuf = 0;
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadXMLFile — 0.98e @ RVA 0x52f20
// ===========================================================================
bool
CMvlWeapon::LoadXMLFile(void)
{
	if(m_pXmlBuf == 0){
		WepErr("CMvlWeapon::LoadXMLFile", "No XML file found.");
		return false;
	}

	TiXmlDocument doc;
	doc.Parse((const char *)m_pXmlBuf, 0, TIXML_ENCODING_UNKNOWN);
	free(m_pXmlBuf);
	m_pXmlBuf = 0;

	if(doc.Error()){
		WepErr("CMvlWeapon::LoadXMLFile", "XML file could not be parsed: %s.",
		       doc.ErrorDesc());
		return false;
	}
	TiXmlElement *root = doc.FirstChildElement();
	if(!root){
		WepErr("CMvlWeapon::LoadXMLFile", "No root element in XML.");
		return false;
	}
	if(root->Type() != TiXmlNode::TINYXML_ELEMENT ||
	   _stricmp(root->Value(), "weapon") != 0){
		WepErr("CMvlWeapon::LoadXMLFile", "Root element has the wrong name.");
		return false;
	}
	return LoadXMLSectionWeapon(root);
}

// ===========================================================================
// CMvlWeapon::LoadXMLSectionWeapon — 0.98e @ RVA 0x53060
// Sector table (24-byte rows): basic(req), properties(req), spawnlist,
// audio, shotgun + the dynamic logic block slot. The original binds the
// logic block name (vtable+0x18, e.g. "hatinfo") into the table after
// <basic> loads; a decompiler stack-slot ambiguity put that write on the
// shotgun row, but the shipped feature (hats/readme.txt <hatinfo>) requires
// the logic row — bound here to the logic slot (see port-notes-weapons.md).
// ===========================================================================
bool
CMvlWeapon::LoadXMLSectionWeapon(TiXmlElement *root)
{
	static const char *const fn = "CMvlWeapon::LoadXMLSectionWeapon";
	const char *logicName = "";   // becomes m_pLogic ? vtable+0x18() : ""

	for(TiXmlElement *e = root->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "basic") == 0){
			if(!LoadXMLSectionBasic(e)){
				WepErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			// bind the logic block name (vtable+0x18 → "hatinfo"/"")
			if(m_pLogic)
				logicName = m_pLogic->GetXmlBlockName();
		}
		else if(_stricmp(name, "properties") == 0){
			if(!LoadXMLSectionProperties(e)){
				WepErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
		}
		else if(_stricmp(name, "spawnlist") == 0){
			// weapon-embedded spawn list: CWeaponPickups::LoadXMLSectionSpawnList
			// @ 0x58610 with this weapon's id (m_nWeaponId @ 0x84).
			if(!CWeaponPickups::LoadXMLSectionSpawnList(e, m_nWeaponId))
				WepErr(fn, "Could not load '%s' sector.", name);
			// original logs and continues
		}
		else if(_stricmp(name, "audio") == 0){
			if(!CWeaponHandler::LoadXMLSectionWeaponAudio(e, &m_aAudio[0]))
				WepErr(fn, "Could not load '%s' sector.", name);
		}
		else if(_stricmp(name, "shotgun") == 0){
			if(!LoadXMLSectionShotgun(e))
				WepErr(fn, "Could not load '%s' sector.", name);
		}
		else if(logicName[0] != '\0' && _stricmp(name, logicName) == 0){
			// logic block (e.g. <hatinfo>) → vtable+0x1c ParseXmlBlock
			if(!m_pLogic->ParseXmlBlock(e)){
				WepErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
		}
		// unknown weapon sector: ignored (table miss -> skip, original)
	}
	// FUN_1001bbb0 required-check: basic + properties required (table flags).
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadXMLSectionBasic — 0.98e @ RVA 0x53390
// ===========================================================================
bool
CMvlWeapon::LoadXMLSectionBasic(TiXmlElement *sector)
{
	static const char *const fn = "CMvlWeapon::LoadXMLSectionBasic";
	bool bName = false, bLogicalId = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "name") == 0){
			if(bName){
				WepErr(fn, "'%s' sector contains multiple '%s' elements.",
				       "basic", name);
				return false;
			}
			const char *t = e->GetText();
			if(!t){
				WepWarn(fn, "Weapon has no name.");
				_snprintf(m_szName, sizeof(m_szName), "%s", "Unknown");
			}else{
				if(strlen(t) < 2)
					WepWarn(fn, "The name of the weapon is less than 2 characters long.");
				_snprintf(m_szName, sizeof(m_szName), "%s", t);
			}
			m_szName[sizeof(m_szName) - 1] = '\0';
			bName = true;
		}
		else if(_stricmp(name, "logicalid") == 0){
			if(bLogicalId){
				WepErr(fn, "'%s' sector contains multiple '%s' elements.",
				       "basic", name);
				return false;
			}
			const char *t = e->GetText();
			if(!t){
				WepErr(fn, "Failed to read data from '%s' element.", name);
				return false;
			}
			if(sscanf(t, "%i", &m_nLogicalId) != 1){
				WepErr(fn, "Failed to parse the contents of '%s' element.", name);
				return false;
			}
			// logic selection (FUN_10059d7f allocs in the original):
			if(m_nLogicalId == 80)                 // 0x50 → CWeaponLogicSpike
				m_pLogic = new CWeaponLogicSpike(this);
			else if(m_nLogicalId == 81)            // 0x51 → CWeaponLogicHat
				m_pLogic = new CWeaponLogicHat(this);
			else if(m_nLogicalId > 0x23)           // other >35 → 0
				m_nLogicalId = 0;
			bLogicalId = true;
		}
		else
			return WepErr(fn, "Unknown element '%s' in '%s' sector.", name, "basic"),
			       false;
	}
	if(!bName)
		return WepErr(fn, "'%s' sector is missing '%s' element.", "basic", "name"), false;
	if(!bLogicalId)
		return WepErr(fn, "'%s' sector is missing '%s' element.", "basic", "logicalid"),
		       false;
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadXMLSectionProperties — 0.98e @ RVA 0x53620
// All 23 elements 1:1. Table types: 'M' type-text, 'f' float, 'u' unsigned,
// 'v' vector, 'i' int, 'X' hex. Post-pass: weaponslot clamped 0..9; the 7
// anim fields /= kAnimFrameScale (DAT_1008dec0).
// ===========================================================================
bool
CMvlWeapon::LoadXMLSectionProperties(TiXmlElement *sector)
{
	static const char *const fn = "CMvlWeapon::LoadXMLSectionProperties";
	static const char *const kNames[23] = {
		"type","range","firerate","reload","clipsize","damage","speed",
		"radius","lifespan","spread","offset","animgroup","animstart",
		"animend","animfire","anim2start","anim2end","anim2fire",
		"animbreak","modelindex","model2index","flags","weaponslot"
	};
	unsigned seen = 0;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t = e->GetText();
		int idx = -1;
		for(int i = 0; i < 23; i++){
			if(_stricmp(name, kNames[i]) == 0){
				idx = i;
				break;
			}
		}
		if(idx < 0)
			continue;   // unknown properties element: ignored (table miss)

		if(idx == 0){   // type (text enum, decomp stricmp chain)
			if(!t)
				return WepErr(fn, "Failed to read data from '%s' element.", name), false;
			if     (_stricmp(t, "melee") == 0)        m_nType = MVL_WEP_MELEE;
			else if(_stricmp(t, "projectile") == 0)   m_nType = MVL_WEP_PROJECTILE;
			else if(_stricmp(t, "instant_hit") == 0)  m_nType = MVL_WEP_INSTANT_HIT;
			else if(_stricmp(t, "area_effect") == 0)  m_nType = MVL_WEP_AREA_EFFECT;
			else if(_stricmp(t, "camera") == 0)       m_nType = MVL_WEP_CAMERA;
			else if(_stricmp(t, "other") == 0)        m_nType = MVL_WEP_OTHER;
			else{
				WepErr(fn, "Unknown weapon type '%s'.", t);
				return false;
			}
			seen |= 1u;
			continue;
		}
		if(idx == 10){  // offset (vector @ 0xb4, table type 'v')
			if(!MvlParseVector(e, fOffset))
				return WepErr(fn, "Could not load '%s' vector.", "offset"), false;
			seen |= (1u << 10);
			continue;
		}
		if(idx == 11){  // animgroup (text → table lookup; -1 → 7 unarmed)
			if(!t)
				return WepErr(fn, "Failed to read data from '%s' element.", name), false;
			int g = MvlLookupWeaponAnimGroup(t);   // FUN_10003090 @ 0x3090 lineage
			if(g == -1){
				WepWarn(fn, "Unknown animation group, switching to 'unarmed'");
				g = 7;
			}
			nAnimGroup = g;
			seen |= (1u << 11);
			continue;
		}
		if(!t)
			return WepErr(fn, "Failed to read data from '%s' element.", name), false;
		float fv; unsigned uv; int iv;
		switch(idx){
		case 1:  if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fRange=fv; break;
		case 2:  if(sscanf(t,"%u",&uv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; nFireRate=uv; break;
		case 3:  if(sscanf(t,"%u",&uv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; nReload=uv; break;
		case 4:  if(sscanf(t,"%u",&uv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; nClipSize=uv; break;
		case 5:  if(sscanf(t,"%u",&uv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; nDamage=uv; break;
		case 6:  if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fSpeed=fv; break;
		case 7:  if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fRadius=fv; break;
		case 8:  if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fLifespan=fv; break;
		case 9:  if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fSpread=fv; break;
		case 12: if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fAnimStart=fv; break;
		case 13: if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fAnimEnd=fv; break;
		case 14: if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fAnimFire=fv; break;
		case 15: if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fAnim2Start=fv; break;
		case 16: if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fAnim2End=fv; break;
		case 17: if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fAnim2Fire=fv; break;
		case 18: if(sscanf(t,"%f",&fv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; fAnimBreak=fv; break;
		case 19: if(sscanf(t,"%i",&iv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; m_nModelIndex=iv; break;
		case 20: if(sscanf(t,"%i",&iv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; m_nModelIndex2=iv; break;
		case 21: if(sscanf(t,"%x",&m_nFlags)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; break;
		case 22: if(sscanf(t,"%u",&uv)!=1) return WepErr(fn,"Failed to parse the contents of '%s' element.",name),false; m_nWeaponSlot=(int)uv; break;
		}
		seen |= (1u << idx);
	}

	// required-check (FUN_1001bbb0): all 23 table entries are required.
	for(int i = 0; i < 23; i++){
		if((seen & (1u << i)) == 0)
			return WepErr(fn, "'%s' sector is missing '%s' element.",
			              "properties", kNames[i]), false;
	}

	// post-pass: weaponslot clamp + anim frame → seconds division.
	if(m_nWeaponSlot < 0)
		m_nWeaponSlot = 0;
	else if(m_nWeaponSlot > 9)
		m_nWeaponSlot = 9;
	fAnimStart  /= kAnimFrameScale;
	fAnimEnd    /= kAnimFrameScale;
	fAnimFire   /= kAnimFrameScale;
	fAnim2Start /= kAnimFrameScale;
	fAnim2End   /= kAnimFrameScale;
	fAnim2Fire  /= kAnimFrameScale;
	fAnimBreak  /= kAnimFrameScale;
	return true;
}

// ===========================================================================
// CMvlWeapon::LoadXMLSectionShotgun — 0.98e @ RVA 0x53cd0
// ===========================================================================
bool
CMvlWeapon::LoadXMLSectionShotgun(TiXmlElement *sector)
{
	static const char *const fn = "CMvlWeapon::LoadXMLSectionShotgun";
	bool bFrag = false, bSpread = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t = e->GetText();
		if(_stricmp(name, "fragments") == 0){
			if(!t)
				return WepErr(fn, "Failed to read data from '%s' element.", name), false;
			if(sscanf(t, "%i", &nShotgunFragments) != 1)
				return WepErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
			bFrag = true;
		}
		else if(_stricmp(name, "spread") == 0){
			if(!t)
				return WepErr(fn, "Failed to read data from '%s' element.", name), false;
			if(sscanf(t, "%f", &fShotgunSpread) != 1)
				return WepErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
			bSpread = true;
		}
		// unknown shotgun element: ignored (table miss)
	}
	// required-check (both table entries required).
	if(!bFrag)
		return WepErr(fn, "'%s' sector is missing '%s' element.", "shotgun",
		              "fragments"), false;
	if(!bSpread)
		return WepErr(fn, "'%s' sector is missing '%s' element.", "shotgun",
		              "spread"), false;
	// clamp + shotgun flag (decomp tail).
	if(nShotgunFragments < 1)
		nShotgunFragments = 1;
	else if(nShotgunFragments > 100)
		nShotgunFragments = 100;
	m_bShotgun = true;   // @ 0xfa
	return true;
}

// ===========================================================================
// CWeaponLogicHat::ParseXmlBlock — 0.98e @ RVA 0x543a0
// <hatinfo> (see hats/readme.txt): isrelative ('t', default true),
// offset (vec @ +0x0c), rotation (vec @ +0x18), scale (float @ +0x08).
// Post: relative → transform by the current frame (FUN_10008ee0 @ 0x8ee0,
// HOOK); absolute → zero both vectors (FUN_10008c70 @ 0x8c70).
// ===========================================================================
bool
CWeaponLogicHat::ParseXmlBlock(TiXmlElement *sector)
{
	static const char *const fn = "CWeaponLogicHat::ParseXmlBlock";
	bIsRelative = true;   // table default (local_99 = 1)

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t = e->GetText();
		if(_stricmp(name, "isrelative") == 0){
			if(!t)
				return WepErr(fn, "Failed to read data from '%s' element.", name), false;
			// table type 't' → true/false
			bIsRelative = (_stricmp(t, "true") == 0);
		}
		else if(_stricmp(name, "offset") == 0){
			if(!MvlParseVector(e, fOffset))
				return WepErr(fn, "Could not load '%s' vector.", "offset"), false;
		}
		else if(_stricmp(name, "rotation") == 0){
			if(!MvlParseVector(e, fRotation))
				return WepErr(fn, "Could not load '%s' vector.", "rotation"), false;
		}
		else if(_stricmp(name, "scale") == 0){
			if(!t)
				return WepErr(fn, "Failed to read data from '%s' element.", name), false;
			if(sscanf(t, "%f", &fScale) != 1)
				return WepErr(fn, "Failed to parse the contents of '%s' element.", name),
				       false;
		}
		// unknown hatinfo element: ignored (table miss)
	}
	// required-check: all four table entries are optional (+12=0).
	if(bIsRelative)
		MvlHatTransformRelative(fOffset, fRotation);   // HOOK @ 0x8ee0
	else{
		fOffset[0] = fOffset[1] = fOffset[2] = 0.0f;
		fRotation[0] = fRotation[1] = fRotation[2] = 0.0f;
	}
	return true;
}

// ===========================================================================
// FUN_10054110 @ RVA 0x54110 — MvlWeaponGetModelAtomic.
// ===========================================================================
void *
MvlWeaponGetModelAtomic(const CMvlWeapon *w)
{
	if(w == 0)
		return 0;
	if(w->m_nModelIndex != -1){
		// HOOK POINT @ 0x54110: FUN_10008bc0 (request model) +
		// *(DAT_100a6314 + modelIndex*4)->+0x28 (game model store).
		MvlWeaponRequestModel(w->m_nModelIndex);
		return MvlWeaponModelGetAtomic(w->m_nModelIndex);
	}
	return w->m_pAtomic;
}

// ===========================================================================
// CMvlWeapon accessors / release
// ===========================================================================
void *
CMvlWeapon::GetSoundSlot(unsigned i)
{
	// FUN_10054010 @ 0x54010 — @ 0x30 + i*20, NULL when the slot pointer is
	// NULL. (The slot's first dword is the LoadSoundFile object.)
	if(i > 3)
		return 0;
	if(*(void **)&m_abSoundSlots[i][0] == 0)
		return 0;
	return &m_abSoundSlots[i][0];
}

SMvlShotAudio *
CMvlWeapon::GetShotAudio(unsigned i)
{
	// FUN_100540d0 @ 0x540d0 — @ 0x114 + i*20; NULL when sample==0. i>2 → NULL.
	if(i > 2)
		return 0;
	if(m_aAudio[i].nSample == 0)
		return 0;
	return &m_aAudio[i];
}

// FUN_10053f10 @ RVA 0x53f10 — full release (buffers → txd/atomic/textures →
// logic dtor → name → ResetPropertiesDefaults).
void
CMvlWeapon::Release(void)
{
	free(m_pXmlBuf);  m_pXmlBuf = 0;  m_anBufSizes[0] = 0;
	free(m_pDffBuf);  m_pDffBuf = 0;  m_anBufSizes[1] = 0;
	free(m_pTxdBuf);  m_pTxdBuf = 0;  m_anBufSizes[2] = 0;
	free(m_pIconBuf); m_pIconBuf = 0; m_anBufSizes[3] = 0;
	free(m_pIfpBuf);  m_pIfpBuf = 0;  m_anBufSizes[4] = 0;
	free(m_pScopeBuf);m_pScopeBuf = 0;m_anBufSizes[5] = 0;
	for(int i = 0; i < 4; i++){
		// sound slot objects freed via their owning handler (0x33e10 lineage);
		// HOOK: the original frees *slot through the sound-object dtor.
		*(void **)&m_abSoundSlots[i][0] = 0;
	}
	m_nWeaponId = 0;                 // decomp clears 0x84 and 0x88
	m_nLogicalId = 0;
	if(m_pAtomic){
		MvlRwAtomicDestroy(m_pAtomic);           // FUN_10031380 @ 0x31380
		m_pAtomic = 0;
	}
	if(m_pTxd){
		MvlRwDestroyTextureDictionary(m_pTxd);  // FUN_10032710 @ 0x32710
		m_pTxd = 0;
	}
	if(m_pIconTex){
		MvlRwDestroyTexture(m_pIconTex);        // FUN_10032730 @ 0x32730
		m_pIconTex = 0;
	}
	if(m_pScopeTex){
		MvlRwDestroyTexture(m_pScopeTex);       // FUN_10032730 @ 0x32730
		m_pScopeTex = 0;
	}
	m_bFullScope = false;
	if(m_pLogic){
		delete m_pLogic;                        // vtable[0] deleting dtor
		m_pLogic = 0;
	}
	m_szName[0] = '\0';
	ResetPropertiesDefaults();
}

// ===========================================================================
// Weapon-lane hook stubs. HARNESS STUBS: log + benign values; the engine lane
// replaces them (target VAs in the comments). Nothing is faked.
// ===========================================================================
void *MvlRwClumpGetFirstAtomic(void *)
{
	MvlErrorLog("TODO(weap): 0x31260 MvlRwClumpGetFirstAtomic (RwClumpGetFirstAtomic)");
	return 0;
}

void MvlRwClumpBindAtomic(void *, void *)
{
	MvlErrorLog("TODO(weap): 0x31470 MvlRwClumpBindAtomic");
}

void MvlRwAtomicFinalise(void)
{
	MvlErrorLog("TODO(weap): 0x31eb0 MvlRwAtomicFinalise");
}

void MvlRwAtomicSetup(void *)
{
	MvlErrorLog("TODO(weap): 0x31ce0 MvlRwAtomicSetup");
}

void MvlRwNormalizeModel(void)
{
	MvlErrorLog("TODO(weap): 0x2eb80 MvlRwNormalizeModel");
}

void MvlRwAtomicDestroy(void *atomic)
{
	MvlErrorLog("TODO(weap): 0x31380 MvlRwAtomicDestroy");
	(void)atomic;
}

void MvlRwDestroyTexture(void *tex)
{
	MvlErrorLog("TODO(weap): 0x32730 MvlRwDestroyTexture");
	(void)tex;
}

void *MvlPngReadImage(void *)
{
	MvlErrorLog("TODO(weap): 0x1c690 MvlPngReadImage (libpng lane)");
	return 0;
}

void *MvlImageToTexture(void *image)
{
	MvlErrorLog("TODO(weap): 0x1ca90 MvlImageToTexture");
	(void)image;
	return 0;
}

void MvlPngDestroyImage(void *image)
{
	MvlErrorLog("TODO(weap): 0x327c0 MvlPngDestroyImage");
	(void)image;
}

const SMvlShotAudio *MvlGetStockWeaponAudio(int, int)
{
	MvlErrorLog("TODO(weap): 0x54e10 MvlGetStockWeaponAudio (stock audio table)");
	return 0;
}

int MvlLookupWeaponAnimGroup(const char *text)
{
	// FUN_10003090 @ 0x3090 — anim-group name → id (same DAT_006857b0 table
	// the vehicle lane resolves via MvlAnimGroupTableName).
	if(!text)
		return -1;
	for(int i = 0; i < 60; i++){
		const char *n = MvlAnimGroupTableName(i);
		if(!n)
			break;
		if(_stricmp(n, text) == 0)
			return i;
	}
	return -1;
}

void MvlHatTransformRelative(float *, float *)
{
	MvlErrorLog("TODO(weap): 0x8ee0 MvlHatTransformRelative (frame-relative hat fit)");
}

void MvlWeaponRequestModel(int)
{
	MvlErrorLog("TODO(weap): 0x8bc0 MvlWeaponRequestModel");
}

void *MvlWeaponModelGetAtomic(int)
{
	MvlErrorLog("TODO(weap): 0x54110 MvlWeaponModelGetAtomic (model store +0x28)");
	return 0;
}
