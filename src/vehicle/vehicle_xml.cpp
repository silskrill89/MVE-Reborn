// vehicle_xml.cpp — CMvlVehicle vehicle-XML parser cluster (see vehicle_xml.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (range-decomp.txt
// 0x46420-0x4e790). Element names come from the decomp error strings and the
// sample XMLs; log strings are verbatim (including the original's typos and
// mislabeled messages, marked "QUIRK"). Table-driven helpers in the original
// (FUN_1001baf0 bind / FUN_1001bb40 find / FUN_1001bc10 parse-entry /
// FUN_1001bbb0 required-check / FUN_1001bdb0 vector) are reimplemented as
// explicit dispatch loops with identical semantics and messages.
#include "vehicle_xml.h"

#include "../loader/log.h"

#include "tinyxml.h"   // vendored classic TinyXML (external/tinyxml)

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrappers (see vehicle_archive.cpp): FUN_1001cc00 msg, FUN_1001cce0 warn,
// FUN_1001cb80 error. Message text is kept 1:1; the original logs the
// function name as context, we prefix it the same way.
// ---------------------------------------------------------------------------
static void
XmlMsg(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[sizeof(buf) - 1] = '\0';
	MvlLog("%s", buf);
}

static void
XmlWarn(const char *fn, const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "%s: %s", fn, msg);
	buf[sizeof(buf) - 1] = '\0';
	MvlLog("%s", buf);
}

static void
XmlErr(const char *fn, const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "%s: %s", fn, msg);
	buf[sizeof(buf) - 1] = '\0';
	MvlErrorLog("%s", buf);
}

// ---------------------------------------------------------------------------
// Shared error patterns (verbatim). All return false so call sites can
//   return MvlErrX(...);
// ---------------------------------------------------------------------------
static bool
MvlErrDup(const char *fn, const char *sector, const char *name)
{
	XmlErr(fn, "'%s' sector contains multiple '%s' elements.", sector, name);
	return false;
}

static bool
MvlErrDupVector(const char *fn, const char *sector, const char *name)
{
	XmlErr(fn, "'%s' sector contains multiple '%s' vectors.", sector, name);
	return false;
}

static bool
MvlErrDupSector(const char *fn, const char *sector, const char *name)
{
	XmlErr(fn, "'%s' sector contains multiple '%s' sectors.", sector, name);
	return false;
}

static bool
MvlErrRead(const char *fn, const char *name)
{
	XmlErr(fn, "Failed to read data from '%s' element.", name);
	return false;
}

static bool
MvlErrParse(const char *fn, const char *name)
{
	XmlErr(fn, "Failed to parse the contents of '%s' element.", name);
	return false;
}

// QUIRK: dockedinfo/armedinfo use the short form without " element".
static bool
MvlErrParseShort(const char *fn, const char *name)
{
	XmlErr(fn, "Failed to parse the contents of '%s'.", name);
	return false;
}

static bool
MvlErrVector(const char *fn, const char *name)
{
	XmlErr(fn, "Could not load '%s' vector.", name);
	return false;
}

static bool
MvlErrMissing(const char *fn, const char *sector, const char *name)
{
	XmlErr(fn, "'%s' sector is missing '%s' element.", sector, name);
	return false;
}

static bool
MvlErrMissingVector(const char *fn, const char *sector, const char *name)
{
	XmlErr(fn, "'%s' sector is missing '%s' vector.", sector, name);
	return false;
}

static bool
MvlErrUnknown(const char *fn, const char *sector, const char *name)
{
	XmlErr(fn, "Unknown element '%s' in '%s' sector.", name, sector);
	return false;
}

// ---------------------------------------------------------------------------
// Generic scalar element: dup check + text read + sscanf + "seen" flag.
// Used where the original's three messages match the standard patterns.
// ---------------------------------------------------------------------------
static bool
MvlElemFloat(TiXmlElement *e, const char *fn, const char *sector,
             const char *name, bool *seen, float *dest)
{
	if(*seen)
		return MvlErrDup(fn, sector, name);
	const char *t = e->GetText();
	if(!t)
		return MvlErrRead(fn, name);
	if(sscanf(t, "%f", dest) != 1)
		return MvlErrParse(fn, name);
	*seen = true;
	return true;
}

static bool
MvlElemInt(TiXmlElement *e, const char *fn, const char *sector,
           const char *name, bool *seen, int *dest)
{
	if(*seen)
		return MvlErrDup(fn, sector, name);
	const char *t = e->GetText();
	if(!t)
		return MvlErrRead(fn, name);
	if(sscanf(t, "%i", dest) != 1)
		return MvlErrParse(fn, name);
	*seen = true;
	return true;
}

// ---------------------------------------------------------------------------
// FUN_1001bdb0 @ RVA 0x1bdb0 — MvlParseVector: <x><y><z> child elements into
// 3 floats. The original helper's own diagnostics are table-side; failures
// surface as the caller's "Could not load 'X' vector." message.
// ---------------------------------------------------------------------------
bool
MvlParseVector(TiXmlElement *e, float *dest)
{
	static const char *const kComp[3] = { "x", "y", "z" };
	if(!e || !dest)
		return false;
	for(int i = 0; i < 3; i++){
		TiXmlElement *c = e->FirstChildElement(kComp[i]);
		if(!c)
			return false;
		const char *t = c->GetText();
		if(!t)
			return false;
		if(sscanf(t, "%f", &dest[i]) != 1)
			return false;
	}
	return true;
}

// ---------------------------------------------------------------------------
// FUN_1001ba00 @ RVA 0x1ba00 — <type> text -> eVehicleType. The name table is
// .rdata in the original (unrecoverable from the decomp); the values are
// pinned by the downstream checks (0 car / 1 boat / 2 train / 3 heli /
// 4 plane / 5 bike — GTA's eVehicleType numbering) and the sample comments
// ("internal types plane and heli are reserved"). Name list: PROVISIONAL.
// ---------------------------------------------------------------------------
int
MvlLookupVehicleType(const char *text)
{
	static const char *const kTypes[6] = {
		"car", "boat", "train", "heli", "plane", "bike"
	};
	if(!text)
		return -1;
	for(int i = 0; i < 6; i++)
		if(_stricmp(text, kTypes[i]) == 0)
			return i;
	return -1;
}

// Degrees -> radians conversion used by bike angles and machinegun accuracy:
// the original computes (v * DAT_1008de7c) / _DAT_1008df08 in double.
static float
MvlDegToRad(float v)
{
	return (float)(((double)v * 3.14159265358979) / 180.0);
}

// ===========================================================================
// CMvlVehicle::LoadXMLBasic — vehmod_retail.dll 0.98e @ RVA 0x46420
// Table-driven in the original (PTR_DAT_100af138, 12 entries bound to the
// per-instance dest array at local_4c..local_20). Explicit dispatch here
// with identical semantics:
//   - unknown elements are silently ignored (fun_1001bb40 miss -> skip)
//   - "Element '%s' is not the first child of '%s' sector." fires when an
//     element after 'type' in the table appears before 'type'
//   - bit 0x10 wheelmodel, 0x20 wheelscale, 0x40 lodmodel, 0x80
//     wheelrotangle, 0x100 comprules, 0x200 extraflags drive the tail checks
// Table slot i=3 -> info+0x8c ("immunity", PROVISIONAL), i=10 ->
// this+0x2838 (element name unrecovered — left unhandled; unknown basic
// elements are ignored by the original anyway), i=11 -> this+0x2bec
// ("maxpassengers", PROVISIONAL). See port-notes-veh.md.
// ===========================================================================
bool
CMvlVehicle::LoadXMLBasic(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLBasic";
	const char *sectorName = "basic";
	unsigned seen = 0;
	SMvlVehicleInfo *info = m_pInfo;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		int idx = -1;
		if(_stricmp(name, "type") == 0)             idx = 0;
		else if(_stricmp(name, "name") == 0)        idx = 1;
		else if(_stricmp(name, "anims") == 0)       idx = 2;
		else if(_stricmp(name, "immunity") == 0)    idx = 3;
		else if(_stricmp(name, "wheelmodel") == 0)  idx = 4;
		else if(_stricmp(name, "wheelscale") == 0)  idx = 5;
		else if(_stricmp(name, "lodmodel") == 0)    idx = 6;
		else if(_stricmp(name, "wheelrotangle") == 0) idx = 7;
		else if(_stricmp(name, "comprules") == 0)   idx = 8;
		else if(_stricmp(name, "extraflags") == 0)  idx = 9;
		else if(_stricmp(name, "maxpassengers") == 0) idx = 11;
		if(idx < 0)
			continue;   // unknown basic element: ignored (original)

		if((seen & 1) == 0 && idx > 0){
			XmlErr(fn, "Element '%s' is not the first child of '%s' sector.",
			       name, sectorName);
			return false;
		}

		switch(idx){
		case 0: {   // type
			const char *t = e->GetText();
			int type = MvlLookupVehicleType(t);
			if(type < 0){
				XmlErr(fn, "Element '%s' has unknown value.", name);
				return false;
			}
			if(info)
				info->m_nVehicleType = type;
			if(type == 4 && info)
				info->m_fWheelSize = 1.0f;
			m_nType = type;
			seen |= 1u;
			break;
		}
		case 1: {   // name
			const char *t = e->GetText();
			if(!t){
				XmlWarn(fn, "Vehicle has no name.");
				MvlVehicleSetNameW(m_wsName, "");
			}else{
				if(strlen(t) < 2)
					XmlWarn(fn, "The name of the vehicle is less than 2 characters long.");
				MvlVehicleSetNameW(m_wsName, t);
			}
			seen |= 2u;
			break;
		}
		case 2: {   // anims
			const char *t = e->GetText();
			strncpy(m_szAnimsName, t ? t : "null", sizeof(m_szAnimsName) - 1);
			m_szAnimsName[sizeof(m_szAnimsName) - 1] = '\0';
			// HOOK POINT @ 0x46420 — info vtable +0x18/+0x1c consume the
			// anims name (FUN_1001ba40 + two virtual calls in the decomp).
			if(info){
				MvlVehicleInfoSetAnimsName(info, m_szAnimsName);
				MvlVehicleInfoPostAnimsName(info);
			}
			seen |= 4u;
			break;
		}
		case 3: {   // immunity (PROVISIONAL name; dest info+0x8c)
			int v = 0;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, name);
			if(info)
				info->nImmunity = v;   // @ 0x8c — table slot i=3 target
			seen |= 8u;
			break;
		}
		case 4: {   // wheelmodel (cars)
			int v = 0;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, name);
			if(m_nType != 0){
				XmlErr(fn, "'%s' element present when the vehicle type is not car.",
				       name);
				return false;
			}
			if(info){
				info->m_nWheelModelIndex = (short)v;
				if(info->m_nWheelModelIndex == 0 || info->m_nWheelModelIndex == 0xf9)
					info->m_nWheelModelIndex = 0xfa;
			}
			seen |= 0x10u;
			break;
		}
		case 5: {   // wheelscale (cars, bikes)
			float v = 0.0f;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%f", &v) != 1)
				return MvlErrParse(fn, name);
			if(m_nType != 0 && m_nType != 5){
				XmlErr(fn, "'%s' element present when the vehicle type is neither car nor bike.",
				       name);
				return false;
			}
			if(info)
				info->m_fWheelSize = v;
			seen |= 0x20u;
			break;
		}
		case 6: {   // lodmodel (planes)
			int v = 0;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, name);
			if(m_nType != 4){
				XmlErr(fn, "'%s' element present when the vehicle type is not plane.",
				       name);
				return false;
			}
			if(info)
				info->m_nWheelModelIndex = (short)v;
			seen |= 0x40u;
			break;
		}
		case 7: {   // wheelrotangle (bikes)
			float v = 0.0f;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%f", &v) != 1)
				return MvlErrParse(fn, name);
			if(m_nType != 5){
				XmlErr(fn, "'%s' element present when the vehicle type is not bike.",
				       name);
				return false;
			}
			if(info)
				info->m_fBikeSteerAngle = v;
			seen |= 0x80u;
			break;
		}
		case 8: {   // comprules
			int v = 0;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, name);
			m_nCompRules = v;
			seen |= 0x100u;
			break;
		}
		case 9: {   // extraflags (hex bitfield per flags.html)
			unsigned v = 0;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%x", &v) != 1)
				return MvlErrParse(fn, name);
			// "'%s' element can only specify one AI flag." — bits 8..17
			// (AI_POLICE..AI_TAXI per flags.html) may hold only one flag.
			if((v & 0x3ff00u) != 0 &&
			   ((((v & 0xffffff00u) - 1) & v & 0x3ff00u) != 0)){
				XmlErr(fn, "'%s' element can only specify one AI flag.", name);
				return false;
			}
			m_nExtraFlags = v;
			seen |= 0x200u;
			break;
		}
		case 11: {  // maxpassengers (PROVISIONAL name; dest this+0x2bec)
			int v = 0;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, name);
			m_nMaxPassengers = (unsigned)v;
			seen |= 0x800u;
			break;
		}
		}
	}

	// Tail checks (verbatim order and text).
	if(m_nType == 0){
		if((seen & 0x10) == 0){
			XmlErr(fn, "'basic' sector is missing 'wheelmodel' element, which is required for cars.");
			return false;
		}
		if((seen & 0x20) == 0){
			XmlErr(fn, "'basic' sector is missing 'wheelscale' element, which is required for cars.");
			return false;
		}
	}
	if(m_nType == 4 && (seen & 0x40) == 0){
		XmlErr(fn, "'basic' sector is missing 'lodmodel' element, which is required for planes.");
		return false;
	}
	if(m_nType == 5 && (seen & 0x80) == 0){
		XmlErr(fn, "'basic' sector is missing 'wheelrotangle' element, which is required for bikes.");
		return false;
	}
	if((seen & 0x100) != 0){
		if((seen & 0x200) == 0){
			XmlWarn(fn, "'%s' sector doesn't have 'extraflags' element, defaulting to 0.",
			        sectorName);
			m_nExtraFlags = 0;
		}
		return true;
	}
	m_nCompRules = -1;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLAIData — vehmod_retail.dll 0.98e @ RVA 0x46960
// ===========================================================================
bool
CMvlVehicle::LoadXMLAIData(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLAIData";
	bool bClass = false, bFreq = false, bLevel = false, bGang = false;
	SMvlVehicleInfo *info = m_pInfo;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "class") == 0){
			if(bClass)
				return MvlErrDup(fn, "aidata", "class"), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, "class");
			int v;
			if     (_stricmp(t, "normal") == 0)     v = 0;
			else if(_stricmp(t, "poorfamily") == 0) v = 1;
			else if(_stricmp(t, "richfamily") == 0) v = 2;
			else if(_stricmp(t, "executive") == 0)  v = 3;
			else if(_stricmp(t, "worker") == 0)     v = 4;
			else if(_stricmp(t, "big") == 0)        v = 5;
			else if(_stricmp(t, "taxi") == 0)       v = 6;
			else if(_stricmp(t, "moped") == 0)      v = 7;
			else if(_stricmp(t, "motorbike") == 0)  v = 8;
			else if(_stricmp(t, "leisureboat") == 0) v = 9;
			else if(_stricmp(t, "workerboat") == 0) v = 10;
			else if(_stricmp(t, "ignore") == 0)     v = 0xff;
			else{
				XmlErr(fn, "Element 'class' has unknown value.");
				return false;
			}
			if(info)
				info->m_nVehicleClass = (char)v;
			bClass = true;
		}
		else if(_stricmp(name, "freq") == 0){
			if(bFreq)
				return MvlErrDup(fn, "aidata", "freq"), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, "freq");
			int v = 0;
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, "freq");
			if(info)
				info->m_nFrq = (short)v;
			bFreq = true;
		}
		else if(_stricmp(name, "level") == 0){
			if(bLevel)
				return MvlErrDup(fn, "aidata", "level"), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, "level");
			int v = 0;
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, "level");
			if(info)
				info->m_nLvl = (char)v;
			bLevel = true;
		}
		else if(_stricmp(name, "gang") == 0){
			if(bGang){
				// QUIRK: original message says 'gamg'.
				XmlErr(fn, "'aidata' sector contains multiple 'gamg' elements.");
				return false;
			}
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, "gang");
			unsigned v = 0;
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, "level");   // QUIRK: shared label
			if(v > 7){
				XmlErr(fn, "The value of 'gang' element must be in the range 0-7.");
				return false;
			}
			m_nGang = v;
			bGang = true;
		}
		else
			return MvlErrUnknown(fn, "aidata", name);
	}

	if(!bClass)
		return MvlErrMissing(fn, "aidata", "class"), false;
	if(!bFreq)
		return MvlErrMissing(fn, "aidata", "freq"), false;
	if(!bLevel)
		return MvlErrMissing(fn, "aidata", "level"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLCarCol — vehmod_retail.dll 0.98e @ RVA 0x46e90
// Elements: carcol "u,u" | carcol4 "u,u,u,u" | rgbcol "u x6" | rgbcol4
// "u x12"; 2/4 palette or RGB colours per entry; max 16 entries.
// ===========================================================================
bool
CMvlVehicle::LoadXMLCarCol(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLCarCol";
	SMvlVehicleInfo *info = m_pInfo;
	int count = 0;

	if(info){
		info->bCarColInit1 = 1;
		info->bCarColInit2 = 1;
		info->bCarColInit3 = 1;
	}
	const unsigned white = MvlPackRgb(0xff, 0xff, 0xff);

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t = e->GetText();
		unsigned v[12];

		if(_stricmp(name, "carcol") == 0){
			if(!t)
				return MvlErrRead(fn, "carcol");
			if(count == 16)
				goto more16;
			if(sscanf(t, "%u,%u", &v[0], &v[1]) != 2)
				return MvlErrParse(fn, "carcol");
			for(int i = 0; i < 2; i++){
				if(v[i] > 0xff){
					XmlErr(fn, "The values of 'carcol' element are out of range 0-255.");
					return false;
				}
				if(v[i] == 0xff)
					v[i] = 0xfe;
			}
			m_anColours[count][0] = MvlGameColourTable[v[0]];
			m_anColours[count][1] = MvlGameColourTable[v[1]];
			m_anColours[count][2] = white;
			m_anColours[count][3] = white;
			count++;
		}
		else if(_stricmp(name, "carcol4") == 0){
			if(!t)
				return MvlErrRead(fn, "carcol4");
			if(count == 16)
				goto more16;
			if(sscanf(t, "%u,%u,%u,%u", &v[0], &v[1], &v[2], &v[3]) != 4)
				return MvlErrParse(fn, "carcol4");
			for(int i = 0; i < 4; i++){
				if(v[i] > 0xff){
					XmlErr(fn, "The values of 'carcol4' element are out of range 0-255.");
					return false;
				}
				if(v[i] == 0xff)
					v[i] = 0xfe;
			}
			m_anColours[count][0] = MvlGameColourTable[v[0]];
			m_anColours[count][1] = MvlGameColourTable[v[1]];
			m_anColours[count][2] = MvlGameColourTable[v[2]];
			m_anColours[count][3] = MvlGameColourTable[v[3]];
			count++;
		}
		else if(_stricmp(name, "rgbcol") == 0){
			if(!t)
				return MvlErrRead(fn, "rgbcol");
			if(count == 16)
				goto more16;
			if(sscanf(t, "%u,%u,%u,%u,%u,%u", &v[0], &v[1], &v[2],
			          &v[3], &v[4], &v[5]) != 6)
				return MvlErrParse(fn, "rgbcol");
			for(int i = 0; i < 6; i++){
				if(v[i] > 0xff){
					XmlErr(fn, "The values of 'rgbcol' element are out of range 0-255.");
					return false;
				}
			}
			m_anColours[count][0] = MvlPackRgb((int)v[0], (int)v[1], (int)v[2]);
			m_anColours[count][1] = MvlPackRgb((int)v[3], (int)v[4], (int)v[5]);
			m_anColours[count][2] = white;
			m_anColours[count][3] = white;
			count++;
		}
		else if(_stricmp(name, "rgbcol4") == 0){
			if(!t)
				return MvlErrRead(fn, "rgbcol4");
			if(count == 16)
				goto more16;
			if(sscanf(t, "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u",
			          &v[0], &v[1], &v[2], &v[3], &v[4], &v[5],
			          &v[6], &v[7], &v[8], &v[9], &v[10], &v[11]) != 12)
				return MvlErrParse(fn, "rgbcol4");
			for(int i = 0; i < 12; i++){
				if(v[i] > 0xff){
					XmlErr(fn, "The values of 'rgbcol4' element are out of range 0-255.");
					return false;
				}
			}
			m_anColours[count][0] = MvlPackRgb((int)v[0], (int)v[1], (int)v[2]);
			m_anColours[count][1] = MvlPackRgb((int)v[3], (int)v[4], (int)v[5]);
			m_anColours[count][2] = MvlPackRgb((int)v[6], (int)v[7], (int)v[8]);
			m_anColours[count][3] = MvlPackRgb((int)v[9], (int)v[10], (int)v[11]);
			count++;
		}
		else
			return MvlErrUnknown(fn, "colors", name);
		continue;

	more16:
		XmlWarn(fn, "More than 16 colour elements present, only 16 will be used.");
	}

	m_nColourCount = (count != 0) ? count : 1;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLAudioBoatEngine — vehmod_retail.dll 0.98e @ RVA 0x474d0
// ===========================================================================
bool
CMvlVehicle::LoadXMLAudioBoatEngine(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLAudioBoatEngine";
	const char *sn = "boatengine";
	bool bType = false, bBaseVol = false, bBaseFreq = false,
	     bVolInc = false, bFreqInc = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t;
		unsigned u;

		if(_stricmp(name, "type") == 0){
			if(bType)
				return MvlErrDup(fn, sn, name), false;
			t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%u", &u) != 1)
				return MvlErrParse(fn, name);
			if(u != 0 && u != 1){
				XmlErr(fn, "The value of '%s' element is out of range 0-1.", name);
				return false;
			}
			m_audio.nBoatEngineType = u;
			bType = true;
		}
		else if(_stricmp(name, "basevolume") == 0){
			if(bBaseVol)
				return MvlErrDup(fn, sn, name), false;
			t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%u", &u) != 1)
				return MvlErrParse(fn, name);
			if(u > 100){
				XmlErr(fn, "The value of '%s' element is out of range 0-100.", name);
				return false;
			}
			m_audio.nBoatEngineBaseVolume = u;
			bBaseVol = true;
		}
		else if(_stricmp(name, "basefrequency") == 0){
			if(bBaseFreq)
				return MvlErrDup(fn, sn, name), false;
			t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%u", &u) != 1)
				return MvlErrParse(fn, name);
			if((unsigned)(u - 5) > 0xf423b){   // decomp range test 5..1000000
				XmlErr(fn, "The value of '%s' element is out of range 5-1000000.", name);
				return false;
			}
			m_audio.nBoatEngineBaseFrequency = u;
			bBaseFreq = true;
		}
		else if(_stricmp(name, "volumeincrease") == 0){
			if(bVolInc)
				return MvlErrDup(fn, sn, name), false;
			t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%f", &m_audio.fBoatEngineVolumeIncrease) != 1)
				return MvlErrParse(fn, name);
			bVolInc = true;
		}
		else if(_stricmp(name, "frequencyincrease") == 0){
			if(bFreqInc)
				return MvlErrDup(fn, sn, name), false;
			t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%f", &m_audio.fBoatEngineFrequencyIncrease) != 1)
				return MvlErrParse(fn, name);
			bFreqInc = true;
		}
		else{
			// QUIRK: the original reports unknown elements against 'audio'.
			XmlErr(fn, "Unknown element '%s' in 'audio' sector.", name);
			return false;
		}
	}

	if(!bType)    return MvlErrMissing(fn, sn, "type"), false;
	if(!bBaseVol) return MvlErrMissing(fn, sn, "basevolume"), false;
	if(!bBaseFreq)return MvlErrMissing(fn, sn, "basefrequency"), false;
	if(!bVolInc)  return MvlErrMissing(fn, sn, "volumeincrease"), false;
	if(!bFreqInc) return MvlErrMissing(fn, sn, "frequencyincrease"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLAudio — vehmod_retail.dll 0.98e @ RVA 0x478a0
// Table-driven (8 entries). Scalar entries carry valid ranges initialised via
// FUN_1001ba60; range failures come from the shared table helper
// (FUN_1001bc10) whose own strings are not in this decomp range — the
// message below follows its closest recovered sibling (reconstructed, see
// port-notes-veh.md). Unknown elements are silently ignored (original).
// ===========================================================================
bool
CMvlVehicle::LoadXMLAudio(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLAudio";
	const char *sn = "audio";
	bool seen[7] = { false, false, false, false, false, false, false };
	bool bBoatEngine = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		int idx = -1;
		if(_stricmp(name, "enginefarsample") == 0)       idx = 0;
		else if(_stricmp(name, "enginenearsample") == 0) idx = 1;
		else if(_stricmp(name, "hornsample") == 0)       idx = 2;
		else if(_stricmp(name, "hornfreq") == 0)         idx = 3;
		else if(_stricmp(name, "sirensample") == 0)      idx = 4;
		else if(_stricmp(name, "sirenfreq") == 0)        idx = 5;
		else if(_stricmp(name, "doorsounds") == 0)       idx = 6;
		else if(_stricmp(name, "boatengine") == 0)       idx = 7;
		if(idx < 0)
			continue;   // unknown audio element: ignored (original)

		if(idx == 7){
			// table bit 0x80 in the original's bitmask
			if(!LoadXMLAudioBoatEngine(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bBoatEngine = true;
			continue;
		}

		static const unsigned kMin[7] = { 268, 0, 0, 5, 0, 5, 0 };
		static const unsigned kMax[7] = { 457, 35, 35, 1000000, 3, 1000000, 3 };
		unsigned u = 0;
		if(seen[idx])
			return MvlErrDup(fn, sn, name), false;
		const char *t = e->GetText();
		if(!t)
			return MvlErrRead(fn, name);
		if(sscanf(t, "%u", &u) != 1)
			return MvlErrParse(fn, name);
		if(u < kMin[idx] || u > kMax[idx]){
			// RECONSTRUCTED message (shared helper string, see notes).
			XmlErr(fn, "The value of '%s' element is out of range %u-%u.",
			       name, kMin[idx], kMax[idx]);
			return false;
		}
		switch(idx){
		case 0: m_audio.nEngineFarSample  = u; break;
		case 1: m_audio.nEngineNearSample = u; break;
		case 2: m_audio.nHornSample       = u; break;
		case 3: m_audio.nHornFreq         = u; break;
		case 4: m_audio.nSirenSample      = u; break;
		case 5: m_audio.nSirenFreq        = u; break;
		case 6: m_nDoorSounds = (unsigned char)u; break;
		}
		seen[idx] = true;
	}

	// Tail: boat without <boatengine> gets defaults (verbatim warning).
	if(!bBoatEngine && m_nType == 1){
		m_audio.nBoatEngineType = 1;
		m_audio.nBoatEngineBaseVolume = 0x14;
		m_audio.nBoatEngineBaseFrequency = 9000;
		m_audio.fBoatEngineVolumeIncrease = 60.0f;       // 0x42700000
		m_audio.fBoatEngineFrequencyIncrease = 6000.0f;  // 0x45bb8000
		XmlWarn(fn, "'%s' sector for a boat has no 'boatengine' sector. Using default values.",
		        sn);
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandlingTraction — vehmod_retail.dll 0.98e @ RVA 0x47c40
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandlingTraction(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandlingTraction";
	const char *sn = "traction";
	bool bMul = false, bLoss = false, bBias = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "multiplier") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bMul, &m_handling.fTractionMultiplier))
				return false;
		}
		else if(_stricmp(name, "loss") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bLoss, &m_handling.fTractionLoss))
				return false;
		}
		else if(_stricmp(name, "bias") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bBias, &m_handling.fTractionBias))
				return false;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bMul)  return MvlErrMissing(fn, sn, "multiplier"), false;
	if(!bLoss) return MvlErrMissing(fn, sn, "loss"), false;
	if(!bBias) return MvlErrMissing(fn, sn, "bias"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandlingTransmission — 0.98e @ RVA 0x47f10
// QUIRK: the tail "missing" messages name 'multiplier'/'loss'/'bias'
// (original copy/paste bug) — kept verbatim.
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandlingTransmission(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandlingTransmission";
	const char *sn = "transmission";
	bool bGears = false, bMax = false, bAcc = false, bDrive = false, bEngine = false;
	SMvlVehicleHandling &h = m_handling;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t;
		int iv;

		if(_stricmp(name, "numofgears") == 0){
			if(!MvlElemInt(e, fn, sn, name, &bGears, &iv))
				return false;
			h.nNumGears = (unsigned char)iv;
		}
		else if(_stricmp(name, "maxspeed") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bMax, &h.fMaxVelocity))
				return false;
		}
		else if(_stricmp(name, "acceleration") == 0){
			float v = 0.0f;
			if(!MvlElemFloat(e, fn, sn, name, &bAcc, &v))
				return false;
			// decomp: *(0x98) = v * DAT_1008ddfc (conversion constant —
			// exact value unrecovered; see port-notes-veh.md).
			h.fAcceleration = v * 1.0f /* DAT_1008ddfc */;
		}
		else if(_stricmp(name, "drivetype") == 0){
			if(bDrive)
				return MvlErrDup(fn, sn, name), false;
			t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%c", &h.nDriveType) != 1)
				return MvlErrParse(fn, name);
			bDrive = true;
		}
		else if(_stricmp(name, "enginetype") == 0){
			if(bEngine)
				return MvlErrDup(fn, sn, name), false;
			t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%c", &h.nEngineType) != 1)
				return MvlErrParse(fn, name);
			bEngine = true;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bGears)  return MvlErrMissing(fn, sn, "multiplier"), false;
	if(!bMax)    return MvlErrMissing(fn, sn, "loss"), false;
	if(!bAcc)    return MvlErrMissing(fn, sn, "bias"), false;
	if(!bDrive)  return MvlErrMissing(fn, sn, "drivetype"), false;
	if(!bEngine) return MvlErrMissing(fn, sn, "enginetype"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandlingBrakes — 0.98e @ RVA 0x483b0
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandlingBrakes(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandlingBrakes";
	const char *sn = "brakes";
	bool bDec = false, bBias = false, bAbs = false;
	SMvlVehicleHandling &h = m_handling;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "deceleration") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bDec, &h.fBrakeDeceleration))
				return false;
		}
		else if(_stricmp(name, "bias") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bBias, &h.fBrakeBias))
				return false;
		}
		else if(_stricmp(name, "abs") == 0){
			if(bAbs)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			int v = 0;
			if(sscanf(t, "%i", &v) != 1)
				return MvlErrParse(fn, name);
			if(v != 0 && v != 1){
				XmlErr(fn, "Element 'abs' can only have values 0 or 1.");
				return false;
			}
			h.nBrakeAbs = (unsigned char)v;
			bAbs = true;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bDec)  return MvlErrMissing(fn, sn, "deceleration"), false;
	if(!bBias) return MvlErrMissing(fn, sn, "bias"), false;
	if(!bAbs)  return MvlErrMissing(fn, sn, "abs"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandlingBoatBrakes — 0.98e @ RVA 0x486c0
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandlingBoatBrakes(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandlingBoatBrakes";
	const char *sn = "boatbrakes";
	bool bVert = false, bFwd = false;
	SMvlVehicleHandling &h = m_handling;

	h.nBrakeAbs = 0;   // decomp pre-store *(0xb4) = 0

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "verticalwavehitlimit") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bVert, &h.fBrakeDeceleration))
				return false;
		}
		else if(_stricmp(name, "forwardwavehitbrake") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bFwd, &h.fBrakeBias))
				return false;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bVert) return MvlErrMissing(fn, sn, "verticalwavehitlimit"), false;
	if(!bFwd)  return MvlErrMissing(fn, sn, "forwardwavehitbrake"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandlingSuspension — 0.98e @ RVA 0x488d0
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandlingSuspension(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandlingSuspension";
	const char *sn = "suspension";
	bool b[6] = { false, false, false, false, false, false };
	float *dest[6] = {
		&m_handling.fSusp1, &m_handling.fSusp2, &m_handling.fSusp3,
		&m_handling.fSusp4, &m_handling.fSusp5, &m_handling.fSusp6
	};
	static const char *const kNames[6] = {
		"forcelevel", "dampening", "upperlimit", "lowerlimit", "bias", "antidive"
	};

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		int i;
		for(i = 0; i < 6; i++)
			if(_stricmp(name, kNames[i]) == 0)
				break;
		if(i == 6)
			return MvlErrUnknown(fn, sn, name);
		if(!MvlElemFloat(e, fn, sn, kNames[i], &b[i], dest[i]))
			return false;
	}

	for(int i = 0; i < 6; i++)
		if(!b[i])
			return MvlErrMissing(fn, sn, kNames[i]), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandlingBoatSuspension — 0.98e @ RVA 0x48e20
// Same slot layout as the car suspension (0xc8..0xdc).
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandlingBoatSuspension(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandlingBoatSuspension";
	const char *sn = "boatsuspension";
	bool b[6] = { false, false, false, false, false, false };
	float *dest[6] = {
		&m_handling.fSusp1, &m_handling.fSusp2, &m_handling.fSusp3,
		&m_handling.fSusp4, &m_handling.fSusp5, &m_handling.fSusp6
	};
	static const char *const kNames[6] = {
		"waterresvolumemult", "waterdampingmult", "upperlimit",
		"handbrakedragmult", "sideslipforce", "antidive"
	};

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		int i;
		for(i = 0; i < 6; i++)
			if(_stricmp(name, kNames[i]) == 0)
				break;
		if(i == 6)
			return MvlErrUnknown(fn, sn, name);
		if(!MvlElemFloat(e, fn, sn, kNames[i], &b[i], dest[i]))
			return false;
	}

	for(int i = 0; i < 6; i++)
		if(!b[i])
			return MvlErrMissing(fn, sn, kNames[i]), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandlingBoatSteering — 0.98e @ RVA 0x49370
// Shares the traction slots (0x48/0xbc/0xc0) — sectors are exclusive.
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandlingBoatSteering(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandlingBoatSteering";
	const char *sn = "boatsteering";
	bool bBank = false, bRud = false, bFall = false;
	SMvlVehicleHandling &h = m_handling;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "bankforcemult") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bBank, &h.fTractionMultiplier))
				return false;
		}
		else if(_stricmp(name, "rudderturnforce") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bRud, &h.fTractionLoss))
				return false;
		}
		else if(_stricmp(name, "speedsteerfalloff") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bFall, &h.fTractionBias))
				return false;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bBank) return MvlErrMissing(fn, sn, "bankforcemult"), false;
	if(!bRud)  return MvlErrMissing(fn, sn, "rudderturnforce"), false;
	if(!bFall) return MvlErrMissing(fn, sn, "speedsteerfalloff"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandlingLights — 0.98e @ RVA 0x49640
// QUIRK: the element is named 'hasboxes' but every error message for it says
// 'lightbox' — kept verbatim.
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandlingLights(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandlingLights";
	const char *sn = "lights";
	bool bFront = false, bRear = false, bBox = false;
	SMvlVehicleHandling &h = m_handling;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t;
		int iv;

		if(_stricmp(name, "front") == 0){
			if(!MvlElemInt(e, fn, sn, name, &bFront, &iv))
				return false;
			h.nLightsFront = (unsigned char)iv;
		}
		else if(_stricmp(name, "rear") == 0){
			if(!MvlElemInt(e, fn, sn, name, &bRear, &iv))
				return false;
			h.nLightsRear = (unsigned char)iv;
		}
		else if(_stricmp(name, "hasboxes") == 0){
			if(bBox){
				XmlErr(fn, "'lights' sector contains multiple 'lightbox' elements.");
				return false;
			}
			t = e->GetText();
			if(!t){
				XmlErr(fn, "Failed to read data from 'lightbox' element.");
				return false;
			}
			if(_stricmp(t, "true") == 0)
				m_audio.bHasLightBox = 1;
			else if(_stricmp(t, "false") == 0)
				m_audio.bHasLightBox = 0;
			else{
				XmlErr(fn, "The value of 'lightbox' can only be either 'true' or 'false'.");
				return false;
			}
			bBox = true;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bFront) return MvlErrMissing(fn, sn, "front"), false;
	if(!bRear)  return MvlErrMissing(fn, sn, "rear"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHandling — 0.98e @ RVA 0x49930
// ===========================================================================
bool
CMvlVehicle::LoadXMLHandling(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHandling";
	const char *sn = "handling";
	bool bMass = false, bPct = false, bSteer = false, bSeat = false,
	     bDmg = false, bValue = false, bFlags = false, bDims = false,
	     bCom = false, bTraction = false, bBoatSteer = false,
	     bTrans = false, bBrakes = false, bBoatBrakes = false,
	     bSusp = false, bBoatSusp = false, bLights = false;
	SMvlVehicleHandling &h = m_handling;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();

		if(_stricmp(name, "mass") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bMass, &h.fMass))
				return false;
		}
		else if(_stricmp(name, "percentsubmerged") == 0){
			if(!MvlElemInt(e, fn, sn, name, &bPct, &h.nPercentSubmerged))
				return false;
		}
		else if(_stricmp(name, "steeringlock") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bSteer, &h.fSteeringLock))
				return false;
		}
		else if(_stricmp(name, "seatoffset") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bSeat, &h.fSeatOffset))
				return false;
		}
		else if(_stricmp(name, "damagemultiplier") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bDmg, &h.fDamageMultiplier))
				return false;
		}
		else if(_stricmp(name, "value") == 0){
			if(!MvlElemInt(e, fn, sn, name, &bValue, &h.nMonetaryValue))
				return false;
		}
		else if(_stricmp(name, "flags") == 0){
			if(bFlags)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			unsigned v = 0;
			if(sscanf(t, "%x", &v) != 1)
				return MvlErrParse(fn, name);
			h.nFlagsLowByte = (unsigned char)v;
			h.nHandlingFlags = v;
			bFlags = true;
		}
		else if(_stricmp(name, "dimensions") == 0){
			// QUIRK: no duplicate check for dimensions (last one wins).
			if(!MvlParseVector(e, h.fDimensions))
				return MvlErrVector(fn, "dimensions");
			bDims = true;
		}
		else if(_stricmp(name, "centreofmass") == 0){
			if(bCom)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, h.fCentreOfMass))
				return MvlErrVector(fn, "centreofmass");
			bCom = true;
		}
		else if(_stricmp(name, "traction") == 0){
			if(m_nType == 1){
				XmlErr(fn, "Boats must have a 'boatsteering' sector instead of 'traction'.");
				return false;
			}
			if(bTraction)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHandlingTraction(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bTraction = true;
		}
		else if(_stricmp(name, "boatsteering") == 0){
			if(m_nType != 1){
				XmlErr(fn, "Non-boats must have a 'traction' sector instead of 'boatsteering'.");
				return false;
			}
			if(bBoatSteer)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHandlingBoatSteering(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bBoatSteer = true;
		}
		else if(_stricmp(name, "transmission") == 0){
			if(bTrans)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHandlingTransmission(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bTrans = true;
		}
		else if(_stricmp(name, "brakes") == 0){
			if(m_nType == 1){
				XmlErr(fn, "Boats must have a 'boatbrakes' sector instead of 'brakes'.");
				return false;
			}
			if(bBrakes)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHandlingBrakes(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bBrakes = true;
		}
		else if(_stricmp(name, "boatbrakes") == 0){
			if(m_nType != 1){
				XmlErr(fn, "Non-boats must have a 'brakes' sector instead of 'boatbrakes'.");
				return false;
			}
			if(bBoatBrakes)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHandlingBoatBrakes(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bBoatBrakes = true;
		}
		else if(_stricmp(name, "suspension") == 0){
			if(m_nType == 1){
				XmlErr(fn, "Boats must have a 'boatsuspension' sector instead of 'suspension'.");
				return false;
			}
			if(bSusp)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHandlingSuspension(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bSusp = true;
		}
		else if(_stricmp(name, "boatsuspension") == 0){
			if(m_nType != 1){
				XmlErr(fn, "Non-boats must have a 'suspension' sector instead of 'boatsuspension'.");
				return false;
			}
			if(bBoatSusp)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHandlingBoatSuspension(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bBoatSusp = true;
		}
		else if(_stricmp(name, "lights") == 0){
			if(bLights)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHandlingLights(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bLights = true;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bMass)  return MvlErrMissing(fn, sn, "mass"), false;
	if(!bPct)   return MvlErrMissing(fn, sn, "percentsubmerged"), false;
	if(!bSteer) return MvlErrMissing(fn, sn, "steeringlock"), false;
	if(!bSeat)  return MvlErrMissing(fn, sn, "seatoffset"), false;
	if(!bDmg)   return MvlErrMissing(fn, sn, "damagemultiplier"), false;
	if(!bFlags) return MvlErrMissing(fn, sn, "flags"), false;
	if(!bDims)
		XmlWarn(fn, "No dimensions specified in handling, going to get it from collision info later.");
	if(!bValue) return MvlErrMissing(fn, sn, "value"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLAeroHandling — 0.98e @ RVA 0x4a610
// QUIRK: 'pitch' has no duplicate check in the original (last one wins).
// ===========================================================================
bool
CMvlVehicle::LoadXMLAeroHandling(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLAeroHandling";
	const char *sn = "aerohandling";
	bool b[12] = { false, false, false, false, false, false,
	               false, false, false, false, false, false };
	float *dest[12] = {
		&m_handling.fAeroThrust, &m_handling.fAeroThrustFalloff,
		&m_handling.fAeroYaw, &m_handling.fAeroYawStab,
		&m_handling.fAeroSideSlip, &m_handling.fAeroRoll,
		&m_handling.fAeroRollStab, &m_handling.fAeroPitch,
		&m_handling.fAeroPitchStab, &m_handling.fAeroFormLift,
		&m_handling.fAeroAttackLift, &m_handling.fAeroMoveRes
	};
	static const char *const kNames[12] = {
		"thrust", "thrustfalloff", "yaw", "yawstab", "sideslip", "roll",
		"rollstab", "pitch", "pitchstab", "formlift", "attacklift", "moveres"
	};
	bool bTurnRes = false, bSpeedRes = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		int i;
		for(i = 0; i < 12; i++)
			if(_stricmp(name, kNames[i]) == 0)
				break;
		if(i == 12){
			if(_stricmp(name, "turnres") == 0){
				if(bTurnRes)
					return MvlErrDupVector(fn, sn, name);
				if(!MvlParseVector(e, m_handling.fAeroTurnRes))
					return MvlErrVector(fn, "turnres");
				bTurnRes = true;
				continue;
			}
			if(_stricmp(name, "speedres") == 0){
				if(bSpeedRes)
					return MvlErrDupVector(fn, sn, name);
				if(!MvlParseVector(e, m_handling.fAeroSpeedRes))
					return MvlErrVector(fn, "speedres");
				bSpeedRes = true;
				continue;
			}
			return MvlErrUnknown(fn, sn, name);
		}
		if(i == 7){   // QUIRK: 'pitch' — no dupe check in the original
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, kNames[i]);
			if(sscanf(t, "%f", dest[i]) != 1)
				return MvlErrParse(fn, kNames[i]);
			continue;
		}
		if(!MvlElemFloat(e, fn, sn, kNames[i], &b[i], dest[i]))
			return false;
	}

	for(int i = 0; i < 12; i++)
		if(!b[i])
			return MvlErrMissing(fn, sn, kNames[i]), false;
	if(!bTurnRes)  return MvlErrMissingVector(fn, sn, "turnres"), false;
	if(!bSpeedRes) return MvlErrMissingVector(fn, sn, "speedres"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLBikeHandling — 0.98e @ RVA 0x4b1a0
// QUIRK: 'speedsteer' has no duplicate check in the original.
// maxlean/fullanimlean/wheelieang/stoppieang are stored converted deg->rad.
// ===========================================================================
bool
CMvlVehicle::LoadXMLBikeHandling(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLBikeHandling";
	const char *sn = "bikehandling";
	static const char *const kNames[15] = {
		"leanfwdcom", "leanfwdforce", "leanbackcom", "leanbackforce",
		"maxlean", "fullanimlean", "deslean", "speedsteer", "slipsteer",
		"noplayercomz", "wheelieang", "stoppieang", "wheeliesteer",
		"wheeliestabmult", "stoppiestabmult"
	};
	SMvlVehicleHandling &h = m_handling;
	float *dest[15] = {
		&h.fBikeLeanFwdCom, &h.fBikeLeanFwdForce,
		&h.fBikeLeanBackCom, &h.fBikeLeanBackForce,
		&h.fBikeMaxLean, &h.fBikeFullAnimLean,
		&h.fBikeDesLean, &h.fBikeSpeedSteer, &h.fBikeSlipSteer,
		&h.fBikeNoPlayerComZ, &h.fBikeWheelieAng, &h.fBikeStopPieAng,
		&h.fBikeWheelieSteer, &h.fBikeWheelieStabMult, &h.fBikeStopPieStabMult
	};
	static const bool kDeg2Rad[15] = {
		false, false, false, false,
		true, true,               // maxlean, fullanimlean
		false, false, false, false,
		true, true,               // wheelieang, stoppieang
		false, false, false
	};
	bool b[15] = { false, false, false, false, false,
	               false, false, false, false, false,
	               false, false, false, false, false };

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		int i;
		for(i = 0; i < 15; i++)
			if(_stricmp(name, kNames[i]) == 0)
				break;
		if(i == 15)
			return MvlErrUnknown(fn, sn, name);
		if(i == 7){   // QUIRK: 'speedsteer' — no dupe check
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, kNames[i]);
			if(sscanf(t, "%f", dest[i]) != 1)
				return MvlErrParse(fn, kNames[i]);
			continue;
		}
		float v = 0.0f;
		if(!MvlElemFloat(e, fn, sn, kNames[i], &b[i], &v))
			return false;
		*dest[i] = kDeg2Rad[i] ? MvlDegToRad(v) : v;
	}

	// Tail: verbatim order (note stoppie/wheelie order quirks).
	if(!b[0])  return MvlErrMissing(fn, sn, "leanfwdcom"), false;
	if(!b[1])  return MvlErrMissing(fn, sn, "leanfwdforce"), false;
	if(!b[2])  return MvlErrMissing(fn, sn, "leanbackcom"), false;
	if(!b[3])  return MvlErrMissing(fn, sn, "leanbackforce"), false;
	if(!b[4])  return MvlErrMissing(fn, sn, "maxlean"), false;
	if(!b[5])  return MvlErrMissing(fn, sn, "fullanimlean"), false;
	if(!b[6])  return MvlErrMissing(fn, sn, "deslean"), false;
	if(!b[7])  return MvlErrMissing(fn, sn, "speedsteer"), false;
	if(!b[8])  return MvlErrMissing(fn, sn, "slipsteer"), false;
	if(!b[9])  return MvlErrMissing(fn, sn, "noplayercomz"), false;
	if(!b[10]) return MvlErrMissing(fn, sn, "wheelieang"), false;
	if(!b[11]) return MvlErrMissing(fn, sn, "stoppieang"), false;
	if(!b[12]) return MvlErrMissing(fn, sn, "wheeliesteer"), false;
	if(!b[13]) return MvlErrMissing(fn, sn, "wheeliestabmult"), false;
	if(!b[14]) return MvlErrMissing(fn, sn, "stoppiestabmult"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLBoatHandling — 0.98e @ RVA 0x4bee0
// ===========================================================================
bool
CMvlVehicle::LoadXMLBoatHandling(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLBoatHandling";
	const char *sn = "boathandling";
	bool bThrustY = false, bThrustZ = false, bThrustAppZ = false,
	     bAqForce = false, bAqLimit = false, bAqOffset = false,
	     bWaveAudio = false, bMoveRes = false, bTurnRes = false,
	     bCamHeight = false;
	SMvlVehicleHandling &h = m_handling;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "thrusty") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bThrustY, &h.fBoatThrustY))
				return false;
		}
		else if(_stricmp(name, "thrustz") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bThrustZ, &h.fBoatThrustZ))
				return false;
		}
		else if(_stricmp(name, "thrustappz") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bThrustAppZ, &h.fBoatThrustAppZ))
				return false;
		}
		else if(_stricmp(name, "aqplaneforce") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bAqForce, &h.fBoatAqPlaneForce))
				return false;
		}
		else if(_stricmp(name, "aqplanelimit") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bAqLimit, &h.fBoatAqPlaneLimit))
				return false;
		}
		else if(_stricmp(name, "aqplaneoffset") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bAqOffset, &h.fBoatAqPlaneOffset))
				return false;
		}
		else if(_stricmp(name, "waveaudiomult") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bWaveAudio, &h.fBoatWaveAudioMult))
				return false;
		}
		else if(_stricmp(name, "moveres") == 0){
			if(bMoveRes)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, h.fBoatMoveRes))
				return MvlErrVector(fn, "moveres");
			bMoveRes = true;
		}
		else if(_stricmp(name, "turnres") == 0){
			if(bTurnRes)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, h.fBoatTurnRes))
				return MvlErrVector(fn, "turnres");
			bTurnRes = true;
		}
		else if(_stricmp(name, "looklrbcamheight") == 0){
			if(bCamHeight)
				return MvlErrDup(fn, sn, name), false;
			if(!MvlElemFloat(e, fn, sn, name, &bCamHeight, &h.fBoatLookLrbCamHeight))
				return false;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bThrustY)   return MvlErrMissing(fn, sn, "thrusty"), false;
	if(!bThrustZ)   return MvlErrMissing(fn, sn, "thrustz"), false;
	if(!bThrustAppZ)return MvlErrMissing(fn, sn, "thrustappz"), false;
	if(!bAqForce)   return MvlErrMissing(fn, sn, "aqplaneforce"), false;
	if(!bAqLimit)   return MvlErrMissing(fn, sn, "aqplanelimit"), false;
	if(!bMoveRes)   return MvlErrMissingVector(fn, sn, "moveres"), false;
	if(!bWaveAudio) return MvlErrMissing(fn, sn, "waveaudiomult"), false;
	if(!bAqOffset)  return MvlErrMissing(fn, sn, "aqplaneoffset"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHeliData — 0.98e @ RVA 0x4c700
// ===========================================================================
bool
CMvlVehicle::LoadXMLHeliData(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHeliData";
	const char *sn = "helidata";
	bool bType = false, bWeps = false, bRadius = false, bDamp = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "helitype") == 0){
			if(bType)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			int v;
			if     (_stricmp(t, "normal") == 0) v = 0;
			else if(_stricmp(t, "remote") == 0) v = 1;
			else if(_stricmp(t, "water") == 0)  v = 2;
			else if(_stricmp(t, "hunter") == 0) v = 3;
			else{
				XmlErr(fn, "Element 'helitype' has unknown value.");
				return false;
			}
			m_nHeliType = v;
			bType = true;
		}
		else if(_stricmp(name, "weapons") == 0){
			if(bWeps)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			int v;
			if     (_stricmp(t, "none") == 0)    v = 0;
			else if(_stricmp(t, "hunter") == 0)  v = 1;
			else if(_stricmp(t, "sparrow") == 0) v = 2;
			else{
				XmlErr(fn, "Element 'weapons' has unknown value.");
				return false;
			}
			m_nHeliWeapons = v;
			bWeps = true;
		}
		else if(_stricmp(name, "rotorradius") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bRadius, &m_fRotorRadius))
				return false;
		}
		else if(_stricmp(name, "rotordammult") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bDamp, &m_fRotorDamageMult))
				return false;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bType)   return MvlErrMissing(fn, sn, "helitype"), false;
	if(!bWeps)   return MvlErrMissing(fn, sn, "weapons"), false;
	if(!bRadius) return MvlErrMissing(fn, sn, "rotorradius"), false;
	if(!bDamp)   return MvlErrMissing(fn, sn, "rotordammult"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLHeliWepsData — 0.98e @ RVA 0x4cb80
// No required elements in the original (empty sector succeeds).
// ===========================================================================
bool
CMvlVehicle::LoadXMLHeliWepsData(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLHeliWepsData";
	const char *sn = "heliweps";
	bool bGun = false, bMissile = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "machinegunpos") == 0){
			if(bGun)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, m_fHeliGunPos))
				return MvlErrVector(fn, "machinegunpos");
			m_bHasHeliGunPos = 1;
			bGun = true;
		}
		else if(_stricmp(name, "missilepos") == 0){
			if(bMissile)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, m_fHeliMissilePos))
				return MvlErrVector(fn, "missilepos");
			m_bHasHeliMissilePos = 1;
			bMissile = true;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLSpecialData — 0.98e @ RVA 0x4ccd0
// EM-light sectors are parsed by CVehicleHandler functions (0x4e8f0 /
// 0x4edc0 / 0x4f090) — OUT OF SCOPE hooks here; only the dispatch and
// messages are ported in this lane.
// ===========================================================================
bool
CMvlVehicle::LoadXMLSpecialData(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLSpecialData";
	const char *sn = "specials";
	int emKind = 0;   // 1 emlights, 2 emlightsingle, 4 taxilight
	bool bHeliWeps = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		bool ok;
		if(_stricmp(name, "emlights") == 0){
			if(emKind != 0)
				goto emDup;
			ok = LoadXMLEmLightData(e, m_abEmLightData);   // 0x4e8f0 HOOK
			if(!ok){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			emKind = 1;
		}
		else if(_stricmp(name, "emlightsingle") == 0){
			if(emKind != 0)
				goto emDup;
			ok = LoadXMLEmLightSingle(e, m_abEmLightData); // 0x4edc0 HOOK
			if(!ok){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			emKind = 2;
		}
		else if(_stricmp(name, "taxilight") == 0){
			if(emKind != 0)
				goto emDup;
			ok = LoadXMLTaxiLight(e, m_abEmLightData);     // 0x4f090 HOOK
			if(!ok){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			emKind = 4;
		}
		else if(_stricmp(name, "heliweps") == 0){
			if(bHeliWeps)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLHeliWepsData(e)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bHeliWeps = true;
		}
		else
			return MvlErrUnknown(fn, sn, name);
		continue;

	emDup:
		XmlErr(fn, "'specials' sector contains multiple EM light sectors ('emlights'/'emlightsingle'/'taxilight').");
		return false;
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLWeaponKeyData — 0.98e @ RVA 0x4cec0
// 5 key combinations, "%u,%u,%u" into 3 bytes each (@ w+0xac + i*4).
// ===========================================================================
bool
CMvlVehicle::LoadXMLWeaponKeyData(TiXmlElement *sector, SMvlVehicleWeaponData *w)
{
	static const char *const fn = "CMvlVehicle::LoadXMLWeaponKeyData";
	const char *sn = "hotkeys";
	static const char *const kNames[5] = {
		"dockkey", "moveupkey", "movedownkey", "restorekey", "shootkey"
	};
	unsigned seen = 0;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		int i;
		for(i = 0; i < 5; i++)
			if(_stricmp(name, kNames[i]) == 0)
				break;
		if(i == 5){
			// QUIRK: generic "%s"/"%s" messages (element, sector).
			XmlErr(fn, "Unknown element '%s' in '%s' sector.", name, sn);
			return false;
		}
		if(seen & (1u << i)){
			XmlErr(fn, "'%s' sector contains multiple '%s' elements.", sn, kNames[i]);
			return false;
		}
		const char *t = e->GetText();
		if(!t){
			XmlErr(fn, "Failed to read data from '%s' element.", kNames[i]);
			return false;
		}
		unsigned a = 0, b = 0, c = 0;
		if(sscanf(t, "%u,%u,%u", &a, &b, &c) != 3){
			XmlErr(fn, "Failed to parse the contents of '%s' element (format 'x,x,x').",
			       kNames[i]);
			return false;
		}
		w->aHotKeys[i][0] = (unsigned char)a;
		w->aHotKeys[i][1] = (unsigned char)b;
		w->aHotKeys[i][2] = (unsigned char)c;
		seen |= 1u << i;
	}

	for(unsigned i = 0; i < 5; i++){
		if((seen & (1u << i)) == 0){
			XmlErr(fn, "'hotkeys' sector is missing '%s' element.", kNames[i]);
			return false;
		}
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLWeaponMissileInfo — 0.98e @ RVA 0x4d0d0
// ===========================================================================
bool
CMvlVehicle::LoadXMLWeaponMissileInfo(TiXmlElement *sector, SMvlVehicleWeaponData *w)
{
	static const char *const fn = "CMvlVehicle::LoadXMLWeaponMissileInfo";
	const char *sn = "missileinfo";
	bool bGravity = false, bTimeout = false, bAddSpeed = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "gravity") == 0){
			if(bGravity)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(_stricmp(t, "on") == 0){
				w->bGravity = 1;
				bGravity = true;
			}
			else if(_stricmp(t, "off") == 0){
				bGravity = true;   // 'off' leaves the default 0
			}
			else{
				XmlErr(fn, "Element 'gravity' may only have values 'on' or 'off'.");
				return false;
			}
		}
		else if(_stricmp(name, "timeout") == 0){
			if(bTimeout)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%u", &w->nMissileTimeout) != 1)
				return MvlErrParse(fn, name);
			bTimeout = true;
		}
		else if(_stricmp(name, "addspeedmul") == 0){
			if(!MvlElemFloat(e, fn, sn, name, &bAddSpeed, &w->fMissileAddSpeedMul))
				return false;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bGravity) return MvlErrMissing(fn, sn, "gravity"), false;
	if(!bTimeout) return MvlErrMissing(fn, sn, "timeout"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLWeaponMachineGunInfo — 0.98e @ RVA 0x4d380
// No required elements. QUIRK: the unknown-element message names
// 'missileinfo' (original copy/paste bug). range clamp: negative -> 1.0
// (DAT_1008de28); accuracy clamp 0..45 deg then deg->rad; timeout 0 -> 25.
// ===========================================================================
bool
CMvlVehicle::LoadXMLWeaponMachineGunInfo(TiXmlElement *sector, SMvlVehicleWeaponData *w)
{
	static const char *const fn = "CMvlVehicle::LoadXMLWeaponMachineGunInfo";
	const char *sn = "machineguninfo";
	bool bRange = false, bAcc = false, bTimeout = false, bSound = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		const char *t;

		if(_stricmp(name, "range") == 0){
			if(bRange){
				XmlErr(fn, "'%s' sector contains multiple '%s' elements.", sn, name);
				return false;
			}
			t = e->GetText();
			if(!t){
				XmlErr(fn, "Failed to read data from '%s' element.", name);
				return false;
			}
			float v = 0.0f;
			if(sscanf(t, "%f", &v) != 1){
				XmlErr(fn, "Failed to parse the contents of '%s' element.", name);
				return false;
			}
			if(v < 0.0f)
				v = 1.0f;   // DAT_1008de28
			w->fMachineGunRange = v;
			bRange = true;
		}
		else if(_stricmp(name, "accuracy") == 0){
			if(bAcc){
				XmlErr(fn, "'%s' sector contains multiple '%s' elements.", sn, name);
				return false;
			}
			t = e->GetText();
			if(!t){
				XmlErr(fn, "Failed to read data from '%s' element.", name);
				return false;
			}
			float v = 0.0f;
			if(sscanf(t, "%f", &v) != 1){
				XmlErr(fn, "Failed to parse the contents of '%s' element.", name);
				return false;
			}
			if(v < 0.0f)
				v = 0.0f;
			else if(v > 45.0f)   // DAT_1008ded4
				v = 45.0f;
			w->fMachineGunAccuracy = MvlDegToRad(v);
			bAcc = true;
		}
		else if(_stricmp(name, "timeout") == 0){
			if(bTimeout){
				XmlErr(fn, "'%s' sector contains multiple '%s' elements.", sn, name);
				return false;
			}
			t = e->GetText();
			if(!t){
				XmlErr(fn, "Failed to read data from '%s' element.", name);
				return false;
			}
			if(sscanf(t, "%u", &w->nMachineGunTimeout) != 1){
				XmlErr(fn, "Failed to parse the contents of '%s' element.", name);
				return false;
			}
			if(w->nMachineGunTimeout == 0)
				w->nMachineGunTimeout = 0x19;
			bTimeout = true;
		}
		else if(_stricmp(name, "sound") == 0){
			if(bSound){
				XmlErr(fn, "'%s' sector contains multiple '%s' elements.", sn, name);
				return false;
			}
			t = e->GetText();
			if(!t){
				XmlErr(fn, "Failed to read data from '%s' element.", name);
				return false;
			}
			if(sscanf(t, "%d", &w->nMachineGunSound) != 1){
				XmlErr(fn, "Failed to parse the contents of '%s' element.", name);
				return false;
			}
			bSound = true;
		}
		else{
			// QUIRK: 'missileinfo' in the machinegun parser's message.
			XmlErr(fn, "Unknown element '%s' in 'missileinfo' sector.", name);
			return false;
		}
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLWeaponDockedInfo — 0.98e @ RVA 0x4d610
// Sets w->bHasDocked on entry. rotation goes through euler->quat
// (FUN_10008250 @ 0x8250). QUIRK: parse messages use the short form.
// ===========================================================================
bool
CMvlVehicle::LoadXMLWeaponDockedInfo(TiXmlElement *sector, SMvlVehicleWeaponData *w)
{
	static const char *const fn = "CMvlVehicle::LoadXMLWeaponDockedInfo";
	const char *sn = "dockedinfo";
	bool bSpeed = false, bPos = false, bRot = false;
	float euler[3];

	w->bHasDocked = 1;
	MvlVectorZero(euler);

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "dockspeed") == 0){
			if(bSpeed)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%u", &w->nDockSpeed) != 1)
				return MvlErrParseShort(fn, "dockspeed");
			bSpeed = true;
		}
		else if(_stricmp(name, "position") == 0){
			if(bPos)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, w->fDockedPosition))
				return MvlErrVector(fn, "position");
			bPos = true;
		}
		else if(_stricmp(name, "rotation") == 0){
			if(bRot)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, euler))
				return MvlErrVector(fn, "rotation");
			MvlEulerToQuat(w->fDockedRotation, euler);
			bRot = true;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bSpeed) return MvlErrMissing(fn, sn, "dockspeed"), false;
	if(!bPos)   return MvlErrMissingVector(fn, sn, "position"), false;
	if(!bRot)   return MvlErrMissingVector(fn, sn, "rotation"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLWeaponArmedInfo — 0.98e @ RVA 0x4d890
// Required: minpos + minrot only. maxpos defaults to minpos, maxrot to
// minrot; then mid rotation + matrix build (FUN_10008590 / FUN_1002e1e0).
// ===========================================================================
bool
CMvlVehicle::LoadXMLWeaponArmedInfo(TiXmlElement *sector, SMvlVehicleWeaponData *w)
{
	static const char *const fn = "CMvlVehicle::LoadXMLWeaponArmedInfo";
	const char *sn = "armedinfo";
	bool bMove = false, bMinPos = false, bMinRot = false,
	     bMaxPos = false, bMaxRot = false, bMiddle = false;
	float euler[3];

	MvlVectorZero(euler);
	w->nMoveSpeed = 0;
	w->fMiddlePos = 0.5f;   // 0x3f000000

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "movespeed") == 0){
			if(bMove)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%u", &w->nMoveSpeed) != 1)
				return MvlErrParseShort(fn, "movespeed");
			bMove = true;
		}
		else if(_stricmp(name, "minpos") == 0){
			if(bMinPos)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, w->fMinPos))
				return MvlErrVector(fn, "minpos");
			bMinPos = true;
		}
		else if(_stricmp(name, "minrot") == 0){
			if(bMinRot)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, euler))
				return MvlErrVector(fn, "minrot");
			MvlEulerToQuat(w->fMinRot, euler);
			bMinRot = true;
		}
		else if(_stricmp(name, "maxpos") == 0){
			if(bMaxPos)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, w->fMaxPos))
				return MvlErrVector(fn, "maxpos");
			bMaxPos = true;
		}
		else if(_stricmp(name, "maxrot") == 0){
			if(bMaxRot)
				return MvlErrDupVector(fn, sn, name);
			if(!MvlParseVector(e, euler))
				return MvlErrVector(fn, "maxrot");
			MvlEulerToQuat(w->fMaxRot, euler);
			bMaxRot = true;
		}
		else if(_stricmp(name, "middlepos") == 0){
			if(bMiddle)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(sscanf(t, "%f", &w->fMiddlePos) != 1)
				return MvlErrParseShort(fn, "middlepos");
			if(w->fMiddlePos < 0.0f)
				w->fMiddlePos = 0.0f;
			else if(w->fMiddlePos > 1.0f)   // DAT_1008de28
				w->fMiddlePos = 1.0f;
			bMiddle = true;
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bMinPos)
		return MvlErrMissingVector(fn, sn, "minpos"), false;
	if(!bMinRot)
		return MvlErrMissingVector(fn, sn, "minrot"), false;

	// Defaults + derived state (decomp tail).
	if(!bMaxPos)
		MvlVectorCopy(w->fMaxPos, w->fMinPos);
	if(!bMaxRot)
		memcpy(w->fMaxRot, w->fMinRot, sizeof(w->fMaxRot));
	MvlQuatMid(w->fMidRot, w->fMinRot, w->fMaxRot);
	MvlVectorCopy(w->fMaxPosCopy, w->fMaxPos);
	// HOOK POINT @ 0x4d890 — FUN_1002e1e0/2e240/2e210 build render matrices.
	MvlWeaponMatrixBuild(w, w->fMinPos);
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLExtraWeaponData — 0.98e @ RVA 0x4dd20
// ===========================================================================
bool
CMvlVehicle::LoadXMLExtraWeaponData(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLExtraWeaponData";
	const char *sn = "weapon";

	if(m_nWeaponCount >= MVL_MAX_WEAPONS)
		return false;   // guarded by LoadXMLWeaponList (original too)
	SMvlVehicleWeaponData *w = &m_aWeapons[m_nWeaponCount];
	m_nWeaponCount++;

	// Defaults (decomp prologue).
	memset(w, 0, sizeof(*w));
	w->nMissileTimeout = 1000;
	w->fMissileAddSpeedMul = 1.0f;
	w->fMachineGunRange = 100.0f;      // 0x42c80000
	w->nMachineGunSound = 0x37;
	w->nMachineGunTimeout = 0x1e;

	bool bType = false, bModel = false, bMissileInfo = false,
	     bFireOffset = false, bDocked = false, bArmed = false,
	     bHotKeys = false;

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "type") == 0){
			if(bType)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			if(_stricmp(t, "missile") == 0)
				w->nType = 1;
			else if(_stricmp(t, "machinegun") == 0)
				w->nType = 2;
			else{
				XmlErr(fn, "Failed to read 'type' field - can only have values 'missile' and 'machinegun'.");
				return false;
			}
			bType = true;
		}
		else if(_stricmp(name, "model") == 0){
			if(bModel)
				return MvlErrDup(fn, sn, name), false;
			const char *t = e->GetText();
			if(!t)
				return MvlErrRead(fn, name);
			static const char *const kModels[7] = {
				"none", "rocketlauncher", "m4", "custom1", "custom2", "custom3", "custom4"
			};
			int i;
			for(i = 0; i < 7; i++)
				if(_stricmp(t, kModels[i]) == 0)
					break;
			if(i == 7){
				XmlErr(fn, "Invalid 'model' value, valid: 'none', 'rlauncher', 'm4', 'custom1'-'custom4'.");
				return false;
			}
			w->nModel = (unsigned)i;
			if(i >= 3){
				// custom1-4 must have been loaded from weapons.dff
				// (flags @ 0x2b68..0x2b74, set by the weapon clump stage).
				static const char *const kCustom[4] = {
					"custom1", "custom2", "custom3", "custom4"
				};
				if(m_anCustomModelLoaded[i - 3] == 0){
					XmlErr(fn, "'model' set to '%s', which is not loaded.",
					       kCustom[i - 3]);
					return false;
				}
			}
			bModel = true;
		}
		else if(_stricmp(name, "missileinfo") == 0){
			if(bMissileInfo)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLWeaponMissileInfo(e, w)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bMissileInfo = true;
		}
		else if(_stricmp(name, "fireoffset") == 0){
			if(bFireOffset){
				// QUIRK: the original names 'heliweps' here.
				XmlErr(fn, "'heliweps' sector contains multiple 'fireoffset' vectors.");
				return false;
			}
			if(!MvlParseVector(e, w->fFireOffset))
				return MvlErrVector(fn, "fireoffset");
			bFireOffset = true;
		}
		else if(_stricmp(name, "dockedinfo") == 0){
			if(bDocked)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLWeaponDockedInfo(e, w)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bDocked = true;
		}
		else if(_stricmp(name, "armedinfo") == 0){
			if(bArmed)
				return MvlErrDupSector(fn, sn, name), false;
			if(!LoadXMLWeaponArmedInfo(e, w)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bArmed = true;
		}
		else if(_stricmp(name, "hotkeys") == 0){
			if(bHotKeys){
				XmlErr(fn, "'%s' sector contains multiple '%s' sectors.", sn, name);
				return false;
			}
			if(!LoadXMLWeaponKeyData(e, w)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
			bHotKeys = true;
		}
		else if(_stricmp(name, "machineguninfo") == 0){
			// QUIRK: no duplicate check for machineguninfo (original).
			if(!LoadXMLWeaponMachineGunInfo(e, w)){
				XmlErr(fn, "Could not load '%s' sector.", name);
				return false;
			}
		}
		else
			return MvlErrUnknown(fn, sn, name);
	}

	if(!bType)       return MvlErrMissing(fn, sn, "type"), false;
	if(!bModel)      return MvlErrMissing(fn, sn, "model"), false;
	if(!bFireOffset) return MvlErrMissingVector(fn, sn, "fireoffset"), false;
	if(!bArmed)      return MvlErrMissing(fn, sn, "armedinfo"), false;
	if(!bHotKeys)    return MvlErrMissing(fn, sn, "hotkeys"), false;
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLWeaponList — 0.98e @ RVA 0x4e430
// ===========================================================================
bool
CMvlVehicle::LoadXMLWeaponList(TiXmlElement *sector)
{
	static const char *const fn = "CMvlVehicle::LoadXMLWeaponList";
	const char *sn = "weaponlist";

	for(TiXmlElement *e = sector->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		if(_stricmp(name, "weapon") != 0)
			return MvlErrUnknown(fn, sn, name);
		if(m_nWeaponCount > 3){
			XmlErr(fn, "'weaponlist' sector contains more than 4 'weapon' sectors.");
			return false;
		}
		if(!LoadXMLExtraWeaponData(e)){
			XmlErr(fn, "Could not load 'weapon' sector.");
			return false;
		}
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLSectionVehicle — 0.98e @ RVA 0x4e4f0
// Unknown sections are silently ignored (original). 'basic' must be the
// first child ("Section '%s' is not the first child of '%s' sector.").
// ===========================================================================
bool
CMvlVehicle::LoadXMLSectionVehicle(TiXmlElement *root)
{
	static const char *const fn = "CMvlVehicle::LoadXMLSectionVehicle";
	const char *sn = "vehicle";
	unsigned seen = 0;

	for(TiXmlElement *e = root->FirstChildElement(); e;
	    e = e->NextSiblingElement()){
		const char *name = e->Value();
		int idx = -1;
		if     (_stricmp(name, "basic") == 0)         idx = 0;
		else if(_stricmp(name, "aidata") == 0)        idx = 1;
		else if(_stricmp(name, "colors") == 0)        idx = 2;
		else if(_stricmp(name, "audio") == 0)         idx = 3;
		else if(_stricmp(name, "handling") == 0)      idx = 4;
		else if(_stricmp(name, "aerohandling") == 0)  idx = 5;
		else if(_stricmp(name, "bikehandling") == 0)  idx = 6;
		else if(_stricmp(name, "boathandling") == 0)  idx = 7;
		else if(_stricmp(name, "helidata") == 0)      idx = 8;
		else if(_stricmp(name, "specials") == 0)      idx = 9;
		else if(_stricmp(name, "weaponlist") == 0)    idx = 10;
		if(idx < 0)
			continue;   // unknown section: ignored (original)

		if((seen & 1) == 0 && idx > 0){
			XmlErr(fn, "Section '%s' is not the first child of '%s' sector.",
			       name, sn);
			return false;
		}

		bool ok = true;
		switch(idx){
		case 0: ok = LoadXMLBasic(e); break;
		case 1: ok = LoadXMLAIData(e); break;
		case 2: ok = LoadXMLCarCol(e); break;
		case 3: ok = LoadXMLAudio(e); break;
		case 4: ok = LoadXMLHandling(e); break;
		case 5:
			ok = LoadXMLAeroHandling(e);
			if(ok)
				m_handling.nAeroSectionId = m_nId;
			break;
		case 6:
			ok = LoadXMLBikeHandling(e);
			if(ok)
				m_handling.nBikeSectionId = m_nId;
			break;
		case 7:
			ok = LoadXMLBoatHandling(e);
			if(ok)
				m_handling.nBoatSectionId = m_nId;
			break;
		case 8: ok = LoadXMLHeliData(e); break;
		case 9: ok = LoadXMLSpecialData(e); break;
		case 10: ok = LoadXMLWeaponList(e); break;
		}
		if(!ok){
			XmlErr(fn, "Could not load '%s' sector.", name);
			return false;
		}
		seen |= 1u << idx;
	}

	// Type-specific required sectors (bitmask values from the decomp).
	if(m_nType == 1 && (seen & 0x80) == 0){
		XmlErr(fn, "Root element is missing 'boathandling' sector, which is required for boats.");
		return false;
	}
	if((m_nType == 3 || m_nType == 4) && (seen & 0x20) == 0){
		XmlErr(fn, "Root element is missing 'aerohandling' sector, which is required for planes and helis.");
		return false;
	}
	if(m_nType == 5 && (seen & 0x40) == 0){
		XmlErr(fn, "Root element is missing 'bikehandling' sector, which is required for bikes.");
		return false;
	}
	return true;
}

// ===========================================================================
// CMvlVehicle::LoadXMLFile — vehmod_retail.dll 0.98e @ RVA 0x4e790
// TinyXML document parse from the in-memory .xml (FUN_10035110 ctor /
// FUN_10036960 Parse / FUN_10034a80 dtor).
// ===========================================================================
bool
CMvlVehicle::LoadXMLFile(void)
{
	static const char *const fn = "CMvlVehicle::LoadXMLFile";

	if(m_apFiles[MVL_SLOT_XML] == 0){
		XmlErr(fn, "No XML file found.");
		return false;
	}

	TiXmlDocument doc;
	doc.Parse((const char *)m_apFiles[MVL_SLOT_XML], 0, TIXML_ENCODING_UNKNOWN);
	free(m_apFiles[MVL_SLOT_XML]);
	m_apFiles[MVL_SLOT_XML] = 0;

	if(doc.Error()){
		XmlErr(fn, "XML file could not be parsed: %s.", doc.ErrorDesc());
		return false;
	}

	TiXmlElement *root = doc.FirstChildElement();
	if(!root){
		XmlErr(fn, "No root element in XML.");
		return false;
	}
	if(root->Type() != TiXmlNode::TINYXML_ELEMENT ||
	   _stricmp(root->Value(), "vehicle") != 0){
		XmlErr(fn, "Root element has the wrong name.");
		return false;
	}
	return LoadXMLSectionVehicle(root);
}
