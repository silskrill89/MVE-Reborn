// weapon_pickups.h — CWeaponPickups: world weapon-pickup spawn system.
//
// Faithful recompilation from decompiled vehmod_retail.dll 0.98e (decomp:
// .harness-memory/limout/savewpn/range-decomp.txt, 0x57d80-0x58990).
// Original names 1:1 (error-string-confirmed); every decl cites
// "0.98e @ RVA 0x…". Opcode 0810 ("new weapon pickup") is registered by
// FUN_10058270 (code-patch init; the opcode hookup itself is
// FUN_100238a0 @ 0x238a0).
//
// Spawn list: mvl\weapons\pickups.xml root <pickups> → <spawnlist> →
// <spawn> {<wepid>, <position>{x,y,z}, <ammo>}; weapon archives may embed
// their own <spawnlist> (CMvlWeapon::LoadXMLSectionWeapon passes the
// weapon id, which overrides <wepid>).
#pragma once

#include "../core/stdafx.h"

#include <vector>

class TiXmlElement;

// One registered spawn (0x14-byte original record; consumers read +0 = id,
// +4..0xc = position, +0x10 = ammo — FUN_10058330 @ 0x58330).
struct SMvlPickupSpawn
{
	int   nWeaponId;   // @ 0x00 (validated: 0..36 stock or registered)
	float fPosition[3];// @ 0x04
	int   nAmmo;       // @ 0x10 (> 0)
};

// ---------------------------------------------------------------------------
// CWeaponPickups
// ---------------------------------------------------------------------------
class CWeaponPickups
{
public:
	// CWeaponPickups::CreateWeaponPickup — 0.98e @ RVA 0x57d80.
	// Finds a free pickup slot in the game pool, validates the weapon id,
	// fills the 0x34-byte pickup record (DAT_100a6b98 + slot*0x34) and
	// returns (counter<<10)|slot, or 0xffffffff on failure (pool full /
	// no slot / invalid id — verbatim errors).
	static int CreateWeaponPickup(int weaponId, int ammo);

	// CWeaponPickups::ProcessNewWeaponPickupOpcode — 0x98e @ RVA 0x584f0
	// (opcode 0810 handler). Reads (weaponId, ammo) from the opcode stream,
	// validates ("Invalid weapon ID in new weapon pickup opcode."), clamps
	// ammo 1..1000000 and stores the CreateWeaponPickup result back.
	static void ProcessNewWeaponPickupOpcode(void);

	// CWeaponPickups::LoadXMLSectionSpawnList — 0x98e @ RVA 0x58610.
	// <spawnlist>: repeating <spawn> sectors; failures log
	// "Could not load %s section." and continue. weaponId > 0 overrides the
	// parsed wepid (weapon-embedded lists); < 0 uses <wepid> (pickups.xml).
	static bool LoadXMLSectionSpawnList(TiXmlElement *sector, int weaponId);

	// CWeaponPickups::LoadXMLSectionSpawn — 0x98e @ RVA 0x586d0.
	// <wepid> (int), <position> (vector, required), <ammo> (int, required);
	// appends to the spawn list.
	static bool LoadXMLSectionSpawn(TiXmlElement *sector, int weaponId);

	// CWeaponPickups::LoadXMLSectionPickups — 0x98e @ RVA 0x58880.
	// <pickups> root → <spawnlist> sub-sectors (weaponId = -1).
	static bool LoadXMLSectionPickups(TiXmlElement *sector);

	// CWeaponPickups::LoadXMLPickupFile — 0x98e @ RVA 0x58990.
	// Loads mvl\weapons\pickups.xml when it exists (GetFileAttributesA
	// gate); root must be <pickups>; success logs
	// "Successfully loaded weapon pickup XML file."
	static bool LoadXMLPickupFile(void);

	// FUN_10058330 @ 0x58330 — (provisional) CreateAllRegisteredPickups:
	// "%d pickups registered, now creating them." then one
	// CreateWeaponPickup per registered spawn. Called by the game after
	// weapon archives load.
	static void CreateAllRegisteredPickups(void);

	// FUN_10058ac0 @ 0x58ac0 — (reconstructed from the consumers; body is
	// just past the dump boundary 0x58a00) append a spawn record.
	static void AppendSpawn(const SMvlPickupSpawn &spawn);

// Registered spawn list (original MSVC vector DAT_10124500..0x10124504,
// 0x14-byte records).
	static std::vector<SMvlPickupSpawn> s_spawnList;
};

// opcode-0810 spawn-height floor (DAT_1008dfc0; positions at/below get the
// DAT_1008de00 spawn-height bump):
#define MVL_PICKUP_MIN_Z   0.0f   // TODO(weap): DAT_1008df08/DAT_1008dfc0 exact values

// Internal helpers shared with the stubs (see weapon_pickups.cpp):
void MvlPickupSetNextPosition(const float p[3]);
const float *MvlPickupGetNextPosition(void);
int  MvlPickupBumpCounter(int slot);

// ---------------------------------------------------------------------------
// Engine hooks (stubs in weapon_pickups.cpp).
// ---------------------------------------------------------------------------
// Pickup pool (gta-vc.exe): free-object count at *(DAT_0094dbe0+8) with the
// used-flag bytes at *(DAT_0094dbe0+4):
void MvlPickupPoolStats(int *total, int *used);          // TODO(weap): 0x94dbe0
// Slot scans FUN_10057b30 @ 0x57b30 (reuse slot) / FUN_10057b60 @ 0x57b60:
int  MvlPickupFindSlot(void);                            // TODO(weap): 0x57b30/0x57b60
// Pickup object base DAT_100a6b98 (0x34-byte records):
void *MvlPickupRecord(int slot);                         // TODO(weap): 0xa6b98
// FUN_10057c30 @ 0x57c30 — build the in-world pickup object for a record:
void  MvlPickupSetupObject(void *record, int weaponId);  // TODO(weap): 0x57c30
void  MvlPickupSetupRecord(void *record, int weaponId, int ammo); // 0x57d80
// FUN_10033730 @ 0x33730 — audio prepare before object creation:
void  MvlPickupAudioPrepare(void);                       // TODO(weap): 0x33730
// SCM opcode stream (opcode 0810 handler glue):
void  MvlScmOpcodeBegin(void);                           // TODO(weap): 0x2e4c0
int   MvlScmOpcodeReadInt(int argIndex);                 // TODO(weap): 0x2e530
void  MvlScmOpcodeSkip(int count);                       // TODO(weap): 0x2e540
void  MvlScmOpcodeWriteInt(int argIndex, int value);     // TODO(weap): 0x2e560
void  MvlScmOpcodeEnd(void);                             // TODO(weap): 0x2e4b0
// Player-position sample for the opcode spawn (FUN_1000b110 @ 0xb110):
void  MvlPlayerGetPosition(float out[3]);                // TODO(weap): 0xb110
// Opcode registration (FUN_100238a0 @ 0x238a0):
void  MvlScmRegisterOpcode(int opcode, void (*handler)(void)); // TODO(weap): 0x238a0
