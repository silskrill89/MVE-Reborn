// stdafx.h — common includes for the MVL clone rebuild.
// Deliberately NOT used as a precompiled header; game plugins stay simple.
#pragma once

#include <windows.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include "mvl.h"

// --- Init chain (original MVL symbol names, re-098e-map.md §2) --------------
// Called from DllMain in 0.98e order.
void PrepareMvlForLoad(void);   // vehmod_retail.dll 0.98e @ RVA 0x2baf0 (stub)
// Real implementations in src/config (naming policy: original symbols).
bool LoadGlobalVXMLFile(void);  // vehmod_retail.dll 0.98e @ RVA 0x38160
bool LoadGlobalMXMLFile(void);  // vehmod_retail.dll 0.98e @ RVA 0x2b390
