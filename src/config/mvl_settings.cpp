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
#include <limits.h>

// Parse <name>int</name> if the element is present. Returns false only when
// the element exists but its text is not a valid integer — mirrors the
// original generic matcher (FUN_1001bc10 returns 0 on parse failure, which
// aborts the enclosing section load). Absent element: returns true, *out
// untouched.
static bool
MvlLoadIntElement(const TiXmlElement *parent, const char *name, int *out)
{
	const TiXmlElement *el = parent->FirstChildElement(name);
	bool ok;

	if(!el)
		return true;
	*out = MvlAtoi(el->GetText(), &ok);
	return ok;
}

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

	// <limits>: ids/instances/modelstore are zeroed by the memset above.
	// streamingmem uses the original's -1 "not configured" sentinel
	// (DAT_100a6a8c; ProcessStreamingMemLimit @ RVA 0x2ba50 tests for it).
	streamingmem.mb = -1;

	// Processor state: dirty/long-model flags off; engine-derived ID
	// boundaries 0 until the engine-init lane fills them (TODO(engine),
	// original globals @ VA 0x100a6320 / 0x100a6328 / 0x100a6318).
	idLimitsDirty = false;
	longModelIdMode = false;
	curModelLimit = 0;
	curTxdLimit = 0;
	curColLimit = 0;
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
	       m_data.ids.models, m_data.ids.texdicts, m_data.ids.colfiles,
	       m_data.streamingmem.mb);

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
// instances/modelstore sub-section parsers. Storage globals: models @ VA
// 0x100b17e4, texdicts @ VA 0x100b17e0, colfiles @ VA 0x100b17bc,
// streamingmem @ VA 0x100a6a8c.
// ---------------------------------------------------------------------------

void
CMvlSettings::LoadMXMLSectionLimits(const TiXmlElement *miscdata)
{
	const TiXmlElement *limits = miscdata->FirstChildElement("limits");

	if(!limits)
		return;

	// Direct int children (original generic element matcher, type 'i').
	// A present element with bad text aborts the section silently (original
	// FUN_1002b030 just returns for direct members).
	if(!MvlLoadIntElement(limits, "models", &m_data.ids.models) ||
	   !MvlLoadIntElement(limits, "texdicts", &m_data.ids.texdicts) ||
	   !MvlLoadIntElement(limits, "colfiles", &m_data.ids.colfiles) ||
	   !MvlLoadIntElement(limits, "streamingmem", &m_data.streamingmem.mb))
		return;

	// Sub-sections: on failure the original logs "Could not load '%s'
	// sector." and stops parsing.
	if(!LoadMXMLSectionLimitsInstances(limits)){
		MvlErrorLog("CMvlSettings::LoadMXMLSectionLimits: Could not load '%s' sector.",
		            "instances");
		return;
	}
	if(!LoadMXMLSectionLimitsModelStore(limits)){
		MvlErrorLog("CMvlSettings::LoadMXMLSectionLimits: Could not load '%s' sector.",
		            "modelstore");
		return;
	}
}

// ---------------------------------------------------------------------------
// LoadMXMLSectionLimitsInstances — vehmod_retail.dll 0.98e @ RVA 0x2ac20.
// <limits><instances> pool counts: vehicles, colmodel (+ the full pool set
// from sample_globalm.xml: ptrnode, entryinfonode, peds, buildings,
// treadables, objects, dummys, audioscriptobj).
// ---------------------------------------------------------------------------

bool
CMvlSettings::LoadMXMLSectionLimitsInstances(const TiXmlElement *limits)
{
	// <instances> children, in the original's storage order (DAT_100b1794..).
	// Original @ RVA 0x2ac20 returns 0 when a matched child fails to parse;
	// unmatched children are skipped. "peds" name string @ VA 0x1007ea78.
	struct SEntry
	{
		const char *name;
		int *value;
	};

	const SEntry entries[10] = {
		{ "ptrnode",        &m_data.instances.ptrnode },
		{ "entryinfonode",  &m_data.instances.entryinfonode },
		{ "peds",           &m_data.instances.peds },
		{ "vehicles",       &m_data.instances.vehicles },
		{ "buildings",      &m_data.instances.buildings },
		{ "treadables",     &m_data.instances.treadables },
		{ "objects",        &m_data.instances.objects },
		{ "dummys",         &m_data.instances.dummys },
		{ "audioscriptobj", &m_data.instances.audioscriptobj },
		{ "colmodel",       &m_data.instances.colmodel },
	};
	const TiXmlElement *inst = limits->FirstChildElement("instances");
	int i;

	if(!inst)
		return true;

	for(i = 0; i < 10; i++){
		const TiXmlElement *el = inst->FirstChildElement(entries[i].name);
		bool ok;

		if(!el)
			continue;
		*entries[i].value = MvlAtoi(el->GetText(), &ok);
		if(!ok)
			return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// LoadMXMLSectionLimitsModelStore — vehmod_retail.dll 0.98e @ RVA 0x2ae60.
// <limits><modelstore>: simple, time, weapon, clump, vehicle, ped, 2dfx.
// (sample_globalm.xml spells the last one <fx2d>; version-matrix says "2dfx"
// — accept both, TODO(schema) confirm which the original reads.)
// ---------------------------------------------------------------------------

bool
CMvlSettings::LoadMXMLSectionLimitsModelStore(const TiXmlElement *limits)
{
	// <modelstore> children, in the original's storage order (DAT_100b17c0..).
	// Original @ RVA 0x2ae60: "time" name string @ VA 0x1007eb08, "ped" @ VA
	// 0x1007d56c, last entry @ VA 0x1007eb20 (spelling unresolved).
	struct SEntry
	{
		const char *name;
		int *value;
	};

	const SEntry entries[6] = {
		{ "simple",  &m_data.modelstore.simple },
		{ "time",    &m_data.modelstore.time },
		{ "weapon",  &m_data.modelstore.weapon },
		{ "clump",   &m_data.modelstore.clump },
		{ "vehicle", &m_data.modelstore.vehicle },
		{ "ped",     &m_data.modelstore.ped },
	};
	const TiXmlElement *ms = limits->FirstChildElement("modelstore");
	int i;

	if(!ms)
		return true;

	for(i = 0; i < 6; i++){
		const TiXmlElement *el = ms->FirstChildElement(entries[i].name);
		bool ok;

		if(!el)
			continue;
		*entries[i].value = MvlAtoi(el->GetText(), &ok);
		if(!ok)
			return false;
	}

	// TODO(schema): last entry name (VA 0x1007eb20) not visible in the decomp
	// chain; samples use <fx2d>, the version matrix says "2dfx". Accept both
	// (fx2d first); a parse failure still aborts, like the original.
	{
		const TiXmlElement *el = ms->FirstChildElement("fx2d");
		bool ok;

		if(!el)
			el = ms->FirstChildElement("2dfx");
		if(el){
			m_data.modelstore.fx2d = MvlAtoi(el->GetText(), &ok);
			if(!ok)
				return false;
		}
	}
	return true;
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
// Process* — validation + engine-patch entry points. 1:1 ports of
// vehmod_retail.dll 0.98e: ProcessPoolLimits @ RVA 0x2b4d0,
// ProcessModelStoreLimits @ RVA 0x2b5a0, ProcessIdLimits @ RVA 0x2b8f0,
// ProcessStreamingMemLimit @ RVA 0x2ba50.
//
// The original reads parallel .data descriptor tables (current/minimum/maximum
// per entry) and writes accepted values straight into VC 1.0 memory. The table
// CONTENTS are not in the decomp chain (data section) — the VAs are cited at
// each table below. Until the engine lane recovers them, unknown min/max use
// INT_MIN/INT_MAX placeholders (never clamps — behaviour-neutral) and
// engine-current values stay -1/0 ("not hooked"). HOOK POINT markers show
// where the original writes go.
// ---------------------------------------------------------------------------

void
CMvlSettings::ProcessPoolLimits(void)
{
	// Descriptor table mirroring the original's parallel arrays (10 entries,
	// loop bound 0x28/4 @ RVA 0x2b4d0):
	//   value   -> DAT_100b1794 + i*4  (settings .data, order above)
	//   current -> DAT_100ae6a0 + i*4  (pointer to engine pool size)
	//   minimum -> DAT_100ae70c + i*4  (VA 0x100ae70c)
	//   maximum -> DAT_100ae6e4 + i*4  (VA 0x100ae6e4)
	// TODO(table): min/max contents not recovered (see port-notes-limits.md).
	// INT_MIN/INT_MAX placeholders disable clamping until the .data tables are
	// dumped. current = -1 means "engine not hooked" — this mirrors the
	// original's negative-pointer path, where the auto-raise step is skipped.
	struct SPoolDesc
	{
		const char *name;
		int *value;
		int current;
		int minimum;
		int maximum;
	};

	SPoolDesc pools[10] = {
		{ "ptrnode",        &m_data.instances.ptrnode,        -1, INT_MIN, INT_MAX },
		{ "entryinfonode",  &m_data.instances.entryinfonode,  -1, INT_MIN, INT_MAX },
		{ "peds",           &m_data.instances.peds,           -1, INT_MIN, INT_MAX },
		{ "vehicles",       &m_data.instances.vehicles,       -1, INT_MIN, INT_MAX },
		{ "buildings",      &m_data.instances.buildings,      -1, INT_MIN, INT_MAX },
		{ "treadables",     &m_data.instances.treadables,     -1, INT_MIN, INT_MAX },
		{ "objects",        &m_data.instances.objects,        -1, INT_MIN, INT_MAX },
		{ "dummys",         &m_data.instances.dummys,         -1, INT_MIN, INT_MAX },
		{ "audioscriptobj", &m_data.instances.audioscriptobj, -1, INT_MIN, INT_MAX },
		{ "colmodel",       &m_data.instances.colmodel,       -1, INT_MIN, INT_MAX },
	};
	int i;

	for(i = 0; i < 10; i++){
		SPoolDesc &p = pools[i];

		// Auto-raise: unset (0) request + engine pool below minimum -> minimum.
		if(*p.value == 0 && p.current >= 0 && p.current < p.minimum)
			*p.value = p.minimum;

		if(*p.value != 0){
			if(*p.value < p.minimum){
				MvlLog("CMvlSettings::ProcessPoolLimits: Pool %s limit below minimum %d.",
				       p.name, p.minimum);
				*p.value = p.minimum;
			} else if(*p.value > p.maximum){
				MvlLog("CMvlSettings::ProcessPoolLimits: Pool %s limit above maximum %d.",
				       p.name, p.maximum);
				*p.value = p.maximum;
			}
			// HOOK POINT (0.98b pool expansion; writers FUN_10022560/
			// FUN_10022590): original stores *p.value into *DAT_100ae6a0[i];
			// if that slot is a negative pointer (0x80000000|addr) it instead
			// patches the push-immediate at that address to 6A 00 68 <le32
			// limit>. "RAN OUT OF ... SLOTS!!!" strings @ VA 0x1007ef64 /
			// 0x1007f10c belong to the hooked pool allocators.
			// TODO(engine): install the CPool patch writer.
		}
	}
}

void
CMvlSettings::ProcessModelStoreLimits(void)
{
	// Descriptor table mirroring the original's parallel arrays (7 entries,
	// loop bound 0x1c/4 @ RVA 0x2b5a0):
	//   value    -> DAT_100b17c0 + i*4  (settings .data, <modelstore> order)
	//   minimum  -> DAT_100ae940 + i*4  (VA 0x100ae940)
	//   maximum  -> VA 0x100ae978 + i*4
	//   elemsize -> DAT_100ae76c + i*4  (array element size, used for realloc)
	//   ctor     -> DAT_100ae750 + i*4  (per-element constructor fn pointer)
	// TODO(table): min/max/elemsize/ctor contents not recovered; INT_MIN/
	// INT_MAX placeholders disable clamping until the .data tables are dumped.
	struct SStoreDesc
	{
		const char *name;
		int *value;
		int minimum;
		int maximum;
	};

	SStoreDesc stores[7] = {
		{ "simple",  &m_data.modelstore.simple,  INT_MIN, INT_MAX },
		{ "time",    &m_data.modelstore.time,    INT_MIN, INT_MAX },
		{ "weapon",  &m_data.modelstore.weapon,  INT_MIN, INT_MAX },
		{ "clump",   &m_data.modelstore.clump,   INT_MIN, INT_MAX },
		{ "vehicle", &m_data.modelstore.vehicle, INT_MIN, INT_MAX },
		{ "ped",     &m_data.modelstore.ped,     INT_MIN, INT_MAX },
		{ "2dfx",    &m_data.modelstore.fx2d,    INT_MIN, INT_MAX },
	};
	int i;

	for(i = 0; i < 7; i++){
		SStoreDesc &s = stores[i];

		if(*s.value != 0){
			if(*s.value < s.minimum){
				MvlLog("CMvlSettings::ProcessModelStoreLimits: Model store %s limit below minimum %d.",
				       s.name, s.minimum);
				*s.value = s.minimum;
			} else if(*s.value > s.maximum){
				MvlLog("CMvlSettings::ProcessModelStoreLimits: Model store %s limit above maximum %d.",
				       s.name, s.maximum);
				*s.value = s.maximum;
			}
			// HOOK POINT (0.98b model-store expansion): original reallocs the
			// store array (_malloc(elemsize * limit + 4)), calls the ctor
			// `limit` times and relinks the store list heads @ VA 0x100ae788.
			// TODO(engine): port once CModelStore/its list heads are mapped.
		}
	}
}

void
CMvlSettings::ProcessIdLimits(void)
{
	// Original @ RVA 0x2b8f0. Reads the engine's current ID boundaries
	// (DAT_100a6320 model end, DAT_100a6328 TXD end, DAT_100a6318 COL end),
	// keeps the larger of requested/current, caps requested values, and
	// switches on experimental long model IDs when the total exceeds 32767.

	// models (DAT_100b17e4): cap 200000.
	if(m_data.ids.models != 0)
		m_data.idLimitsDirty = true;               // DAT_100b178f
	{
		int limit = m_data.curModelLimit;          // DAT_100a6320
		if(m_data.curModelLimit <= m_data.ids.models){
			limit = m_data.ids.models;
			if(m_data.ids.models > 200000){
				MvlLog("CMvlSettings::ProcessIdLimits: Requested model limit over 200000, reducing back to 200000.");
				limit = 200000;
			}
		}
		m_data.ids.models = limit;
	}

	// texdicts (DAT_100b17e0): span = TXD end - model end; cap 50000.
	if(m_data.ids.texdicts != 0)
		m_data.idLimitsDirty = true;
	{
		int limit = m_data.curTxdLimit - m_data.curModelLimit;
		if(m_data.curTxdLimit - m_data.curModelLimit <= m_data.ids.texdicts){
			limit = m_data.ids.texdicts;
			if(m_data.ids.texdicts > 50000){
				MvlLog("CMvlSettings::ProcessIdLimits: Requested TXD limit over 50000, reducing back to 50000.");
				limit = 50000;
			}
		}
		m_data.ids.texdicts = limit;
	}

	// colfiles (DAT_100b17bc): span = COL end - TXD end; cap 1000.
	if(m_data.ids.colfiles != 0)
		m_data.idLimitsDirty = true;
	{
		int limit = m_data.curColLimit - m_data.curTxdLimit;
		if(m_data.curColLimit - m_data.curTxdLimit <= m_data.ids.colfiles){
			limit = m_data.ids.colfiles;
			if(m_data.ids.colfiles > 1000){
				MvlLog("CMvlSettings::ProcessIdLimits: Requested COL limit over 1000, reducing back to 1000.");
				limit = 1000;
			}
		}
		m_data.ids.colfiles = limit;
	}

	// Long model ID switch: original tests 0x7fff < texdicts + 100 + colfiles
	// + models (the +100 is the ANIM allowance).
	if(0x7fff < m_data.ids.texdicts + 100 + m_data.ids.colfiles + m_data.ids.models){
		MvlLog("CMvlSettings::ProcessIdLimits: Model, TXD, COL and ANIM limits totalling over 32767, switching to experimental long model ID mode.");
		m_data.longModelIdMode = true;             // DAT_100b0c41
	}

	// TODO(engine): original calls FUN_100173d0 (RVA 0x173d0) and, when it
	// returns non-zero, FUN_10017390 (RVA 0x17390) — CLongModelId state.
	if(m_data.idLimitsDirty){
		// TODO(engine): rebuild model/TXD/COL ID tables (FUN_10017820,
		// RVA 0x17820) — CModelIndexLimit / CLongModelId patch lane.
	}
}

void
CMvlSettings::ProcessStreamingMemLimit(void)
{
	// Original @ RVA 0x2ba50. Value global DAT_100a6a8c; -1 = not configured.
	// Out-of-range values are IGNORED (not clamped), with these exact logs.
	if(m_data.streamingmem.mb == -1)
		return;
	if(m_data.streamingmem.mb < 65){       // 0x41
		MvlLog("CMvlSettings::ProcessStreamingMemLimit: Ignoring streaming memory limit below 65MB.");
		return;
	}
	if(m_data.streamingmem.mb > 1900){     // 0x76c
		MvlLog("CMvlSettings::ProcessStreamingMemLimit: Ignoring streaming memory limit above 1900MB.");
		return;
	}

	// HOOK POINT (0.98e): original stores (streamingmem << 20) — MB -> bytes —
	// to VC 1.0 game globals DAT_0041079f and DAT_0094dd54 (game VAs
	// 0x0041079f / 0x0094dd54). TODO(engine): port with the streaming-buffer
	// hook lane.
}

// ---------------------------------------------------------------------------
// ValidateIdStart — 0.98e rule: "has to be between 250 and
// MIN(65000, modelLimit - 100)" (string @ 0x10081260).
// ---------------------------------------------------------------------------

int
CMvlSettings::ValidateIdStart(int idstart) const
{
	int modelLimit = m_data.ids.models > 0 ? m_data.ids.models : 65000 + 100;
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
