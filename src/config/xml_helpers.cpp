// xml_helpers.cpp — implementation of the shared XML parsing helpers.
#include "../core/stdafx.h"
#include "xml_helpers.h"

#include <stdlib.h>

// ---------------------------------------------------------------------------
// String-to-number helpers
// ---------------------------------------------------------------------------

int
MvlAtoi(const char *s, bool *ok)
{
	char *end = NULL;
	long v;

	if(ok)
		*ok = false;
	if(!s || !*s)
		return 0;

	v = strtol(s, &end, 10);
	if(end == s)
		return 0;
	if(ok)
		*ok = true;
	return (int)v;
}

bool
MvlAtob(const char *s, bool *ok)
{
	if(ok)
		*ok = false;
	if(!s)
		return false;

	// MVL globalm.xml/globalv.xml use "true"/"false" and bare "0"/"1";
	// antialias uses "on"/"off" (sample_globalm.xml line 10).
	if(!_stricmp(s, "true") || !_stricmp(s, "on") || !strcmp(s, "1")){
		if(ok)
			*ok = true;
		return true;
	}
	if(!_stricmp(s, "false") || !_stricmp(s, "off") || !strcmp(s, "0")){
		if(ok)
			*ok = true;
		return false;
	}
	return false;
}

float
MvlAtof(const char *s, bool *ok)
{
	char *end = NULL;
	double v;

	if(ok)
		*ok = false;
	if(!s || !*s)
		return 0.0f;

	v = strtod(s, &end);
	if(end == s)
		return 0.0f;
	if(ok)
		*ok = true;
	return (float)v;
}

// ---------------------------------------------------------------------------
// Element accessors
// ---------------------------------------------------------------------------

static const char *
XmlTextOf(const TiXmlElement *parent, const char *name)
{
	if(!parent || !name)
		return NULL;
	const TiXmlElement *el = parent->FirstChildElement(name);
	if(!el)
		return NULL;
	return el->GetText();
}

int
XmlGetInt(const TiXmlElement *parent, const char *name, bool *ok)
{
	return MvlAtoi(XmlTextOf(parent, name), ok);
}

bool
XmlGetBool(const TiXmlElement *parent, const char *name, bool *ok)
{
	return MvlAtob(XmlTextOf(parent, name), ok);
}

float
XmlGetFloat(const TiXmlElement *parent, const char *name, bool *ok)
{
	return MvlAtof(XmlTextOf(parent, name), ok);
}

// ---------------------------------------------------------------------------
// XmlParseVector — clone of vehmod_retail.dll 0.98e @ RVA 0x1bdb0.
// Parses comma-separated float triples from element text ("x,y,z").
// ---------------------------------------------------------------------------

bool
XmlParseVector(const TiXmlElement *el, float out[3])
{
	const char *text;
	char buf[128];
	char *p;
	int i;

	if(!el || !out)
		return false;

	text = el->GetText();
	if(!text)
		return false;

	// Copy into a bounded buffer; strtol/strtod need NUL-terminated input.
	strncpy(buf, text, sizeof(buf) - 1);
	buf[sizeof(buf) - 1] = '\0';

	p = buf;
	for(i = 0; i < 3; i++){
		char *end = NULL;
		double v;

		while(*p == ' ' || *p == '\t')
			p++;
		if(*p == '\0')
			return false;

		v = strtod(p, &end);
		if(end == p)
			return false;
		out[i] = (float)v;
		p = end;

		// Skip to the next comma (or end for the last component).
		while(*p && *p != ',')
			p++;
		if(*p == ',')
			p++;
	}
	return true;
}
