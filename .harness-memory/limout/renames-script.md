# Rename sweep — src/script/ (python_host.{h,cpp}) — branch 'continued'

Scope: code identifiers (defs/decls/calls) only. Method: python word-boundary
regex replace over both files; definitions carry a trailing
`// <oldFUN> @ RVA 0x…` comment so the Ghidra names stay grep-able.
Comment-only mentions of OTHER modules' FUN_ names were left untouched.
The 16 module methods (MvlPyMbase*/MvlPyMveh*/MvlPyMped*/MvlPyManim*) were
already renamed in the earlier pass (mbase binding doctrine) — not listed here.

## 1. Own functions renamed (27)

| old | new | behavior | confidence |
|---|---|---|---|
| FUN_100287c0 | MvlPyLoadInterpreter | LoadLibraryA(python33.dll) + GetProcAddress table fill + inittab registrations + Py_Initialize + script scan (0.98e @ RVA 0x287c0) | HIGH (loader body, error strings) |
| FUN_10028cd0 | MvlPyRegisterEvents | fills event name→id table (eventGameStart/Frame/InternalAddAnim/InternalBlendAnim/BulletFired) | HIGH |
| FUN_10029310 | MvlPyScanScripts | FindFirstFileA("*.py") over mvl\scripts, import each under stderr capture | HIGH |
| FUN_10029580 | MvlPyScriptDirGuard | enter/leave mvl\scripts CWD (one-shot GetModuleFileNameA path build, depth counter) | HIGH |
| FUN_1002a170 | MvlPyDeleteErrFile | DeleteFileA("<name>.err.txt") inside scripts dir | HIGH |
| FUN_1002a1d0 | MvlPyWriteErrFile | append captured stderr to "<name>.err.txt" (fopen a+, fprintf "%s\n"); original __thiscall lowered | HIGH |
| FUN_10029ff0 | MvlPyBeginStderrCapture | swap sys.stderr to io.StringIO (re-entrant depth counter) | HIGH |
| FUN_1002a0a0 | MvlPyEndStderrCapture | StringIO.getvalue → captured bytes, close(), restore sys.stderr | HIGH |
| FUN_10025010 | MvlPyCopyAsciiArg | PyObject → bounded ASCII buffer (returns 1/0) | HIGH |
| FUN_10025100 | MvlPyMakeEntityId | (kind,index) → 2-tuple identifier; index -1 → None | HIGH |
| FUN_100250e0 | MvlPyIdTupleIndex | identifier tuple field 1 as long | HIGH |
| FUN_10025160 | MvlPyIdTupleIndex64 | identifier tuple field 1 as longlong (no clone call sites) | HIGH |
| FUN_10025060 | MvlPyIsEntityIdTuple | validate (kind,index) tuple via tp_flags 0x4000000 + size==2 (static) | HIGH |
| FUN_10025a70 | MvlPyResolveEntityHandle | handle → game entity (kind dispatch; pool getters = HOOK POINT) | HIGH |
| FUN_10025b00 | MvlPyMakeEntityHandle | game entity → handle (pool slot lookups = HOOK POINT) | HIGH |
| FUN_10025ba0 | MvlPyArgToEntity | PyObject → game entity with PyErr reporting | HIGH |
| FUN_10029660 | MvlPyEventSlotByName | event name → handler-slot index (-1 unknown; flat-table scan) | HIGH |
| FUN_100290d0 | MvlPyAddEventHandler | event-handler insert (dedup, incref, append) | HIGH |
| FUN_10023db0 | MvlPyAnimTypeToAssoc | anim-type identifier tuple → association object | HIGH |
| FUN_10025a30 | MvlPyAppendInittabMent | PyImport_AppendInittab("ment", …) | HIGH (PE evidence) |
| FUN_10026940 | MvlPyAppendInittabMped | PyImport_AppendInittab("mped", …) | HIGH |
| FUN_10026f80 | MvlPyAppendInittabMrw | PyImport_AppendInittab("mrw", …) | HIGH |
| FUN_10027370 | MvlPyAppendInittabMscm | PyImport_AppendInittab("mscm", …) | HIGH |
| FUN_10027a30 | MvlPyAppendInittabMveh | PyImport_AppendInittab("mveh", …) | HIGH |
| FUN_100281c0 | MvlPyAppendInittabMworld | PyImport_AppendInittab("mworld", …) | HIGH |
| FUN_10024fd0 | MvlPyAppendInittabMbase | PyImport_AppendInittab("mbase", …) (static; HOOK POINT body, below-window) | HIGH |
| FUN_10023d10 | MvlPyAppendInittabManim | PyImport_AppendInittab("manim", …) (static; HOOK POINT body, below-window) | HIGH |

## 2. Engine-seam TODO labels converted (7) — other modules, label tokens only

`MVL_HELPERS_TODO(FUN_xxx)` stringifies its argument into a TODO log; these
args named ENGINE functions (not defined in src/script/). Converted to semantic
label tokens so no bare FUN_<va> identifier remains in code; old id + RVA kept
in a trailing comment. Log text changes accordingly (diagnostic only).

| old label | new label | engine fn | confidence |
|---|---|---|---|
| FUN_1002d760 | PedCast | ped cast (engine) @ RVA 0x2d760 | HIGH |
| FUN_1002d790 | VehicleCast | vehicle cast (engine) @ RVA 0x2d790 | HIGH |
| FUN_1002d870 | EntityTypeQuery | entity-type check (engine) @ RVA 0x2d870 | HIGH |
| FUN_1002e870 | HudPush | HUD push (engine) @ RVA 0x2e870 | HIGH |
| FUN_1003b780 | TuningPartCountLookup | tuning-part count (engine) @ RVA 0x3b780 | HIGH |
| FUN_1003b740 | TuningPartNameLookup | tuning-part name (engine) @ RVA 0x3b740 | HIGH |
| FUN_1003df80 | VehicleSpawnChain | vehicle spawn chain (engine) @ RVA 0x3df80 | HIGH |

## 3. Deliberately left (not FUN_<va> code identifiers)

- `MVL_HELPERS_TODO(FUN_10027c50_body)`, `FUN_10027cb0_body`, `FUN_10027d50_body`,
  `FUN_10027de0_body`, `FUN_10027f60_body`, `FUN_10028110_vtable80` — extended
  TODO-label tokens (suffix after the VA), not identifiers of any function;
  they document the unported engine body behind an already-renamed module method.
- Comment-only mentions of other modules' functions (e.g. FUN_10002f60 below-
  window seam, FUN_1005abd0 strchr, FUN_10058e3e list-too-long, FUN_100061f0
  map-find, FUN_1001cc00 message logger) — per instructions.
- String literals naming below-window callees (e.g. the "FUN_10002f60 -
  below-window, body not recovered" HOOK POINT log) — data, not code.

## Verification (after sweep)

- `python_host.h`: FUN_<va> tokens in code (comments+strings stripped) = **0** (was 18)
- `python_host.cpp`: FUN_<va> tokens in code = **0** (was 86)
- Raw-token counts (incl. comments/strings): .h 47→17, .cpp 148→82 — remaining
  are the preserved `// <oldFUN> @ RVA 0x…` definition cites (27 + 16 pre-existing
  module-method cites = 43 in .cpp), engine-seam trailing comments (7), and
  comment-only mentions.
- Brace balance unchanged: .h 3/3, .cpp 162/162.
- No new-symbol collisions outside src/script/ (grepped the 27 new names).
