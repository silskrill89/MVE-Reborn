# Port notes — MVL 0.98e hook helpers + directory manager

Ports: `src/engine/model_helpers.{h,cpp}`, `src/engine/dirmgr.{h,cpp}`.
Sources: `.harness-memory/limout/helpers/decomp-chain.txt` (54 fns, 12 depth-0
roots) and `.harness-memory/limout/dirmgr/range-{functions,decomp}.txt` (3 fns).
All addresses are vehmod_retail.dll **0.98e** (image base 0x10000000); game
addresses are VC 1.0 US VAs (0x00xxxxxx), DLL-internal are 0x100xxxxx.

## 1. Function inventory (name → RVA → status)

### Depth-0 roots — model_helpers.cpp (all exported, declared in model_helpers.h)

| Name | RVA | Status |
|------|-----|--------|
| FUN_1001b610 | 0x1b610 | ported (calls stubbed FUN_1001b640/FUN_1001b5b0 chain) |
| FUN_1003a450 | 0x3a450 | ported (FUN_1002f890 ECX-arg guessed, TODO) |
| FUN_1003a810 | 0x3a810 | ported (IsBadReadPtr-guarded teardown; EAX 1/0 semantics kept) |
| FUN_1003af20 | 0x3af20 | ported; 3 decompiler losses marked TODO (entity base, record stride 0x24 vs +0x74 fields, frame+0x10 double-store) |
| FUN_1003b580 | 0x3b580 | ported (byte patch VC 0x571820 after MvlUnlockGameText) |
| FUN_1003b5d0 | 0x3b5d0 | ported (fastcall, Ghidra frame layout preserved byte-accurate) |
| FUN_1003b840 | 0x3b840 | ported (16-byte paint record stride marked TODO) |
| FUN_1003b9d0 | 0x3b9d0 | ported (lost-EDX variant byte = fresh rand, TODO) |
| FUN_1003bae0 | 0x3bae0 | ported (DAT_100a6314 lookup skipped while NULL, see Q1) |
| FUN_1003de20 | 0x3de20 | ported (snprintf "vc%u" into DAT_1013c290) |
| FUN_1004f7e0 | 0x4f7e0 | ported (fastcall; DAT_100b2804/DAT_100b51d8 sizes provisional) |
| FUN_10017820 | 0x17820 | ported (driver; delegates to CModelIndexLimit::* + FUN_10017940 + FUN_100180a0 + FUN_10017290) |

### Depth-1 ports — model_helpers.cpp (file-local)

| Name | RVA | Status |
|------|-----|--------|
| FUN_1004f400 | 0x4f400 | ported (parts accessor, *(e+0x1D4)) |
| FUN_1002fb00 | 0x2fb00 | ported |
| FUN_10031ca0 | 0x31ca0 | ported (part→frame, *(p+4)) |
| FUN_100080f0 | 0x80f0 | ported (quat identity) |
| FUN_10008c90 | 0x8c90 | ported (CVector copy; thiscall→fastcall+edx) |
| FUN_10008990 | 0x8990 | ported (quat→matrix 3x3; thiscall→fastcall+edx) |
| FUN_1004f3a0 | 0x4f3a0 | ported (calls stubbed FUN_1004f380 → returns NULL until wired) |
| FUN_1003cfc0 | 0x3cfc0 | ported |
| FUN_10059f55 | 0x59f55 | ported as process-wide LCG (original: per-thread _holdrand) |
| FUN_1003a410 | 0x3a410 | ported |
| FUN_1003a350 | 0x3a350 | ported (0x58 alloc via malloc stand-in; 3 .text bytes zero/restore) |
| FUN_1002f890 | 0x2f890 | ported wrapper (both sub-inits stubbed) |
| FUN_1002e8e0 | 0x2e8e0 | ported wrapper (FUN_1002e8d0 stubbed) |
| FUN_10008d90 / 8e30 / 8d10 | 0x8d90/8e30/8d10 | ported shells (leaf CVector ops 0x8f50/8fc0/8ee0 stubbed) |
| FUN_1001b5b0 | 0x1b5b0 | ported (depends on stub chain → inert until wired) |
| FUN_1003a900 | 0x3a900 | ported (effect slotter; FUN_1002e750 stub returns 0 → early-out) |
| FUN_10017290 | 0x17290 | ported (anim pool grow; own DAT_100a6318/DAT_100b0c41 copies, Q1) |
| FUN_10017940 | 0x17940 | ported; byte/word patches applied, 5 E9-JMPs NOT installed (handlers 0x17890..0x17920 unported, TODO) |

### Depth-1 stubs — model_helpers.cpp, each marked `// TODO(helpers): <VA> unported`, log-once via MvlErrorLog, decomp-default return

FUN_1001af70(0), FUN_1001b280(noop), **FUN_1001b640**(implicit-XMM loss),
**FUN_1002d880**(0), FUN_1003d120(0), FUN_1004f380(0), thunk_FUN_1002faa0(noop),
FUN_10059d7f(malloc stand-in), FUN_1002f690(pass-through), FUN_1002f8b0(noop),
FUN_1002f8a0/FUN_1002f880(noop), FUN_1002e8d0(noop),
RW thunk family FUN_10031ec0/31da0/31d90/31ee0/31d40/32140/92f0/9300/80d0 (VC
0x64xxxx tail calls), **FUN_10031380**(returns 1), **FUN_1003ab00**(FPU loss),
FUN_1002dff0(0), CVector leaves FUN_10008f50/8fc0/8ee0/8590/8ce0/9060/9190/
9330/8c30/8c50/8250, effect family FUN_1002e750/2e780/2e170/b170, render
family FUN_10037a60/b130/b120/b070/b090/b080/2dfd0/6cc5a/6d730/6e460/6ccc0/
2e340/9380/9490/9480/2ec60, attachment family FUN_1001b100/1b240/1af90/1b080/
1b1a0, part-frame family FUN_10032bf0/32b70/32300/32810/32b30/31cc0.
Loggers FUN_1001cb80 / FUN_1001cce0 are ported (printf-shape into MvlErrorLog).

### Directory manager — dirmgr.cpp (exported, declared in dirmgr.h)

| Name | RVA | Status |
|------|-----|--------|
| FUN_10017f60 | 0x17f60 | delegates to CModelIndexLimit::VerifyModelHasColModel (identical port in model_id.cpp; single-owner for DAT_100a6314) |
| FUN_10018030 | 0x18030 | delegates to CModelIndexLimit::VerifyDirectoryCanAdd (same reasoning) |
| FUN_100180a0 | 0x180a0 | ported guard+flag; 9 E9-JMPs NOT installed (handlers 0x17db0..0x18060 unported, TODO) |

## 2. Calling conventions used

- MSVC x86, **C3865 respected**: no `__thiscall` on any free function.
  Ghidra `__thiscall` reads are lowered to `__fastcall` with a dummy
  `void* /*edx*/` 2nd param (this→ECX): FUN_10008c90, FUN_10008990, FUN_10008d90,
  FUN_10008e30, FUN_10008d10 and the leaf stubs of that family.
- Ghidra `__fastcall` kept verbatim: FUN_1003b5d0 (3 params: 2 regs + 1 stack),
  FUN_1003b840 (4 params: ECX/EDX + 2 stack), FUN_1004f7e0, plus all
  single/double-param depth-1 helpers.
- Plain cdecl roots unchanged: FUN_1001b610, FUN_1003a450, FUN_1003a810,
  FUN_1003af20, FUN_1003b580, FUN_1003b9d0, FUN_1003bae0, FUN_1003de20,
  FUN_10017820, FUN_10017290, FUN_10017940, FUN_1002d880, FUN_10059f55.
- `game_hooks.cpp` keeps its own **file-local static** stubs with the same
  FUN_ names (separate TUs → no ODR/link clash). When the orchestrator rewires
  the hook handlers, delete those statics and call through `model_helpers.h`.
  Note game_hooks.cpp declares FUN_1003de20 as `uint32_t(void)` and
  FUN_1003b840/FUN_1004f7e0 without `__fastcall` — its decls must be aligned
  with model_helpers.h at wiring time (pointers vs ints, ECX/EDX placement).

## 3. Layout assumptions

- Part records in FUN_1003af20: base = parts + i*0x24 (literal decomp) with
  fields up to +0x74 — internally inconsistent in the decomp; transcribed
  literally + TODO. Verify against disasm before enabling the hook.
- FUN_1003af20 entity base: decomp lost the ECX/EDX arg to FUN_1004f400;
  DAT_100b2450 ("active entity temp") used for both the parts lookup and the
  +0x1A8 speed-object reads (TODO at use site).
- FUN_1003b840 paint override record: 16 bytes/entry (4 colours) inferred from
  the garbled stride arithmetic (TODO).
- FUN_1003b5d0: Ghidra stack frame reproduced byte-accurate in a 0x60 buffer
  (local_60 @+0x00, out @+0x0C, int copy @+0x18).
- FUN_1004f7e0: DAT_100b2804[16] and DAT_100b51d8[16*0x61] are **provisional**
  sizes (original array extents not dumped); slot index assumed < 16.
- VC .text byte/word writes (0x571820, 0x63C6F2/F71D/CE27, 0x48DE71…,
  0x629FCC…, 0x62A038…) call `MvlUnlockGameText()` first, per model_id.h
  invariant.
- FUN_10008e30 (thiscall→fastcall) is used with `this` sometimes pointing at
  plain float buffers (transient vectors) — original did the same (copied
  12 bytes to a local first); ports keep the copy-then-operate shape.

## 4. Open questions

- **Q1 — shared-global fragmentation (needs orchestrator decision).** The
  original has ONE copy of DAT_100b2450 / DAT_100b641e / DAT_100a6318 /
  DAT_100b0c41 / DAT_100a6314; the clone now has per-TU statics in
  game_hooks.cpp, model_id.cpp and model_helpers.cpp. Consequences:
  (a) `MvlSetLongModelIdsEnabled()` (model_id.cpp) doesn't reach
  FUN_10017290's DAT_100b0c41 gate; (b) FUN_1003bae0's DAT_100a6314 lookup is
  inert (NULL-guarded); (c) FUN_10017290's anim count diverges from
  CModelIndexLimit's DAT_100a6318. Recommended fix: an `engine_globals.h/.cpp`
  TU owning the plugin .data block, with all three TUs including it.
- **Q2** — FUN_10059f55 (rand): original is per-thread CRT state; clone uses a
  process-wide LCG. Irrelevant until two threads call FUN_1003b9d0/3bae0.
- **Q3** — unported handler bodies needed to finish two installers:
  col/anim JMP targets 0x17890/0x178b0/0x178d0/0x178f0/0x17920 (used by
  FUN_10017940) and directory-hook handlers 0x17db0..0x18060 (used by
  FUN_100180a0). Both dumps were not in limout; request a Ghidra export of
  RVA 0x17890..0x180A0 minus the three ported fns.
- **Q4** — FUN_1001b640 and FUN_1003ab00 carry implicit XMM register inputs in
  the decomp (`in_XMM0_Da` etc.); a raw disassembly pass is required before
  they can be ported 1:1. Stubs keep their callers linkable meanwhile.
- **Q5** — FUN_1003a810 frees the parts block with the plugin CRT heap
  (originally `__free_base` on DAT_100b0790); the clone uses free() from the
  static CRT — consistent with model_id.cpp, but the alloc side of that block
  (not in this chain) must use the same heap.

## 5. Evidence

See the orchestrator report for `ls -la src/engine/` and
`wc -l src/engine/model_helpers.cpp src/engine/dirmgr.cpp` output captured at
port completion (2026-10-11).
