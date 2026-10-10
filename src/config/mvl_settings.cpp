// mvl_settings.cpp — CMvlSettings implementation (see mvl_settings.h for RVAs).
//
// Parse coverage vs real schema samples:
//   - original_files/maxovehmod098e/mvl/globalm.xml (shipped default):
//     <miscdata><internal> windowed/resizable/usemenus/fixframewait/
//     menuonrestore/rtshadowonplanes/rtshadowonplayer/autointeriors/testmenus/
//     trashbin/antialias/decapitations/roadthingy — ALL handled.
//   - original_files/maxovehmod098e/mvl/help/sample_globalm.xml: adds
//     <limits> (models/texdicts/colfiles/streamingmem + <instances> +
//     <modelstore>), noloadfade, autoloadsave — ALL handled.
//
// Schema ambiguities (TODO, verify against 0.98e disasm):
//   TODO(schema): antialias accepts "off"/"on" (samples) — unknown whether a
//     numeric MS level (2/4/8) is also accepted; we parse numerics via
//     MvlAtoi fallback and treat "on" as 1.
//   TODO(schema): autoloadsave is "0" in sample_globalm.xml — bool or save
//     slot index? Stored as int; bool semantics at use site.
//   TODO(schema): <camera> element (0.98a version-matrix) not present in any
//     sample — schema unknown, not parsed.
//   TODO(schema): pedhead (0.98a) not present in samples — parsed as bool.
#include "../core/stdafx.h"
#include "mvl_settings.h"
#include "xml_helpers.h"
#include "../loader/log.h"

#include <stdlib.h>

void
SMvlSettingsData::Reset(void)
{
	memset(this, 0, sizeof(*this));
	// Defaults matching the shipped mvl\globalm.xml (0.98e).
	windowed = false;
	resizable = false;
	usemenus = false;
	fixframewait = false;
	menuonrestore = false;
	rtshadowonplanes = false;
	rtshadowonplayer = false;
	autointeriors = false;
	testmenus = false;
	trashbin = false;
	decapitations = false;
	roadthingy = false;
	debugidedata = false;
	antialias = 0;
	autoloadsave = 0;
	noloadfade = false;
	pedhead = false;
	custominit = false;
	models = 0;
	texdicts = 0;
	colfiles = 0;
	streamingmem = 0;
	ptrnode = 0;
	entryinfonode = 0;
	peds = 0;
	vehicles = 0;
	buildings = 0;
	treadables = 0;
	objects = 0;
	dummys = 0;
	audioscriptobj = 0;
	colmodel = 0;
	msSimple = 0;
	msTime = 0;
	msWeapon = 0;
	msClump = 0;
	msVehicle = 0;
	msPed = 0;
	msFx2d = 0;
}

CMvlSettings &
CMvlSettings::Get(void)
{
	static CMvlSettings s_settings;
	return s_settings;
}

// ---------------------------------------------------------------------------
// LoadGlobalMXMLFile — vehmod_retail.dll 0.98e @ RVA 0x2b390.
// Parses mvl\globalm.xml, root element <miscdata>.
// ---------------------------------------------------------------------------

// Free entry point matching the original call chain (FUN_10043c00 →
// LoadGlobalMXMLFile); the work lives in CMvlSettings::LoadGlobalMXMLFile.
bool
LoadGlobalMXMLFile(void)
{
	return CMvlSettings::Get().LoadGlobalMXMLFile();
}

bool
CMvlSettings::LoadGlobalMXMLFile(void)
{
	const char *path = MVL_LOG_DIR "\\globalm.xml";
	TiXmlDocument doc(path);

	if(!doc.LoadFile(TIXML_DEFAULT_ENCODING)){
		MvlLog("LoadGlobalMXMLFile: %s not found or invalid (%s) - using defaults",
		       path, doc.ErrorDesc());
		return false;
	}

	const TiXmlElement *root = doc.RootElement();
	if(!root || strcmp(root->Value(), "miscdata") != 0){
		MvlErrorLog("LoadGlobalMXMLFile: %s root is not <miscdata>", path);
		return false;
	}

	LoadMXMLSectionInternal(root);
	LoadMXMLSectionLimits(root);
	LoadMXMLSectionMiscData(root);

	MvlLog("LoadGlobalMXMLFile: parsed %s (windowed=%d usemenus=%d antialias=%d "
	       "models=%d texdicts=%d colfiles=%d streamingmem=%d)",
	       path,
	       m_data.windowed ? 1 : 0, m_data.usemenus ? 1 : 0, m_data.antialias,
	       m_data.models, m_data.texdicts, m_data.colfiles, m_data.streamingmem);

	// Validation / engine-patch pass (0.98b pool/limit expansion).
	ProcessPoolLimits();
	ProcessModelStoreLimits();
	ProcessIdLimits();
	ProcessStreamingMemLimit();
	return true;
}

// ---------------------------------------------------------------------------
// LoadMXMLSectionInternal — vehmod_retail.dll 0.98e @ RVA 0x2a7f0.
// <internal> toggles: windowed, resizable, usemenus, fixframewait,
// menuonrestore, rtshadowonplanes, rtshadowonplayer, autointeriors, testmenus,
// trashbin, decapitations, roadthingy, debugidedata, antialias, autoloadsave.
// ---------------------------------------------------------------------------

void
CMvlSettings::LoadMXMLSectionInternal(const TiXmlElement *miscdata)
{
	const TiXmlElement *el = miscdata->FirstChildElement("internal");
	bool ok;

	if(!el)
		return;

	m_data.windowed         = XmlGetBool(el, "windowed", &ok);
	m_data.resizable        = XmlGetBool(el, "resizable", &ok);
	m_data.usemenus         = XmlGetBool(el, "usemenus", &ok);
	m_data.fixframewait     = XmlGetBool(el, "fixframewait", &ok);
	m_data.menuonrestore    = XmlGetBool(el, "menuonrestore", &ok);
	m_data.rtshadowonplanes = XmlGetBool(el, "rtshadowonplanes", &ok);
	m_data.rtshadowonplayer = XmlGetBool(el, "rtshadowonplayer", &ok);
	m_data.autointeriors    = XmlGetBool(el, "autointeriors", &ok);
	m_data.testmenus        = XmlGetBool(el, "testmenus", &ok);
	m_data.trashbin         = XmlGetBool(el, "trashbin", &ok);
	m_data.decapitations    = XmlGetBool(el, "decapitations", &ok);
	m_data.roadthingy       = XmlGetBool(el, "roadthingy", &ok);
	m_data.debugidedata     = XmlGetBool(el, "debugidedata", &ok);

	// antialias: "off"/"on" in samples; accept numeric MS level too.
	{
		const char *text = NULL;
		const TiXmlElement *aa = el->FirstChildElement("antialias");
		if(aa)
			text = aa->GetText();
		if(text){
			if(!_stricmp(text, "on"))
				m_data.antialias = 1;
			else if(!_stricmp(text, "off"))
				m_data.antialias = 0;
			else
				m_data.antialias = MvlAtoi(text, &ok);
		}
	}

	// autoloadsave: "0" in sample_globalm.xml — stored as int (TODO schema).
	m_data.autoloadsave = XmlGetInt(el, "autoloadsave", &ok);
}

// ---------------------------------------------------------------------------
// LoadMXMLSectionLimits — vehmod_retail.dll 0.98e @ RVA 0x2b030.
// <limits>: models, texdicts, colfiles, streamingmem; dispatches to the
// instances/modelstore sub-section parsers.
// ---------------------------------------------------------------------------

void
CMvlSettings::LoadMXMLSectionLimits(const TiXmlElement *miscdata)
{
	const TiXmlElement *limits = miscdata->FirstChildElement("limits");
	bool ok;

	if(!limits)
		return;

	m_data.models      = XmlGetInt(limits, "models", &ok);
	m_data.texdicts    = XmlGetInt(limits, "texdicts", &ok);
	m_data.colfiles    = XmlGetInt(limits, "colfiles", &ok);
	m_data.streamingmem = XmlGetInt(limits, "streamingmem", &ok);

	LoadMXMLSectionLimitsInstances(limits);
	LoadMXMLSectionLimitsModelStore(limits);
}

// ---------------------------------------------------------------------------
// LoadMXMLSectionLimitsInstances — vehmod_retail.dll 0.98e @ RVA 0x2ac20.
// <limits><instances> pool counts: vehicles, colmodel (+ the full pool set
// from sample_globalm.xml: ptrnode, entryinfonode, peds, buildings,
// treadables, objects, dummys, audioscriptobj).
// ---------------------------------------------------------------------------

void
CMvlSettings::LoadMXMLSectionLimitsInstances(const TiXmlElement *limits)
{
	const TiXmlElement *inst = limits->FirstChildElement("instances");
	bool ok;

	if(!inst)
		return;

	m_data.ptrnode       = XmlGetInt(inst, "ptrnode", &ok);
	m_data.entryinfonode = XmlGetInt(inst, "entryinfonode", &ok);
	m_data.peds          = XmlGetInt(inst, "peds", &ok);
	m_data.vehicles      = XmlGetInt(inst, "vehicles", &ok);
	m_data.buildings     = XmlGetInt(inst, "buildings", &ok);
	m_data.treadables    = XmlGetInt(inst, "treadables", &ok);
	m_data.objects       = XmlGetInt(inst, "objects", &ok);
	m_data.dummys        = XmlGetInt(inst, "dummys", &ok);
	m_data.audioscriptobj = XmlGetInt(inst, "audioscriptobj", &ok);
	m_data.colmodel      = XmlGetInt(inst, "colmodel", &ok);
}

// ---------------------------------------------------------------------------
// LoadMXMLSectionLimitsModelStore — vehmod_retail.dll 0.98e @ RVA 0x2ae60.
// <limits><modelstore>: simple, time, weapon, clump, vehicle, ped, 2dfx.
// (sample_globalm.xml spells the last one <fx2d>; version-matrix says "2dfx"
// — accept both, TODO(schema) confirm which the original reads.)
// ---------------------------------------------------------------------------

void
CMvlSettings::LoadMXMLSectionLimitsModelStore(const TiXmlElement *limits)
{
	const TiXmlElement *ms = limits->FirstChildElement("modelstore");
	bool ok;

	if(!ms)
		return;

	m_data.msSimple  = XmlGetInt(ms, "simple", &ok);
	m_data.msTime    = XmlGetInt(ms, "time", &ok);
	m_data.msWeapon  = XmlGetInt(ms, "weapon", &ok);
	m_data.msClump   = XmlGetInt(ms, "clump", &ok);
	m_data.msVehicle = XmlGetInt(ms, "vehicle", &ok);
	m_data.msPed     = XmlGetInt(ms, "ped", &ok);

	m_data.msFx2d = XmlGetInt(ms, "fx2d", &ok);
	if(m_data.msFx2d == 0)
		m_data.msFx2d = XmlGetInt(ms, "2dfx", &ok);
}

// ---------------------------------------------------------------------------
// LoadMXMLSectionMiscData — vehmod_retail.dll 0.98e @ RVA 0x2b230.
// miscdata-level elements outside <internal>/<limits>: noloadfade, pedhead,
// custominit. (<camera> is TODO(schema) — no sample available.)
// ---------------------------------------------------------------------------

void
CMvlSettings::LoadMXMLSectionMiscData(const TiXmlElement *miscdata)
{
	bool ok;

	m_data.noloadfade = XmlGetBool(miscdata, "noloadfade", &ok);
	m_data.pedhead    = XmlGetBool(miscdata, "pedhead", &ok);
	m_data.custominit = XmlGetBool(miscdata, "custominit", &ok);
}

// ---------------------------------------------------------------------------
// Process* — validation + engine-patch entry points.
// vehmod_retail.dll 0.98e RVAs: ProcessPoolLimits 0x2b4d0,
// ProcessModelStoreLimits 0x2b5a0, ProcessIdLimits 0x2b8f0,
// ProcessStreamingMemLimit 0x2ba50.
//
// HOOK POINT: the original patches VC 1.0 pool sizes / model-store sizes /
// streaming memory here (0.98b feature). The clone currently validates and
// logs; patch installation goes at the marked sites once the hook
// installers (FUN_10022560/0x10022590) are cloned.
// ---------------------------------------------------------------------------

void
CMvlSettings::ProcessPoolLimits(void)
{
	// Only log pools the user actually raised (non-zero in globalm.xml).
	if(m_data.vehicles || m_data.colmodel || m_data.peds || m_data.ptrnode){
		MvlLog("ProcessPoolLimits: ptrnode=%d entryinfonode=%d peds=%d "
		       "vehicles=%d buildings=%d treadables=%d objects=%d dummys=%d "
		       "audioscriptobj=%d colmodel=%d",
		       m_data.ptrnode, m_data.entryinfonode, m_data.peds,
		       m_data.vehicles, m_data.buildings, m_data.treadables,
		       m_data.objects, m_data.dummys, m_data.audioscriptobj,
		       m_data.colmodel);
	}
	// HOOK POINT: patch CPool sizes in VC 1.0 (0.98b; "RAN OUT OF ... SLOTS!!!"
	// strings @ 0x1007ef64/0x1007f10c belong to the hooked pool allocators).
}

void
CMvlSettings::ProcessModelStoreLimits(void)
{
	if(m_data.msSimple || m_data.msClump || m_data.msVehicle){
		MvlLog("ProcessModelStoreLimits: simple=%d time=%d weapon=%d clump=%d "
		       "vehicle=%d ped=%d fx2d=%d",
		       m_data.msSimple, m_data.msTime, m_data.msWeapon, m_data.msClump,
		       m_data.msVehicle, m_data.msPed, m_data.msFx2d);
	}
	// HOOK POINT: patch CModelStore array sizes (0.98b).
}

void
CMvlSettings::ProcessIdLimits(void)
{
	// 0.98b: "Model, TXD, COL and ANIM limits totalling over 32767, switching
	// to experimental long model ID mode." (version-matrix.md §0.98b).
	int total = m_data.models + m_data.texdicts + m_data.colfiles;

	if(total > 32767)
		MvlLog("ProcessIdLimits: limits total %d over 32767 - long model ID "
		       "mode required (TODO: CLongModelId engine)", total);
	else if(total)
		MvlLog("ProcessIdLimits: limits total %d", total);
	// HOOK POINT: CModelIndexLimit / CLongModelId patch installation
	// (re-098e-map.md §Limits / model-ID engine).
}

void
CMvlSettings::ProcessStreamingMemLimit(void)
{
	// 0.98e: streamingmem clamp 65-1900 MB (version-matrix.md §0.98e).
	int mb = m_data.streamingmem;

	if(mb <= 0)
		return;
	if(mb < 65){
		MvlLog("ProcessStreamingMemLimit: %d MB below minimum, clamping to 65", mb);
		mb = 65;
	} else if(mb > 1900){
		MvlLog("ProcessStreamingMemLimit: %d MB above maximum, clamping to 1900", mb);
		mb = 1900;
	}
	m_data.streamingmem = mb;
	MvlLog("ProcessStreamingMemLimit: %d MB", mb);
	// HOOK POINT: patch VC 1.0 streaming buffer size (0.98e).
}

// ---------------------------------------------------------------------------
// ValidateIdStart — 0.98e rule: "has to be between 250 and
// MIN(65000, modelLimit - 100)" (string @ 0x10081260).
// ---------------------------------------------------------------------------

int
CMvlSettings::ValidateIdStart(int idstart) const
{
	int modelLimit = m_data.models > 0 ? m_data.models : 65000 + 100;
	int maxId = 65000;

	if(modelLimit - 100 < maxId)
		maxId = modelLimit - 100;

	if(idstart < 250 || idstart > maxId){
		MvlErrorLog("ValidateIdStart: idstart %d has to be between 250 and "
		            "MIN(65000, modelLimit - 100) = %d", idstart, maxId);
		return -1;
	}
	return idstart;
}
