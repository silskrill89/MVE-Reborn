// vehicle_archive.cpp — CMvlVehicle archive pipeline (see vehicle_archive.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (range-decomp.txt,
// 0x44f20-0x463f0). Log strings are verbatim from the decomp. Engine
// internals that are not yet ported are called through the HOOK POINT
// declarations in vehicle_archive.h (stubs below log and fail cleanly —
// game behaviour is NOT faked).
#include "vehicle_archive.h"

#include "../loader/log.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrappers matching the original three logging helpers:
//   FUN_1001cc00(fmt,...)          -> message log
//   FUN_1001cce0(fn,fmt,...)       -> warning (message log)
//   FUN_1001cb80(fn,fmt,...)       -> error log
// The original prefixes the function name; we keep the message text 1:1.
// ---------------------------------------------------------------------------
static void
MvlVehMsg(const char *fmt, ...)
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
MvlVehWarn(const char *fn, const char *fmt, ...)
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
MvlVehError(const char *fn, const char *fmt, ...)
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

// LoadFromArchive's stage failures throw a string exception caught by its own
// catch filter (Catch@10045318 -> "CMvlVehicle::LoadFromArchive" "%s"). The
// thrown literals are immediate args the decompiler dropped; stage names are
// used (see port-notes-veh.md).
struct SMvlLoadError
{
	char msg[512];
};

static void
MvlThrow(const char *fmt, ...)
{
	SMvlLoadError e;
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(e.msg, sizeof(e.msg), fmt, ap);
	va_end(ap);
	e.msg[sizeof(e.msg) - 1] = '\0';
	throw e;
}

// FUN_1001b970 @ 0x1b970 — case-insensitive "name ends with needle" (the
// RegisterArchiveFile chain matches ".xml"/"tuning1.dff" style needles).
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

// ===========================================================================
// Construction
// ===========================================================================
CMvlVehicle::CMvlVehicle()
{
	memset(this, 0, sizeof(*this));
	m_nCompRules = -1;
	m_nAnimsIndex = -1;
	m_handling.nDriveType = 0;
	m_handling.nEngineType = 0;
}

// ===========================================================================
// FUN_10044fc0 @ RVA 0x44fc0 — (provisional) ResolveAnimIndex
// Resolve m_szAnimsName against the anim-group name table (DAT_0x6857b0,
// 24-byte rows, scan bound 0x685d67 => ~60 entries). "null" keeps -1.
// ===========================================================================
void
CMvlVehicle::ResolveAnimIndex(void)
{
	m_nAnimsIndex = -1;
	if(_stricmp(m_szAnimsName, "null") != 0){
		for(int i = 0; i < 60; i++){   // table bound from the decomp loop
			const char *name = MvlAnimGroupTableName(i);
			if(!name)
				break;
			if(_stricmp(name, m_szAnimsName) == 0){
				m_nAnimsIndex = i;
				return;
			}
		}
	}
}

// ===========================================================================
// FUN_10045030 @ RVA 0x45030 — (provisional) SetDimensionsFromCollision
// When <dimensions> was absent, derive it from the collision bounding box.
// ===========================================================================
void
CMvlVehicle::SetDimensionsFromCollision(void)
{
	SMvlVehicleHandling &h = m_handling;
	if(h.fDimensions[0] <= 0.0f &&
	   (h.fDimensions[1] <= 0.0f || h.fDimensions[2] <= 0.0f)){
		// *( *(m_pInfo) + 0x1c ) -> CColModel*; box max-min at +0x10..0x24.
		// HOOK POINT @ 0x45030 — reads the game CColModel layout directly.
		const float *box = MvlColModelGetBBox(m_pInfo ? m_pInfo->pColModel : 0);
		if(box){
			h.fDimensions[0] = box[3] - box[0];
			h.fDimensions[1] = box[4] - box[1];
			h.fDimensions[2] = box[5] - box[2];
			MvlVehMsg("Dimensions set to %f %f %f.",
			          h.fDimensions[0], h.fDimensions[1], h.fDimensions[2]);
		}
	}
}

// ===========================================================================
// FUN_100450d0 @ RVA 0x450d0 — (provisional) ResolveComprules
// Default m_nCompRules when the XML had no <comprules>: cars take the IDE
// comprules byte (info+0x48) minus 1 (0 stays 0); bikes get 1, others 0.
// ===========================================================================
void
CMvlVehicle::ResolveComprules(void)
{
	if(m_nCompRules == -1){
		if(m_nType == 0){
			unsigned char b = m_pInfo ? m_pInfo->nComprulesSrc : 0;
			if(b == 0){
				m_nCompRules = 0;
				return;
			}
			m_nCompRules = b - 1;
			return;
		}
		m_nCompRules = (m_nType == 5) ? 1 : 0;
	}
}

// ===========================================================================
// FUN_10046370 @ RVA 0x46370 — (provisional) IsSaDff
// Peek the clump chunk version of a DFF buffer. Original expression:
//   (chunkVersion - 0x34004U) < 0x2000U   (true for the SA RW version line).
// ===========================================================================
bool
CMvlVehicle::IsSaDff(const void *data, unsigned size)
{
	void *stream = MvlRwMemoryStreamOpen(data, size);
	if(!stream)
		return false;
	unsigned version = MvlRwStreamFindChunkHeader(stream);
	MvlRwMemoryStreamClose(stream);
	if(version == 0)
		return false;
	return (unsigned)(version - 0x34004u) < 0x2000u;
}

// ===========================================================================
// FUN_100463f0 @ RVA 0x463f0 — (provisional) GetWeaponSoundSlot
// Slots live at m_abSoundSlots[i] (@ 0x2cf8 + i*20); only i in {0,1} and only
// when the slot's first dword is non-zero.
// ===========================================================================
void *
CMvlVehicle::GetWeaponSoundSlot(unsigned i)
{
	if(i > 1)
		return 0;
	void *slot = &m_abSoundSlots[i][0];
	if(*(int *)slot == 0)
		return 0;
	return slot;
}

// ===========================================================================
// CMvlVehicle::RegisterArchiveFile — vehmod_retail.dll 0.98e @ RVA 0x455d0
// Classification chain (fun_1001b970 ends-with tests, exact decomp order):
//   tuning1-9.dff -> weapons.txd -> weapons.dff -> .xml -> .dff -> .txd
//   -> .col -> .ifp -> .wav (sound handler) -> free+ignore.
// Slots 0..3 keep their file name (m_aszFileNames). Ownership: on success
// 'data' (malloc'd by ParseArchive) is owned by m_apFiles; on discard or
// error it is freed here (matches the original FID_conflict__free calls).
// ===========================================================================
bool
CMvlVehicle::RegisterArchiveFile(const char *name, void *data, unsigned size)
{
	int slot = -1;

	if(MvlNameEndsWithIgnoreCase(name, "tuning1.dff"))       slot = MVL_SLOT_TUNING1;
	else if(MvlNameEndsWithIgnoreCase(name, "tuning2.dff"))  slot = MVL_SLOT_TUNING2;
	else if(MvlNameEndsWithIgnoreCase(name, "tuning3.dff"))  slot = MVL_SLOT_TUNING3;
	else if(MvlNameEndsWithIgnoreCase(name, "tuning4.dff"))  slot = MVL_SLOT_TUNING4;
	else if(MvlNameEndsWithIgnoreCase(name, "tuning5.dff"))  slot = MVL_SLOT_TUNING5;
	else if(MvlNameEndsWithIgnoreCase(name, "tuning6.dff"))  slot = MVL_SLOT_TUNING6;
	else if(MvlNameEndsWithIgnoreCase(name, "tuning7.dff"))  slot = MVL_SLOT_TUNING7;
	else if(MvlNameEndsWithIgnoreCase(name, "tuning8.dff"))  slot = MVL_SLOT_TUNING8;
	else if(MvlNameEndsWithIgnoreCase(name, "tuning9.dff"))  slot = MVL_SLOT_TUNING9;
	else if(MvlNameEndsWithIgnoreCase(name, "weapons.txd"))  slot = MVL_SLOT_WEAPONS_TXD;
	else if(MvlNameEndsWithIgnoreCase(name, "weapons.dff"))  slot = MVL_SLOT_WEAPONS_DFF;
	else if(MvlNameEndsWithIgnoreCase(name, ".xml"))         slot = MVL_SLOT_XML;
	else if(MvlNameEndsWithIgnoreCase(name, ".dff"))         slot = MVL_SLOT_DFF;
	else if(MvlNameEndsWithIgnoreCase(name, ".txd"))         slot = MVL_SLOT_TXD;
	else if(MvlNameEndsWithIgnoreCase(name, ".col"))         slot = MVL_SLOT_COL;
	else if(MvlNameEndsWithIgnoreCase(name, ".ifp"))         slot = MVL_SLOT_IFP;
	else if(MvlNameEndsWithIgnoreCase(name, ".wav")){
		// Sound files go straight to the sound handler (CSoundHandler::
		// LoadSoundFile @ 0x33e10, name table PTR_s_rcengine1_100af104,
		// per-vehicle slot block @ 0x2cf8). Failure fails the archive.
		bool ok = MvlSoundLoadFile("rcengine1", &m_abSoundSlots[0][0],
		                           name, data, size);
		if(!ok)
			MvlVehError("CMvlVehicle::RegisterArchiveFile",
			            "Loading sound file %s failed.", name);
		free(data);
		return ok;
	}
	else{
		// Unknown file: freed and ignored (original returns success).
		free(data);
		return true;
	}

	if(size > MVL_MAX_ARCHIVE_FILE_SIZE){
		MvlVehError("CMvlVehicle::RegisterArchiveFile",
		            "File '%s' is larger than 12582912 bytes.", name);
		free(data);
		return false;
	}
	if(m_apFiles[slot] != 0){
		MvlVehError("CMvlVehicle::RegisterArchiveFile",
		            "File '%s' is the second file of this type, bb %08X, dat %08X, i %d.",
		            name, (unsigned)(size_t)m_apFiles[slot], (unsigned)size, slot);
		free(data);
		return false;
	}
	m_apFiles[slot] = data;
	m_anFileSizes[slot] = size;
	if(slot < 4){
		// strncpy 0x104 bytes + forced terminator @ 0x4b7 (decomp).
		strncpy(m_aszFileNames[slot], name, 0x104);
		m_aszFileNames[slot][0x103] = '\0';
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::VerifyRequiredFiles — vehmod_retail.dll 0.98e @ RVA 0x45870
// ===========================================================================
bool
CMvlVehicle::VerifyRequiredFiles(void)
{
	if(m_apFiles[MVL_SLOT_XML] == 0){
		MvlVehError("CMvlVehicle::VerifyRequiredFiles",
		            "Main XML is missing from the archive.");
		return false;
	}
	if(m_apFiles[MVL_SLOT_DFF] == 0){
		MvlVehError("CMvlVehicle::VerifyRequiredFiles",
		            "Main DFF is missing from the archive.");
		return false;
	}
	if(m_apFiles[MVL_SLOT_TXD] == 0){
		MvlVehError("CMvlVehicle::VerifyRequiredFiles",
		            "Main TXD is missing from the archive.");
		return false;
	}
	m_bIsSaModel = IsSaDff(m_apFiles[MVL_SLOT_DFF],
	                       m_anFileSizes[MVL_SLOT_DFF]) ? 1 : 0;
	if(!m_bIsSaModel && m_apFiles[MVL_SLOT_COL] == 0){
		MvlVehError("CMvlVehicle::VerifyRequiredFiles",
		            "Main COL is missing from the archive and DFF model isn't SA version.");
		return false;
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::ParseArchive — vehmod_retail.dll 0.98e @ RVA 0x45360
// CSevenWalker (LoadSeven @ 0x2bf00) does the 7z work. Duplicate archives
// are detected by total uncompressed size (m_nArchiveTotalSize @ 0x330).
// ===========================================================================
bool
CMvlVehicle::ParseArchive(const char *archivePath)
{
	CSevenWalker walker;
	bool ok = true;

	if(!walker.LoadSeven(archivePath)){
		MvlVehError("CMvlVehicle::ParseArchive",
		            "Could not load 7zip archive '%s'.", archivePath);
		return false;
	}

	for(int i = 0; i < MVL_SLOT_COUNT; i++){
		m_apFiles[i] = 0;
		m_anFileSizes[i] = 0;
	}
	for(int i = 0; i < 4; i++)
		m_aszFileNames[i][0] = '\0';
	memset(m_abSoundSlots, 0, sizeof(m_abSoundSlots));

	const UInt32 num = walker.GetNumEntries();
	m_nArchiveTotalSize = 0;
	for(UInt32 i = 0; i < num; i++)
		m_nArchiveTotalSize += (unsigned)walker.GetEntrySize(i);

	// Duplicate check against the already-registered vehicles
	// (FUN_1004f3f0/FUN_1004f380 — vehicle registry, HOOK POINT). The decomp
	// compares id @ 0, identity with `this`, and totalSize @ 0x330.
	int regCount = MvlVehicleRegCount();
	for(int i = 0; i < regCount; i++){
		CMvlVehicle *other = MvlVehicleRegGet(i);
		if(other != 0 && other->m_nId > 0 &&
		   other != this &&
		   other->m_nArchiveTotalSize == m_nArchiveTotalSize){
			MvlVehError("CMvlVehicle::ParseArchive",
			            "Archive '%s' already loaded as '%s'.",
			            archivePath, other->m_szArchivePath);
			ok = false;
			goto done;
		}
	}

	for(UInt32 i = 0; i < num && ok; i++){
		const char *name = walker.GetEntryName(i);
		std::vector<unsigned char> tmp;
		if(!walker.ExtractToMemory(i, tmp)){
			MvlVehError("CMvlVehicle::ParseArchive",
			            "Could not load file '%s' from archive '%s'.",
			            name, archivePath);
			ok = false;
			break;
		}
		// The original mallocs the entry and hands ownership to
		// RegisterArchiveFile; mirror that with an exact-size copy.
		unsigned size = (unsigned)tmp.size();
		void *data = malloc(size ? size : 1);
		if(!data){
			ok = false;
			break;
		}
		if(size)
			memcpy(data, &tmp[0], size);
		if(!RegisterArchiveFile(name, data, size)){
			MvlVehError("CMvlVehicle::ParseArchive",
			            "Could not register file '%s' from archive '%s'.",
			            name, archivePath);
			ok = false;
			break;
		}
	}

	if(ok && !VerifyRequiredFiles()){
		MvlVehError("CMvlVehicle::ParseArchive",
		            "Archive '%s' is not complete.", archivePath);
		ok = false;
	}

done:
	walker.Close();
	return ok;
}

// ===========================================================================
// CMvlVehicle::LoadTextureFile — vehmod_retail.dll 0.98e @ RVA 0x45900
// catch filter @ 0x45a5a ("CMvlVehicle::LoadTextureFile" "%s"); shared
// cleanup tail = FUN_10045a98 (close stream, free the .txd buffer).
// ===========================================================================
bool
CMvlVehicle::LoadTextureFile(void)
{
	void *stream = 0;
	bool ok = true;

	if(m_apFiles[MVL_SLOT_TXD] == 0)
		return true;

	try{
		if(!MvlRecognizeTextureData(m_apFiles[MVL_SLOT_TXD],
		                            m_anFileSizes[MVL_SLOT_TXD]))
			MvlThrow("Failed to recognize texture.");
		stream = MvlRwMemoryStreamOpen(m_apFiles[MVL_SLOT_TXD],
		                               m_anFileSizes[MVL_SLOT_TXD]);
		if(!stream)
			MvlThrow("Could not open memory stream for texture file.");
		if(MvlRwStreamFindChunkHeader(stream) == 0)
			MvlThrow("Could not find texture dictionary chunk header from .txd file.");
		m_pTxd = MvlRwReadTextureDictionary(stream);
		if(!m_pTxd)
			MvlThrow("Loading texture dictionary from .txd file failed.");
	}catch(const SMvlLoadError &e){
		MvlVehError("CMvlVehicle::LoadTextureFile", "%s", e.msg);
		ok = false;
	}

	// FUN_10045a98 tail — both paths free the raw .txd buffer.
	if(stream)
		MvlRwMemoryStreamClose(stream);
	free(m_apFiles[MVL_SLOT_TXD]);
	m_apFiles[MVL_SLOT_TXD] = 0;
	return ok;
}

// ===========================================================================
// CMvlVehicle::LoadModelFile — vehmod_retail.dll 0.98e @ RVA 0x45af0
// catch filter @ 0x45ccb ("CMvlVehicle::LoadModelFile" "%s"); cleanup tail =
// FUN_10045d00 (stream close + free the .dff buffer).
// ===========================================================================
bool
CMvlVehicle::LoadModelFile(void)
{
	void *stream = 0;
	void *clump = 0;
	bool ok = true;

	if(m_pInfo && m_pInfo->nFlags028 != 0)
		return true;   // clump already installed

	MvlRwClumpReadPrep();   // FUN_10032760 @ 0x32760
	if(m_pInfo && m_pInfo->nSpecialModelId != -1){
		// HOOK POINT @ 0x45af0 — FUN_10008b90/FUN_10008b40 special model prep.
		MvlSpecialModelPrep();
	}

	try{
		stream = MvlRwMemoryStreamOpen(m_apFiles[MVL_SLOT_DFF],
		                               m_anFileSizes[MVL_SLOT_DFF]);
		if(!stream)
			MvlThrow("Could not open memory stream for DFF file.");
		if(MvlRwStreamFindChunkHeader(stream) == 0)
			MvlThrow("Could not find clump chunk header from DFF file.");
		MvlRwTexDictReadPrep();   // FUN_10032750 @ 0x32750
		// Globals around the clump read (DAT_100b3f08 isSA / DAT_100b77d0
		// "vehicle being loaded") — HOOK POINT @ 0x45af0.
		MvlSetLoadingContext(this, m_bIsSaModel != 0);
		clump = MvlRwReadClump(stream);
		MvlSetLoadingContext(0, false);
		if(!clump)
			MvlThrow("Loading clump from DFF file failed.");
		MvlRwClumpPostRead(clump);    // FUN_100312c0
		MvlRwClumpSetup(clump);       // FUN_10031ca0
		MvlRwClumpFinalise();         // FUN_10031f10
		if(m_pInfo && m_pInfo->pColModel)
			m_pColModelDerived = MvlColModelGetField0C(m_pInfo->pColModel);
		// virtual slot +0x24 of the model info: install the clump
		// (HOOK POINT @ 0x45af0).
		MvlSetLoadingContext(this, m_bIsSaModel != 0);
		if(m_pInfo)
			MvlVehicleInfoSetClump(m_pInfo, clump);
		MvlSetLoadingContext(0, false);
		MvlProcessLoadedClump(this, clump);   // FUN_1003c700
		MvlRwClumpPostRead(clump);            // FUN_100312c0 (second call)
	}catch(const SMvlLoadError &e){
		MvlVehError("CMvlVehicle::LoadModelFile", "%s", e.msg);
		ok = false;
	}

	// FUN_10045d00 tail.
	MvlRwTexDictReadPrep();   // FUN_10032750
	if(stream)
		MvlRwMemoryStreamClose(stream);
	free(m_apFiles[MVL_SLOT_DFF]);
	m_apFiles[MVL_SLOT_DFF] = 0;
	return ok;
}

// ===========================================================================
// CMvlVehicle::LoadCollisionFile — vehmod_retail.dll 0.98e @ RVA 0x45d60
// catch filter @ 0x45ed7 ("CMvlVehicle::LoadCollisionFile" "%s"); cleanup =
// FUN_10045f0f (free the .col buffer).
// ===========================================================================
bool
CMvlVehicle::LoadCollisionFile(void)
{
	bool ok = true;

	if(m_pInfo && m_pInfo->pColModel != 0)
		return true;   // already have a colmodel

	try{
		if(m_apFiles[MVL_SLOT_COL] == 0)
			MvlThrow("Collision file missing.");
		if(m_anFileSizes[MVL_SLOT_COL] < 0x20)
			MvlThrow("Collision file does not have any entries.");
		const unsigned char *colData =
			(const unsigned char *)m_apFiles[MVL_SLOT_COL];
		if(strncmp((const char *)colData, "COLL", 4) != 0)
			MvlThrow("Collision file is missing signature.");
		unsigned colSize = *(const unsigned *)(colData + 4);
		if(colSize > m_anFileSizes[MVL_SLOT_COL] - 8 || colSize <= 0x43)
			MvlThrow("Invalid colmodel size.");
		void *raw = MvlColModelAlloc();                  // thunk_FUN_1002c240
		void *col = raw ? MvlColModelInit(raw) : 0;      // FUN_1002c290
		MvlColModelReadBuffer(col, colData, m_anFileSizes[MVL_SLOT_COL]); // FUN_1002e610
		if(col){
			// Stamp the vehicle id into the colmodel (+0x31/+0x32, decomp).
			MvlColModelSetVehicleId(col, (unsigned short)m_nId);
		}
		if(m_pInfo){
			m_pInfo->pad020 = 1;   // *(info+0x1b) = 1 — col from file
			m_pInfo->pColModel = col;
		}
		if(col)
			m_pColModelDerived = MvlColModelGetField0C(col);
	}catch(const SMvlLoadError &e){
		MvlVehError("CMvlVehicle::LoadCollisionFile", "%s", e.msg);
		ok = false;
	}

	// FUN_10045f0f tail.
	free(m_apFiles[MVL_SLOT_COL]);
	m_apFiles[MVL_SLOT_COL] = 0;
	return ok;
}

// ===========================================================================
// CMvlVehicle::LoadAnimationFile — vehmod_retail.dll 0.98e @ RVA 0x45f50
// ===========================================================================
bool
CMvlVehicle::LoadAnimationFile(void)
{
	bool ok = true;

	if(m_apFiles[MVL_SLOT_IFP] == 0)
		return true;

	m_pAnimFile = MvlAnimFileNew();   // FUN_10059d7f new + FUN_100012b0 ctor
	if(!MvlAnimFileLoad((CCustomAnimFile *)m_pAnimFile,
	                    m_apFiles[MVL_SLOT_IFP], m_anFileSizes[MVL_SLOT_IFP])){
		MvlVehError("CMvlVehicle::LoadAnimationFile", "Failed to read animation file.");
		MvlAnimFileFree((CCustomAnimFile *)m_pAnimFile);
		m_pAnimFile = 0;
		ok = false;
	}else{
		int count = MvlAnimFilePostProcess((CCustomAnimFile *)m_pAnimFile); // FUN_100027a0
		MvlVehMsg("Loaded %d anims from IFP file for vehicle.", count);
		ok = true;
	}
	free(m_apFiles[MVL_SLOT_IFP]);
	m_apFiles[MVL_SLOT_IFP] = 0;
	return ok;
}

// ===========================================================================
// CMvlVehicle::LoadWeaponFiles — vehmod_retail.dll 0.98e @ RVA 0x46010
// catch filter @ 0x462fe ("CMvlVehicle::LoadWeaponFiles" "%s"; on exception
// the weapons.txd dict is destroyed — FUN_10032710).
// ===========================================================================
bool
CMvlVehicle::LoadWeaponFiles(void)
{
	void *stream = 0;
	bool ok = true;

	try{
		if(m_apFiles[MVL_SLOT_WEAPONS_TXD] != 0){
			if(!MvlRecognizeTextureData(m_apFiles[MVL_SLOT_WEAPONS_TXD],
			                            m_anFileSizes[MVL_SLOT_WEAPONS_TXD]))
				MvlThrow("Failed to recognize texture.");
			stream = MvlRwMemoryStreamOpen(m_apFiles[MVL_SLOT_WEAPONS_TXD],
			                               m_anFileSizes[MVL_SLOT_WEAPONS_TXD]);
			if(!stream)
				MvlThrow("Could not open memory stream for texture file.");
			if(MvlRwStreamFindChunkHeader(stream) == 0)
				MvlThrow("Could not find texture dictionary chunk header from .txd file.");
			m_pWeaponsTxd = MvlRwReadTextureDictionary(stream);
			if(!m_pWeaponsTxd)
				MvlThrow("Loading texture dictionary from .txd file failed.");
			MvlRwMemoryStreamClose(stream);
			stream = 0;
		}
		if(m_apFiles[MVL_SLOT_WEAPONS_DFF] != 0){
			stream = MvlRwMemoryStreamOpen(m_apFiles[MVL_SLOT_WEAPONS_DFF],
			                               m_anFileSizes[MVL_SLOT_WEAPONS_DFF]);
			if(!stream)
				MvlThrow("Could not open memory stream for weapon DFF file.");
			if(MvlRwStreamFindChunkHeader(stream) == 0)
				MvlThrow("Could not find clump chunk header from weapon DFF file.");
			MvlRwClumpReadPrep();   // FUN_10032760
			if(m_pWeaponsTxd)
				MvlRwTexDictReadPrep();   // FUN_10032750
			void *clump = MvlRwReadClump(stream);
			MvlRwTexDictReadPrep();       // FUN_10032750
			if(!clump)
				MvlThrow("Loading clump from weapon DFF file failed.");
			MvlRwClumpPostRead(clump);        // FUN_100312c0
			MvlWeaponClumpRegister(clump);    // FUN_10031450
		}
	}catch(const SMvlLoadError &e){
		// Catch@100462fe: destroy the half-loaded weapons txd.
		if(m_pWeaponsTxd){
			MvlRwDestroyTextureDictionary(m_pWeaponsTxd);
			m_pWeaponsTxd = 0;
		}
		MvlVehError("CMvlVehicle::LoadWeaponFiles", "%s", e.msg);
		ok = false;
	}

	if(stream)
		MvlRwMemoryStreamClose(stream);
	free(m_apFiles[MVL_SLOT_WEAPONS_TXD]);
	m_apFiles[MVL_SLOT_WEAPONS_TXD] = 0;
	free(m_apFiles[MVL_SLOT_WEAPONS_DFF]);
	m_apFiles[MVL_SLOT_WEAPONS_DFF] = 0;
	return ok;
}

// ===========================================================================
// CMvlVehicle::LoadFromArchive — vehmod_retail.dll 0.98e @ RVA 0x45110
// (catch filter / logging at @ 0x45318: "CMvlVehicle::LoadFromArchive" "%s").
// ===========================================================================
void
CMvlVehicle::LoadFromArchive(const char *archivePath, int id)
{
	strncpy(m_szArchivePath, archivePath ? archivePath : "", 0x104);
	m_szArchivePath[0x103] = '\0';
	m_nId = id;
	m_nIdCopy = id;
	if(m_pInfo)
		m_pInfo->nVehicleId = (short)id;

	try{
		if(!ParseArchive(m_szArchivePath))
			MvlThrow("ParseArchive failed");

		// @ 0x2838 pre-default: DAT_1008de28 (1.0f), SA models use
		// DAT_1008de40 — value unrecovered (see port-notes-veh.md).
		m_fBasicUnk10 = m_bIsSaModel ? 2.0f : 1.0f;

		if(!LoadWeaponFiles())
			MvlThrow("LoadWeaponFiles failed");

		if(!LoadXMLFile())
			MvlThrow("LoadXMLFile failed");

		ResolveAnimIndex();

		if(LoadTextureFile()){
			if(!LoadModelFile())
				MvlThrow("LoadModelFile failed");
			if(LoadCollisionFile()){
				// HOOK POINT @ 0x45110 — FUN_10039ec0(this) post-col setup.
				MvlPostColModelSetup(this);
				LoadAnimationFile();   // return not checked (original)
				// Registry writes: *(DAT_100a6314 + id*4) = m_pInfo and
				// *(DAT_100a631c + 8 + id*0x14) = 1 — HOOK POINT @ 0x45110.
				MvlRegisterVehicleSlot(id, m_pInfo);
				InitialiseLoaded();
				MvlVehMsg("ID %d: '%ls' from archive '%s'",
				          m_nId, m_wsName, m_szArchivePath);
				return;
			}
			MvlThrow("LoadCollisionFile failed");
		}
		MvlThrow("LoadTextureFile failed");
	}catch(const SMvlLoadError &e){
		MvlVehError("CMvlVehicle::LoadFromArchive", "%s", e.msg);
	}
}

// ===========================================================================
// CMvlVehicle::InitialiseLoaded — vehmod_retail.dll 0.98e @ RVA 0x44f20
// ===========================================================================
void
CMvlVehicle::InitialiseLoaded(void)
{
	SetDimensionsFromCollision();   // FUN_10045030

	// One-shot warning (DAT_100b3f09 latch, DAT_100b51d0 = sa_vehicle.txd
	// availability — HOOK POINT globals).
	static bool s_saWarnLatch = false;
	if(!s_saWarnLatch && m_bIsSaModel && !MvlSaTexDictAvailable()){
		MvlVehWarn("CMvlVehicle::InitialiseLoaded",
		           "Loaded a San Andreas vehicle model, but SA vehicle texture dictionary was not found.");
		s_saWarnLatch = true;
	}

	if(m_pInfo && m_pInfo->nAiClass != -1)
		MvlHandlerSetAiClass(m_nId, m_pInfo->nAiClass);   // FUN_1004f7e0

	ResolveComprules();             // FUN_100450d0
	MvlSoundNotifyLoaded();         // FUN_100336f0
	MvlHandlerApplyGang(m_nGang, m_nId);   // FUN_1004f820(this+0x2cf4, id)
	MvlEngineRegisterVehicle(m_nId);       // FUN_1003d790
	if(m_pInfo && m_pInfo->nAiClass != -1)
		MvlSoundNotifyAiClass();    // FUN_10032f80
}

// ===========================================================================
// Engine / game hook stubs. Each maps to a specific original VA (see
// vehicle_archive.h). These are HARNESS STUBS: they log "HOOK POINT" and
// return benign values so the pipeline can be exercised; the engine lane
// replaces them with real game/RW behaviour. Nothing is faked here.
// ===========================================================================

int MvlVehicleRegCount(void)
{
	MvlLog("HOOK POINT 0x4f3f0 MvlVehicleRegCount");
	return 0;
}

CMvlVehicle *MvlVehicleRegGet(int)
{
	MvlLog("HOOK POINT 0x4f380 MvlVehicleRegGet");
	return 0;
}

void *MvlRwMemoryStreamOpen(const void *, unsigned)
{
	MvlLog("HOOK POINT 0x320a0 MvlRwMemoryStreamOpen");
	return 0;
}

void MvlRwMemoryStreamClose(void *)
{
	MvlLog("HOOK POINT 0x32090 MvlRwMemoryStreamClose");
}

unsigned MvlRwStreamFindChunkHeader(void *)
{
	MvlLog("HOOK POINT 0x32770 MvlRwStreamFindChunkHeader");
	return 0;
}

void MvlRwClumpReadPrep(void)
{
	MvlLog("HOOK POINT 0x32760 MvlRwClumpReadPrep");
}

void MvlRwTexDictReadPrep(void)
{
	MvlLog("HOOK POINT 0x32750 MvlRwTexDictReadPrep");
}

void *MvlRwReadTextureDictionary(void *)
{
	MvlLog("HOOK POINT 0x31290 MvlRwReadTextureDictionary");
	return 0;
}

void *MvlRwReadClump(void *)
{
	MvlLog("HOOK POINT 0x31670 MvlRwReadClump");
	return 0;
}

void MvlRwClumpPostRead(void *)
{
	MvlLog("HOOK POINT 0x312c0 MvlRwClumpPostRead");
}

void MvlRwClumpSetup(void *)
{
	MvlLog("HOOK POINT 0x31ca0 MvlRwClumpSetup");
}

void MvlRwClumpFinalise(void)
{
	MvlLog("HOOK POINT 0x31f10 MvlRwClumpFinalise");
}

void MvlRwDestroyTextureDictionary(void *)
{
	MvlLog("HOOK POINT 0x32710 MvlRwDestroyTextureDictionary");
}

int MvlRecognizeTextureData(const void *, unsigned)
{
	MvlLog("HOOK POINT 0xd340 MvlRecognizeTextureData");
	return 0;
}

void MvlProcessLoadedClump(void *, void *)
{
	MvlLog("HOOK POINT 0x3c700 MvlProcessLoadedClump");
}

void MvlWeaponClumpRegister(void *)
{
	MvlLog("HOOK POINT 0x31450 MvlWeaponClumpRegister");
}

void *MvlColModelAlloc(void)
{
	MvlLog("HOOK POINT 0x2c240 MvlColModelAlloc");
	return 0;
}

void *MvlColModelInit(void *raw)
{
	MvlLog("HOOK POINT 0x2c290 MvlColModelInit");
	return raw;
}

void MvlColModelReadBuffer(void *, const void *, unsigned)
{
	MvlLog("HOOK POINT 0x2e610 MvlColModelReadBuffer");
}

void MvlColModelSetVehicleId(void *, unsigned short)
{
	MvlLog("HOOK POINT 0x45d60 MvlColModelSetVehicleId (col +0x31/+0x32)");
}

const float *MvlColModelGetBBox(void *col)
{
	MvlLog("HOOK POINT 0x45030 MvlColModelGetBBox (col +0x10..0x24)");
	(void)col;
	return 0;
}

void *MvlColModelGetField0C(void *col)
{
	MvlLog("HOOK POINT 0x2e610 MvlColModelGetField0C (col +0x0c)");
	(void)col;
	return 0;
}

CCustomAnimFile *MvlAnimFileNew(void)
{
	MvlLog("HOOK POINT 0x12b0 MvlAnimFileNew");
	return 0;
}

bool MvlAnimFileLoad(CCustomAnimFile *, const void *, unsigned)
{
	MvlLog("HOOK POINT 0x14e0 MvlAnimFileLoad");
	return false;
}

int MvlAnimFilePostProcess(CCustomAnimFile *)
{
	MvlLog("HOOK POINT 0x27a0 MvlAnimFilePostProcess");
	return 0;
}

void MvlAnimFileFree(CCustomAnimFile *)
{
	MvlLog("HOOK POINT 0x12f0 MvlAnimFileFree");
}

bool MvlSoundLoadFile(const void *, void *, const char *, const void *, unsigned)
{
	MvlLog("HOOK POINT 0x33e10 MvlSoundLoadFile");
	return false;
}

void MvlSoundNotifyLoaded(void)
{
	MvlLog("HOOK POINT 0x336f0 MvlSoundNotifyLoaded");
}

void MvlSoundNotifyAiClass(void)
{
	MvlLog("HOOK POINT 0x32f80 MvlSoundNotifyAiClass");
}

void MvlHandlerSetAiClass(int, int)
{
	MvlLog("HOOK POINT 0x4f7e0 MvlHandlerSetAiClass");
}

void MvlHandlerApplyGang(unsigned, int)
{
	MvlLog("HOOK POINT 0x4f820 MvlHandlerApplyGang");
}

void MvlEngineRegisterVehicle(int)
{
	MvlLog("HOOK POINT 0x3d790 MvlEngineRegisterVehicle");
}

void MvlVehicleSetNameW(wchar_t *dst, const char *src)
{
	// FUN_1004f360 @ 0x4f360 — swprintf(dst, L"%hs", src). Pure string work,
	// no game behaviour involved.
	if(!dst)
		return;
	dst[0] = 0;
	if(src){
		size_t i = 0;
		for(; src[i] && i < 31; i++)
			dst[i] = (wchar_t)(unsigned char)src[i];
		dst[i] = 0;
	}
}

unsigned MvlPackRgb(int r, int g, int b)
{
	// FUN_10032f10 @ 0x32f10 — game RGBA pack (RW D3D8 format).
	return ((unsigned)(r & 0xff) << 16) | ((unsigned)(g & 0xff) << 8) |
	       (unsigned)(b & 0xff);
}

// Game car-colour palette (DAT_0x939228). HARNESS STUB: zeros; the engine
// lane provides the real table (palette lookups land here via LoadXMLCarCol).
const unsigned MvlGameColourTable[256] = { 0 };

void MvlVectorZero(float *v)
{
	if(v){ v[0] = v[1] = v[2] = 0.0f; }
}

void MvlVectorCopy(float *dst, const float *src)
{
	if(dst && src){ dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2]; }
}

void MvlEulerToQuat(float *quat, const float *euler)
{
	// FUN_10008250 @ 0x8250 — euler -> quaternion. HOOK POINT: math helper,
	// engine lane may replace with the game's exact routine.
	MvlLog("HOOK POINT 0x8250 MvlEulerToQuat");
	if(quat && euler){
		quat[0] = euler[0];
		quat[1] = euler[1];
		quat[2] = euler[2];
		quat[3] = 0.0f;
	}
}

void MvlQuatMid(float *dst, const float *a, const float *b)
{
	// FUN_10008590 @ 0x8590 — mid rotation between two quats.
	MvlLog("HOOK POINT 0x8590 MvlQuatMid");
	if(dst && a){
		dst[0] = a[0]; dst[1] = a[1]; dst[2] = a[2]; dst[3] = a[3];
		(void)b;
	}
}

void MvlWeaponMatrixBuild(void *, const float *)
{
	MvlLog("HOOK POINT 0x2e1e0 MvlWeaponMatrixBuild");
}

int MvlLookupAnimGroupIndex(const char *)
{
	MvlLog("HOOK POINT 0x6857b0 MvlLookupAnimGroupIndex (table scan)");
	return -1;
}

const char *MvlAnimGroupTableName(int)
{
	MvlLog("HOOK POINT 0x6857b0 MvlAnimGroupTableName");
	return 0;
}

bool LoadXMLEmLightData(TiXmlElement *, void *)
{
	MvlLog("HOOK POINT 0x4e8f0 CVehicleHandler::LoadXMLEmLightData");
	return false;
}

bool LoadXMLEmLightSingle(TiXmlElement *, void *)
{
	MvlLog("HOOK POINT 0x4edc0 CVehicleHandler::LoadXMLEmLightSingle");
	return false;
}

bool LoadXMLTaxiLight(TiXmlElement *, void *)
{
	MvlLog("HOOK POINT 0x4f090 CVehicleHandler::LoadXMLTaxiLight");
	return false;
}

void MvlSpecialModelPrep(void)
{
	MvlLog("HOOK POINT 0x8b90/0x8b40 MvlSpecialModelPrep");
}

void MvlSetLoadingContext(void *, bool)
{
	MvlLog("HOOK POINT 0x45af0 MvlSetLoadingContext (DAT_100b77d0/DAT_100b3f08)");
}

void MvlVehicleInfoSetClump(void *, void *)
{
	MvlLog("HOOK POINT 0x45af0 MvlVehicleInfoSetClump (vtable +0x24)");
}

void MvlVehicleInfoSetAnimsName(void *, const char *)
{
	MvlLog("HOOK POINT 0x46420 MvlVehicleInfoSetAnimsName (vtable +0x18)");
}

void MvlVehicleInfoPostAnimsName(void *)
{
	MvlLog("HOOK POINT 0x46420 MvlVehicleInfoPostAnimsName (vtable +0x1c)");
}

void MvlPostColModelSetup(void *)
{
	MvlLog("HOOK POINT 0x39ec0 MvlPostColModelSetup");
}

void MvlRegisterVehicleSlot(int, void *)
{
	MvlLog("HOOK POINT 0x45110 MvlRegisterVehicleSlot (DAT_100a6314/0x100a631c)");
}

bool MvlSaTexDictAvailable(void)
{
	MvlLog("HOOK POINT 0x44f20 MvlSaTexDictAvailable (DAT_100b51d0)");
	return true;
}
