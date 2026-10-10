// vehicle_archive.h — CMvlVehicle archive-parsing layer (7z side).
//
// Symbol names recovered from vehmod_retail.dll 0.98e RTTI/error strings
// (.serena/memories/re-098e-map.md §3 "Vehicle archive pipeline"):
//   CMvlVehicle::LoadFromArchive      @ RVA 0x45318
//   CMvlVehicle::ParseArchive         @ RVA 0x45360
//   CMvlVehicle::RegisterArchiveFile  @ RVA 0x455d0 (tuning1-9.dff, weapons.txd/dff)
//   CMvlVehicle::VerifyRequiredFiles  @ RVA 0x45870
//   CMvlVehicle::LoadTextureFile      @ RVA 0x45a5a
//   CMvlVehicle::LoadModelFile        @ RVA 0x45ccb
//   CMvlVehicle::LoadCollisionFile    @ RVA 0x45ed7
//   CMvlVehicle::LoadAnimationFile    @ RVA 0x45f50
//   CMvlVehicle::LoadWeaponFiles      @ RVA 0x462fe
//
// This lane implements the archive-content stage only: walk the 7z, classify
// entries by extension, extract buffers. RW/D3D consumption (model/texture/
// collision/anim loading) is a clearly-marked TODO for later stages.
#pragma once

#include "../core/stdafx.h"
#include "seven_walker.h"

#include <string>
#include <vector>

// One classified archive entry with its extracted bytes.
struct SMvlArchiveEntry
{
	std::string name;                    // UTF-8 name as stored in the 7z
	std::vector<unsigned char> data;     // extracted file contents
};

// Archive content grouped by the classification the original applies in
// ParseArchive/RegisterArchiveFile (0.98e @ 0x45360/0x455d0).
struct SMvlArchiveContent
{
	SMvlArchiveEntry xml;                // the single vehicle XML (any filename)
	bool hasXml;
	SMvlArchiveEntry model;              // main .dff
	bool hasModel;
	SMvlArchiveEntry texture;            // main .txd
	bool hasTexture;
	SMvlArchiveEntry collision;          // .col
	bool hasCollision;
	SMvlArchiveEntry animation;          // .ifp
	bool hasAnimation;
	SMvlArchiveEntry tuning[9];          // tuning1.dff .. tuning9.dff
	bool hasTuning[9];
	SMvlArchiveEntry weaponsTxd;         // weapons.txd
	bool hasWeaponsTxd;
	SMvlArchiveEntry weaponsDff;         // weapons.dff
	bool hasWeaponsDff;
	std::vector<SMvlArchiveEntry> images;// .png (icon.png etc.)

	void Clear(void);
};

class CMvlVehicle
{
public:
	CMvlVehicle();

	// ParseArchive — vehmod_retail.dll 0.98e @ RVA 0x45360.
	// Walks the already-opened archive, locates the single .xml (any
	// filename), classifies the remaining entries by extension and extracts
	// everything into content. Returns true when the archive parsed far
	// enough for VerifyRequiredFiles to be meaningful.
	bool ParseArchive(CSevenWalker &walker, SMvlArchiveContent &content);

	// VerifyRequiredFiles — vehmod_retail.dll 0.98e @ RVA 0x45870.
	// The vehicle XML is mandatory; every other part is optional and only
	// logged when present. Returns false (and error-logs) when the XML is
	// missing.
	bool VerifyRequiredFiles(const SMvlArchiveContent &content, const char *archiveName);

private:
	// RegisterArchiveFile — vehmod_retail.dll 0.98e @ RVA 0x455d0.
	// Special-name handling: tuning1-9.dff, weapons.txd, weapons.dff.
	// Returns true when the entry was consumed as a special-name file.
	bool RegisterArchiveFile(const char *name, const std::vector<unsigned char> &data,
	                         SMvlArchiveContent &content);

	// TODO(hook) LoadTextureFile   @ 0x45a5a — feed content.texture into the RW texture stage
	// TODO(hook) LoadModelFile     @ 0x45ccb — feed content.model into the RW DFF stage
	// TODO(hook) LoadCollisionFile @ 0x45ed7 — feed content.collision into the COL stage
	// TODO(hook) LoadAnimationFile @ 0x45f50 — feed content.animation into CCustomAnimFile
	// TODO(hook) LoadWeaponFiles   @ 0x462fe — feed weapons.txd/dff into the weapon stage
};
