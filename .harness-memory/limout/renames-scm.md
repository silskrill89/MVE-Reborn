# Renames — SCM lane (`src/scm/`) FUN_ sweep

**Scope**: `src/scm/vehicle_scm.{h,cpp}`, `src/scm/scm_handler.{h,cpp}` only.
**Branch**: `continued` (no git operations performed).
**Method**: python word-boundary replace, line-targeted (comments untouched).
**Partition source**: `%TEMP%/opencode/fun_partition.txt` → `## scm (11)`.

## 0. Finding: defs/decls/calls were already canonical

A comment+string-stripping scan of `src/scm/` found **zero** `FUN_*` used as a
code identifier (no definition, declaration or call). Prior passes already
carried the ports:

| original | clone symbol (already in place) | site |
|---|---|---|
| `FUN_1001cce0` 0x1cce0 | `MvlScmWarn` (real port) | scm_handler.cpp:28 |
| `FUN_1001cb80` 0x1cb80 | `ScmErr` | vehicle_scm.cpp:29 |
| `FUN_1002e4c0` 0x2e4c0 | `MvlScmHook_GameCall451010` / `ScmScriptArgPrep` | scm_handler.cpp:53 / vehicle_scm.cpp:89 |
| `FUN_1002e4d0` 0x2e4d0 | `MvlScmHook_GameCall463f00` | scm_handler.cpp:62 |
| `FUN_1002e4b0` 0x2e4b0 | `MvlScmHook_GameCall450e50` | scm_handler.cpp:71 |
| `FUN_1002e560` 0x2e560 | `MvlScmHook_SetGameDwordSlot` | scm_handler.cpp:84 |
| `FUN_1002e530` 0x2e530 | `ScmReadScriptArg` | vehicle_scm.cpp:100 |
| `FUN_10023920/10023940` | `MvlScmHandlePythonLoadedOpcode` / `MvlScmHandleVersionOpcode` | scm_handler.cpp:211/225 |
| `FUN_100234c0` 0x234c0 | `CScmHandler::IgnoreOpcode` (RTTI) | scm_handler.cpp:179 |
| `FUN_10051cb0` 0x51cb0 | `CVehicleSCM::InstallScmHooks` (provisional) | vehicle_scm.cpp:321 |
| `FUN_10051e20` 0x51e20 | `CVehicleSCM::SetNextTextForVehicleId` (RTTI) | vehicle_scm.cpp:161 |
| `FUN_10051eb0` 0x51eb0 | `CVehicleSCM::ProcessNextTextOpcode` (RTTI) | vehicle_scm.cpp:201 |
| `FUN_10051f90` 0x51f90 | `CVehicleSCM::ProcessHookStubTable` (provisional) | vehicle_scm.cpp:261 |
| `FUN_10051fb0` 0x51fb0 | `CVehicleSCM::RegisterNextTextOpcode` (provisional) | vehicle_scm.cpp:276 |
| `FUN_10051fd0` 0x51fd0 | `CVehicleSCM::FormatNextTextBuffer` (provisional) | vehicle_scm.cpp:245 |

`MvlScmWarn` was **not** renamed (canonical per instruction).

## 1. What was renamed this pass — string literals inside code

The 11 `## scm (11)` identifiers had *no* code-identifier occurrences left; they
survived only inside **string literals on code lines** (a comment-stripping grep
flags those as "code"). Each was replaced with the semantic clone name where a
port exists, or with a VA-only cite where no port exists; in every case the
original Ghidra name + RVA was moved to a trailing `// <oldFUN> @ 0x…` comment.

| # | old (string content) | new | file:line |
|---|---|---|---|
| 1 | `FUN_1002e4c0` | `MvlScmHook_GameCall451010` | scm_handler.cpp:55 |
| 2 | `FUN_1002e4d0` | `MvlScmHook_GameCall463f00` | scm_handler.cpp:64 |
| 3 | `FUN_1002e4b0` | `MvlScmHook_GameCall450e50` | scm_handler.cpp:73 |
| 4 | `FUN_1002e560` | `MvlScmHook_SetGameDwordSlot` | scm_handler.cpp:88 |
| 5 | `FUN_10051fb0` | `CVehicleSCM::RegisterNextTextOpcode` | scm_handler.cpp:109 |
| 6 | `FUN_100238a0` | `CScmHandler::RegisterOpcode` (clone-side scaffold, port-notes-scm.md §2) | scm_handler.cpp:109 |
| 7 | `FUN_100238d0` | *(dropped — no port; VA cite kept)* | scm_handler.cpp:111,113 |
| 8 | `FUN_10058270` | *(dropped — no port; VA cite kept)* | scm_handler.cpp:115 |
| 9 | `FUN_1002e4c0` | `ScmScriptArgPrep` | vehicle_scm.cpp:91 |
| 10 | `FUN_1002e530` | `ScmReadScriptArg` | vehicle_scm.cpp:102 |
| 11 | `FUN_10051fd0` | `CVehicleSCM::FormatNextTextBuffer` | vehicle_scm.cpp:152 |
| 12 | `FUN_10022570` | *(dropped — no port; replaced by "hook-stub callee", VA cite kept)* | vehicle_scm.cpp:268 |

**Not renamed (deliberate)**

- All `//`-comment mentions (leading and trailing) — per instruction.
- `MvlTextGet` / `MvlGetModelDirRecord` / `MvlGetActiveModelIndex` strings in
  vehicle_scm.cpp (just renamed cross-module).
- `FUN_10051db0` (SaveSprintf) and `FUN_10023d10`: **absent** from `src/scm/`
  (`FUN_10051db0` is not in the `## scm (11)` list either; `FUN_10023d10` is in
  `## script` = `src/script/python_host.cpp`).

## 2. Cross-lane name divergence (informational, out of scope)

`src/weapons/weapon_pickups.{h,cpp}` uses its own seam names for four of the
same originals: `0x2e4c0 → MvlScmOpcodeBegin`, `0x2e530 → MvlScmOpcodeReadInt`,
`0x2e560 → MvlScmOpcodeWriteInt`, `0x2e4b0 → MvlScmOpcodeEnd`,
`0x238a0 → MvlScmRegisterOpcode` (all `TODO(weap)` stubs). The SCM lane keeps
its own names (`ScmScriptArgPrep`, `ScmReadScriptArg`, `MvlScmHook_*`,
`CScmHandler::RegisterOpcode`) — documented in port-notes-scm.md §4. Unifying
them is an orchestrator decision, not part of this sweep.

## 3. Verification

- `FUN_` outside `//` comments (strings counted as code): **0 hits** in `src/scm/`.
- `FUN_` outside comments *and* strings: **0 hits**.
- All remaining `FUN_` mentions (85) are comment-only, RVA cites intact on
  every definition (`// FUN_… @ RVA 0x…`).
- Line counts unchanged: scm_handler.cpp 233, scm_handler.h 88,
  vehicle_scm.cpp 339, vehicle_scm.h 87. Brace balance unchanged per file
  (23/23, 2/2, 57/57, 1/1) — no brace-bearing text was added or removed.
- Files changed: `src/scm/scm_handler.cpp`, `src/scm/vehicle_scm.cpp` only
  (14 line-edits). Headers untouched. No builds, no git.
