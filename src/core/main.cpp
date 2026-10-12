// main.cpp — DLL entry point + init chain skeleton for the MVL clone.
//
// Chain doctrine per vehmod_retail.dll 0.98e (.serena/memories/re-098e-map.md §2):
//   entry 0x10005c1d1 -> DllMainCRTStartup -> FUN_10043c00 (main init orchestrator)
//     |-> PrepareMvlForLoad   (RVA 0x2baf0; banner "----- MVL version 0.98e,
//     |                        compiled on %s -----" @ 0x1007f34c)
//     |-> LoadGlobalVXMLFile  (RVA 0x38160; mvl\vehicles\globalv.xml)
//     `- hook installers FUN_10022560 (write hook at VC addr) / FUN_10022590
//        (VirtualProtect variant) — NOT cloned yet, stubs log TODO.
//
// Log targets: mvl\messagelog.txt / mvl\errorlog.txt (0.98e strings
// @ 0x1007cad0 / 0x1007c180), via src/loader/log.h.
#include "stdafx.h"
#include "loader/log.h"
#include "config/global_veh_xml.h"
#include "config/mvl_settings.h"
#include "init.h"

// thelink2012/injector — header-only, already on the include path
// (external/injector/include). Included now so future hook code needs no
// include churn; nothing is patched in this build.
#include <injector/injector.hpp>

// ---------------------------------------------------------------------------
// Init chain (original MVL symbol names per the naming policy).
// ---------------------------------------------------------------------------

// PrepareMvlForLoad — clone of vehmod_retail.dll 0.98e @ RVA 0x2baf0
// (CMvlSettings::PrepareMvlForLoad). The original writes the version banner
// and installs the engine hooks. Stub: banner is written by DllMain; hook
// installation is TODO.
void
PrepareMvlForLoad(void)
{
	MvlLog("PrepareMvlForLoad: TODO hook install");
}

// LoadGlobalVXMLFile — real implementation in src/config/global_veh_xml.cpp
// (vehmod_retail.dll 0.98e @ RVA 0x38160; mvl\vehicles\globalv.xml).
// LoadGlobalMXMLFile — real implementation in src/config/mvl_settings.cpp
// (vehmod_retail.dll 0.98e @ RVA 0x2b390; mvl\globalm.xml).

// ---------------------------------------------------------------------------
// Future hook examples (commented — NO live hooks in this build).
//
// 0.98e installs patches via FUN_10022560 (direct write at a VC address) and
// FUN_10022590 (VirtualProtect variant; PUSH 0x401000 seen @ 0x100225af —
// patches the VC code region). When we clone those installers, the injector
// idioms will be:
//
//   // Locate VC 1.0 code by byte signature instead of fixed addresses:
//   auto site = mem::make_pattern("8B 44 24 04 85 C0 74 0C").get_one();
//
//   // Neutralize an original call/jump, or detour it:
//   injector::MakeNOP(site.get<void>());
//   // injector::MakeJMP(site.get<void>(), &MyHookBody);
//
// Every hook must cite binary + version + RVA/VA (see AGENTS.md RE rules).
// ---------------------------------------------------------------------------

BOOL WINAPI
DllMain(HINSTANCE hInst, DWORD reason, LPVOID reserved)
{
	(void)reserved;

	switch(reason){
	case DLL_PROCESS_ATTACH:
		DisableThreadLibraryCalls(hInst);
		MvlLogInit(); // creates mvl\ dir if missing, opens both logs
		MvlLog("----- MVL-clone version %s, compiled on %s %s -----",
		       MVL_VERSION_STRING, __DATE__, __TIME__);
		PrepareMvlForLoad();
		// TRANCHE-5B: init-chain patch roots (FUN_10043c00 callees,
		// 0.98e @ RVA 0x43810/0x43a10/0x43900/0x43870/0x43b30); engine
		// writes log through HOOK POINT seams (src/core/init.cpp).
		MvlInstallInitChainPatches();
		LoadGlobalVXMLFile();
		LoadGlobalMXMLFile();
		break;

	case DLL_PROCESS_DETACH:
		MvlLogShutdown();
		break;
	}
	return TRUE;
}
