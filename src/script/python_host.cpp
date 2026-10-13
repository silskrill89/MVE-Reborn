// python_host.cpp - MVL 0.98e Python 3.3 host implementation.
//
// Source: .harness-memory/limout/pyaudio/range-decomp.txt (Ghidra decomp of
// vehmod_retail.dll 0.98e, image base 0x10000000). FUN_<VA> names kept 1:1;
// every port cites its RVA. Unknown engine/game callees are HOOK POINT seams
// (log-and-skip, never faked).
//
// C2 black box: the Python C-API is reached ONLY through g_MvlPyApi; the
// minimal PyObject head shim below (refcnt + ob_type, tp_dealloc at +0x18) is
// the CPython 3.3 stable layout the decomp's refcount traffic assumes - it is
// structural, not a vendored header.
#include "../core/stdafx.h"
#include "python_host.h"
#include "../loader/log.h"
#include "../engine/engine_globals.h"

#include <string.h>
#include <stdio.h>

#if !defined(_M_IX86)
#error "MVL clone: python_host.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// Storage.
// ---------------------------------------------------------------------------
MvlPyApi g_MvlPyApi = {};                          // 50 slots (per-member DAT_ comments in header)
uint8_t MvlPyLoaded = 0;                           // DAT_100b0da3 @ RVA 0xb0da3
char MvlScriptDirPath[260] = {};                   // DAT_10124f48 @ RVA 0x124f48
char MvlSavedCwd[260] = {};                        // DAT_10125050 @ RVA 0x125050
uint8_t MvlScriptPathBuilt = 0;                    // DAT_10124537 @ RVA 0x124537
int32_t MvlScriptDirDepth = 0;                     // DAT_10124538 @ RVA 0x124538
SMvlPyErrCapture MvlPyErrCapture = {};             // DAT_1011c328 @ RVA 0x11c328

// Event name -> id (0.98e @ RVA 0x28cd0 fills these; original storage was the
// MSVC std::map at DAT_1011c344 @ RVA 0x11c344 - flat table + trailing count
// replaces it, same keys/ids).
SMvlPyEvent MvlPyEventTable[8] = {
	{ "eventGameStart",          -1 }, // id 0 assigned by MvlPyRegisterEvents
	{ "eventFrame",              -1 }, // id 1
	{ "eventInternalAddAnim",    -1 }, // id 2
	{ "eventInternalBlendAnim",  -1 }, // id 3
	{ "eventBulletFired",        -1 }, // id 5
	{ NULL, -1 },
};

// Per-event handler lists (original: std::list per event; list heads stride 8
// @ DAT_1011c43c, sizes stride 8 @ DAT_1011c440; map of nodes @ DAT_1011c34c).
// Flat structural replacement (byte-identical NON-goal): per-slot handler
// arrays, dedup by callable pointer.
enum { MVL_PY_MAX_HANDLERS = 128 };
struct SMvlPyHandlerSlot
{
	void   *handlers[MVL_PY_MAX_HANDLERS]; // PyFunction* (incref'd on insert)
	int32_t count;
};
static SMvlPyHandlerSlot MvlPyEventHandlers[8]; // DAT_1011c34c/43c/440

// ---------------------------------------------------------------------------
// Section 2: CPython-3.3-stable object head shim (structural, C2).
// ---------------------------------------------------------------------------

// Unported-callee stub: log once per site (per-frame paths would flood).
#define MVL_HELPERS_TODO(name)                                                \
	do {                                                                      \
		static bool s_logged = false;                                         \
		if (!s_logged) {                                                      \
			s_logged = true;                                                  \
			MvlErrorLog("helpers: TODO(helpers): " #name " unported - inert");\
		}                                                                     \
	} while (0)

// Raw peek at a VC 1.0 data dword (valid only when reached from an installed
// hook inside the game process).
inline uint32_t PeekU32Raw(uint32_t va)
{
	return *reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(va));
}

// Player entity seam (VC global 0x94AD28 - decomp _DAT_0094ad28).
static int32_t PeekPlayerGlobal(void)
{
	return static_cast<int32_t>(PeekU32Raw(0x0094AD28u)); // HOOK POINT
}

struct MvlPyObjectHead
{
	int32_t  refcnt;   // +0x00
	void    *obType;   // +0x04 (MvlPyTypeHead*)
};
struct MvlPyTypeHead
{
	uint8_t pad[0x18];        // up to tp_dealloc
	void  (*tp_dealloc)(void *); // +0x18
	// +0x54 = tp_flags (read by validation helpers; see MvlPyIsEntityIdTuple)
};

// Original decrements ob_refcnt and runs tp_dealloc at 0 when it hits 0 - the
// decomp inlines that everywhere. One helper keeps the ports readable.
static void MvlPyDecref(void *obj)
{
	if (obj == NULL)
		return;
	MvlPyObjectHead *h = static_cast<MvlPyObjectHead *>(obj);
	if (--h->refcnt == 0 && h->obType != NULL)
		static_cast<MvlPyTypeHead *>(h->obType)->tp_dealloc(obj);
}

// ---------------------------------------------------------------------------
// Section 3: error-file writers.
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x2a170 (92 B) - delete "<name>.err.txt" in the scripts dir.
void MvlPyDeleteErrFile(const char *scriptName) // FUN_1002a170 @ RVA 0x2a170
{
	char path[260];
	if (MvlPyScriptDirGuard('\0') == 0)
		return;
	snprintf(path, sizeof(path), "%s.err.txt", scriptName);
	DeleteFileA(path);
	MvlPyScriptDirGuard('\x01');
}

// 0.98e @ RVA 0x2a1d0 (148 B) - append captured stderr text to
// "<name>.err.txt". Original __thiscall (capture object); lowered.
void __fastcall MvlPyWriteErrFile(void *capture, void* /*edx*/, const char *scriptName) // FUN_1002a1d0 @ RVA 0x2a1d0
{
	char path[260];
	int32_t *cap = static_cast<int32_t *>(capture);

	if (cap[0] >= 1)   // capture still open - deferred (original: < 1 required)
		return;
	if (MvlPyScriptDirGuard('\0') == 0)
		return;

	// BUGFIX(stable-1.0): cap[3] (SMvlPyErrCapture::captured) holds the
	// PyBytes* produced by PyUnicode_AsASCIIString in MvlPyEndStderrCapture (0.98e @
	// RVA 0x2a0a0) - NOT a char*. The original fetches the text through its
	// capture-object getter FUN_1002a2e0 (0.98e @ RVA 0x2a2e0, below-window)
	// before writing; the old raw-pointer cast fputs()'ed the bytes OBJECT
	// header (garbage output + unbounded read past the object). Route through
	// the PyBytes_AsString slot instead (unreachable with cap[3]==0, i.e.
	// only after a successful interpreter load, so the slot is populated).
	const char *text = NULL;
	if (cap[3] != 0)
		text = g_MvlPyApi.PyBytes_AsString(
		    reinterpret_cast<void *>(static_cast<intptr_t>(cap[3])));
	if (text != NULL && *text != '\0') {
		snprintf(path, sizeof(path), "%s.err.txt", scriptName);
		FILE *f = fopen(path, "a+");
		if (f != NULL) {
			// 0.98e @ RVA 0x2a1d0: fprintf(f, "%s\n", text) - format
			// string "%s\n" @ RVA 0x7e8b4; the old fputs() dropped the
			// trailing newline the original wrote per error line.
			fprintf(f, "%s\n", text);
			fclose(f);
		}
	}
	MvlPyScriptDirGuard('\x01');
}

// ---------------------------------------------------------------------------
// Section 4: stderr capture (io.StringIO swap).
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x29ff0 (161 B) - begin capture.
uint32_t __fastcall MvlPyBeginStderrCapture(int32_t *capture) // FUN_10029ff0 @ RVA 0x29ff0
{
	if (capture[0] > 0) {              // nested: just count
		capture[0]++;
		return 1;
	}
	if (capture[1] == 0) {             // cache io.StringIO class
		void *io = g_MvlPyApi.PyImport_ImportModule("io");
		if (io == NULL)
			return 0;
		capture[1] = static_cast<int32_t>(reinterpret_cast<intptr_t>(
		    g_MvlPyApi.PyObject_GetAttrString(io, "StringIO")));
		if (capture[1] == 0)
			return 0;
	}
	if (capture[3] != 0) {             // release previous captured bytes
		MvlPyDecref(reinterpret_cast<void *>(static_cast<intptr_t>(capture[3])));
		capture[3] = 0;
	}
	void *sio = g_MvlPyApi.PyObject_CallObject(
	    reinterpret_cast<void *>(static_cast<intptr_t>(capture[1])), NULL);
	capture[2] = static_cast<int32_t>(reinterpret_cast<intptr_t>(sio));
	if (sio == NULL)
		return 0;
	capture[4] = static_cast<int32_t>(reinterpret_cast<intptr_t>(
	    g_MvlPyApi.PySys_GetObject("stderr")));
	g_MvlPyApi.PySys_SetObject("stderr", sio);
	capture[0] = 1;
	return 1;
}

// 0.98e @ RVA 0x2a0a0 (194 B) - end capture: getvalue() -> captured, close,
// restore sys.stderr.
void __fastcall MvlPyEndStderrCapture(int32_t *capture) // FUN_1002a0a0 @ RVA 0x2a0a0
{
	int32_t depth = capture[0];
	if (depth <= 0)
		return;
	if (depth > 1) {                   // nested: just count
		capture[0] = depth - 1;
		return;
	}
	void *sio = reinterpret_cast<void *>(static_cast<intptr_t>(capture[2]));
	void *getter = g_MvlPyApi.PyObject_GetAttrString(sio, "getvalue");
	if (getter != NULL) {
		void *value = g_MvlPyApi.PyObject_CallObject(getter, NULL);
		if (value != NULL) {
			// Original checks ob_type tp_flags & 0x10000000 (str) before
			// PyUnicode_AsASCIIString; flag bit is tp_flags-dependent -> keep
			// the call unconditional and let AsASCIIString fail (C2 shim).
			capture[3] = static_cast<int32_t>(reinterpret_cast<intptr_t>(
			    g_MvlPyApi.PyUnicode_AsASCIIString(value)));
			MvlPyDecref(value);
		}
		void *closer = g_MvlPyApi.PyObject_GetAttrString(sio, "close");
		if (closer != NULL)
			g_MvlPyApi.PyObject_CallObject(closer, NULL);
	}
	MvlPyDecref(sio);
	capture[2] = 0;
	g_MvlPyApi.PySys_SetObject("stderr",
	    reinterpret_cast<void *>(static_cast<intptr_t>(capture[4])));
	capture[0] = 0;
}

// ---------------------------------------------------------------------------
// Section 5: scripts-dir CWD guard.
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x29580 (215 B) - enter/leave the scripts dir; one-shot path
// build (GetModuleFileNameA + last '\' truncate + "mvl\scripts\"
// string @ 0x1007e85c).
uint32_t MvlPyScriptDirGuard(char enter) // FUN_10029580 @ RVA 0x29580
{
	if (MvlScriptPathBuilt == 0) {
		GetModuleFileNameA(NULL, MvlScriptDirPath, 0x104);
		char *slash = strrchr(MvlScriptDirPath, '\\');
		if (slash != NULL)
			slash[1] = '\0'; // keep "<game dir>\" prefix
		strncat(MvlScriptDirPath, "mvl\\scripts\\",
		        sizeof(MvlScriptDirPath) - strlen(MvlScriptDirPath) - 1);
		MvlScriptPathBuilt = 1;
	}

	if (enter == '\0') {
		if (MvlScriptDirDepth == 0)
			GetCurrentDirectoryA(0x104, MvlSavedCwd);
		if (SetCurrentDirectoryA(MvlScriptDirPath) == 0)
			return 0;
		MvlScriptDirDepth++;
		return 1;
	}
	if (MvlScriptDirDepth > 0) {
		if (MvlScriptDirDepth == 1)
			SetCurrentDirectoryA(MvlSavedCwd);
		MvlScriptDirDepth--;
	}
	return 1;
}

// === APPEND-MARKER-PY-2 ===

// ===========================================================================
// Section 6: entity-handle marshalling.
// ===========================================================================

// 0.98e @ RVA 0x25010 (67 B) - PyObject -> bounded ASCII buffer (returns 1/0).
uint32_t __fastcall MvlPyCopyAsciiArg(void *obj, char *out, int32_t outLen) // FUN_10025010 @ RVA 0x25010
{
	void *bytes = g_MvlPyApi.PyUnicode_AsASCIIString(obj);
	if (bytes == NULL) {
		out[0] = '\0';
		return 0;
	}
	const char *s = g_MvlPyApi.PyBytes_AsString(bytes);
	strncpy(out, s, static_cast<size_t>(outLen - 1));
	out[outLen - 1] = '\0';
	return 1;
}

// 0.98e @ RVA 0x25100 (82 B) - (kind, index) -> 2-tuple identifier; -1 -> None.
void *MvlPyMakeEntityId(int32_t kind, int32_t index) // FUN_10025100 @ RVA 0x25100
{
	if (index == -1)
		return g_MvlPyApi.Py_BuildValue("");
	void *tup = g_MvlPyApi.PyTuple_New(2);
	g_MvlPyApi.PyTuple_SetItem(tup, 0, g_MvlPyApi.PyLong_FromLong(kind));
	g_MvlPyApi.PyTuple_SetItem(tup, 1, g_MvlPyApi.PyLong_FromLong(index));
	return tup;
}

// 0.98e @ RVA 0x250e0 (20 B) - tuple field 1 as long.
int32_t __fastcall MvlPyIdTupleIndex(void *tuple) // FUN_100250e0 @ RVA 0x250e0
{
	void *item = g_MvlPyApi.PyTuple_GetItem(tuple, 1);
	return static_cast<int32_t>(g_MvlPyApi.PyLong_AsLong(item));
}

// 0.98e @ RVA 0x25160 (20 B) - tuple field 1 as long long.
int64_t __fastcall MvlPyIdTupleIndex64(void *tuple) // FUN_10025160 @ RVA 0x25160
{
	void *item = g_MvlPyApi.PyTuple_GetItem(tuple, 1);
	return static_cast<int64_t>(g_MvlPyApi.PyLong_AsLongLong(item));
}

// 0.98e @ RVA 0x25060 (114 B) - "is this a (kind, index) identifier tuple?"
// Original reads ob_type tp_flags & 0x4000000 (tuple) and Tuple_Size==2 +
// field types. C2 shim: flag test via PyType_IsSubtype against the cached
// tuple slot is unavailable (no PyTuple_Type slot resolved by 0x287c0!), so
// the flag word at +0x54 is read directly as the decomp does.
static uint32_t MvlPyIsEntityIdTuple(void *obj, uint32_t expectKind) // FUN_10025060 @ RVA 0x25060
{
	(void)expectKind;
	MvlPyObjectHead *h = static_cast<MvlPyObjectHead *>(obj);
	uint32_t tpFlags = *reinterpret_cast<const uint32_t *>(
	    static_cast<const uint8_t *>(h->obType) + 0x54);
	if ((tpFlags & 0x4000000u) == 0)
		return 0;
	if (g_MvlPyApi.PyTuple_Size(obj) != 2)
		return 0;
	return 1;
}

// 0.98e @ RVA 0x25a70 (115 B) - handle -> game entity. Kind lives in bits
// 0x1c.. (1 ped, 2 vehicle, 3 object, 4 building per pool getters).
// HOOK POINT: the four pool getters (VC globals 0x97F240 / 0xA0FDE4 /
// 0x97F2AC / 0x94DBE0) and index fetchers are engine seams.
int32_t __fastcall MvlPyResolveEntityHandle(uint32_t handle) // FUN_10025a70 @ RVA 0x25a70
{
	static uint32_t (*const kPoolGetters[4])(uint32_t, uint32_t) = { NULL, NULL, NULL, NULL };
	// TODO(engine): FUN_100267a0/268b0/26840/267d0 pool getters unported.
	if (handle == 0xFFFFFFFEu)
		return static_cast<int32_t>(PeekPlayerGlobal()); // HOOK POINT (below)
	if (static_cast<int32_t>(handle) < 0)
		return 0;
	uint32_t kind = (handle >> 0x1c) & 7u;
	if (kind < 1 || kind > 4 || kPoolGetters[kind - 1] == NULL) {
		MvlErrorLog("helpers: TODO(helpers): MvlPyResolveEntityHandle pool getter kind %u - HOOK POINT", kind);
		return 0;
	}
	return static_cast<int32_t>(kPoolGetters[kind - 1](
	           handle & 0x0FFFFFFFu, kind));
}

// 0.98e @ RVA 0x25b00 (131 B) - game entity -> handle (kind from entity+0x50
// & 7). HOOK POINT: pool slot lookups (FUN_10026760/26870/26800/0008040).
uint32_t MvlPyMakeEntityHandle(int32_t entity) // FUN_10025b00 @ RVA 0x25b00
{
	if (entity == 0)
		return 0xFFFFFFFFu;
	MVL_HELPERS_TODO(MvlPyMakeEntityHandle); // pool slot lookups unported
	(void)entity;
	return 0xFFFFFFFFu;
}

// 0.98e @ RVA 0x25ba0 (122 B) - PyObject -> game entity with errors.
int32_t __fastcall MvlPyArgToEntity(void *obj, char errOnNone) // FUN_10025ba0 @ RVA 0x25ba0
{
	if (obj == NULL)
		return 0;
	if (obj == g_MvlPyApi._Py_NoneStruct) {
		if (errOnNone != '\0') {
			g_MvlPyApi.PyErr_SetString(g_MvlPyApi.PyExc_KeyError,
			                          "Argument value is None.");
			return 0;
		}
		return 0;
	}
	if (MvlPyIsEntityIdTuple(obj, 1) == 0) {
		g_MvlPyApi.PyErr_SetString(g_MvlPyApi.PyExc_KeyError,
		                          "Argument is not an entity identifier.");
		return 0;
	}
	int32_t entity = MvlPyResolveEntityHandle(static_cast<uint32_t>(MvlPyIdTupleIndex(obj)));
	if (entity != 0)
		return entity;
	g_MvlPyApi.PyErr_SetString(g_MvlPyApi.PyExc_KeyError,
	                          "Entity does not exist.");
	return 0;
}

// === APPEND-MARKER-PY-3 ===

// ===========================================================================
// Section 7: event registry + script scan.
// ===========================================================================

// 0.98e @ RVA 0x28cd0 (1009 B) - registers event names -> ids in the event
// map (original: MSVC std::map<std::string,int> at DAT_1011c344, inserted via
// FUN_1002a3f0/1002a570/100186d0). Names/ids verbatim from the decomp.
void MvlPyRegisterEvents(void) // FUN_10028cd0 @ RVA 0x28cd0
{
	static const SMvlPyEvent kEvents[] = {
		{ "eventGameStart",         0 },
		{ "eventFrame",             1 },
		{ "eventInternalAddAnim",   2 },
		{ "eventInternalBlendAnim", 3 },
		{ "eventBulletFired",       5 },
	};
	for (size_t i = 0; i < sizeof(kEvents) / sizeof(kEvents[0]); ++i) {
		MvlPyEventTable[i].name = kEvents[i].name;
		MvlPyEventTable[i].id   = kEvents[i].id;
	}
}

// 0.98e @ RVA 0x29310 (622 B) - scan *.py in the scripts dir and import each.
void MvlPyScanScripts(void) // FUN_10029310 @ RVA 0x29310
{
	if (MvlPyScriptDirGuard('\0') == 0)
		return;

	char oldCwd[260];
	GetCurrentDirectoryA(0x100, oldCwd); // original saves cwd at frame top

	WIN32_FIND_DATAA fd;
	HANDLE find = FindFirstFileA("*.py", &fd);
	if (find != INVALID_HANDLE_VALUE) {
		do {
			if ((fd.dwFileAttributes & 0x10) != 0)
				continue; // skip directories (original spins on this)
			char name[64];
			strncpy(name, fd.cFileName, 0x3f);
			name[0x3f] = '\0';
			// BUGFIX(stable-1.0): the original strips at the FIRST dot
			// (strchr, FUN_1005abd0 @ RVA 0x5abd0; decomp MvlPyScanScripts @
			// RVA 0x29310) - the old strrchr() imported "my.script.py" as
			// "my.script" instead of "my". The original then faults on a
			// dotless name (impossible for *.py matches); the NULL guard
			// stays as a defensive net.
			char *dot = strchr(name, '.');
			if (dot != NULL)
				*dot = '\0'; // strip extension

			MvlPyDeleteErrFile(name); // drop stale <name>.err.txt

			void *pyName = g_MvlPyApi.PyUnicode_FromString(name);
			MvlPyBeginStderrCapture(reinterpret_cast<int32_t *>(&MvlPyErrCapture));
			void *mod = g_MvlPyApi.PyImport_Import(pyName);
			g_MvlPyApi.PyErr_Print();
			MvlPyEndStderrCapture(reinterpret_cast<int32_t *>(&MvlPyErrCapture));
			MvlPyWriteErrFile(&MvlPyErrCapture, NULL, name);

			if (mod == NULL) {
				// BUGFIX(stable-1.0): both scan messages went through
				// FUN_1001cc00 (0.98e @ RVA 0x1cc00) = message channel ->
				// MvlLog per the repo mapping, not the error log.
				MvlLog("Failed to load script '%s'.", fd.cFileName);
			} else {
				MvlLog("Successfully loaded script '%s'.", fd.cFileName);
				// Original records module name into the loaded-script map
				// (DAT_1011c33c) keyed by module object; clone keeps the
				// registry as a TODO (see port-notes-py.md Q2).
				const char *modName = g_MvlPyApi.PyModule_GetName(mod);
				if (modName != NULL)
					MvlErrorLog("script: registered module '%s'", modName);
			}
		} while (FindNextFileA(find, &fd) != 0);
		FindClose(find);
	}

	MvlPyScriptDirGuard('\x01');
	(void)oldCwd;
}

// ===========================================================================
// Section 8: the loader.
// ===========================================================================

// Inittab registrations (defined Section 9) + below-window stubs.
void MvlPyAppendInittabMent(void);
void MvlPyAppendInittabMped(void);
void MvlPyAppendInittabMrw(void);
void MvlPyAppendInittabMscm(void);
void MvlPyAppendInittabMveh(void);
void MvlPyAppendInittabMworld(void);
static void MvlPyAppendInittabMbase(void);
static void MvlPyAppendInittabManim(void);

// 0.98e @ RVA 0x287c0 (1289 B) - python33.dll load + proc-table fill.
// The original resolves 51 symbols one-by-one and ORs the failure flags;
// the same set is resolved here through a name table into g_MvlPyApi.
uint32_t MvlPyLoadInterpreter(void) // FUN_100287c0 @ RVA 0x287c0
{
	if ((char)MvlPyScriptDirGuard('\0') == '\0')
		return 0;

	HMODULE hMod = LoadLibraryA("python33.dll");
	if (hMod == NULL) {
		// BUGFIX(stable-1.0): original logs this via FUN_1001cc00 (0.98e @
		// RVA 0x1cc00) = message channel -> MvlLog per the repo mapping
		// (vehicle_archive.cpp / log.h), not the error log.
		MvlLog("Couldn't load python interpreter.");
		MvlPyScriptDirGuard('\x01');
		return 0;
	}

	// name -> slot table (order = original GetProcAddress order, RVA order of
	// the DAT_ slots is preserved in the struct comments).
	struct Slot { size_t offset; const char *name; };
	static const Slot kSlots[] = {
		{ offsetof(MvlPyApi, PyModule_Create2),         "PyModule_Create2" },
		{ offsetof(MvlPyApi, PyImport_Import),          "PyImport_Import" },
		{ offsetof(MvlPyApi, PyFloat_FromDouble),       "PyFloat_FromDouble" },
		{ offsetof(MvlPyApi, PyObject_IsTrue),          "PyObject_IsTrue" },
		{ offsetof(MvlPyApi, PyCallable_Check),         "PyCallable_Check" },
		{ offsetof(MvlPyApi, PyTuple_New),              "PyTuple_New" },
		{ offsetof(MvlPyApi, PyTuple_Size),             "PyTuple_Size" },
		{ offsetof(MvlPyApi, PyTuple_GetItem),          "PyTuple_GetItem" },
		{ offsetof(MvlPyApi, PyTuple_SetItem),          "PyTuple_SetItem" },
		{ offsetof(MvlPyApi, PyObject_CallObject),      "PyObject_CallObject" },
		{ offsetof(MvlPyApi, PyLong_FromUnsignedLong),  "PyLong_FromUnsignedLong" },
		{ offsetof(MvlPyApi, PyLong_FromLong),          "PyLong_FromLong" },
		{ offsetof(MvlPyApi, PyByteArray_AsString),     "PyByteArray_AsString" },
		{ offsetof(MvlPyApi, PyUnicode_FromString),     "PyUnicode_FromString" },
		{ offsetof(MvlPyApi, PyType_IsSubtype),         "PyType_IsSubtype" },
		{ offsetof(MvlPyApi, PyLong_AsLong),            "PyLong_AsLong" },
		{ offsetof(MvlPyApi, PyUnicode_AsASCIIString),  "PyUnicode_AsASCIIString" },
		{ offsetof(MvlPyApi, PyErr_Print),              "PyErr_Print" },
		{ offsetof(MvlPyApi, PyFloat_AsDouble),         "PyFloat_AsDouble" },
		{ offsetof(MvlPyApi, PyObject_GetAttrString),   "PyObject_GetAttrString" },
		{ offsetof(MvlPyApi, Py_Initialize),            "Py_Initialize" },
		{ offsetof(MvlPyApi, Py_Finalize),              "Py_Finalize" },
		{ offsetof(MvlPyApi, PyArg_ParseTuple),         "PyArg_ParseTuple" },
		{ offsetof(MvlPyApi, Py_BuildValue),            "Py_BuildValue" },
		{ offsetof(MvlPyApi, PySys_GetObject),          "PySys_GetObject" },
		{ offsetof(MvlPyApi, PySys_SetObject),          "PySys_SetObject" },
		{ offsetof(MvlPyApi, PyImport_ImportModule),    "PyImport_ImportModule" },
		{ offsetof(MvlPyApi, PyImport_AppendInittab),   "PyImport_AppendInittab" },
		{ offsetof(MvlPyApi, PyBytes_AsString),         "PyBytes_AsString" },
		{ offsetof(MvlPyApi, PyModule_GetName),         "PyModule_GetName" },
		{ offsetof(MvlPyApi, PyList_New),               "PyList_New" },
		{ offsetof(MvlPyApi, PyList_Append),            "PyList_Append" },
		{ offsetof(MvlPyApi, PyBool_FromLong),          "PyBool_FromLong" },
		{ offsetof(MvlPyApi, PyErr_Occurred),           "PyErr_Occurred" },
		{ offsetof(MvlPyApi, PyErr_SetString),          "PyErr_SetString" },
		{ offsetof(MvlPyApi, PyList_SetItem),           "PyList_SetItem" },
		{ offsetof(MvlPyApi, PyFunction_GetModule),     "PyFunction_GetModule" },
		{ offsetof(MvlPyApi, PyList_Size),              "PyList_Size" },
		{ offsetof(MvlPyApi, PyList_GetItem),           "PyList_GetItem" },
		{ offsetof(MvlPyApi, PyLong_AsLongLong),        "PyLong_AsLongLong" },
		{ offsetof(MvlPyApi, PyLong_FromLongLong),      "PyLong_FromLongLong" },
		{ offsetof(MvlPyApi, PyErr_Clear),              "PyErr_Clear" },
		{ offsetof(MvlPyApi, PyUnicode_Type),           "PyUnicode_Type" },
		{ offsetof(MvlPyApi, PyFloat_Type),             "PyFloat_Type" },
		{ offsetof(MvlPyApi, PyLong_Type),              "PyLong_Type" },
		{ offsetof(MvlPyApi, PyBool_Type),              "PyBool_Type" },
		{ offsetof(MvlPyApi, PyFunction_Type),          "PyFunction_Type" },
		{ offsetof(MvlPyApi, PyModule_Type),            "PyModule_Type" },
		{ offsetof(MvlPyApi, _Py_NoneStruct),           "_Py_NoneStruct" },
		{ offsetof(MvlPyApi, PyExc_KeyError),           "PyExc_KeyError" },
	};

	int missing = 0;
	for (size_t i = 0; i < sizeof(kSlots) / sizeof(kSlots[0]); ++i) {
		FARPROC p = GetProcAddress(hMod, kSlots[i].name);
		*reinterpret_cast<FARPROC *>(
		    reinterpret_cast<uint8_t *>(&g_MvlPyApi) + kSlots[i].offset) = p;
		if (p == NULL)
			++missing;
	}
	if (missing > 0) {
		// BUGFIX(stable-1.0): cc00 = message channel (see above).
		MvlLog("Couldn't load all python functions %08X %08X.",
		       (unsigned)missing, 0u);
		MvlPyScriptDirGuard('\x01');
		return 0;
	}

	MvlPyLoaded = 1; // DAT_100b0da3 @ RVA 0xb0da3
	// BUGFIX(stable-1.0): cc00 = message channel (see above).
	MvlLog("Python interpreter loaded.");

	MvlPyRegisterEvents();      // event name registration
	MvlPyAppendInittabMbase();      // HOOK POINT: mbase inittab (below-window callee)
	MvlPyAppendInittabMped();      // mped inittab
	MvlPyAppendInittabMent();      // ment inittab
	MvlPyAppendInittabMveh();      // mveh inittab
	MvlPyAppendInittabMworld();      // mworld inittab
	MvlPyAppendInittabManim();      // HOOK POINT: manim inittab (below-window callee)
	MvlPyAppendInittabMrw();      // mrw inittab
	MvlPyAppendInittabMscm();      // mscm inittab
	g_MvlPyApi.Py_Initialize();
	MvlPyScanScripts();      // script scan
	MvlPyScriptDirGuard('\x01');
	return 1;
}

// === APPEND-MARKER-PY-4 ===

// ===========================================================================
// Section 9: module inittab registrations (PyImport_AppendInittab wrappers).
// Each 0.98e stub passes (module-name ptr in plugin .data, module init fn);
// the init fns (LAB_10025a50 etc.) build the PyModuleDef + method table via
// PyModule_Create2 - method tables are ported in Section 10, the LAB init
// thunks stay HOOK POINTs until the def-blocks are recovered (Q3).
// ===========================================================================

// Below-window registration stubs called by MvlPyLoadInterpreter (bodies at 0x24fd0 /
// 0x23d10 are outside the 0x25000..0x2a200 dump). PE evidence (immediates in
// the wrappers): 0x24fd0 = push init 0x24ff0; push [0xa65f4] (PyModuleDef
// m_name = "mbase"); 0x23d10 = push init 0x23d30; push [0xa6400] ("manim").
// TODO(helpers): 0x10024fd0 unported (mbase inittab, init fn 0x24ff0);
// TODO(helpers): 0x10023d10 unported (manim inittab, init fn 0x23d30).
static void MvlPyAppendInittabMbase(void) // FUN_10024fd0 @ RVA 0x24fd0
{
	MVL_HELPERS_TODO(MvlPyAppendInittabMbase); // mbase module init registration
}
static void MvlPyAppendInittabManim(void) // FUN_10023d10 @ RVA 0x23d10
{
	MVL_HELPERS_TODO(MvlPyAppendInittabManim); // manim module init registration
}

// Module init thunks (LAB_ stubs in the original - PyModule_Create2 with the
// module's method table). HOOK POINT until PyModuleDef shapes are needed.
static void MvlPyModuleInitHook(const char *module)
{
	MvlErrorLog("script: TODO(helpers): module init '%s' (PyModule_Create2 "
	            "def block unported) - HOOK POINT", module);
}

// 0.98e @ RVA 0x25a30 (21 B) - AppendInittab(PTR_DAT_100a6628, LAB_10025a50).
// Name is loaded from [0xa6628] = PyModuleDef m_name -> "ment" (the clone
// previously passed a stale "mvlbase" parse-label; PE + scripts say ment).
void MvlPyAppendInittabMent(void) // FUN_10025a30 @ RVA 0x25a30
{
	g_MvlPyApi.PyImport_AppendInittab("ment",
	    reinterpret_cast<void *>(&MvlPyModuleInitHook));
}

// 0.98e @ RVA 0x26940 (21 B) - AppendInittab(PTR_DAT_100a682c, LAB_10026960).
void MvlPyAppendInittabMped(void) // FUN_10026940 @ RVA 0x26940
{
	g_MvlPyApi.PyImport_AppendInittab("mped",
	    reinterpret_cast<void *>(&MvlPyModuleInitHook));
}

// 0.98e @ RVA 0x26f80 (21 B) - AppendInittab(PTR_DAT_100a6860, LAB_10026fa0).
// Name from [0xa6860] = PyModuleDef m_name -> "mrw" (clone said "ment").
void MvlPyAppendInittabMrw(void) // FUN_10026f80 @ RVA 0x26f80
{
	g_MvlPyApi.PyImport_AppendInittab("mrw",
	    reinterpret_cast<void *>(&MvlPyModuleInitHook));
}

// 0.98e @ RVA 0x27370 (21 B) - AppendInittab(PTR_DAT_100a6944, LAB_10027390).
// Name from [0xa6944] = PyModuleDef m_name -> "mscm" (clone said "mrw").
void MvlPyAppendInittabMscm(void) // FUN_10027370 @ RVA 0x27370
{
	g_MvlPyApi.PyImport_AppendInittab("mscm",
	    reinterpret_cast<void *>(&MvlPyModuleInitHook));
}

// 0.98e @ RVA 0x27a30 (21 B) - AppendInittab(PTR_DAT_100a6978, LAB_10027a50).
void MvlPyAppendInittabMveh(void) // FUN_10027a30 @ RVA 0x27a30
{
	g_MvlPyApi.PyImport_AppendInittab("mveh",
	    reinterpret_cast<void *>(&MvlPyModuleInitHook));
}

// 0.98e @ RVA 0x281c0 (21 B) - AppendInittab(PTR_s_mworld_100a6a6c, LAB_100281e0).
void MvlPyAppendInittabMworld(void) // FUN_100281c0 @ RVA 0x281c0
{
	g_MvlPyApi.PyImport_AppendInittab("mworld",
	    reinterpret_cast<void *>(&MvlPyModuleInitHook));
}

// === APPEND-MARKER-PY-5 ===

// ===========================================================================
// Section 10: module method ports (PyCFunction bodies).
// Parse format strings are verbatim from the decomp ("s:mvlbase.logPrint" etc).
// Engine/game callees (FUN_1002d870 entity-type, FUN_1002d790 vehicle cast,
// vtable+0x80 blow-up, ...) are HOOK POINT seams - never faked.
// ===========================================================================

// BuildValue result constants (original .data immediates used by every method):
static void *MvlPyNoneResult(void)   { return g_MvlPyApi.Py_BuildValue(""); }
static void *MvlPyTrueResult(void)   { return g_MvlPyApi.Py_BuildValue("i", 1); }

// 0.98e @ RVA 0x25280 (66 B) - mbase.logPrint("s"). (The verbatim parse
// label in the binary reads "s:mvlbase.logPrint" - stale dev-era text; the
// PyMethodDef/PyModuleDef + sample scripts prove the module is "mbase".)
void *MvlPyMbaseLogPrint(void *self, void *args)                 // FUN_10025280 @ RVA 0x25280 (mbase.logPrint)
{
	(void)self;
	const char *text = NULL;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "s:mvlbase.logPrint", &text) == 0)
		return MvlPyNoneResult();
	MvlErrorLog("%s", text); // original: FUN_1001cc00("%s") message log
	return MvlPyTrueResult();
}

// 0.98e @ RVA 0x252d0 (236 B) - mbase.registerCallback (event-handler bind,
// 2 args: name str, callable). PyMethodDef ml_name "registerCallback" (mbase
// table @ RVA 0xa6648). Verifies arg types then calls MvlPyAddEventHandler(name, fn)
// (the map insert - flat-table port in Section 11).
void *MvlPyMbaseRegisterCallback(void *self, void *args)         // FUN_100252d0 @ RVA 0x252d0 (mbase.registerCallback)
{
	(void)self;
	if (g_MvlPyApi.PyTuple_Size(args) < 2)
		return MvlPyNoneResult();
	void *nameObj = g_MvlPyApi.PyTuple_GetItem(args, 0);
	void *fnObj = g_MvlPyApi.PyTuple_GetItem(args, 1);
	char name[64];
	if (MvlPyCopyAsciiArg(nameObj, name, 0x40) == 0) {
		g_MvlPyApi.PyErr_SetString(g_MvlPyApi.PyExc_KeyError,
		                          "Event name string read error.");
		return MvlPyNoneResult();
	}
	if (g_MvlPyApi.PyCallable_Check(fnObj) == 0) {
		g_MvlPyApi.PyErr_SetString(g_MvlPyApi.PyExc_KeyError,
		                          "Argument two should be a function.");
		return MvlPyNoneResult();
	}
	MvlPyAddEventHandler(name, fnObj); // 0.98e @ RVA 0x290d0 (event-handler insert)
	return MvlPyTrueResult();
}

// 0.98e @ RVA 0x254b0 (357 B) - mbase.setHudMessage("is") -> HUD text. (The
// verbatim parse label "is:mvlbase.setHudMessage" is stale dev-era text; the
// real module is "mbase" - see the Section 5 note in python_host.h.)
// Original widens the msg to UTF-16 (FUN_100258a0) and pushes it via
// FUN_1002e870 (HUD seam).
void *MvlPyMbaseSetHudMessage(void *self, void *args)            // FUN_100254b0 @ RVA 0x254b0 (mbase.setHudMessage)
{
	(void)self;
	int32_t slot = 0;
	const char *text = NULL;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "is:mvlbase.setHudMessage",
	                                &slot, &text) == 0)
		return MvlPyNoneResult();
	if (slot == 0) {
		// TODO(helpers): FUN_1002e870 HUD push unported - log-only seam.
		MVL_HELPERS_TODO(HudPush); // engine seam: FUN_1002e870 @ RVA 0x2e870
		MvlErrorLog("script: setHudMessage \"%s\" - HOOK POINT", text ? text : "");
	}
	return MvlPyTrueResult();
}

// ---------------------------------------------------------------------------
// mveh module.
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x27a70 (285 B) - mveh.createVehicle("i(fff)(fff)i").
// Validation chain: model in [0, MvlModelIdBoundary), registry entry != 0,
// info+0x19 == 6 (vehicle class). Spawn + handle-wrap via engine seams.
void *MvlPyMvehCreateVehicle(void *self, void *args)             // FUN_10027a70 @ RVA 0x27a70 (mveh.createVehicle)
{
	(void)self;
	int32_t modelIndex = 0;
	float pos[3] = { 0, 0, 0 };
	float rot[3] = { 0, 0, 0 };
	int32_t flags = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "i(fff)(fff)i:mveh.createVehicle",
	                                &modelIndex, &pos[0], &pos[1], &pos[2],
	                                &rot[0], &rot[1], &rot[2], &flags) == 0)
		return MvlPyNoneResult();

	// Registry checks use the engine_globals shared registry (original
	// DAT_100a6314 / DAT_100a6320 - model-lookup registry + model boundary).
	// BUGFIX(stable-1.0): also refuse when the registry table itself is
	// unwired - the original always had DAT_100a6314/DAT_100a6320 populated
	// together, but a future engine-lane state of (boundary>0, registry==NULL)
	// would deref NULL on the line below.
	if (modelIndex < 0 || modelIndex >= MvlModelIdBoundary ||
	    MvlModelLookupRegistry == NULL)
		return MvlPyNoneResult();
	int32_t info = reinterpret_cast<int32_t *>(MvlModelLookupRegistry)[modelIndex];
	if (info == 0)
		return MvlPyNoneResult();
	if (*reinterpret_cast<const char *>(static_cast<uintptr_t>(info + 0x19)) != 6)
		return MvlPyNoneResult();

	// TODO(helpers): FUN_10008bc0 / FUN_1003df80 spawn chain unported
	// (engine seams - vehicle creation + flag stamps 0x1F8/0x50/0x230).
	MVL_HELPERS_TODO(VehicleSpawnChain); // engine seam: FUN_1003df80 @ RVA 0x3df80
	MvlErrorLog("script: createVehicle model=%d - HOOK POINT (spawn chain)",
	            modelIndex);
	return MvlPyNoneResult();
}

// 0.98e @ RVA 0x28000 (124 B) - mveh.getVehicleHealth("O") -> float.
void *MvlPyMvehGetVehicleHealth(void *self, void *args)          // FUN_10028000 @ RVA 0x28000 (mveh.getVehicleHealth)
{
	(void)self;
	void *obj = NULL;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "O:mveh.getVehicleHealth", &obj) == 0)
		return NULL;
	int32_t entity = MvlPyArgToEntity(obj, '\x01');
	if (entity == 0)
		return NULL;
	// TODO(helpers): FUN_1002d870 entity-type / FUN_1002d790 vehicle-cast
	// engine seams (type 2 = vehicle).
	MVL_HELPERS_TODO(EntityTypeQuery); // engine seam: FUN_1002d870 @ RVA 0x2d870
	g_MvlPyApi.PyErr_SetString(g_MvlPyApi.PyExc_KeyError,
	                          "Entity type is not vehicle.");
	return NULL;
}

// 0.98e @ RVA 0x28080 (133 B) - mveh.setVehicleHealth("Of").
void *MvlPyMvehSetVehicleHealth(void *self, void *args)          // FUN_10028080 @ RVA 0x28080 (mveh.setVehicleHealth)
{
	(void)self;
	void *obj = NULL;
	double health = 0.0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "Of:mveh.setVehicleHealth",
	                                &obj, &health) == 0)
		return NULL;
	int32_t entity = MvlPyArgToEntity(obj, '\x01');
	if (entity == 0)
		return NULL;
	// TODO(helpers): FUN_1002d870/FUN_1002d790 seams; store at vehicle+0x204.
	MVL_HELPERS_TODO(VehicleCast); // engine seam: FUN_1002d790 @ RVA 0x2d790
	MvlErrorLog("script: setVehicleHealth(%d, %f) - HOOK POINT",
	            entity, health);
	return MvlPyTrueResult();
}

// 0.98e @ RVA 0x28110 (167 B) - mveh.blowVehicleUp("OO") (veh, attacker).
void *MvlPyMvehBlowVehicleUp(void *self, void *args)             // FUN_10028110 @ RVA 0x28110 (mveh.blowVehicleUp)
{
	(void)self;
	void *vehObj = NULL;
	void *attObj = NULL;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "OO:mveh.blowVehicleUp",
	                                &vehObj, &attObj) == 0)
		return NULL;
	int32_t vehicle = MvlPyArgToEntity(vehObj, '\x01');
	if (vehicle == 0)
		return NULL;
	int32_t attacker = MvlPyArgToEntity(attObj, '\0');
	if (g_MvlPyApi.PyErr_Occurred() != NULL)
		return NULL;
	// TODO(helpers): vtable+0x80 blow-up call on the vehicle (engine seam).
	MVL_HELPERS_TODO(FUN_10028110_vtable80);
	MvlErrorLog("script: blowVehicleUp(%d,%d) - HOOK POINT", vehicle, attacker);
	return MvlPyTrueResult();
}

// 0.98e @ RVA 0x26990 (146 B) - mped.getPedVehicle("O") -> identifier tuple.
void *MvlPyMpedGetPedVehicle(void *self, void *args)             // FUN_10026990 @ RVA 0x26990 (mped.getPedVehicle)
{
	(void)self;
	void *obj = NULL;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "O:mped.getPedVehicle", &obj) == 0)
		return NULL;
	int32_t ped = MvlPyArgToEntity(obj, '\x01');
	if (ped == 0)
		return NULL;
	// TODO(helpers): FUN_1002d870 type-3 (ped) check + FUN_1002d760 ped cast +
	// +0x3A8 vehicle slot / +0x3AC inside-vehicle flag (engine seams).
	MVL_HELPERS_TODO(PedCast); // engine seam: FUN_1002d760 @ RVA 0x2d760
	return MvlPyNoneResult();
}

// ---------------------------------------------------------------------------
// mveh tuning-part family (parse strings verbatim; all engine lookups
// FUN_1003b780/3b740/... are HOOK POINT seams).
// ---------------------------------------------------------------------------

// 0.98e @ RVA 0x27b90 (81 B) - mveh.getTuningPartCount("i").
void *MvlPyMvehGetTuningPartCount(void *self, void *args)        // FUN_10027b90 @ RVA 0x27b90 (mveh.getTuningPartCount)
{
	(void)self;
	int32_t model = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "i:mveh.getTuningPartCount",
	                                &model) == 0)
		return MvlPyNoneResult();
	MVL_HELPERS_TODO(TuningPartCountLookup); // engine seam: FUN_1003b780 @ RVA 0x3b780
	return g_MvlPyApi.Py_BuildValue("i", 0);
}

// 0.98e @ RVA 0x27bf0 (90 B) - mveh.getTuningPartName("ii").
void *MvlPyMvehGetTuningPartName(void *self, void *args)         // FUN_10027bf0 @ RVA 0x27bf0 (mveh.getTuningPartName)
{
	(void)self;
	int32_t model = 0, index = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "ii:mveh.getTuningPartName",
	                                &model, &index) == 0)
		return MvlPyNoneResult();
	MVL_HELPERS_TODO(TuningPartNameLookup); // engine seam: FUN_1003b740 @ RVA 0x3b740
	return MvlPyNoneResult();
}

// 0.98e @ RVA 0x27c50 (90 B) - mveh.getTuningPartVarCount("ii").
void *MvlPyMvehGetTuningPartVarCount(void *self, void *args)     // FUN_10027c50 @ RVA 0x27c50 (mveh.getTuningPartVarCount)
{
	(void)self;
	int32_t model = 0, index = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "ii:mveh.getTuningPartVarCount",
	                                &model, &index) == 0)
		return MvlPyNoneResult();
	MVL_HELPERS_TODO(FUN_10027c50_body);
	return g_MvlPyApi.Py_BuildValue("i", 0);
}

// 0.98e @ RVA 0x27cb0 (146 B) - mveh.getVehUsedTuningPart("Oi").
void *MvlPyMvehGetVehUsedTuningPart(void *self, void *args)      // FUN_10027cb0 @ RVA 0x27cb0 (mveh.getVehUsedTuningPart)
{
	(void)self;
	void *obj = NULL;
	int32_t index = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "Oi:mveh.getVehUsedTuningPart",
	                                &obj, &index) == 0)
		return MvlPyNoneResult();
	if (MvlPyArgToEntity(obj, '\x01') == 0)
		return NULL;
	MVL_HELPERS_TODO(FUN_10027cb0_body);
	return g_MvlPyApi.Py_BuildValue("i", 0);
}

// 0.98e @ RVA 0x27d50 (136 B) - mveh.setVehUsedTuningPart("Oii").
void *MvlPyMvehSetVehUsedTuningPart(void *self, void *args)      // FUN_10027d50 @ RVA 0x27d50 (mveh.setVehUsedTuningPart)
{
	(void)self;
	void *obj = NULL;
	int32_t index = 0, value = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "Oii:mveh.setVehUsedTuningPart",
	                                &obj, &index, &value) == 0)
		return MvlPyNoneResult();
	if (MvlPyArgToEntity(obj, '\x01') == 0)
		return NULL;
	MVL_HELPERS_TODO(FUN_10027d50_body);
	return MvlPyTrueResult();
}

// 0.98e @ RVA 0x27de0 (378 B) - mveh.getVehModelAIType("i").
void *MvlPyMvehGetVehModelAIType(void *self, void *args)         // FUN_10027de0 @ RVA 0x27de0 (mveh.getVehModelAIType)
{
	(void)self;
	int32_t model = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "i:mveh.getVehModelAIType",
	                                &model) == 0)
		return MvlPyNoneResult();
	MVL_HELPERS_TODO(FUN_10027de0_body);
	return g_MvlPyApi.Py_BuildValue("i", 0);
}

// 0.98e @ RVA 0x27f60 (158 B) - mveh.setCarTaxiLightState("Oi").
void *MvlPyMvehSetCarTaxiLightState(void *self, void *args)      // FUN_10027f60 @ RVA 0x27f60 (mveh.setCarTaxiLightState)
{
	(void)self;
	void *obj = NULL;
	int32_t on = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "Oi:mveh.setCarTaxiLightState",
	                                &obj, &on) == 0)
		return MvlPyNoneResult();
	if (MvlPyArgToEntity(obj, '\x01') == 0)
		return NULL;
	MVL_HELPERS_TODO(FUN_10027f60_body);
	return MvlPyTrueResult();
}

// ===========================================================================
// Section 11: event-handler registry tail + manim module (final three ports).
// Sources: pyaudio/range-decomp.txt MvlPyEventSlotByName @ 0x29660 and
// missing33/decomp-chain.txt MvlPyAddEventHandler / FUN_10024110 (=
// MvlPyManimInitAnimType) / MvlPyAnimTypeToAssoc
// (depth-0 roots + the 0x23db0 depth-1 export). The original event registry
// is an MSVC std::map + per-event std::list pair (DAT_1011c344 map of nodes
// @ DAT_1011c34c, list heads stride 8 @ DAT_1011c43c, sizes stride 8 @
// DAT_1011c440); the flat-table replacement (MvlPyEventTable +
// MvlPyEventHandlers) is the structural decision recorded in this file's
// header comment - byte-identical NON-goal.
// ===========================================================================

// ---------------------------------------------------------------------------
// 0.98e @ RVA 0x29660 (157 B) - event name -> handler-slot index, -1 when
// unknown. Original: strlen + MSVC std::string build + map find
// (FUN_100181c0 @ DAT_1011c344); the index EAX return the decompiler lost is
// read by the caller MvlPyAddEventHandler (iVar3, "if (-1 < iVar3)"). Flat-table
// strcmp scan = the recorded structural decision (same keys/order).
// ---------------------------------------------------------------------------
int32_t MvlPyEventSlotByName(const char *name) // FUN_10029660 @ RVA 0x29660
{
	if (name == NULL)
		return -1;
	for (int32_t i = 0; i < 8; i++) {
		if (MvlPyEventTable[i].name == NULL)
			break;   // NULL sentinel + trailing unused slots
		if (strcmp(MvlPyEventTable[i].name, name) == 0)
			return i;
	}
	return -1;       // map::end (decomp local_28 == DAT_1011c344)
}

// ---------------------------------------------------------------------------
// 0.98e @ RVA 0x290d0 (520 B) - event-handler insert. Original: slot lookup
// via MvlPyEventSlotByName, map-find dedup on the callable pointer (FUN_100061f0),
// ob_refcnt incref, node alloc (FUN_1002a4c0) + list insert (FUN_1002a720)
// + map rebalance (FUN_1002a610), size guard "list<T> too long" (abort via
// FUN_10058e3e, does not return). Flat-table port per the structural
// decision: dedup by scanning handlers[0..count), append at count.
// ---------------------------------------------------------------------------
void MvlPyAddEventHandler(const char *name, void *callable) // FUN_100290d0 @ RVA 0x290d0
{
	int32_t idx = MvlPyEventSlotByName(name);
	if (idx < 0)
		return;                                        // unknown event: silent (original)
	SMvlPyHandlerSlot &slot = MvlPyEventHandlers[idx];

	for (int32_t i = 0; i < slot.count; i++) {
		if (slot.handlers[i] == callable)
			return;                                    // dedup (replaces the map-find)
	}

	// incref FIRST - decomp order: *param_2 += 1 precedes the node machinery
	// (line 1388); the original then aborts at the size guard, so its leaked
	// incref never mattered there.
	static_cast<MvlPyObjectHead *>(callable)->refcnt += 1;

	if (slot.count >= MVL_PY_MAX_HANDLERS) {
		// ORIGINAL: FUN_10058e3e("list<T> too long") - does not return.
		// Clone logs the verbatim message and returns (task-prescribed).
		// BUGFIX(stable-1.0): undo the incref before returning - the original
		// aborted the process here so its unmatched +1 never mattered; the
		// clone returns, and the leaked incref would pin the callable object
		// (refcount never reaches 0 -> tp_dealloc never runs).
		MvlPyDecref(callable);
		MvlErrorLog("list<T> too long");
		return;
	}
	slot.handlers[slot.count++] = callable;
}

// ---------------------------------------------------------------------------
// 0.98e @ RVA 0x2f60 - (below-window) anim-type-id -> anim-association
// object. Body NOT recovered: HOOK POINT seam, honest fail (0 -> the
// caller's "Animation type does not exist." path). Logged once.
// ---------------------------------------------------------------------------
static int32_t MvlAnimAssocLookup(int32_t typeId)
{
	static bool s_logged = false;
	if (!s_logged) {
		s_logged = true;
		MvlErrorLog("script: HOOK POINT 0x2f60 MvlAnimAssocLookup (FUN_10002f60 - below-window, body not recovered)");
	}
	(void)typeId;
	return 0;
}

// ---------------------------------------------------------------------------
// 0.98e @ RVA 0x23db0 - animation-type identifier -> anim-association object.
// Body RECOVERED as a depth-1 export (missing33/decomp-chain.txt:3539-3559;
// the task brief's "below-window, body not recovered" applies to its callee
// FUN_10002f60 @ 0x2f60, seamed above): kind-check via MvlPyIsEntityIdTuple(obj, 3)
// -> PyErr_SetString(PyExc_KeyError, verbatim), tuple field 1 via
// MvlPyIdTupleIndex, lookup, miss -> the second verbatim error.
// ---------------------------------------------------------------------------
int32_t __fastcall MvlPyAnimTypeToAssoc(void *obj) // FUN_10023db0 @ RVA 0x23db0
{
	if ((char)MvlPyIsEntityIdTuple(obj, 3) == 0) {             // decomp: (char)uVar1 == '\0'
		g_MvlPyApi.PyErr_SetString(g_MvlPyApi.PyExc_KeyError,
			"Argument is not an animation type identifier.");
		return 0;
	}
	int32_t assoc = MvlAnimAssocLookup(MvlPyIdTupleIndex(obj));
	if (assoc == 0) {
		g_MvlPyApi.PyErr_SetString(g_MvlPyApi.PyExc_KeyError,
			"Animation type does not exist.");
		return 0;
	}
	return assoc;
}

// ---------------------------------------------------------------------------
// 0.98e @ RVA 0x24110 - manim.initAnimType("Oiii") - writes the three
// anim-type u16 fields (+0x2c / +0x0e / +0x2e) on the association object.
// PyCFunction shape like the other module methods.
// ---------------------------------------------------------------------------
void *MvlPyManimInitAnimType(void *self, void *args)             // FUN_10024110 @ RVA 0x24110 (manim.initAnimType)
{
	(void)self;
	void *obj = NULL;
	int32_t a = 0, b = 0, c = 0;
	if (g_MvlPyApi.PyArg_ParseTuple(args, "Oiii:manim.initAnimType",
	                                &obj, &a, &b, &c) == 0)
		// decomp return 0 (NULL) - NOT MvlPyNoneResult(): PyArg_ParseTuple
		// has already set the TypeError, so NULL propagates it (CPython-
		// correct); every other method returns None here, this one is 1:1.
		return NULL;
	int32_t assoc = MvlPyAnimTypeToAssoc(obj);
	if (assoc == 0)
		return NULL;                                   // decomp return 0 (error already set)
	{
		unsigned char *rec = (unsigned char *)(uintptr_t)assoc;
		// decomp order/widths verbatim (u16 stores):
		*(uint16_t *)(rec + 0x2C) = (uint16_t)a;       // *(u16*)(iVar1 + 0x2c) = local_c[0]
		*(uint16_t *)(rec + 0x0E) = (uint16_t)b;       // *(u16*)(iVar1 + 0x0e) = local_10[0]
		*(uint16_t *)(rec + 0x2E) = (uint16_t)c;       // *(u16*)(iVar1 + 0x2e) = local_14[0]
	}
	// Py_BuildValue(&DAT_1007c575) - the format string @ RVA 0x7c575 is EMPTY
	// (PE-verified) -> returns None.
	return g_MvlPyApi.Py_BuildValue("");
}
