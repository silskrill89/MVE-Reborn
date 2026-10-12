# port-notes-py.md — TRANCHE-5B: Python 3.3 host + main init chain

Binary: vehmod_retail.dll 0.98e, image base 0x10000000 (RVA = VA − 0x10000000).
Sources: `.harness-memory/limout/pyaudio/range-decomp.txt` (4521 ln, 103 fns,
0x25000–0x2a200), `.harness-memory/limout/initchain/decomp-chain.txt` (5 roots).
Files: `src/script/python_host.{h,cpp}`, `src/core/init.{h,cpp}`,
`src/core/main.cpp` (one wiring call).

## 1. Function inventory

| Name | RVA | Status | Notes |
|---|---|---|---|
| FUN_100287c0 | 0x287c0 | DONE | loader: LoadLibraryA("python33.dll") + 50 GetProcAddress → g_MvlPyApi, missing>0 → "Couldn't load all python functions %08X %08X.", MvlPyLoaded=1 + "Python interpreter loaded.", 8 inittab regs, Py_Initialize, FUN_10029310 |
| FUN_10028cd0 | 0x28cd0 | DONE | event name → id table: 0 eventGameStart / 1 eventFrame / 2 eventInternalAddAnim / 3 eventInternalBlendAnim / 5 eventBulletFired (original std::map DAT_1011c344 → flat table) |
| FUN_10029310 | 0x29310 | DONE | *.py scan in scripts dir; strip ext, FUN_1002a170 stale err delete, PyUnicode_FromString + PyImport_Import under capture (29ff0/2a0a0), logs "Successfully loaded script '%s'." / "Failed to load script '%s'." |
| FUN_10029580 | 0x29580 | DONE | scripts-dir CWD guard (enter/leave, depth refcount); one-shot path build GetModuleFileNameA + "mvl\scripts\" @ 0x1007e85c |
| FUN_1002a170 | 0x2a170 | DONE | DeleteFileA "<name>.err.txt" |
| FUN_1002a1d0 | 0x2a1d0 | DONE | append captured stderr to "<name>.err.txt" (fopen "a+"); thiscall lowered __fastcall+edx |
| FUN_10029ff0 | 0x29ff0 | DONE | capture begin: cache io.StringIO (PyImport_ImportModule("io")/GetAttr "StringIO"), swap sys.stderr |
| FUN_1002a0a0 | 0x2a0a0 | DONE | capture end: getvalue() → captured, close(), restore sys.stderr |
| FUN_10025010 | 0x25010 | DONE | PyObject → bounded ASCII buffer (PyUnicode_AsASCIIString + PyBytes_AsString) |
| FUN_10025100 | 0x25100 | DONE | (kind,index) → 2-tuple identifier; -1 → Py_BuildValue("") None |
| FUN_100250e0 | 0x250e0 | DONE | tuple[1] as long |
| FUN_10025160 | 0x25160 | DONE | tuple[1] as long long |
| FUN_10025060 | 0x25060 | DONE (static) | "is identifier tuple" check (tp_flags & 0x4000000 + Tuple_Size==2) |
| FUN_10025a70 | 0x25a70 | SEAM | handle → entity; pool getters FUN_100267a0/268b0/26840/267d0 + VC pools 0x97F240/0xA0FDE4/0x97F2AC/0x94DBE0 = HOOK POINT |
| FUN_10025b00 | 0x25b00 | SEAM | entity → handle; slot lookups FUN_10026760/26870/26800/0008040 = HOOK POINT |
| FUN_10025ba0 | 0x25ba0 | DONE | PyObject → entity; errors "Argument value is None." / "Argument is not an entity identifier." / "Entity does not exist." |
| FUN_10025280 | 0x25280 | DONE | mvlbase.logPrint "s:mvlbase.logPrint" |
| FUN_100252d0 | 0x252d0 | DONE (bind seam) | event-handler bind; FUN_100290d0 map insert unported (TODO) |
| FUN_100254b0 | 0x254b0 | DONE (HUD seam) | mvlbase.setHudMessage "is:mvlbase.setHudMessage"; FUN_1002e870 HUD push + FUN_100258a0 UTF-16 widen unported |
| FUN_10027a70 | 0x27a70 | DONE (spawn seam) | mveh.createVehicle "i(fff)(fff)i"; validation chain 1:1 (MvlModelLookupRegistry[MvlModelIdBoundary] gates, info+0x19==6); FUN_10008bc0/1003df80 spawn chain = HOOK POINT |
| FUN_10028000 | 0x28000 | DONE (seam) | mveh.getVehicleHealth "O" → float @ veh+0x204; FUN_1002d870/2d790 seams |
| FUN_10028080 | 0x28080 | DONE (seam) | mveh.setVehicleHealth "Of" → store veh+0x204; same seams |
| FUN_10028110 | 0x28110 | DONE (seam) | mveh.blowVehicleUp "OO"; vtable+0x80 call = HOOK POINT |
| FUN_10026990 | 0x26990 | DONE (seam) | **mped**.getPedVehicle "O" (recon listed it under mveh — parse string wins); ped+0x3A8/0x3AC seams |
| FUN_10027b90 | 0x27b90 | SEAM | mveh.getTuningPartCount "i"; FUN_1003b780 engine |
| FUN_10027bf0 | 0x27bf0 | SEAM | mveh.getTuningPartName "ii"; FUN_1003b740 |
| FUN_10027c50 | 0x27c50 | SEAM | mveh.getTuningPartVarCount "ii" |
| FUN_10027cb0 | 0x27cb0 | SEAM | mveh.getVehUsedTuningPart "Oi" |
| FUN_10027d50 | 0x27d50 | SEAM | mveh.setVehUsedTuningPart "Oii" |
| FUN_10027de0 | 0x27de0 | SEAM | mveh.getVehModelAIType "i" |
| FUN_10027f60 | 0x27f60 | SEAM | mveh.setCarTaxiLightState "Oi" |
| FUN_10025a30 | 0x25a30 | DONE | AppendInittab("mvlbase", LAB_10025a50) — init thunk = HOOK POINT |
| FUN_10026940 | 0x26940 | DONE | AppendInittab("mped", LAB_10026960) |
| FUN_10026f80 | 0x26f80 | DONE | AppendInittab("ment", LAB_10026fa0) |
| FUN_10027370 | 0x27370 | DONE | AppendInittab("mrw", LAB_10027390) |
| FUN_10027a30 | 0x27a30 | DONE | AppendInittab("mveh", LAB_10027a50) |
| FUN_100281c0 | 0x281c0 | DONE | AppendInittab("mworld", LAB_100281e0) (PTR_s_mworld_100a6a6c) |
| FUN_10024fd0 | 0x24fd0 | STUB | below 0x25000 dump window — mvlbase inittab (TODO) |
| FUN_10023d10 | 0x23d10 | STUB | below window — manim inittab (TODO) |
| FUN_100290d0 | 0x290d0 | TODO | event-handler map insert (520 B) — called from FUN_100252d0 |
| FUN_10024110 | 0x24110 | TODO | manim.initAnimType — below window (re-098e-map) |

### Init chain (src/core/init.cpp)

| Name | RVA | Status | Notes |
|---|---|---|---|
| FUN_10043810 | 0x43810 | DONE (seams) | 9 code-site immediates ← &MvlModelRedirectSlots[i]; sites 0x58BFC4/BE/B8, 0x58BFF6/F0/EA, 0x58C912/0C/06 |
| FUN_10043870 | 0x43870 | DONE (seams) | 3 sites: 0x597FA4, 0x5980BD ← &MvlAnimRedirectSlot; 0x598221 ← &MvlModelRedirectAux |
| FUN_10043900 | 0x43900 | DONE (seams) | 14 stubs: 0x80 / byteA / 0x7C / byteB / 10×NOP at MvlCodePatchSites; +0xC is a WORD store (two byte seams) |
| FUN_10043a10 | 0x43a10 | DONE (seams) | 61-row remap (0x8118D0..0x8118FC → +0xF92ACE0 = 0x1013C5B0..0x1013C5DC) + 8 byte patches 0x10→0x7F |
| FUN_10043b30 | 0x43b30 | DONE (seams) | 23-row clump chunk-version remap 0x34003 → 0x36003 |

All engine writes route through `MvlInitPatchDword/MvlInitPatchByte` — HOOK
POINT seams that log `init: HOOK POINT write *VA = value (why) - TODO(engine)`
and are the single choke point for the wiring lane to enable live stores.
Nothing is written live today; nothing is faked.

## 2. MvlPyApi proc-table layout (g_MvlPyApi, 50 slots)

Members carry their original `DAT_* @ RVA` in python_host.h. Order of
resolution = original GetProcAddress order in FUN_100287c0. Grouped:

- import: PyImport_Import (b1744), PyImport_ImportModule (b1754), PyImport_AppendInittab (b170c)
- tuple: PyTuple_New (b16fc), PyTuple_Size (b16d8), PyTuple_GetItem (b16f0), PyTuple_SetItem (b16c8)
- list: PyList_New (b16d0), PyList_Append (b16e4), PyList_SetItem (b16dc), PyList_Size (b1728), PyList_GetItem (b1718)
- long: PyLong_FromLong (b1730), PyLong_FromUnsignedLong (b175c), PyLong_FromLongLong (b1714), PyLong_AsLong (b16bc), PyLong_AsLongLong (b172c)
- float: PyFloat_FromDouble (b177c), PyFloat_AsDouble (b171c)
- str/bytes: PyUnicode_FromString (b1748), PyUnicode_AsASCIIString (b1704), PyBytes_AsString (b1710), PyByteArray_AsString (b1740)
- object: PyObject_CallObject (b1700), PyObject_GetAttrString (b174c), PyObject_IsTrue (b1774), PyCallable_Check (b1764), PyType_IsSubtype (b1724)
- err: PyErr_Print (b16ec), PyErr_SetString (b16e0), PyErr_Occurred (b1780), PyErr_Clear (b176c)
- sys/module: PySys_GetObject (b1750), PySys_SetObject (b173c), PyModule_Create2 (b16c4), PyModule_GetName (b1760)
- misc: Py_Initialize (b1738), Py_Finalize (b1768), PyArg_ParseTuple (b16e8), Py_BuildValue (b16d4), PyBool_FromLong (b1708), PyFunction_GetModule (b16f8)
- data: PyUnicode_Type (b1758), PyFloat_Type (b1720), PyLong_Type (b1770), PyBool_Type (b1778), PyFunction_Type (b16cc), PyModule_Type (b1734), _Py_NoneStruct (b16f4), PyExc_KeyError (b16c0)

Signatures are CPython-3.3-stable-shaped `void*`/`int` typedefs (C2: no
Python headers); widening happens only in the struct when the late-phase
bridge lands. Type-checks the decomp does on `ob_type+0x54 tp_flags` are kept
as flag-word reads through the minimal `MvlPyObjectHead` shim (refcnt +
ob_type + tp_dealloc at +0x18) — structural, not a vendored header.

## 3. Event-ID map (FUN_10028cd0, verbatim)

| name | id |
|---|---|
| eventGameStart | 0 |
| eventFrame | 1 |
| eventInternalAddAnim | 2 |
| eventInternalBlendAnim | 3 |
| eventBulletFired | 5 |

IDs 4 and ≥6 are unused in the registration (gap between 3 and 5 preserved).

## 4. Corrections to prior recon

- `getPedVehicle` is **mped**.getPedVehicle (`"O:mped.getPedVehicle"` parse
  string at FUN_10026990), not mveh as re-098e-map lists. Full parse-string
  census proves 8 modules exist: mvlbase, mbase, ment, mped, mveh, mrw,
  mworld, mscm (+ manim at 0x24110, outside this dump).
- FUN_10028cd0 registers into a map keyed by name→id; the "IDs 0/1/2/3/5"
  from recon confirmed exactly.

## 5. Remaining gaps / open questions

1. **Module init thunks** (LAB_10025a50/26960/26fa0/27390/27a50/281e0): these
   are the PyModule_Create2 def-block builders + method tables. Method
   *implementations* are ported; the PyMethodDef tables/def blocks need a
   .rdata/.data extraction (like the model_id site tables) before the modules
   are importable. Currently AppendInittab receives `MvlPyModuleInitHook`
   (log-and-skip).
2. **FUN_100290d0** (520 B) event-handler map insert + FUN_10029700/299a0/
   29b60/29cb0 dispatch family (event firing into registered callbacks).
3. **Below-window functions** (dump starts at 0x25000): FUN_10023d10,
   FUN_10024fd0, FUN_10024110 (manim.initAnimType), FUN_1002a2e0 (error-text
   getter used by FUN_1002a1d0 — ported inline off capture[3]).
4. **Table contents** for the init chain (DAT_100aed68 / DAT_100af014 /
   DAT_100af060 / DAT_100aee5c / DAT_100af0a8 / DAT_100aeff0..af00c) were not
   in the decomp dump — rows are zero, loops log HOOK POINT per row. Extract
   from the DLL .data (same method as model_id.cpp site tables).
5. **FUN_10025060** type checks read `tp_flags` at ob_type+0x54 (0x4000000 =
   tuple, 0x1000000 = long, 0x2000000 = list, 0x10000000 = str, 0x10000 =
   subtype bits per the decomp constants) — flag values are stable CPython 3.3
   but should be re-verified against python33.dll when the bridge lands.
6. **Parse-format string constants** at DAT_1007c575 (None builder), 
   DAT_1007cdd8 ("i"), DAT_1007ce40 ("d") — ported as literals "" / "i" / "d";
   verify against the binary .rdata.
7. **Loaded-script registry** (DAT_1011c33c map + the per-script registry
   writes in FUN_10029310's success path) — logged only; needs the
   std::map→flat-registry decision that FUN_100290d0's port will force.
8. **Wiring**: main.cpp got exactly one call (MvlInstallInitChainPatches);
   FUN_100287c0 (host load) still needs an orchestrator decision on where the
   original invokes it (likely inside FUN_10043c00 after the patch roots).
