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

// Typed limits blocks. Member order mirrors the original storage arrays so the
// engine-patch lane can walk them with plain pointer arithmetic, exactly like
// the decompiled loops (vehmod_retail.dll 0.98e, RVAs cited per struct).

// <limits><instances> — pool instance counts. Original .data array @ VA
// 0x100b1794..0x100b17b8 (10 ints), parser @ RVA 0x2ac20.
struct SMvlLimitsInstances
{
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
};

// <limits><modelstore> — model-store counts. Original .data array @ VA
// 0x100b17c0..0x100b17d8 (7 ints), parser @ RVA 0x2ae60.
struct SMvlLimitsModelStore
{
	int simple;
	int time;
	int weapon;
	int clump;
	int vehicle;
	int ped;
	int fx2d;
};

// <limits> — direct ID limits. Original .data globals: models @ VA 0x100b17e4,
// texdicts @ VA 0x100b17e0, colfiles @ VA 0x100b17bc; parser @ RVA 0x2b030.
struct SMvlLimitsIds
{
	int models;
	int texdicts;
	int colfiles;
};

// <limits><streamingmem> — streaming memory limit in MB. Original global @ VA
// 0x100a6a8c (int), processor @ RVA 0x2ba50. -1 = not configured (the original
// returns early when the value is 0xFFFFFFFF).
struct SMvlLimitsStreamingMem
{
	int mb;
};

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
	SMvlLimitsIds ids;                    // models / texdicts / colfiles
	SMvlLimitsStreamingMem streamingmem;  // MB; -1 = unset

	// <limits><instances> (LoadMXMLSectionLimitsInstances / ProcessPoolLimits)
	SMvlLimitsInstances instances;

	// <limits><modelstore> (LoadMXMLSectionLimitsModelStore)
	SMvlLimitsModelStore modelstore;

	// Processor state (original .data globals).
	bool idLimitsDirty;    // DAT_100b178f — a model/TXD/COL limit was requested
	bool longModelIdMode;  // DAT_100b0c41 — experimental long model ID mode
	// Current engine ID boundaries (original globals @ VA 0x100a6320 model end,
	// 0x100a6328 TXD end, 0x100a6318 COL end). Engine-derived; stay 0 in the
	// clone until the engine-init lane computes them (TODO(engine)).
	int curModelLimit;
	int curTxdLimit;
	int curColLimit;

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
	// The two limits sub-parsers return false when a matched child element has
	// an unparseable value (original returned char; LoadMXMLSectionLimits then
	// logs "Could not load '%s' sector." and stops).
	bool LoadMXMLSectionLimitsInstances(const TiXmlElement *limits);     // 0x2ac20
	bool LoadMXMLSectionLimitsModelStore(const TiXmlElement *limits);    // 0x2ae60
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
