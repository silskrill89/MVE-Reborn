// weapon_handler.h — CWeaponHandler: weapon registry, archives, audio XML,
// save-XML load, wlist dump.
//
// Faithful recompilation from decompiled vehmod_retail.dll 0.98e (decomp:
// .harness-memory/limout/savewpn/range-decomp.txt, 0x54850-0x57320 subset).
// Original names 1:1 (error-string-confirmed); every decl cites
// "0.98e @ RVA 0x…". The RTTI census names (re-098e-map.md) are upgraded by
// the decomp strings: WeaponAudio → LoadXMLSectionWeaponAudio, ShotAudio →
// LoadXMLSectionWeaponShotAudio, save-XML split into three fns.
//
// Registry model (original globals):
//   DAT_10117820 — weapon pointer array indexed by ABSOLUTE id (weapons live
//   at [100..1099]; slot = id-100). DAT_101179b0 is the same array's
//   slot-view base (base+100*4). DAT_1011795c = high-water id counter.
//   DAT_10117960..0x101179b0 — 10 player-save slots × (weaponId, ammo).
#pragma once

#include "../core/stdafx.h"
#include "mvl_weapon.h"     // CMvlWeapon, SMvlShotAudio

#include <map>

class TiXmlElement;

#define MVL_WEAPON_SLOT_MIN   100        // first weapon id (slot 0 + 100)
#define MVL_WEAPON_SLOT_MAX   1099       // last  weapon id (slot 999 + 100)
#define MVL_WEAPON_SLOT_COUNT 1000
#define MVL_PLAYER_WEP_SAVE_SLOTS 10
#define MVL_STOCK_WEAPON_ID_MAX   0x24   // 36 — stock ids are 0..36

// ---------------------------------------------------------------------------
// CWeaponHandler
// ---------------------------------------------------------------------------
class CWeaponHandler
{
public:
	// === Audio XML ========================================================

	// CWeaponHandler::LoadXMLSectionWeaponAudio — 0x98e @ RVA 0x54910.
	// <audio>: <gunshot>(req), <reload>(req), <echo> — each a shot-audio
	// block at out[0]/out[1]/out[2] (weapon @ 0x114/0x128/0x13c).
	static bool LoadXMLSectionWeaponAudio(TiXmlElement *sector, SMvlShotAudio *out);

	// CWeaponHandler::LoadXMLSectionWeaponShotAudio — 0x98e @ RVA 0x54ae0.
	// sample/volume/frequency/displacement/voldisplacement/distance/loopcount/
	// nospam → the 0x14-byte SMvlShotAudio; volume+voldisplacement clamp 127;
	// loopcount forced 1 when sample==0.
	static bool LoadXMLSectionWeaponShotAudio(TiXmlElement *sector, SMvlShotAudio *out);

	// === Registry / archives ==============================================

	// CWeaponHandler::GetNewSlotForArchive — 0x98e @ RVA 0x550c0.
	// Slot from the archive file name ("w<N>_" → slot N; FUN_10055040 @
	// 0x55040); in-use/out-of-range names fall back to the first free slot
	// (warns verbatim). Allocates the 0x1fb CMvlWeapon and returns it (id =
	// slot+100), or NULL when all 1000 slots are full.
	static CMvlWeapon *GetNewSlotForArchive(const char *fileName);

	// CWeaponHandler::LoadWeaponArchives — 0x98e @ RVA 0x55140.
	// FindFirstFileA("mvl\\weapons\\*.7z") in two passes (named-slot files
	// first, then unnamed), CMvlWeapon::LoadFromArchive per file, then
	// CWeaponPickups::LoadXMLPickupFile (pickups.xml).
	static void LoadWeaponArchives(void);

	// FUN_10054850 @ 0x54850 — id valid: 0..36 (stock) or a registered id.
	static bool IsValidWeaponId(int id);
	// FUN_10054870 @ 0x54870 — weapon at slot view (NULL when free / >=1000).
	static CMvlWeapon *GetWeaponBySlot(unsigned slot);
	// FUN_10054890 @ 0x54890 — weapon by absolute id (100..1099; validates
	// that the object's own id matches).
	static CMvlWeapon *GetWeaponById(int id);
	// FUN_100548c0 @ 0x548c0 — display name: custom weapon name, else the
	// stock weapon table (engine data → HOOK).
	static const char *GetWeaponDisplayName(int id);
	// FUN_100550a0 @ 0x550a0 — first free slot (0..999) or -1.
	static int FindFirstFreeSlot(void);

	// === Save-XML load ====================================================

	// CWeaponHandler::LoadSaveXmlWeaponInfoIdentity — 0x98e @ RVA 0x56560.
	// <identity>: archivehash (hex) + archivename; matches a loaded weapon
	// via FindWeaponForIdentity; error "No weapon match for hash %08X or
	// archive '%s'." returns 0.
	static CMvlWeapon *LoadSaveXmlWeaponInfoIdentity(TiXmlElement *sector);

	// CWeaponHandler::LoadSaveXmlPlayerWeaponInfo — 0x98e @ RVA 0x56640.
	// <weapon>: <identity>(req) + <ammo> (clamp 1..100000); stores
	// (weaponId, ammo) into the player-save slot for the weapon's
	// weaponslot; warns "Cannot apply weapon %s, slot collision.".
	static bool LoadSaveXmlPlayerWeaponInfo(TiXmlElement *sector);

	// CWeaponHandler::LoadSaveXmlPlayerWeapons — 0x98e @ RVA 0x567a0.
	// <weapon> list under the save's <playerweps> sector.
	static bool LoadSaveXmlPlayerWeapons(TiXmlElement *sector);

	// FUN_10056cb0 @ 0x56cb0 — (provisional) FindWeaponForIdentity: scan all
	// slots by archive totalSize (hash) then by archive path stricmp.
	static CMvlWeapon *FindWeaponForIdentity(void);

	// === Dump / cleanup ===================================================

	// CWeaponHandler::DumpWeaponXML — 0x98e @ RVA 0x57320.
	// Writes mvl\wlist\<name>.xml for the stock weapon table (creates the
	// directory; shift key disables the audio-value compression). Stock
	// table iteration is engine data → HOOK; format strings 1:1.
	static void DumpWeaponXML(void);

	// CWeaponHandler::ApplySaveData — 0x98e @ RVA 0x56db0 — engine save-apply
	// (give weapons to the player); NOT ported this lane (see notes).
	static void ApplySaveDataHooked(void);   // TODO(weap): 0x56db0

	// --- registry state (original globals, see header top) ---
	static CMvlWeapon *s_weapons[MVL_WEAPON_SLOT_COUNT];   // @ DAT_10117820[100..1099]
	static int  s_nMaxWeaponId;                            // @ DAT_1011795c
	// player save slots: [i] = weaponId (@ DAT_10117960+i*8), ammo (@ +4)
	static int  s_anPlayerSaveWeapon[MVL_PLAYER_WEP_SAVE_SLOTS];  // @ DAT_10117960
	static int  s_anPlayerSaveAmmo[MVL_PLAYER_WEP_SAVE_SLOTS];    // @ DAT_10117964
	// save-XML scratch (DAT_101247e4 name, 0x101247e8 hash, 0x101247ec ammo)
	static const char *s_szSaveArchiveName;   // @ DAT_101247e4
	static unsigned    s_nSaveArchiveHash;    // @ DAT_101247e8
	static int         s_nSaveAmmo;           // @ DAT_101247ec
};

// ---------------------------------------------------------------------------
// Free-function entry points referenced by src/vehicle/vehicle_handler.{h,cpp}
// (signatures MUST stay identical to the stubs that lane declared).
// ---------------------------------------------------------------------------
bool CWeaponHandlerLoadSaveXmlPlayerWeapons(TiXmlElement *sector); // 0x98e @ RVA 0x567a0
void CWeaponHandlerClearSaveData(void);                            // 0x98e @ RVA 0x56d40

// Engine hooks for the handler lane (stubs in weapon_handler.cpp):
// stock weapon info table (100-byte records @ gta-vc/MVL .data 0x782a14):
int   MvlStockWeaponTableCount(void);                    // TODO(weap): 0x782a14/0x783890
const char *MvlStockWeaponTableName(int index);          // TODO(weap): DAT_0069d99c
int   MvlStockWeaponLogicalId(int index);                // TODO(weap): rec+0x34
const char *MvlStockWeaponAnimgroupName(int index);      // TODO(weap): DAT_006857b0 row
bool  MvlStockWeaponFillProperties(int index, CMvlWeapon *out); // TODO(weap): 0x782a14 rec
// FUN_1000cbb0 @ 0xcbb0 — insert the weapon into the game's weapon store.
void  MvlGameRegisterWeapon(int id, CMvlWeapon *w); // TODO(weap): 0xcbb0
