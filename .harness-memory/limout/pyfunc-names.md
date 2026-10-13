# Python-module method names recovered from vehmod_retail.dll 0.98e (PyMethodDef extraction)

**Source**: `original_files/maxovehmod098e/mvl/vehmod_retail.dll` (READ-ONLY; image base 0x10000000).
**Method**: direct PE32 parse (`e_lfanew` → COFF → optional header → section table; pointers in
the file image are ABSOLUTE VAs, rebased via `.reloc`). CPython 3.3 `PyMethodDef` = 16 B
`{ml_name; ml_meth; ml_flags; ml_doc}` null-terminated; rows live in **.data** (e.g. first row
at file off 0xa4648), linked from a `PyModuleDef` (32 B, `m_name@0 / m_size@8=-1 / m_methods@12`).
Cross-checks: `.harness-memory/d3out/strings.txt` parse-string census (`fmt:module.method →
handler RVA`) + `original_files/**/samplescripts/*.py` imports.
**Extractor**: `%TEMP%\opencode\pyfunc_extract.py` → `%TEMP%\opencode\pyfunc_extract.txt` (78 rows,
8 tables, 8 defs). All function citations below: `0.98e @ RVA 0x…`.

## 1. Rename table (applied in `src/script/python_host.{h,cpp}` only)

All 16 ported module methods now carry `MvlPy<Module><Method>` + trailing
`// FUN_<id> @ RVA 0x… (<module.method>)` on both the decl and the def.

| module | method | flags | ml_meth RVA | current FUN_ | new name | evidence |
|---|---|---|---|---|---|---|
| mbase | logPrint | 0x1 VARARGS | 0x25280 | FUN_10025280 | **MvlPyMbaseLogPrint** | PyMethodDef row table@0xa6648; def m_name "mbase" @0xa65f4; parse label `s:mvlbase.logPrint` (stale); sample scripts `mbase.logPrint(...)` |
| mbase | registerCallback | 0x1 | 0x252d0 | FUN_100252d0 | **MvlPyMbaseRegisterCallback** | row table@0xa6648 (ml_name was UNKNOWN before — mvlbase event-handler bind) |
| mbase | setHudMessage | 0x1 | 0x254b0 | FUN_100254b0 | **MvlPyMbaseSetHudMessage** | row table@0xa6648; parse label `is:mvlbase.setHudMessage` (stale); scripts `mbase.setHudMessage(...)` |
| mped | getPedVehicle | 0x1 | 0x26990 | FUN_10026990 | **MvlPyMpedGetPedVehicle** | row table@0xa6748; parse `O:mped.getPedVehicle` |
| mveh | createVehicle | 0x1 | 0x27a70 | FUN_10027a70 | **MvlPyMvehCreateVehicle** | row table@0xa6998; parse `i(fff)(fff)i:mveh.createVehicle` |
| mveh | getTuningPartCount | 0x1 | 0x27b90 | FUN_10027b90 | **MvlPyMvehGetTuningPartCount** | row + parse `i:mveh.getTuningPartCount` |
| mveh | getTuningPartName | 0x1 | 0x27bf0 | FUN_10027bf0 | **MvlPyMvehGetTuningPartName** | row + parse `ii:…` |
| mveh | getTuningPartVarCount | 0x1 | 0x27c50 | FUN_10027c50 | **MvlPyMvehGetTuningPartVarCount** | row + parse `ii:…` |
| mveh | getVehUsedTuningPart | 0x1 | 0x27cb0 | FUN_10027cb0 | **MvlPyMvehGetVehUsedTuningPart** | row + parse `Oi:…` |
| mveh | setVehUsedTuningPart | 0x1 | 0x27d50 | FUN_10027d50 | **MvlPyMvehSetVehUsedTuningPart** | row + parse `Oii:…` |
| mveh | getVehModelAIType | 0x1 | 0x27de0 | FUN_10027de0 | **MvlPyMvehGetVehModelAIType** | row + parse `i:…` |
| mveh | setCarTaxiLightState | 0x1 | 0x27f60 | FUN_10027f60 | **MvlPyMvehSetCarTaxiLightState** | row + parse `Oi:…` |
| mveh | getVehicleHealth | 0x1 | 0x28000 | FUN_10028000 | **MvlPyMvehGetVehicleHealth** | row + parse `O:…` |
| mveh | setVehicleHealth | 0x1 | 0x28080 | FUN_10028080 | **MvlPyMvehSetVehicleHealth** | row + parse `Of:…` |
| mveh | blowVehicleUp | 0x1 | 0x28110 | FUN_10028110 | **MvlPyMvehBlowVehicleUp** | row + parse `OO:…` |
| manim | initAnimType | 0x1 | 0x24110 | FUN_10024110 | **MvlPyManimInitAnimType** | row table@0xa6420; parse `Oiii:manim.initAnimType` |

## 2. Full PyMethodDef census (78 rows / 8 tables; flags column: every row = 0x1 METH_VARARGS)

| table@RVA | module (def m_name) | method | ml_meth RVA | clone status |
|---|---|---|---|---|
| 0xa6420 | manim | loadAnimationFile | 0x24020 | not ported |
| 0xa6420 | manim | unloadAnimationFile | 0x240d0 | not ported |
| 0xa6420 | manim | initAnimType | 0x24110 | **renamed MvlPyManimInitAnimType** |
| 0xa6420 | manim | getStockAnimation | 0x24180 | not ported (no parse string) |
| 0xa6420 | manim | addPedAnimation | 0x24330 | not ported |
| 0xa6420 | manim | blendPedAnimation | 0x243e0 | not ported |
| 0xa6420 | manim | getAllPedAnimations | 0x244a0 | not ported |
| 0xa6420 | manim | findPedAnimation | 0x24560 | not ported — census parse label says "getAllPedAnimations" (original's stale label) |
| 0xa6420 | manim | getAnimPed | 0x24620 | not ported |
| 0xa6420 | manim | getAnimIndex | 0x24690 | not ported |
| 0xa6420 | manim | getAnimGroupIndex | 0x246e0 | not ported |
| 0xa6420 | manim | getAnimName | 0x24730 | not ported |
| 0xa6420 | manim | getAnimFlags | 0x24780 | not ported |
| 0xa6420 | manim | getAnimLength | 0x247d0 | not ported |
| 0xa6420 | manim | getAnimProgress | 0x24830 | not ported |
| 0xa6420 | manim | getAnimSpeed | 0x24890 | not ported |
| 0xa6420 | manim | getAnimBlendProgress | 0x248f0 | not ported — census labels 0x248f0 "getAnimSpeed" (stale) |
| 0xa6420 | manim | getAnimBlendSpeed | 0x24950 | not ported |
| 0xa6420 | manim | getAnimCallback | 0x249b0 | not ported |
| 0xa6420 | manim | setAnimCallback | 0x24a40 | not ported |
| 0xa6420 | manim | setAnimFlags | 0x24bc0 | not ported |
| 0xa6420 | manim | setAnimProgress | 0x24c20 | not ported |
| 0xa6420 | manim | setAnimSpeed | 0x24c80 | not ported |
| 0xa6420 | manim | setAnimBlendProgress | 0x24ce0 | not ported |
| 0xa6420 | manim | setAnimBlendSpeed | 0x24d40 | not ported |
| 0xa6420 | manim | removeAnim | 0x24da0 | not ported |
| 0xa6420 | manim | invokeAnimCallback | 0x24df0 | not ported |
| 0xa6648 | mbase | logPrint | 0x25280 | **renamed MvlPyMbaseLogPrint** |
| 0xa6648 | mbase | registerCallback | 0x252d0 | **renamed MvlPyMbaseRegisterCallback** |
| 0xa6648 | mbase | getFrameCount | 0x253c0 | not ported (no parse string) |
| 0xa6648 | mbase | checkKeysPressed | 0x253e0 | not ported (parse `O:mbase.checkKeysPressed`) |
| 0xa6648 | mbase | setHudMessage | 0x254b0 | **renamed MvlPyMbaseSetHudMessage** |
| 0xa66a8 | ment | getEntModelIndex | 0x25c50 | not ported |
| 0xa66a8 | ment | getEntPosition | 0x25ca0 | not ported |
| 0xa66a8 | ment | setEntPosition | 0x25d20 | not ported |
| 0xa66a8 | ment | getEntSpeed | 0x25e00 | not ported |
| 0xa66a8 | ment | setEntSpeed | 0x25eb0 | not ported (parse label "setEntPosition" stale) |
| 0xa66a8 | ment | getEntType | 0x25f40 | not ported |
| 0xa66a8 | ment | findEntitiesInRange | 0x26010 | not ported |
| 0xa66a8 | ment | getAllEntities | 0x26460 | not ported — shares ml_meth with next row |
| 0xa66a8 | ment | getEntFromScmPoolId | 0x26460 | not ported — same handler as getAllEntities (0x26460) |
| 0xa6748 | mped | getPlayerPed | 0x26980 | not ported |
| 0xa6748 | mped | getPedVehicle | 0x26990 | **renamed MvlPyMpedGetPedVehicle** |
| 0xa6748 | mped | getPedHealth | 0x26a30 | not ported |
| 0xa6748 | mped | setPedHealth | 0x26ab0 | not ported |
| 0xa6748 | mped | getPedArmour | 0x26b40 | not ported |
| 0xa6748 | mped | setPedArmour | 0x26bc0 | not ported |
| 0xa6748 | mped | givePedWeapon | 0x26c50 | not ported |
| 0xa6748 | mped | getPedCurrentWeapon | 0x26d00 | not ported |
| 0xa6748 | mped | getPedCurrentTotalAmmo | 0x26d80 | not ported |
| 0xa6748 | mped | getPedCurrentLoadedAmmo | 0x26e00 | not ported |
| 0xa6748 | mped | getCurrentWeaponSlot | 0x26e80 | not ported |
| 0xa6748 | mped | setCurrentWeaponSlot | 0x26f00 | not ported (parse label "getCurrentWeaponSlot" stale) |
| 0xa6880 | mrw | getEntityObject | 0x27080 | not ported |
| 0xa6880 | mrw | getAtomicFrame | 0x27120 | not ported |
| 0xa6880 | mrw | getClumpFrame | 0x27190 | not ported |
| 0xa6880 | mrw | getFrameName | 0x27200 | not ported |
| 0xa6880 | mrw | getFrameRelativeMatrix | 0x27250 | not ported |
| 0xa6880 | mrw | getFrameAbsoluteMatrix | 0x272a0 | not ported |
| 0xa6880 | mrw | getFrameFirstChild | 0x272f0 | not ported |
| 0xa6900 | mscm | registerHandler | 0x278a0 | not ported (parse `iOs:mscm.registerHandler`) |
| 0xa6900 | mscm | scriptWait | 0x27a10 | not ported (no parse string) |
| 0xa6998 | mveh | createVehicle | 0x27a70 | **renamed MvlPyMvehCreateVehicle** |
| 0xa6998 | mveh | getTuningPartCount | 0x27b90 | **renamed MvlPyMvehGetTuningPartCount** |
| 0xa6998 | mveh | getTuningPartName | 0x27bf0 | **renamed MvlPyMvehGetTuningPartName** |
| 0xa6998 | mveh | getTuningPartVarCount | 0x27c50 | **renamed MvlPyMvehGetTuningPartVarCount** |
| 0xa6998 | mveh | getVehUsedTuningPart | 0x27cb0 | **renamed MvlPyMvehGetVehUsedTuningPart** |
| 0xa6998 | mveh | setVehUsedTuningPart | 0x27d50 | **renamed MvlPyMvehSetVehUsedTuningPart** |
| 0xa6998 | mveh | getVehModelAIType | 0x27de0 | **renamed MvlPyMvehGetVehModelAIType** |
| 0xa6998 | mveh | setCarTaxiLightState | 0x27f60 | **renamed MvlPyMvehSetCarTaxiLightState** |
| 0xa6998 | mveh | getVehicleHealth | 0x28000 | **renamed MvlPyMvehGetVehicleHealth** |
| 0xa6998 | mveh | setVehicleHealth | 0x28080 | **renamed MvlPyMvehSetVehicleHealth** |
| 0xa6998 | mveh | blowVehicleUp | 0x28110 | **renamed MvlPyMvehBlowVehicleUp** |
| 0xa6a90 | mworld | getWeather | 0x28200 | not ported |
| 0xa6a90 | mworld | getTime | 0x28220 | not ported |
| 0xa6a90 | mworld | rayTrace | 0x28240 | not ported (parse `(fff)(fff)Os:mworld.rayTrace`) |
| 0xa6a90 | mworld | startFireOnEntity | 0x28560 | not ported |
| 0xa6a90 | mworld | createExplosion | 0x285e0 | not ported |

## 3. Module census: PyModuleDef ↔ AppendInittab wrapper ↔ init fn

Wrapper bytes decode as `push <init fn>; push dword ptr [<PyModuleDef m_name>];
call [PyImport_AppendInittab @ 0xb170c]; add esp,8; ret` (21 B) — so the inittab
name IS the def's m_name, and the wrapper↔def link is the imm32 def reference.

| wrapper 0.98e @ RVA | PyModuleDef @ RVA | inittab name (real) | init fn 0.98e @ RVA | clone status |
|---|---|---|---|---|
| FUN_10023d10 @ 0x23d10 | 0xa6400 | manim | 0x23d30 | HOOK POINT (label "manim" correct; init fn cite added) |
| FUN_10024fd0 @ 0x24fd0 | 0xa65f4 | **mbase** | 0x24ff0 | HOOK POINT (label was "mvlbase" — fixed; init fn cite added) |
| FUN_10025a30 @ 0x25a30 | 0xa6628 | **ment** | 0x25a50 | ported; literal was "mvlbase" — **fixed to "ment"** |
| FUN_10026940 @ 0x26940 | 0xa682c | mped | 0x26960 | ported; name was already correct |
| FUN_10026f80 @ 0x26f80 | 0xa6860 | **mrw** | 0x26fa0 | ported; literal was "ment" — **fixed to "mrw"** |
| FUN_10027370 @ 0x27370 | 0xa6944 | **mscm** | 0x27390 | ported; literal was "mrw" — **fixed to "mscm"** |
| FUN_10027a30 @ 0x27a30 | 0xa6978 | mveh | 0x27a50 | ported; correct |
| FUN_100281c0 @ 0x281c0 | 0xa6a6c | mworld | 0x281e0 | ported; correct |

**8 modules exist — not 9.** There is NO `mvlbase` anywhere in the DLL except the two stale
PyArg labels (`s:mvlbase.logPrint`, `is:mvlbase.setHudMessage`): no standalone string, no
PyModuleDef, and sample scripts import `mbase` (and call `mbase.logPrint` /
`mbase.setHudMessage`). The prior clone labels/comments claiming "mvlbase" (and the shifted
ment/mrw/mscm inittab literals) were corrected in this pass.

## 4. Internal helpers — names kept as FUN_ (checked, no evidence)

Grep of `missing33/decomp-chain.txt` + `pyaudio/range-decomp.txt` logger calls
(`FUN_1001cb80/1cce0/1cc00`) for `FUN_10029660 / FUN_100290d0 / FUN_10025010 / FUN_10025060 /
FUN_10023db0 / FUN_10029580 / FUN_10029ff0 / FUN_1002a0a0 / FUN_1002a1d0`: **zero name literals**
→ per naming policy these keep their FUN_ ids (Ghidra placeholders, RVA-cited):
FUN_10029660 (0x29660), FUN_100290d0 (0x290d0), FUN_10023db0 (0x23db0), FUN_10025010,
FUN_10025100, FUN_100250e0, FUN_10025160, FUN_10025060, FUN_10025a70, FUN_10025b00,
FUN_10025ba0, FUN_100287c0, FUN_10028cd0, FUN_10029310, FUN_10029580, FUN_1002a170,
FUN_1002a1d0, FUN_10029ff0, FUN_1002a0a0, FUN_10024fd0, FUN_10023d10, FUN_10025a30,
FUN_10026940, FUN_10026f80, FUN_10027370, FUN_10027a30, FUN_100281c0 (inittab wrappers),
plus the `MVL_HELPERS_TODO(FUN_1002…_body)` macro tags (kept: grep-able RVA markers).

## 5. Discrepancies found (table beats labels, always)

- `mbase.*` vs parse-label `mvlbase.*` → module is **mbase** (def + scripts).
- 0x24560 = `findPedAnimation` (parse label says getAllPedAnimations — original's copy-paste).
- 0x248f0 = `getAnimBlendProgress` (census labels it getAnimSpeed — original's copy-paste).
- 0x25eb0 = `setEntSpeed` (parse label says setEntPosition).
- 0x26460 serves **two** rows: `getAllEntities` + `getEntFromScmPoolId`.
- 0x26f00 = `setCurrentWeaponSlot` (parse label says getCurrentWeaponSlot).
- Inittab literals ment/mrw/mscm were shifted by one in the clone — corrected (see §3).
- Old init-fn prologue guesses from a prior def-ref scan were superseded by the wrapper
  immediates in §3 (those are the authoritative LAB labels: 0x24ff0/0x23d30/0x25a50/0x26960/
  0x26fa0/0x27390/0x27a50/0x281e0).
