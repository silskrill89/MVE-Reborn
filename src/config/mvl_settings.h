// mvl_settings.h — CMvlSettings: global config + limits engine.
//
// All method names are recovered original symbols (naming policy, BINDING):
//   vehmod_retail.dll 0.98e, RTTI/string-derived names, re-098e-map.md §3:
//     LoadMXMLSectionInternal          @ RVA 0x2a7f0
//     LoadMXMLSectionLimitsInstances   @ RVA 0x2ac20
//     LoadMXMLSectionLimitsModelStore  @ RVA 0x2ae60
//     LoadMXMLSectionLimits            @ RVA 0x2b030
//     LoadMXMLSectionMiscData          @ RVA 0x2b230
//     LoadGlobalMXMLFile               @ RVA 0x2b390
//     ProcessPoolLimits                @ RVA 0x2b4d0
//     ProcessModelStoreLimits          @ RVA 0x2b5a0
//     ProcessIdLimits                  @ RVA 0x2b8f0
//     ProcessStreamingMemLimit         @ RVA 0x2ba50
//     PrepareMvlForLoad                @ RVA 0x2baf0
//
// Scope note (lane C): these functions VALIDATE + STORE parsed values and log
// them. Engine patching (pool/limit expansion, 0.98b feature) comes later —
// each Process* function has a clearly-marked HOOK POINT.
#pragma once

#include "../core/stdafx.h"
#include "../../external/tinyxml/tinyxml.h"

// Parsed settings storage. Field names mirror the XML element names 1:1.
struct SMvlSettingsData
{
	// <internal> (LoadMXMLSectionInternal, 0.98a+ windowed-mode cluster)
	bool windowed;
	bool resizable;
	bool usemenus;
	bool fixframewait;
	bool menuonrestore;
	bool rtshadowonplanes;
	bool rtshadowonplayer;
	bool autointeriors;   // 0.98d
	bool testmenus;       // 0.98e
	bool trashbin;
	bool decapitations;
	bool roadthingy;
	bool debugidedata;    // 0.98e
	int  antialias;       // "off"/"on"/numeric MS level; 0 = off
	int  autoloadsave;    // sample_globalm.xml uses "0"; slot index or bool

	// miscdata-level extras (LoadMXMLSectionMiscData)
	bool noloadfade;      // 0.98a
	bool pedhead;         // 0.98a
	bool custominit;      // 0.98d

	// <limits> (LoadMXMLSectionLimits)
	int models;
	int texdicts;
	int colfiles;
	int streamingmem;     // MB, 0.98e clamp 65-1900

	// <limits><instances> (LoadMXMLSectionLimitsInstances / ProcessPoolLimits)
	int ptrnode;
	int entryinfonode;
	int peds;
	int vehicles;
	int buildings;
	int treadables;
	int objects;
	int dummys;
	int audioscriptobj;
	int colmodel;

	// <limits><modelstore> (LoadMXMLSectionLimitsModelStore)
	int msSimple;
	int msTime;
	int msWeapon;
	int msClump;
	int msVehicle;
	int msPed;
	int msFx2d;

	void Reset(void);
};

class CMvlSettings
{
public:
	static CMvlSettings &Get(void);

	SMvlSettingsData &Data(void)       { return m_data; }
	const SMvlSettingsData &Data(void) const { return m_data; }

	// LoadGlobalMXMLFile — parse mvl\globalm.xml (root <miscdata>).
	// vehmod_retail.dll 0.98e @ RVA 0x2b390.
	bool LoadGlobalMXMLFile(void);

	// Section parsers (called by LoadGlobalMXMLFile; public for testability).
	// All take the parent element; missing elements are a no-op.
	void LoadMXMLSectionInternal(const TiXmlElement *miscdata);          // 0x2a7f0
	void LoadMXMLSectionLimitsInstances(const TiXmlElement *limits);     // 0x2ac20
	void LoadMXMLSectionLimitsModelStore(const TiXmlElement *limits);    // 0x2ae60
	void LoadMXMLSectionLimits(const TiXmlElement *miscdata);            // 0x2b030
	void LoadMXMLSectionMiscData(const TiXmlElement *miscdata);          // 0x2b230

	// Validation + engine-patch entry points. Currently validate + store +
	// log; the actual VC 1.0 engine patching is TODO (HOOK POINT markers in
	// mvl_settings.cpp). Original RVAs: 0x2b4d0 / 0x2b5a0 / 0x2b8f0 / 0x2ba50.
	void ProcessPoolLimits(void);
	void ProcessModelStoreLimits(void);
	void ProcessIdLimits(void);
	void ProcessStreamingMemLimit(void);

	// idstart validation rule recovered from 0.98e: "has to be between 250
	// and MIN(65000, modelLimit - 100)" (string @ 0x10081260). Returns the
	// clamped/validated value or -1 if invalid.
	int ValidateIdStart(int idstart) const;

private:
	CMvlSettings(void) { m_data.Reset(); }
	CMvlSettings(const CMvlSettings &);            // non-copyable
	CMvlSettings &operator=(const CMvlSettings &);

	SMvlSettingsData m_data;
};
