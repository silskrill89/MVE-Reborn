// python_host.h - MVL 0.98e Python 3.3 host (mbase/mveh/manim/... modules).
//
// Ported from vehmod_retail.dll 0.98e (image base 0x10000000),
// .harness-memory/limout/pyaudio/range-decomp.txt (range 0x25000-0x2a200).
// Every function carries "0.98e @ RVA 0x..."; Ghidra FUN_<VA> names kept per
// the binding naming policy.
//
// Constraint C2 (no vendored Python headers yet): all Python C-API traffic
// goes through the MvlPyApi proc table below - the same indirection the
// original uses (its own .data slots DAT_100b16bc..DAT_100b1780 filled by
// GetProcAddress inside MvlPyLoadInterpreter). When the late-phase bridge lands,
// only MvlPyApi's signatures need widening, not the call sites.
#pragma once

#include <stdint.h>
#include <stddef.h>

// ---------------------------------------------------------------------------
// 1. MvlPyApi - the python33.dll proc table (original slot VAs per member).
//    Types are void*/int stand-ins (C2); varargs members keep '...'.
// ---------------------------------------------------------------------------

// Generic pointer/args types (opaque under C2 - NO Python headers).
typedef void        MvlPyVoidFn(void);
typedef void       *MvlPyRetP_VoidFn(void);
typedef void        MvlPyVoid_PFn(void *a);
typedef int32_t     MvlPyInt_PFn(void *a);
typedef void       *MvlPyRetP_PFn(void *a);
typedef void       *MvlPyRetP_PPFn(void *a, void *b);
typedef int32_t     MvlPyInt_PPFn(void *a, void *b);

struct MvlPyApi
{
	// --- call slots (original GetProcAddress targets; signatures are the
	//     CPython 3.3 stable ABI used by the decomp's call sites) ---
	void       *(*PyImport_Import)(void *name);            // DAT_100b1744 @ RVA 0xb1744
	void       *(*PyFloat_FromDouble)(double v);           // DAT_100b177c @ RVA 0xb177c
	int32_t     (*PyObject_IsTrue)(void *o);               // DAT_100b1774 @ RVA 0xb1774
	int32_t     (*PyCallable_Check)(void *o);              // DAT_100b1764 @ RVA 0xb1764
	void       *(*PyTuple_New)(int32_t n);                 // DAT_100b16fc @ RVA 0xb16fc
	int32_t     (*PyTuple_Size)(void *t);                  // DAT_100b16d8 @ RVA 0xb16d8
	void       *(*PyTuple_GetItem)(void *t, int32_t i);    // DAT_100b16f0 @ RVA 0xb16f0
	int32_t     (*PyTuple_SetItem)(void *t, int32_t i, void *o); // DAT_100b16c8 @ RVA 0xb16c8
	void       *(*PyObject_CallObject)(void *f, void *args); // DAT_100b1700 @ RVA 0xb1700
	void       *(*PyLong_FromUnsignedLong)(unsigned long v); // DAT_100b175c @ RVA 0xb175c
	void       *(*PyLong_FromLong)(long v);                // DAT_100b1730 @ RVA 0xb1730
	char       *(*PyByteArray_AsString)(void *o);          // DAT_100b1740 @ RVA 0xb1740
	void       *(*PyUnicode_FromString)(const char *s);    // DAT_100b1748 @ RVA 0xb1748
	int32_t     (*PyType_IsSubtype)(void *a, void *b);     // DAT_100b1724 @ RVA 0xb1724
	long        (*PyLong_AsLong)(void *o);                 // DAT_100b16bc @ RVA 0xb16bc
	void       *(*PyUnicode_AsASCIIString)(void *o);       // DAT_100b1704 @ RVA 0xb1704
	void        (*PyErr_Print)(void);                      // DAT_100b16ec @ RVA 0xb16ec
	double      (*PyFloat_AsDouble)(void *o);              // DAT_100b171c @ RVA 0xb171c
	void       *(*PyObject_GetAttrString)(void *o, const char *name); // DAT_100b174c @ RVA 0xb174c
	void        (*Py_Initialize)(void);                    // DAT_100b1738 @ RVA 0xb1738
	void        (*Py_Finalize)(void);                      // DAT_100b1768 @ RVA 0xb1768
	int32_t     (*PyArg_ParseTuple)(void *args, const char *fmt, ...); // DAT_100b16e8 @ RVA 0xb16e8
	void       *(*Py_BuildValue)(const char *fmt, ...);    // DAT_100b16d4 @ RVA 0xb16d4
	void       *(*PySys_GetObject)(const char *name);      // DAT_100b1750 @ RVA 0xb1750
	int32_t     (*PySys_SetObject)(const char *name, void *o); // DAT_100b173c @ RVA 0xb173c
	void       *(*PyImport_ImportModule)(const char *name); // DAT_100b1754 @ RVA 0xb1754
	int32_t     (*PyImport_AppendInittab)(const char *name, void *initfn); // DAT_100b170c @ RVA 0xb170c
	char       *(*PyBytes_AsString)(void *o);              // DAT_100b1710 @ RVA 0xb1710
	char       *(*PyModule_GetName)(void *m);              // DAT_100b1760 @ RVA 0xb1760
	void       *(*PyList_New)(int32_t n);                  // DAT_100b16d0 @ RVA 0xb16d0
	int32_t     (*PyList_Append)(void *l, void *o);        // DAT_100b16e4 @ RVA 0xb16e4
	void       *(*PyBool_FromLong)(long v);                // DAT_100b1708 @ RVA 0xb1708
	void       *(*PyErr_Occurred)(void);                   // DAT_100b1780 @ RVA 0xb1780
	void        (*PyErr_SetString)(void *exc, const char *msg); // DAT_100b16e0 @ RVA 0xb16e0
	int32_t     (*PyList_SetItem)(void *l, int32_t i, void *o); // DAT_100b16dc @ RVA 0xb16dc
	void       *(*PyFunction_GetModule)(void *f);          // DAT_100b16f8 @ RVA 0xb16f8
	int32_t     (*PyList_Size)(void *l);                   // DAT_100b1728 @ RVA 0xb1728
	void       *(*PyList_GetItem)(void *l, int32_t i);     // DAT_100b1718 @ RVA 0xb1718
	long long   (*PyLong_AsLongLong)(void *o);             // DAT_100b172c @ RVA 0xb172c
	void       *(*PyLong_FromLongLong)(long long v);       // DAT_100b1714 @ RVA 0xb1714
	void        (*PyErr_Clear)(void);                      // DAT_100b176c @ RVA 0xb176c
	void       *(*PyModule_Create2)(void *def, int32_t apiver); // DAT_100b16c4 @ RVA 0xb16c4

	// --- data slots (GetProcAddress of DATA symbols; used as opaque ptrs) ---
	void *PyUnicode_Type;   // DAT_100b1758 @ RVA 0xb1758
	void *PyFloat_Type;     // DAT_100b1720 @ RVA 0xb1720
	void *PyLong_Type;      // DAT_100b1770 @ RVA 0xb1770
	void *PyBool_Type;      // DAT_100b1778 @ RVA 0xb1778
	void *PyFunction_Type;  // DAT_100b16cc @ RVA 0xb16cc
	void *PyModule_Type;    // DAT_100b1734 @ RVA 0xb1734
	void *_Py_NoneStruct;   // DAT_100b16f4 @ RVA 0xb16f4
	void *PyExc_KeyError;   // DAT_100b16c0 @ RVA 0xb16c0
};

// Canonical storage (python_host.cpp) - the original keeps the same table as
// 50 separate .data dwords; one struct keeps the mapping 1:1 (slot VAs are
// on each member). Do NOT redeclare per-TU.
extern MvlPyApi g_MvlPyApi;

// HOOK-POINT calling convention note: real python33.dll signatures widen in
// the late-phase bridge; until then every member is called through these
// exact types only.

// ---------------------------------------------------------------------------
// 2. Host state globals (semantic Mvl* names; original DAT_ in trailing
//    comments per AGENTS.md Globals rule).
// ---------------------------------------------------------------------------

// DAT_100b0da3 @ RVA 0xb0da3 - "Python interpreter loaded" flag (set by MvlPyLoadInterpreter).
extern uint8_t MvlPyLoaded;

// DAT_10124f48 @ RVA 0x124f48 - <game dir>\mvl\scripts\ path buffer (260).
extern char MvlScriptDirPath[260];

// DAT_10125050 @ RVA 0x125050 - saved current directory (260).
extern char MvlSavedCwd[260];

// DAT_10124537 @ RVA 0x124537 - one-shot: script path built.
extern uint8_t MvlScriptPathBuilt;

// DAT_10124538 @ RVA 0x124538 - chdir depth for the scripts-dir guard.
extern int32_t MvlScriptDirDepth;

// Error-capture state (original object at DAT_1011c328; layout recovered from
// MvlPyBeginStderrCapture / MvlPyEndStderrCapture field uses).
struct SMvlPyErrCapture
{
	int32_t depth;        // +0x00 reentrancy count
	void   *stringioCls;  // +0x04 cached io.StringIO class
	void   *stringio;     // +0x08 live StringIO instance
	void   *captured;     // +0x0C PyBytes* of captured stderr text
	void   *savedStderr;  // +0x10 previous sys.stderr
};
extern SMvlPyErrCapture MvlPyErrCapture; // DAT_1011c328 @ RVA 0x11c328

// Event registry: name -> id, populated by MvlPyRegisterEvents (original
// MSVC std::map at DAT_1011c344 @ RVA 0x11c344).
struct SMvlPyEvent { const char *name; int32_t id; };
extern SMvlPyEvent MvlPyEventTable[8]; // DAT_1011c344 (map -> static table)

// Loaded-script registry (original map at DAT_1011c33c @ RVA 0x11c33c) and
// the event-callback lists (DAT_1011c43c / DAT_1011c454 @ RVA 0x11c43c/454).
// Ported as flat arrays; see port-notes-py.md Q2.

// ---------------------------------------------------------------------------
// 3. Host control functions.
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x287c0 (1289 B) - LoadLibraryA("python33.dll") + GetProcAddress
// fill of g_MvlPyApi (50 slots, failure of any = "Couldn't load python
// interpreter."), MvlPyLoaded=1 + "Python interpreter loaded.", module
// inittab registration (MvlPyAppendInittabMbase/26940/25a30/27a30/281c0/23d10/26f80/
// 27370), Py_Initialize, script scan (MvlPyScanScripts). Returns 1 in AL.
uint32_t MvlPyLoadInterpreter(void);

// 0.98e @ RVA 0x28cd0 (1009 B) - registers event names into the event table:
//   0 eventGameStart / 1 eventFrame / 2 eventInternalAddAnim /
//   3 eventInternalBlendAnim / 5 eventBulletFired.
void MvlPyRegisterEvents(void);

// 0.98e @ RVA 0x29310 (622 B) - scans *.py in the scripts dir (chdir guard
// MvlPyScriptDirGuard), strips extension, drops stale <name>.err.txt (MvlPyDeleteErrFile),
// imports via PyUnicode_FromString + PyImport_Import under stderr capture
// (MvlPyBeginStderrCapture/2a0a0), logs "Successfully/Failed to load script '%s'.",
// writes errors (MvlPyWriteErrFile).
void MvlPyScanScripts(void);

// 0.98e @ RVA 0x29580 (215 B) - scripts-dir CWD guard: 0 = enter (saves CWD,
// SetCurrentDirectoryA(MvlScriptDirPath), depth++), non-0 = leave (depth--,
// restore at depth 0). Also one-shot builds MvlScriptDirPath from
// GetModuleFileNameA + "mvl\scripts\" (string @ 0x1007e85c). Returns 1 in AL.
uint32_t MvlPyScriptDirGuard(char enter);

// 0.98e @ RVA 0x2a170 (92 B) - deletes "<name>.err.txt" (inside scripts dir).
void MvlPyDeleteErrFile(const char *scriptName);

// 0.98e @ RVA 0x2a1d0 (148 B) - appends captured error text to
// "<name>.err.txt" (fopen "a+", fprintf "%s\n", fclose) when capture depth==0
// and text non-empty. Original __thiscall (capture object); lowered to
// __fastcall + dummy EDX (MSVC C3865).
void __fastcall MvlPyWriteErrFile(void *capture, void* /*edx*/, const char *scriptName);

// 0.98e @ RVA 0x29ff0 (161 B) - begin stderr capture: caches io.StringIO
// (PyImport_ImportModule("io") + GetAttr "StringIO"), swaps sys.stderr.
uint32_t __fastcall MvlPyBeginStderrCapture(int32_t *capture);

// 0.98e @ RVA 0x2a0a0 (194 B) - end capture: StringIO.getvalue -> captured
// bytes, close(), restore sys.stderr.
void __fastcall MvlPyEndStderrCapture(int32_t *capture);

// ---------------------------------------------------------------------------
// 4. Entity-handle marshalling (shared by all module methods).
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x25010 (67 B) - PyObject -> ASCII char buffer (bounded copy).
uint32_t __fastcall MvlPyCopyAsciiArg(void *obj, char *out, int32_t outLen);

// 0.98e @ RVA 0x25100 (82 B) - (kind, index) -> entity-identifier tuple
// (PyTuple of 2 longs; index -1 -> Py_BuildValue(None)).
void *MvlPyMakeEntityId(int32_t kind, int32_t index);

// 0.98e @ RVA 0x25b00 (131 B) - game entity -> handle (kind in bits 0x1c..0x1e
// from entity+0x50&7; 0xFFFFFFFF = none, 0xFFFFFFFE = player).
uint32_t MvlPyMakeEntityHandle(int32_t entity);

// 0.98e @ RVA 0x25ba0 (122 B) - PyObject -> game entity (None + errOK flag,
// "Argument is not an entity identifier." / "Entity does not exist." errors).
int32_t __fastcall MvlPyArgToEntity(void *obj, char errOnNone);

// 0.98e @ RVA 0x25a70 (115 B) - handle -> game entity (kind dispatch on
// game pools; HOOK POINT - pool getters unported).
int32_t __fastcall MvlPyResolveEntityHandle(uint32_t handle);

// 0.98e @ RVA 0x250e0 (20 B) / 0x25160 (20 B) - tuple field 1 as
// long / longlong.
int32_t __fastcall MvlPyIdTupleIndex(void *tuple);
int64_t __fastcall MvlPyIdTupleIndex64(void *tuple);

// ---------------------------------------------------------------------------
// 5. Module method ports (original PyCFunction: (self, args) -> PyObject*).
//    Names recovered 1:1 from the DLL's own PyMethodDef tables in .data
//    (method-name ptr -> PyModuleDef.m_name/m_methods link -> module; full
//    extraction table in .harness-memory/limout/pyfunc-names.md). NOTE: the
//    PyArg_ParseTuple labels for the first three say "mvlbase.*" - STALE
//    dev-era text; the real module is "mbase" (PyModuleDef.m_name @
//    RVA 0xa65f4 + sample scripts do `import mbase` / `mbase.logPrint`).
//    Verbatim parse-format literals in the .cpp stay untouched.
// ---------------------------------------------------------------------------

// mbase module (0.98e @ RVA 0x25280/0x252d0/0x254b0):
void *MvlPyMbaseLogPrint(void *self, void *args);              // FUN_10025280 @ RVA 0x25280 (mbase.logPrint)        "s"
void *MvlPyMbaseSetHudMessage(void *self, void *args);         // FUN_100254b0 @ RVA 0x254b0 (mbase.setHudMessage)   "is"
void *MvlPyMbaseRegisterCallback(void *self, void *args);      // FUN_100252d0 @ RVA 0x252d0 (mbase.registerCallback) (event-handler bind)

// mveh module (0.98e @ RVA 0x27a70..0x28110):
void *MvlPyMvehCreateVehicle(void *self, void *args);          // FUN_10027a70 @ RVA 0x27a70 (mveh.createVehicle)      "i(fff)(fff)i"
void *MvlPyMvehGetVehicleHealth(void *self, void *args);       // FUN_10028000 @ RVA 0x28000 (mveh.getVehicleHealth)   "O"
void *MvlPyMvehSetVehicleHealth(void *self, void *args);       // FUN_10028080 @ RVA 0x28080 (mveh.setVehicleHealth)   "Of"
void *MvlPyMvehBlowVehicleUp(void *self, void *args);          // FUN_10028110 @ RVA 0x28110 (mveh.blowVehicleUp)      "OO"
void *MvlPyMvehGetTuningPartCount(void *self, void *args);     // FUN_10027b90 @ RVA 0x27b90 (mveh.getTuningPartCount) "i"
void *MvlPyMvehGetTuningPartName(void *self, void *args);      // FUN_10027bf0 @ RVA 0x27bf0 (mveh.getTuningPartName)  "ii"
void *MvlPyMvehGetTuningPartVarCount(void *self, void *args);  // FUN_10027c50 @ RVA 0x27c50 (mveh.getTuningPartVarCount) "ii"
void *MvlPyMvehGetVehUsedTuningPart(void *self, void *args);   // FUN_10027cb0 @ RVA 0x27cb0 (mveh.getVehUsedTuningPart)  "Oi"
void *MvlPyMvehSetVehUsedTuningPart(void *self, void *args);   // FUN_10027d50 @ RVA 0x27d50 (mveh.setVehUsedTuningPart)  "Oii"
void *MvlPyMvehGetVehModelAIType(void *self, void *args);      // FUN_10027de0 @ RVA 0x27de0 (mveh.getVehModelAIType)     "i"
void *MvlPyMvehSetCarTaxiLightState(void *self, void *args);   // FUN_10027f60 @ RVA 0x27f60 (mveh.setCarTaxiLightState)  "Oi"

// mped module (0.98e @ RVA 0x26990):
void *MvlPyMpedGetPedVehicle(void *self, void *args);          // FUN_10026990 @ RVA 0x26990 (mped.getPedVehicle)      "O"

// ---------------------------------------------------------------------------
// 6. Event-handler registry + manim tail (final three ports).
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x29660 (157 B) - event name -> handler-slot index (-1 when
// unknown); original was the std::map find at DAT_1011c344 (flat-table
// strcmp scan per the recorded structural decision).
int32_t MvlPyEventSlotByName(const char *name);

// 0.98e @ RVA 0x290d0 (520 B) - event-handler insert: slot via MvlPyEventSlotByName,
// dedup by callable pointer, ob_refcnt incref, append (original std::map +
// std::list machinery replaced by the flat MvlPyEventHandlers arrays -
// structural decision in the .cpp file header).
void MvlPyAddEventHandler(const char *name, void *callable);

// 0.98e @ RVA 0x23db0 - animation-type identifier -> anim-association object
// ("Argument is not an animation type identifier." / "Animation type does
// not exist."); its callee FUN_10002f60 @ 0x2f60 is a below-window HOOK
// POINT seam (body not recovered).
int32_t __fastcall MvlPyAnimTypeToAssoc(void *obj);

// 0.98e @ RVA 0x24110 - manim.initAnimType("Oiii") - writes the three
// anim-type u16 fields (+0x2c / +0x0e / +0x2e) on the association object;
// returns None via the empty Py_BuildValue format (@ RVA 0x7c575).
void *MvlPyManimInitAnimType(void *self, void *args);           // FUN_10024110 @ RVA 0x24110 (manim.initAnimType)
