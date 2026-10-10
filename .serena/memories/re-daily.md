# RE Daily Log

## 2026-10-10 — loader skeleton implemented (clone side)

**Scope**: src/loader/** (new), src/core/main.cpp (rewrite), src/core/stdafx.h, src/mvl.h. Build target: `vehmod.dll` (Win32, /MT).

### Done
- **src/loader/chooser.cpp** — faithful clone of vehmod_chooser.flt logic (citation: vehmod_chooser.flt 0.98e @ RVA 0x154, re-loader-shim.md). DllMain-equivalent exposed as `VehmodChooserDllMain` (provisional name; original EP was unnamed, logic COMPLETE per re-loader-shim.md): DLL_PROCESS_ATTACH-only, dword read at fixed gta-vc.exe VA 0x61A7F8, compare 0x9753C405 (retail) / 0xE8535004 (steam), `LoadLibraryA("mvl\vehmod_retail.dll" | "mvl\vehmod_steam.dll")`, silent no-op otherwise. Added SEH (`__try/__except`) around the fixed-VA read — original has no guard (always loaded by gta-vc.exe) but we attach to arbitrary exes during testing.
- **src/loader/log.h/.cpp** — `MvlLog` → mvl\messagelog.txt, `MvlErrorLog` → mvl\errorlog.txt (0.98e strings @ 0x1007cad0/0x1007c180), timestamped, + OutputDebugStringA mirror, lazy reopen, truncation-safe (fopen_s/snprintf/vsnprintf only). `MvlLogInit` creates mvl\ dir (CreateDirectoryA) and opens both logs append.
- **src/core/main.cpp** — init chain skeleton in 0.98e order (FUN_10043c00 doctrine): DllMain → DisableThreadLibraryCalls → MvlLogInit → banner `----- MVL-clone version 0.0.1, compiled on %s %s -----` (__DATE__ __TIME__; original banner format @ 0x1007f34c) → `PrepareMvlForLoad()` → `LoadGlobalVXMLFile()`. Includes `<injector/injector.hpp>` (thelink2012) for future hooks; commented mem::make_pattern / injector::MakeNOP examples, no live hooks.
- **Stubs**: `PrepareMvlForLoad` (0.98e @ RVA 0x2baf0) logs "TODO hook install"; `LoadGlobalVXMLFile` (0.98e @ RVA 0x38160) logs existence of mvl\vehicles\globalv.xml only — no XML parsing yet.
- **src/core/stdafx.h** — added <stdarg.h> + init-chain prototypes. **src/mvl.h** — added `MVL_LOG_DIR "mvl"` (game-CWD-relative path doctrine).

### Stubbed / open
- Hook installers FUN_10022560 / FUN_10022590 not cloned (no live patches).
- globalv.xml parsing not started (TinyXML is vendored and linked, ready).
- Chooser is compiled in but NOT invoked from our DllMain (single-DLL build; dispatching it would self-load). Wire it as the real DllMain when the standalone .flt shim target is split out of premake5.lua.
- Fingerprint A (0x9753C405) still unverified against retail gta-vc.exe bytes @ 0x61A7F8 (open item in re-loader-shim.md).

### Build
- premake5.lua untouched (src/**.* glob picks up new files). **GREEN**: Release|Win32 msbuild → `bin\Release\vehmod.dll` (+ deploy copy to MVL_GAME_DIR as vehmod_retail.dll). One fix during build: loader .cpp files must include `"../core/stdafx.h"` (stdafx.h is in src/core, only `src` root is on the include path).
