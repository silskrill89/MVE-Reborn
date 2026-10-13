# Renames — src/extras/, src/core/, src/render/ (branch: continued)

Scope: eliminated **10 unique `FUN_<va>` CODE identifiers** (defs + decls + calls)
across the three small modules. The other **30 `FUN_` tokens** in these dirs are
comment-only / string mentions (RVA cites, log-format text, decomp references) and
were **left untouched** per task rule. Cross-module identifiers recently renamed
(`MvlTextGet`, `MvlGetExtraFlags`, `MvlGetModelDirRecord`) were **not** re-touched.
No CRT/mapped identifiers (`memset` / `CrtXxx`, e.g. `FUN_10063900`) were renamed.

Method: per-identifier lexical classifier separated code occurrences from
comment/string occurrences; a word-boundary regex replace (python) rewrote only the
code tokens; each **definition** keeps a trailing `// <oldFUN> @ RVA 0x…` comment
(block comments above defs also still cite the old `FUN_` + RVA). ast_grep does not
match bare identifiers, so python was used (proven).

## Mapping

| old | new | module | kind | RVA cite kept on def |
|---|---|---|---|---|
| `FUN_10013310` | `MvlSecWndProc` | extras | secwnd WNDPROC stub (lpfnWndProc seam, logs-once + returns 0) | `secwnd.cpp:19` |
| `FUN_1003cfe0` | `MvlInitDefaultExtraFlags` | extras | xmlgen lazy-init of default-vehicle extraflags table (DAT_100b2438, ids 130..236) | `xmlgen.cpp:96` |
| `FUN_1003cc70` | `MvlGetBoatEngine` | extras | xmlgen boat-engine settings getter (record copy + per-model defaults) | `xmlgen.cpp:495` |
| `FUN_1006d1e0` | `MvlRadRoundTrip` | extras | xmlgen identity double helper on `<maxlean>/<wheelieang>/<stoppieang>` before 180/pi | `xmlgen.cpp:388` |
| `FUN_10043810` | `MvlInitWireModelRedirects` | core | init patch root: 9 model-redirect-slot immediates | `init.cpp:56` |
| `FUN_10043870` | `MvlInitWireAnimRedirects` | core | init patch root: 3 anim-redirect / aux-slot immediates | `init.cpp:82` |
| `FUN_10043900` | `MvlInitPatchCodeStubs` | core | init patch root: 14 code-stub builds (0x80/byteA/0x7C/byteB + NOPs) | `init.cpp:109` |
| `FUN_10043a10` | `MvlInitRemapImmediates` | core | init patch root: 61 immediate remap + 8 byte patches (0x10→0x7F) | `init.cpp:149` |
| `FUN_10043b30` | `MvlInitRemapClumpVersions` | core | init patch root: 23 RW clump chunk-version remap (0x34003→0x36003) | `init.cpp:183` |
| `FUN_10032f30` | `MvlRwLibIdToVersion` | render | dx_handler static helper: RW library-ID → comparable version word | `dx_handler.cpp:40` |

## Files changed (6)
- `src/extras/secwnd.cpp`, `src/extras/secwnd.h`
- `src/extras/xmlgen.cpp`
- `src/core/init.cpp`, `src/core/init.h`
- `src/render/dx_handler.cpp`

(`src/core/main.cpp`, `src/core/stdafx.h`, `src/extras/xmlgen.h`, `src/render/png_funcs.{h,cpp}`,
`src/render/dx_handler.h` had **no code-identifier `FUN_`** — comment-only mentions remain.)

## Left deliberately
- **30 comment-only `FUN_` mentions** (RVA cites, `// call 0x…` asm notes, log-format strings,
  decomp references) — task rule: leave comment-only.
- **png_funcs internals** (`PngReadCallback`, `RtPNGImageStreamRead`, `LoadPngFuncs`, etc.) were
  already canonical — no `FUN_` code identifiers there.
- **init.cpp seams** `MvlInitPatchDword` / `MvlInitPatchByte` were already canonical.
- **Heli getters** (`HeliGetType` @ 0x3ce30, `HeliGetWeaponType` @ 0x3cea0,
  `HeliGetRotorSettings` @ 0x3cf00) and `SetupBasicHandling` @ 0x3c9b0 already named
  (referenced only in comments here).
- `MvlTextGet` / `MvlGetExtraFlags` / `MvlGetModelDirRecord` — just renamed cross-module, not re-touched.

## Verify results
- Code-identifier `FUN_` remaining in the three dirs: **0**.
- Trailing `// <oldFUN> @ RVA 0x…` comment present on all 10 renamed definitions: **yes**.
- Brace balance `{…}` unchanged per file (all balanced): **yes**.
- Cross-module identifiers (`FUN_1002ebb0`/`FUN_1003d120`/`FUN_1004f3a0`/`FUN_1002d880` from
  `fun_partition.txt`): **not present in these three dirs** (owned by engine/scm lanes).
