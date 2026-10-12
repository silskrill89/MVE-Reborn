// mvl_weapon.h — CMvlWeapon: custom weapon loaded from mvl\weapons\*.7z.
//
// Faithful recompilation from decompiled vehmod_retail.dll 0.98e (decomp:
// .harness-memory/limout/savewpn/range-decomp.txt, weapons range 0x52150-0x54a00).
// Original names 1:1 (error-string-confirmed); every decl cites
// "0.98e @ RVA 0x…". The RTTI census names (re-098e-map.md) are upgraded by
// the decomp's own error strings: SectionWeapon → LoadXMLSectionWeapon etc.
//
// Weapon archive: any .7z in mvl\weapons\ with one .xml (root <weapon>),
// optional .dff/.txd/.ifp/icon.png/scope.png(.full/stretch)/.wav — see
// RegisterArchiveFile. Element names are1:1 with the decomp + the sample
// XMLs in original_files/maxovehmod098e/mvl/help/samplewepxmls/.
#pragma once

#include "../core/stdafx.h"

#include <string>
#include <vector>

class TiXmlElement;
class CCustomAnimFile;     // src/anim/custom_anim.h (same loader as vehicles) — must be 'class' for MSVC mangling match

// ---------------------------------------------------------------------------
// One shot-audio descriptor — output of CWeaponHandler::
// LoadXMLSectionWeaponShotAudio (0.98e @ RVA 0x54ae0). 0x14 bytes; layout
// recovered from the decomp output stores + DumpWeaponXML field reads.
// ---------------------------------------------------------------------------
struct SMvlShotAudio
{
	unsigned nSample;              // @ 0x00 sample (%u)
	unsigned nFrequency;           // @ 0x04 frequency (%u, default 0)
	unsigned nDisplacement;        // @ 0x08 displacement (%u, default 1)
	float    fDistance;            // @ 0x0c distance (float, default 120.0)
	unsigned char nLoopCount;      // @ 0x10 loopcount (default 1; forced 1 when sample==0)
	unsigned char nVolume;         // @ 0x11 volume (clamp 0..127)
	unsigned char nVolDisplacement;// @ 0x12 voldisplacement (vol+this clamp 0..127)
	unsigned char bNoSpam;         // @ 0x13 nospam (default true)
};

// Weapon types (LoadXMLSectionProperties @ 0x53620):
enum EMvlWeaponType
{
	MVL_WEP_MELEE        = 0,
	MVL_WEP_INSTANT_HIT  = 1,
	MVL_WEP_PROJECTILE   = 2,
	MVL_WEP_AREA_EFFECT  = 3,
	MVL_WEP_CAMERA       = 4,
	MVL_WEP_OTHER        = 5
};

// ---------------------------------------------------------------------------
// CWeaponLogic hierarchy. Original is vtable-based (CWeaponLogic::vftable /
// CWeaponLogicSpike::vftable / CWeaponLogicHat::vftable confirmed in the
// decomp). Virtual slots used by the pipeline:
//   +0x10 GetLogicalId (FUN_10054220), +0x14 (FUN_10054230, returns 1),
//   +0x18 GetXmlBlockName (FUN_10054240 → ""; Hat → "hatinfo" @ 0x54390),
//   +0x1c ParseXmlBlock (FUN_10054250 → no-op; Hat → 0x543a0).
// ---------------------------------------------------------------------------
class CWeaponLogic
{
public:
	CWeaponLogic(void *weapon) : m_pWeapon(weapon) {}        // base ctor 0x541c0
	virtual ~CWeaponLogic() {}
	virtual int  GetLogicalId(void);                          // 0x98e @ RVA 0x54220
	virtual int  GetUnk14(void) { return 1; }                 // 0x98e @ RVA 0x54230
	virtual const char *GetXmlBlockName(void) { return ""; }  // 0x98e @ RVA 0x54240
	virtual bool ParseXmlBlock(TiXmlElement *e) { return false; } // 0x98e @ RVA 0x54250
protected:
	void *m_pWeapon;   // @ +0x04 (owner CMvlWeapon*)
};

// CWeaponLogicSpike — logicalid 80; ctor FUN_100545c0 @ 0x545c0 (8-byte
// object: vtable only). No XML block.
class CWeaponLogicSpike : public CWeaponLogic
{
public:
	CWeaponLogicSpike(void *weapon) : CWeaponLogic(weapon) {}   // 0x98e @ RVA 0x545c0
};

// CWeaponLogicHat — logicalid 81; ctor FUN_10054260 @ 0x54260 (0x24 bytes).
// <hatinfo> block: isrelative / offset / rotation / scale (see hats/readme.txt).
class CWeaponLogicHat : public CWeaponLogic
{
public:
	CWeaponLogicHat(void *weapon);                             // 0x98e @ RVA 0x54260
	const char *GetXmlBlockName(void) { return "hatinfo"; }    // 0x98e @ RVA 0x54390
	bool ParseXmlBlock(TiXmlElement *sector);                  // 0x98e @ RVA 0x543a0

	float fScale;            // @ +0x08 (default 1.0f)
	float fOffset[3];        // @ +0x0c
	float fRotation[3];      // @ +0x18
	bool  bIsRelative;       // table default true; drives the frame transform
};

// ---------------------------------------------------------------------------
// CMvlWeapon — one custom weapon (0x1fb-byte original object; offsets cited).
// ---------------------------------------------------------------------------
class CMvlWeapon
{
public:
	CMvlWeapon();

	// === Archive pipeline ===============================================

	// CMvlWeapon::LoadFromArchive — 0.98e @ RVA 0x52150 (catch @ 0x5235a,
	// "CMvlWeapon::LoadFromArchive"). Stage order: ParseArchive → LoadXMLFile
	// → LoadIconFile → LoadScopeFile → LoadTextureFile → LoadModelFile →
	// LoadAnimFile → fill empty audio slots from the stock weapon →
	// "Successfully loaded weapon %d from file %s". Throws the same
	// "%s" string-exception family as the vehicle pipeline; the catch logs
	// under the function name and returns false (the caller in
	// LoadWeaponArchives checks the bool return — decomp cVar1).
	// id = slot+100 weapon id.
	bool LoadFromArchive(const char *archivePath, int id);

	// CMvlWeapon::ParseArchive — 0.98e @ RVA 0x523b0.
	// CSevenWalker 7z walk; sums entry sizes into m_nArchiveTotalSize @ 0x190
	// (save-identity hash); each entry → RegisterArchiveFile.
	bool ParseArchive(const char *archivePath);

	// CMvlWeapon::RegisterArchiveFile — 0.98e @ RVA 0x52540.
	// Extension chain (FUN_1001b970 ends-with): .xml/.dff/.txd (single each,
	// "Another … file is already registered."), .wav → sound handler slot,
	// icon.png (single), scope.png|full_scope.png|stretch_scope.png (single
	// scope slot + flavor flags @ 0xf8/0xf9), .ifp (single). Unmatched files
	// are freed and ignored. Buffer ownership transfers on success.
	bool RegisterArchiveFile(const char *name, void *data, unsigned size);

	// CMvlWeapon::LoadTextureFile — 0.98e @ RVA 0x52780 (catch @ 0x528d1).
	// .txd → RwTexDictionary @ 0x108; frees the raw buffer (tail 0x5290f).
	bool LoadTextureFile(void);

	// CMvlWeapon::LoadModelFile — 0.98e @ RVA 0x52960 (catch @ 0x52b2b).
	// .dff → clump; first atomic stored @ 0x10c ("Clump contains no atomics.");
	// modelindex/model2index reset to -1 (tail 0x52b75 frees the buffer).
	bool LoadModelFile(void);

	// CMvlWeapon::LoadIconFile — 0.98e @ RVA 0x52dc0. icon.png → texture @ 0xf0.
	bool LoadIconFile(void);

	// CMvlWeapon::LoadScopeFile — 0.98e @ RVA 0x52e00. scope.png → texture @ 0xf4.
	bool LoadScopeFile(void);

	// CMvlWeapon::LoadAnimFile — 0.98e @ RVA 0x52e40. .ifp → CCustomAnimFile
	// @ 0x110 ("Loaded %d anims from IFP file for weapon."); frees the ifp buffer.
	bool LoadAnimFile(void);

	// CMvlWeapon::LoadXMLFile — 0.98e @ RVA 0x52f20. TinyXML parse of the
	// in-memory .xml; root must be <weapon>; frees the xml buffer.
	bool LoadXMLFile(void);

	// === XML sections ====================================================

	// CMvlWeapon::LoadXMLSectionWeapon — 0.98e @ RVA 0x53060.
	// Sectors: basic(req), properties(req), spawnlist (→ CWeaponPickups with
	// this weapon's id), audio (→ CWeaponHandler audio), shotgun, and the
	// logic block (<hatinfo> for hats, name from the logic vtable+0x18).
	bool LoadXMLSectionWeapon(TiXmlElement *root);

	// CMvlWeapon::LoadXMLSectionBasic — 0.98e @ RVA 0x53390.
	// <name> (→ name @ 0x150; empty → warn "Weapon has no name." +
	// "Unknown"), <logicalid> (→ 0x88; 80 → CWeaponLogicSpike, 81 →
	// CWeaponLogicHat; other >35 clamped to 0).
	bool LoadXMLSectionBasic(TiXmlElement *sector);

	// CMvlWeapon::LoadXMLSectionProperties — 0x98e @ RVA 0x53620.
	// All <properties> fields 1:1; anim values divided by the frame-scale
	// constant after the required-check; weaponslot clamped 0..9.
	bool LoadXMLSectionProperties(TiXmlElement *sector);

	// CMvlWeapon::LoadXMLSectionShotgun — 0x98e @ RVA 0x53cd0.
	// <fragments> (clamp 1..100), <spread>; sets shotgun flag @ 0xfa.
	bool LoadXMLSectionShotgun(TiXmlElement *sector);

	// === Accessors (FUN_10054010..0x541b0, all 0x98e) ====================
	void  *GetSoundSlot(unsigned i);      // 0x54010 — @ 0x30 + i*20, NULL if empty
	int    GetSlotIndex(void) const       // 0x54030 — @ 0x80
	       { return m_nSlotIndex; }
	int    GetWeaponId(void) const        // 0x54090 — @ 0x84
	       { return m_nWeaponId; }
	int    GetWeaponSlot(void) const      // @ 0xec (save-slot 0..9, clamp per ctor)
	       { return m_nWeaponSlot; }
	int    GetLogicalId(void) const       // 0x540a0 — @ 0x88
	       { return m_nLogicalId; }
	const char *GetName(void) const       // 0x54150 — @ 0x150
	       { return m_szName; }
	const char *GetArchivePath(void) const // 0x54160 — @ 0x194
	       { return m_szArchivePath; }
	unsigned GetArchiveTotalSize(void) const // 0x54170 — @ 0x190 (save hash)
	       { return m_nArchiveTotalSize; }
	SMvlShotAudio *GetShotAudio(unsigned i); // 0x540d0 — @ 0x114 + i*20
	CWeaponLogic *GetLogic(void) const    // 0x54100 — @ 0x104
	       { return m_pLogic; }

	// FUN_10053e10 @ 0x53e10 — properties defaults (type=instant_hit, anim
	// group 7=unarmed, modelindex/model2index -1, flags 0x3b, weaponslot 0).
	void ResetPropertiesDefaults(void);
	// FUN_10053f10 @ 0x53f10 — full release (buffers, txd/atomic/textures,
	// logic dtor, name, then ResetPropertiesDefaults).
	void Release(void);

	// --- data members (original offsets in comments) ---
	void        *m_pXmlBuf;             // @ 0x00
	void        *m_pDffBuf;             // @ 0x04
	void        *m_pTxdBuf;             // @ 0x08
	void        *m_pIconBuf;            // @ 0x0c
	void        *m_pIfpBuf;             // @ 0x10
	void        *m_pScopeBuf;           // @ 0x14
	unsigned     m_anBufSizes[6];       // @ 0x18 (xml,dff,txd,icon,ifp,scope)
	unsigned char m_abSoundSlots[4][20];// @ 0x30 (.wav sound slots, LoadSoundFile)
	int          m_nSlotIndex;          // @ 0x80 (0..999)
	int          m_nWeaponId;           // @ 0x84 (slot+100)
	int          m_nLogicalId;          // @ 0x88
	// properties block @ 0x8c .. 0xec (LoadXMLSectionProperties)
	int          m_nType;               // @ 0x8c EMvlWeaponType
	float        fRange;                // @ 0x90
	unsigned     nFireRate;             // @ 0x94
	unsigned     nReload;               // @ 0x98
	unsigned     nClipSize;             // @ 0x9c
	unsigned     nDamage;               // @ 0xa0
	float        fSpeed;                // @ 0xa4
	float        fRadius;               // @ 0xa8
	float        fLifespan;             // @ 0xac
	float        fSpread;               // @ 0xb0
	float        fOffset[3];            // @ 0xb4
	int          nAnimGroup;            // @ 0xc0 (table lookup; -1 → 7 unarmed)
	float        fAnimStart;            // @ 0xc4
	float        fAnimEnd;              // @ 0xc8
	float        fAnimFire;             // @ 0xcc
	float        fAnim2Start;           // @ 0xd0
	float        fAnim2End;             // @ 0xd4
	float        fAnim2Fire;            // @ 0xd8
	float        fAnimBreak;            // @ 0xdc
	int          m_nModelIndex;         // @ 0xe0 (-1 = none)
	int          m_nModelIndex2;        // @ 0xe4 (-1 = none)
	unsigned     m_nFlags;              // @ 0xe8 ('X' hex, default 0x3b)
	int          m_nWeaponSlot;         // @ 0xec (clamp 0..9)
	// textures / shotgun / logic / model @ 0xf0..0x110
	void        *m_pIconTex;            // @ 0xf0
	void        *m_pScopeTex;           // @ 0xf4
	bool         m_bFullScope;          // @ 0xf8 (full_scope.png)
	bool         m_bStretchScope;       // @ 0xf9 (stretch_scope.png)
	bool         m_bShotgun;            // @ 0xfa (set by <shotgun>)
	float        fShotgunSpread;        // @ 0xfc
	int          nShotgunFragments;     // @ 0x100 (clamp 1..100)
	CWeaponLogic *m_pLogic;             // @ 0x104 (CWeaponLogicSpike/Hat or NULL)
	void        *m_pTxd;                // @ 0x108 (RwTexDictionary)
	void        *m_pAtomic;             // @ 0x10c (first atomic of the clump)
	CCustomAnimFile *m_pAnimFile;        // @ 0x110
	SMvlShotAudio m_aAudio[3];          // @ 0x114 gunshot, 0x128 reload, 0x13c echo
	char         m_szName[0x40];        // @ 0x150
	unsigned     m_nArchiveTotalSize;   // @ 0x190 (save-identity hash)
	char         m_szArchivePath[100];  // @ 0x194 (null @ 0x1f7)
	unsigned char m_abExtra[3];         // @ 0x1f8 (FUN_10054180 tail buffer)
};

// --- CMvlWeapon static helpers (0x98e) ---
// FUN_10052be0 @ 0x52be0 — PNG buffer → RwTexture (icon/scope path).
bool MvlWeaponLoadImageTexture(void **buf, unsigned size, void **outTex);
// FUN_10054110 @ 0x54110 — model atomic: request model when modelindex != -1.
void *MvlWeaponGetModelAtomic(const CMvlWeapon *w);

// ---------------------------------------------------------------------------
// Weapon-lane engine hooks (stubs in mvl_weapon.cpp log + carry TODO VAs).
// Shared RW/anim/sound hooks are reused from ../archive/vehicle_archive.h
// (same original routines: stream open/close, chunk find, clump read,
// tex-dict read, anim files, sound handler, 7z walker).
// ---------------------------------------------------------------------------
void *MvlRwClumpGetFirstAtomic(void *clump);      // FUN_10031260 @ 0x31260
void  MvlRwClumpBindAtomic(void *clump, void *atomic); // FUN_10031470 @ 0x31470
void  MvlRwAtomicFinalise(void);                  // FUN_10031eb0 @ 0x31eb0
void  MvlRwAtomicSetup(void *atomic);             // FUN_10031ce0 @ 0x31ce0
void  MvlRwNormalizeModel(void);                  // FUN_1002eb80 @ 0x2eb80
void *MvlPngReadImage(void *stream);              // FUN_1001c690 @ 0x1c690 (RtPNGImageStreamRead lineage)
void *MvlImageToTexture(void *image);             // FUN_1001ca90 @ 0x1ca90
void  MvlPngDestroyImage(void *image);            // FUN_100327c0 @ 0x327c0
void  MvlRwAtomicDestroy(void *atomic);           // FUN_10031380 @ 0x31380
void  MvlRwDestroyTexture(void *tex);             // FUN_10032730 @ 0x32730
// FUN_10054e10 @ 0x54e10 — stock-weapon audio table (gunshot/reload/echo for
// a logical id); NULL when the weapon has no stock audio entry.
const SMvlShotAudio *MvlGetStockWeaponAudio(int logicalId, int index);
// FUN_10003090 @ 0x3090 — anim-group name → id (-1 unknown); same
// DAT_006857b0 table as the vehicle lane (MvlAnimGroupTableName).
int  MvlLookupWeaponAnimGroup(const char *text);
// FUN_10008ee0 @ 0x8ee0 — frame-relative hat fit transform.
void MvlHatTransformRelative(float *offset, float *rotation);
// FUN_10008bc0 @ 0x8bc0 + model-store lookup (DAT_100a6314[id*4]->+0x28).
void  MvlWeaponRequestModel(int modelIndex);
void *MvlWeaponModelGetAtomic(int modelIndex);
