// global_veh_xml.cpp — implementation (see global_veh_xml.h for RVAs).
//
// Parse coverage vs real schema sample
// (original_files/maxovehmod098e/mvl/vehicles/globalv.xml):
//   <vehdata><nospawn><v>200</v></nospawn>, idstart, usespawner, usetuner,
//   disablelightboxes — ALL handled.
// Elements from version-matrix.md not present in the shipped sample:
//   disableflipdamage (0.97), tuningfix (0.93), maxaltitude (0.98),
//   dumprgbcolors (0.98), antialias (0.98a) — parsed, TODO(schema) flags.
//
// Schema ambiguities (TODO, verify against 0.98e disasm):
//   TODO(schema): <nospawn> may also accept ranges ("130-236" appears in the
//     0.91 version-matrix note) — currently only single ids parsed; range
//     syntax support TODO after disasm of LoadGlobalXMLNoSpawn @ 0x37a90.
//   TODO(schema): aispecials/extraflags/stockvehiclesettings sectors have no
//     shipped sample; parsed generically with counts logged.
#include "../core/stdafx.h"
#include "global_veh_xml.h"
#include "mvl_settings.h"
#include "xml_helpers.h"
#include "../loader/log.h"

#include <stdlib.h>

static SMvlGlobalVehData s_globalVeh;

void
SMvlGlobalVehData::Reset(void)
{
	noSpawnIds.clear();
	idstart = 0;
	usespawner = false;
	usetuner = false;
	disablelightboxes = false;
	disableflipdamage = false;
	tuningfix = false;
	maxaltitude = 0;
	dumprgbcolors = false;
	antialias = 0;
	stockVehicleCount = 0;
}

SMvlGlobalVehData &
MvlGlobalVehData(void)
{
	return s_globalVeh;
}

// ---------------------------------------------------------------------------
// LoadGlobalXMLNoSpawn — vehmod_retail.dll 0.98e @ RVA 0x37a90.
// <nospawn> sector: list of <v>id</v> model IDs that must never spawn.
// TODO(schema): possible "min-max" range entries (0.91 note) not handled.
// ---------------------------------------------------------------------------

void
LoadGlobalXMLNoSpawn(const TiXmlElement *vehdata, SMvlGlobalVehData &out)
{
	const TiXmlElement *ns = vehdata->FirstChildElement("nospawn");
	const TiXmlElement *v;

	if(!ns)
		return;

	for(v = ns->FirstChildElement("v"); v; v = v->NextSiblingElement("v")){
		bool ok = false;
		int id = MvlAtoi(v->GetText(), &ok);

		if(ok)
			out.noSpawnIds.push_back(id);
		else
			MvlErrorLog("LoadGlobalXMLNoSpawn: bad <v> entry '%s'",
			            v->GetText() ? v->GetText() : "(empty)");
	}
	MvlLog("LoadGlobalXMLNoSpawn: %u model(s) blocked from spawning",
	       (unsigned)out.noSpawnIds.size());
}

// ---------------------------------------------------------------------------
// LoadGlobalAISpecials — vehmod_retail.dll 0.98e @ RVA 0x37b80.
// TODO(schema): no shipped sample for the <aispecials> sector; parsed
// generically (count of child elements logged). Real field mapping TODO
// after disasm.
// ---------------------------------------------------------------------------

void
LoadGlobalAISpecials(const TiXmlElement *vehdata, SMvlGlobalVehData &out)
{
	const TiXmlElement *ai = vehdata->FirstChildElement("aispecials");
	const TiXmlElement *child;
	int count = 0;

	if(!ai)
		return;

	for(child = ai->FirstChildElement(); child;
	    child = child->NextSiblingElement())
		count++;

	out.stockVehicleCount += 0; // placeholder; specials stored per-vehicle later
	MvlLog("LoadGlobalAISpecials: %d entr(ies) (TODO: field mapping)", count);
}

// ---------------------------------------------------------------------------
// LoadGlobalVehExtraFlags — vehmod_retail.dll 0.98e @ RVA 0x37e70.
// TODO(schema): global extraflags overrides have no shipped sample; parsed
// generically. Flags are numeric (0.97b note: flag-name strings not in
// binary — flags parsed numerically).
// ---------------------------------------------------------------------------

void
LoadGlobalVehExtraFlags(const TiXmlElement *vehdata, SMvlGlobalVehData &out)
{
	const TiXmlElement *ef = vehdata->FirstChildElement("extraflags");
	const TiXmlElement *child;
	int count = 0;

	(void)out;

	if(!ef)
		return;

	for(child = ef->FirstChildElement(); child;
	    child = child->NextSiblingElement())
		count++;

	MvlLog("LoadGlobalVehExtraFlags: %d entr(ies) (TODO: field mapping)", count);
}

// ---------------------------------------------------------------------------
// LoadGlobalStockAISpecials — vehmod_retail.dll 0.98e @ RVA 0x380b0.
// 0.97c: <stockvehiclesettings> (ainame/aispecials/setextraflags for
// police/ambulance/firetruck). TODO(schema): exact layout unverified.
// ---------------------------------------------------------------------------

void
LoadGlobalStockAISpecials(const TiXmlElement *vehdata, SMvlGlobalVehData &out)
{
	const TiXmlElement *sv = vehdata->FirstChildElement("stockvehiclesettings");
	const TiXmlElement *child;

	if(!sv)
		return;

	out.stockVehicleCount = 0;
	for(child = sv->FirstChildElement(); child;
	    child = child->NextSiblingElement())
		out.stockVehicleCount++;

	MvlLog("LoadGlobalStockAISpecials: %d stock vehicle(s) configured "
	       "(TODO: field mapping)", out.stockVehicleCount);
}

// ---------------------------------------------------------------------------
// LoadGlobalVXMLFile — vehmod_retail.dll 0.98e @ RVA 0x38160.
// Parses mvl\vehicles\globalv.xml, root <vehdata>.
// ---------------------------------------------------------------------------

bool
LoadGlobalVXMLFile(void)
{
	const char *path = MVL_LOG_DIR "\\vehicles\\globalv.xml";
	TiXmlDocument doc(path);
	SMvlGlobalVehData &out = s_globalVeh;
	bool ok;

	out.Reset();

	if(!doc.LoadFile(TIXML_DEFAULT_ENCODING)){
		MvlLog("LoadGlobalVXMLFile: %s not found or invalid (%s)",
		       path, doc.ErrorDesc());
		return false;
	}

	const TiXmlElement *root = doc.RootElement();
	if(!root || strcmp(root->Value(), "vehdata") != 0){
		MvlErrorLog("LoadGlobalVXMLFile: %s root is not <vehdata>", path);
		return false;
	}

	LoadGlobalXMLNoSpawn(root, out);
	LoadGlobalAISpecials(root, out);
	LoadGlobalVehExtraFlags(root, out);
	LoadGlobalStockAISpecials(root, out);

	out.idstart           = XmlGetInt(root, "idstart", &ok);
	out.usespawner        = XmlGetBool(root, "usespawner", &ok);
	out.usetuner          = XmlGetBool(root, "usetuner", &ok);
	out.disablelightboxes = XmlGetBool(root, "disablelightboxes", &ok);
	out.disableflipdamage = XmlGetBool(root, "disableflipdamage", &ok);
	out.tuningfix         = XmlGetBool(root, "tuningfix", &ok);
	out.maxaltitude       = XmlGetInt(root, "maxaltitude", &ok);
	out.dumprgbcolors     = XmlGetBool(root, "dumprgbcolors", &ok);

	// antialias mirrors the globalm.xml setting (0.98a); accept on/off/num.
	{
		const TiXmlElement *aa = root->FirstChildElement("antialias");
		if(aa && aa->GetText()){
			if(!_stricmp(aa->GetText(), "on"))
				out.antialias = 1;
			else if(!_stricmp(aa->GetText(), "off"))
				out.antialias = 0;
			else
				out.antialias = MvlAtoi(aa->GetText(), &ok);
		}
	}

	// idstart validation — 0.98e rule, string @ 0x10081260.
	if(out.idstart != 0){
		int valid = CMvlSettings::Get().ValidateIdStart(out.idstart);
		if(valid < 0){
			out.idstart = 0; // invalid -> no custom-ID base
		} else {
			out.idstart = valid;
		}
	}

	MvlLog("LoadGlobalVXMLFile: parsed %s (nospawn=%u idstart=%d "
	       "usespawner=%d usetuner=%d disablelightboxes=%d "
	       "disableflipdamage=%d tuningfix=%d maxaltitude=%d "
	       "dumprgbcolors=%d antialias=%d)",
	       path, (unsigned)out.noSpawnIds.size(), out.idstart,
	       out.usespawner ? 1 : 0, out.usetuner ? 1 : 0,
	       out.disablelightboxes ? 1 : 0, out.disableflipdamage ? 1 : 0,
	       out.tuningfix ? 1 : 0, out.maxaltitude, out.dumprgbcolors ? 1 : 0,
	       out.antialias);
	return true;
}
