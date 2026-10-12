# Port notes — TRANCHE-4 SCM lane (CVehicleSCM / CScmHandler)

**Binary**: `vehmod_retail.dll` 0.98e (RVAs = Ghidra addr − 0x10000000; image base 0x10000000).
**Decomp sources on disk**:
- `.harness-memory/limout/savewpn/range-decomp.txt` + `range-functions.txt` (151 fns, VA 0x50A30–0x58A00)
- `.harness-memory/limout/decomp-chain.txt` (81 fns, init chain)
- `.harness-memory/limout/scmop/decomp-chain.txt` (142 lines; **2026-10-12 lane dump** — IgnoreOpcode 0x234c0 + opcode 0801/0802 bodies 0x23920/0x23940 + depth-1 callees)
- `.serena/memories/re-098e-map.md` §3 SCM census

**Files created (this lane, exclusive scope)**:
| File | Lines |
|---|---|
| `src/scm/vehicle_scm.h` | see `wc -l` in deliverable report |
| `src/scm/vehicle_scm.cpp` | " |
| `src/scm/scm_handler.h` | " |
| `src/scm/scm_handler.cpp` | " |
| `.harness-memory/limout/port-notes-scm.md` | this file |

`src/weapons/` untouched (sibling lane); `src/vehicle/` read-only reference.

---

## 1. Inventory — every function in the gap 0x51720–0x52350

| RVA | Ghidra | Size | Disposition |
|---|---|---|---|
| 0x51720 | FUN_10051720 | 394 | **DECLINED (save lane)** — only caller is `LoadExtraSaveFile` 0x51340 (range-decomp:542); already ported by the vehicle lane as `(provisional) ClearSaveXmlRecords` (vehicle_handler.h:185) |
| 0x518b0 | FUN_100518b0 | 79 | **DECLINED (save lane)** — save-apply helper: copies a 0x50-byte garage record from `&DAT_100b3008 + idx*0x50` (= vehicle lane's `MvlGarageSlotRecord` seam) using globals `DAT_1013dd18/dd1c` in the save-apply block (vehicle_handler.h:83-87); inside their stated range 0x50A30–0x51900 (not yet declared by them — see Open questions) |
| 0x51900 | FUN_10051900 | 60 | **DECLINED (save lane)** — same pattern over `DAT_1013dcec/dcfc`, calls FUN_1003c4a0; vehicle-lane range |
| 0x51cb0 | FUN_10051cb0 | 248 | **PORTED** → `CVehicleSCM::InstallScmHooks` (provisional, MEDIUM): 14 game hooks (FUN_10022560 @ 0x22560) + 4 byte stores + tail-call to opcode-0800 registration |
| 0x51db0 | FUN_10051db0 | 28 | **UNATTRIBUTED — not ported** (open question): cdecl `_vsprintf_s(buf, 0x104, fmt, …)` helper; its only in-range callers are weapon save-XML functions 0x56880 / 0x56aa0 (range-decomp:4966/4997/5061) |
| 0x51dd0 | FUN_10051dd0 | 77 | **UNATTRIBUTED — not ported** (open question): wide-char compare of `DAT_1013dd28` vs `DAT_1008c928`, returns `&DAT_100b77d8` on match else 0; no in-range callers; its global sits in the save-apply block but its return is the SCM text buffer |
| 0x51e20 | FUN_10051e20 | 137 | **PORTED** → `CVehicleSCM::SetNextTextForVehicleId` — RTTI-recovered name (error string inside the call) |
| 0x51eb0 | FUN_10051eb0 | 151 | **PORTED** → `CVehicleSCM::ProcessNextTextOpcode` — RTTI-recovered name |
| 0x51f90 | FUN_10051f90 | 25 | **PORTED (hook point)** → `(provisional) ProcessHookStubTable`, LOW confidence |
| 0x51fb0 | FUN_10051fb0 | 24 | **PORTED** → `(provisional) RegisterNextTextOpcode`, MEDIUM — registers 0800 via `FUN_100238a0(0x800, FUN_10051eb0, 0)` (range-decomp:1188) |
| 0x51fd0 | FUN_10051fd0 | 28 | **PORTED** → `(provisional) FormatNextTextBuffer`, MEDIUM — `_vswprintf_s(dest, 200, fmt, …)`; only caller 0x51e20 |
| 0x51ff0 | FUN_10051ff0 | 335 | **DECLINED (weapon lane)** — thiscall ctor, called from FUN_100550c0 (range-decomp:4297) |
| 0x52140 | thunk_FUN_10053f10 | 5 | **DECLINED (weapon lane)** — dtor thunk |
| 0x52150 | FUN_10052150 | 522 | **DECLINED (weapon lane)** — `CMvlWeapon::LoadFromArchive` body ("Successfully loaded weapon %d from file %s") |
| 0x5235a | Catch@1005235a | 54 | **DECLINED (weapon lane)** — logs "CMvlWeapon::LoadFromArchive" |
| 0x516e0 | FUN_100516e0 | 54 | below window; inside vehicle-lane stated range but **not declared** in vehicle_handler.h (their gap — noted for orchestrator) |

## 2. Opcode registration census (all four sites found on disk)

| Opcode | Handler (orig) | Registered by | Source line |
|---|---|---|---|
| 0x0800 name-draw | `CVehicleSCM::ProcessNextTextOpcode` 0x51eb0 | FUN_10051fb0 @ 0x51fb0 | range-decomp.txt:1188 |
| 0x0801 python-loaded | FUN_10023920 @ 0x23920 (**body PORTED** — scmop/decomp-chain.txt:54-63) | FUN_100238d0 @ 0x238d0 | decomp-chain.txt:1668 |
| 0x0802 version=9850 | FUN_10023940 @ 0x23940 (**body PORTED** — scmop/decomp-chain.txt:66-76) | FUN_100238d0 @ 0x238d0 | decomp-chain.txt:1669 |
| 0x0810 weapon pickup | `CWeaponPickups::ProcessNewWeaponPickupOpcode` 0x584f0 (**src/weapons/ — sibling lane**) | FUN_10058270 @ 0x58270 | range-decomp.txt:6029 |

Registration primitive: `FUN_100238a0(opcode, handler, flags)` @ **0x238a0** — referenced by all four sites, **body not dumped**; clone-side scaffold = `CScmHandler::RegisterOpcode`.
FUN_100238d0 @ 0x238d0 also installs 2 game hooks (0x44fdaf→0x100237f0, 0x45023f→0x10023830) before registering 0801/0802 — not ported (CScmHandler-adjacent, hook-point only if a later lane wants it).

## 3. `CScmHandler::IgnoreOpcode` 0x234c0 — **dump found 2026-10-12, PORTED**

The needed dump arrived as `limout/scmop/decomp-chain.txt` (FUN_100234c0, lines 1-51). Ported 1:1 into `scm_handler.cpp` (`CScmHandler::IgnoreOpcode`, same signature):

- **Semantics**: `__fastcall(this, opcode)` — compare tree over the **11 ignorable game opcodes `0x55, 0x16a, 0x171, 0x1b4, 0x1eb, 0x22a, 0x22b, 0x3cb, 0x4bb, 0x4f9, 0x4fa`**. All three decomp labels (`LAB_100234e1` / `LAB_1002350b` / `LAB_10023519`) do the same — `FUN_1002e4c0(); return 1` — so the port flattens them into one switch case set. Unknown opcode → `FUN_1001cce0("CScmHandler::IgnoreOpcode", "Cannot ignore unknown opcode %04X.", opcode)` → `return 0` (decomp: `uVar2 & 0xffffff00`). Both strings preserved **verbatim**.
- **`FUN_1002e4c0` body now known** (same dump, lines 79-86): a tail-call thunk into `gta-vc.exe!0x451010` → **HOOK POINT** `MvlScmHook_GameCall451010` (VA logged, no call performed). Note `vehicle_scm.cpp` carries the same thunk as provisional `ScmScriptArgPrep` ("opcode-argument context prep") — identity of 0x451010 still unknown either way.
- **`FUN_1001cce0` body now known** (lines 89-108): the warning logger — `_vsprintf` into `local_808[2048]`, append `"Warning in %s: %s\n"` (fn, msg) to the log file `DAT_100b0db0`. Clone = file-local `MvlScmWarn(fn, fmt, ...)` → `MvlErrorLog("Warning in %s: %s", …)`, `vsnprintf` hardening (original `_vsprintf` is unbounded).
- **Param width**: the original takes `this` in ECX (unused) and the opcode as a **32-bit int in EDX**; the clone keeps the established `unsigned short` signature — high words are not modelled (original would reject `0x1_0055`, clone truncates to `0x55`).
- **Return word**: `CONCAT31(...,1)` = `return true`, `... & 0xffffff00` = `return false` — a plain `bool` in the clone (the upper bytes are the untouched EAX of the thunk call, no semantic content).

## 3b. Opcode 0801/0802 handler bodies — **dump found 2026-10-12, PORTED**

| Handler | Body (verbatim decomp shape) | Port |
|---|---|---|
| `FUN_10023920` @ 0x23920 (0801 "python-loaded") | `return FUN_1002e4d0() & 0xffffff00` | file-local `MvlScmHandlePythonLoadedOpcode()` → `MvlScmHook_GameCall463f00()` |
| `FUN_10023940` @ 0x23940 (0802 "version=9850") | `FUN_1002e560(0, 0x267a); return FUN_1002e4b0() & 0xffffff00` | file-local `MvlScmHandleVersionOpcode()` → `MvlScmHook_SetGameDwordSlot(0, 0x267a)` + `MvlScmHook_GameCall450e50()` |

- **`FUN_1002e560` body now known** (lines 121-130): `__fastcall(slot, value)` — `if (slot < 12) dword[0x7d7438 + slot*4] = value`. The bounds check is portable and ported; the **store is an engine write** → HOOK POINT `MvlScmHook_SetGameDwordSlot` logs `gta-vc.exe 0x%08X = 0x%08X` and performs **NO write**. `0x267a == 9850` decimal (the census label "version=9850" — game slot 0 is where the version word goes).
- **`FUN_1002e4d0` / `FUN_1002e4b0` bodies now known** (lines 111-118 / 133-140): tail-call thunks into `gta-vc.exe!0x463f00` / `!0x450e50` → HOOK POINTs `MvlScmHook_GameCall463f00` / `MvlScmHook_GameCall450e50`.
- **Return word**: both handlers return `thunk_ret & 0xffffff00` — low byte always 0. `TOpcodeFn` in the clone is `void (*)(void)` (fixed by the 0800 binding `CVehicleSCM::ProcessNextTextOpcode` = `void(void)` in `vehicle_scm.cpp`, out of scope), so the return word is **documented, not modelled** — see Open questions.

## 4. Stub / hook-point inventory (16 total)

vehicle_scm.cpp (12): `ScmLookupModelSlot` (0x4f3a0 — also exposed cross-lane as vehicle lane's `MvlLookupModelInfoByModelIndex`, EAX half only), `ScmTxdBaseIndex` (gta-vc.exe 0xa06320), `ScmModelInfoForSlot` (gta-vc.exe 0xa06314), `ScmResolveModelName` (0x2ebb0), `ScmScriptArgPrep` (0x2e4c0), `ScmReadScriptArg` (0x2e530, EAX+EDX channels), `ScmScriptVarVehicleId` (0x2d880), `ScmScriptGlobals` (gta-vc.exe 0xa0fde4), `ScmWriteHook` (0x22560), `ScmPatchGameBytes` (0x51cb0 direct stores), `ScmFormatResolvedName` (0x51e20 success path — hidden `%s` operand), `ProcessHookStubTable` body (0x51f90 → 0x22570).
scm_handler.cpp (4, all `MvlScmHook_*` file-local seams): `MvlScmHook_GameCall451010` (0x2e4c0 thunk → gta-vc.exe 0x451010), `MvlScmHook_GameCall463f00` (0x2e4d0 thunk → 0x463f00), `MvlScmHook_GameCall450e50` (0x2e4b0 thunk → 0x450e50), `MvlScmHook_SetGameDwordSlot` (0x2e560 store into gta-vc.exe 0x7d7438[slot]; bounds check ported, write NOT performed). Plus `MvlScmWarn` — a **real port** of the FUN_1001cce0 warning logger, not a stub.
**No behaviour is faked**: every stub logs `MvlErrorLog("… hook point … [VA]")` (stores log the exact VA + intended value) and returns a documented default; the only data written is the verbatim `L"Invalid"` fallback (decomp's own failure path).

## 5. Needed dumps

> 2026-10-12: former items 1-2 (IgnoreOpcode 0x234c0, opcode-0801/0802 bodies) **resolved** by `limout/scmop/decomp-chain.txt` — see §3/§3b. Replacement need below is now #4.

1. **14 MVL hook-stub bodies @ RVA 0x51940–0x51c90** — referenced as jump targets by FUN_10051cb0, but the gap 0x5193c–0x51cb0 contains **no functions in range-functions.txt** (likely a Ghidra function-discovery miss — they are only reached from patched game code).
2. **`FUN_100238a0` @ 0x238a0** (registration primitive) and **`FUN_10022560` / `FUN_10022570` @ 0x22560/0x22570** (hook writer / hook-table callee) — install-vs-restore direction of 0x51f90's loop is unknowable without 0x22570.
3. Vararg recovery for `FUN_10051fd0` calls (Ghidra's 2-param prototype **hides the `%s` name operand** of the 0x51e20 success path) — re-decompile with the prototype marked varargs, or read the pushes at 0x51e63.
4. **Caller trace for `CScmHandler::IgnoreOpcode` (0x234c0)** — who invokes it (the FUN_100238d0-installed hook stubs at 0x237f0/0x23830?), and who consumes the opcode-handler return words (`thunk_ret & 0xffffff00`) — dispatcher VA not in any dump.

## 6. Open questions

- **0x51cb0 site attribution**: the vehicle lane cites it for the save flow ("the original patches at 0x51cb0", vehicle_handler.h:82), this lane owns it because its tail registers opcode 0800. The 14 sites are probably a **mix of SCM and save-flow hooks** — needs mkfuncs-xrefs per-site tracing (no writes to `src/vehicle/` were made).
- **Who calls FUN_10051cb0?** No in-range caller; likely the main init orchestrator `FUN_10043c00` @ 0x43c00 (re-098e-map §2) — unverified. Wiring lane must call `CVehicleSCM::InstallScmHooks()` from plugin init.
- **Who reads `DAT_100b77d8` (the next-text buffer) on the game side?** Presumably one of the 14 patched sites / 19 stub-table entries — undiscovered; clone exposes `CVehicleSCM::NextText()` for the wiring lane.
- **Return-word oddity**: decomp returns `vswprintf count & 0xffffff00` from 0x51e20 (= 0 for any count < 256) — ported verbatim on the failure path; verify against asm (Ghidra artifact?) on the next dump pass.
- **Byte-patch widths** at 0x4bfc70 (`_DAT = 0x9090`: word or dword?) — Ghidra store-width inference, marked UNVERIFIED in `kScmBytePatches`.
- **Retail vs steam**: the 14 site addresses are retail (dump640FE5h); the steam build retargets exe addresses (re-098e-map §4) — table needs a steam variant later.
- **0x51db0 / 0x51dd0 ownership** (see inventory) — caller traces would settle it; declining avoids collisions with either sibling lane.
- **0x518b0 / 0x51900** fall inside the vehicle lane's stated range but are not declared in their header — flagged to the orchestrator (declined here, not implemented).
- `FUN_100238a0` flags argument (always 0 at all 4 sites) — semantics unknown.
- Type-2 check details (`varIdx*0x5dc + globals[0] != 0`, type byte at `globals[1]+varIdx`) ported bit-faithfully but only exercised once the gta-vc.exe seams are wired.
- **gta-vc.exe callee identities** (from the scmop thunk bodies — no labels in any dump): `0x451010` (FUN_1002e4c0 tail-call — the ignore-commit step of IgnoreOpcode + first call of ProcessNextTextOpcode), `0x463f00` (FUN_1002e4d0 tail-call — 0801 handler), `0x450e50` (FUN_1002e4b0 tail-call — 0802 handler). Plugin-sdk/reVC address pass would settle all three.
- **12-slot dword array at gta-vc.exe 0x7d7438** (FUN_1002e560 target): slot 0 := 0x267a (9850) from the 0802 handler. Identity unknown — script globals? version block? TODO(scm) once annotated.
- **Handler return-word semantics**: 0801/0802 return `thunk_ret & 0xffffff00` (always 0 in the low byte). The clone's `TOpcodeFn` is `void (*)(void)` (pinned by the 0800 binding in vehicle_scm.cpp, out of scope here), so the word is documented in comments only; when the dispatcher is ported, the typedef may need to widen to `unsigned int (*)(void)`.
- **IgnoreOpcode param width**: original compares a 32-bit EDX int; clone keeps `unsigned short` per the established signature — high words unmodelled.
