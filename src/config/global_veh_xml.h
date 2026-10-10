// global_veh_xml.h — global vehicle XML (mvl\vehicles\globalv.xml).
//
// All names are recovered original symbols (free functions, not CMvl*):
//   vehmod_retail.dll 0.98e, re-098e-map.md §3:
//     LoadGlobalXMLNoSpawn        @ RVA 0x37a90
//     LoadGlobalAISpecials        @ RVA 0x37b80
//     LoadGlobalVehExtraFlags     @ RVA 0x37e70
//     LoadGlobalStockAISpecials   @ RVA 0x380b0
//     LoadGlobalVXMLFile          @ RVA 0x38160
//   idstart validation string "has to be between 250 and
//   MIN(65000, modelLimit - 100)" @ 0x10081260.
#pragma once

#include "../core/stdafx.h"
#include "../../external/tinyxml/tinyxml.h"

#include <vector>

// Parsed global vehicle settings (root <vehdata>).
struct SMvlGlobalVehData
{
	std::vector<int> noSpawnIds;  // <nospawn><v>id</v></nospawn>
	int  idstart;                 // first custom model ID
	bool usespawner;              // 0.96a
	bool usetuner;                // 0.96a
	bool disablelightboxes;       // 0.96c
	bool disableflipdamage;       // 0.97
	bool tuningfix;               // 0.93
	int  maxaltitude;             // 0.98 (NOMAXALTITUDE flag interplay)
	bool dumprgbcolors;           // 0.98
	int  antialias;               // 0.98a ("off"/"on"/numeric)

	// 0.97c stock-vehicle overrides (LoadGlobalStockAISpecials).
	// TODO(schema): exact sub-element layout (ainame/aispecials/setextraflags
	// per stock vehicle) not present in any shipped sample — parsed
	// generically, counts logged only.
	int stockVehicleCount;

	void Reset(void);
};

// LoadGlobalVXMLFile — parse mvl\vehicles\globalv.xml.
// vehmod_retail.dll 0.98e @ RVA 0x38160. Returns true on successful parse.
bool LoadGlobalVXMLFile(void);

// Section parsers (called by LoadGlobalVXMLFile; public for testability).
void LoadGlobalXMLNoSpawn(const TiXmlElement *vehdata, SMvlGlobalVehData &out);      // 0x37a90
void LoadGlobalAISpecials(const TiXmlElement *vehdata, SMvlGlobalVehData &out);      // 0x37b80
void LoadGlobalVehExtraFlags(const TiXmlElement *vehdata, SMvlGlobalVehData &out);   // 0x37e70
void LoadGlobalStockAISpecials(const TiXmlElement *vehdata, SMvlGlobalVehData &out); // 0x380b0

// Access to the parsed data (filled by LoadGlobalVXMLFile).
SMvlGlobalVehData &MvlGlobalVehData(void);
