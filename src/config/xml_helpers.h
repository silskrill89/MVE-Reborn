// xml_helpers.h — shared XML parsing helpers for the config layer.
//
// CHelpers::XmlParseVector is a recovered original symbol:
//   vehmod_retail.dll 0.98e @ RVA 0x1bdb0 (re-098e-map.md §3 Misc).
// The remaining helpers are provisional (no RTTI/string evidence for their
// individual names); they follow the same CHelpers convention.
//
// Exception-free: TinyXML 2.6.2 classic API (TIXML_USE_STL off), no exceptions
// cross the DLL boundary.
#pragma once

#include "../core/stdafx.h"
#include "../../external/tinyxml/tinyxml.h"

// Parse "x,y,z"-style element text into out[3]. Returns false if the element
// is missing, has no text, or fewer than 3 components parse.
// Clone of vehmod_retail.dll 0.98e @ RVA 0x1bdb0.
bool XmlParseVector(const TiXmlElement *el, float out[3]);

// Safe text accessors. All return the parsed value and report success via
// *ok (may be NULL). Missing element / unparseable text => *ok=false.
int  XmlGetInt(const TiXmlElement *parent, const char *name, bool *ok);
bool XmlGetBool(const TiXmlElement *parent, const char *name, bool *ok);
float XmlGetFloat(const TiXmlElement *parent, const char *name, bool *ok);

// String-to-number helpers (strtol/strtod based, always NUL-safe).
int   MvlAtoi(const char *s, bool *ok);
bool  MvlAtob(const char *s, bool *ok);   // "true"/"false"/"1"/"0"/"on"/"off"
float MvlAtof(const char *s, bool *ok);
