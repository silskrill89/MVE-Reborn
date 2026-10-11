# port-notes-engine — src/engine port (MVL 0.98e → clone)

Source inputs: `.harness-memory/limout/decomp-chain.txt` (depth=0 sections), `.harness-memory/limout/stubs/range-decomp.txt` (19 handlers), `.harness-memory/limout/stubs2/range-decomp.txt` (3 handlers).
Written: `src/engine/model_id.{h,cpp}` (CLongModelId + CModelIndexLimit), `src/engine/game_hooks.{h,cpp}` (trio + 22 handler stubs + patch table). No build run.

## Site tables — extracted from the original DLL (not TODO placeholders)
- All 10 site tables are file-backed constant arrays in `vehmod_retail.dll` `.data` (image base 0x10000000); extracted verbatim into `model_id.cpp`:
  | array | 0.98e RVA | entries | used by |
  |---|---|---|---|
  | kEntityLevelSites | 0xACC78 | 27 (0x6C/4) | CLongModelId::RelocateEntityLevel (fn @ 0x17170) |
  | kEntityZoneSites | 0xACCE8 | 27 (0x6C/4) | RelocateEntityZone (0x171a0) |
  | kModelIdMovsxSites | 0xAC218 | 664 (0xA60/4) | ExtendModelIdMovsx (0x171d0) |
  | kModelIdOrSites | 0xACDCC | 7 (0x1C/4) | ExtendModelIdOr (0x17220) |
  | kTxdIndexMovsxSites | 0xACD58 | 29 (0x74/4) | ExtendTxdIndexMovsx (0x17250) |
  | kStreamingBufferSites | 0xACDE8 | 689 (0xAC4/4) | ReplaceStreamingBufferRefs (0x17410) |
  | kModelPointerSites | 0xADA18 | 606 (0x978/4) | ReplaceModelPointerBufferRefs (0x17510) |
  | kTexStartSites | 0xAE390 | 150 (600/4) | ReplaceTexStartIndexRefs (0x175f0) |
  | kColStartSites | 0xAD948 | 51 (0xCC/4) | ReplaceColStartIndexRefs (0x17720) |
  | kAnimStartSites | 0xAD8B0 | 38 (0x98/4) | ReplaceAnimStartIndexRefs (0x177c0) |
- Total 2288 entries; region 0xAC218..0xAE5E8 is contiguous (tables adjacent, 4-byte gaps) inside the section that RVA 0xA5000 maps; every value verified inside VC .text [0x401000, 0x6A0000); counts match the decomp loop bounds exactly. `static_assert`s lock the counts.
- Opcode semantics confirmed by matches: e.g. kModelIdMovsxSites[0]=0x406087, kStreamingBufferSites[0]=0x4099F0, kModelPointerSites[0]=0x4014F0.

## VC 1.0 exe addresses by ported function (RVA = VA − 0x400000)
CLongModelId (decomp @ 0x17170..0x17250):
- 0x5B8520 — skip site in ExtendModelIdMovsx (fn @ 0x171d0). Byte patterns: 0x5E→0x56, 0x5F→0x57 (0x17170/0x171a0); word 0xBF0F→0x8B90 (0x171d0); 0x66→0x90 (0x17220); word 0xBF0F→0xB70F (0x17250).
CModelIndexLimit:
- ReplaceStreamingBufferRefs (0x17410): 0x4102B2 = new capacity; skip sites 0x40BB6B/0x40FDC6 (only when long-ids on); exception sites 0x40BD2D/0x410651/0x62A309/0x62A01B; buffer span 0x26D2C; new size (n*5+0x23)*4.
- ReplaceModelPointerBufferRefs (0x17510): skip site 0x578A77; special site 0x45E866 with unaligned value at 0x45E865; range literal 26000; new size n*4+0x1C.
- ReplaceTexStartIndexRefs (0x175f0): 0x40BD2D, 0x410651 (= base+8+tex*0x14), 0x41065F, 0x55FB1C, 0x580FD6, 0x58102D; VC pool pointer global 0x9B6A4C; VC fns 0x581010, 0x580FF0; entry stride 0x1C.
- ReplaceColStartIndexRefs (0x17720): 0x62A01B (base+9), 0x62A309 (base+8), 0x62AA50=0x6A, 0x62AA52=0x68, 0x62AA53=count.
- CaptureVcLimits mirror of FUN_10018180 (0x18180): old limits read from VC .text immediates 0x55F636 (model-ptr base), 0x40AF00 (streaming base), 0x40C417 (tex start), 0x40C507 (col start), 0x40C582 (anim start).
- Config counts used by the (unported) driver FUN_10017820: DAT_100b17bc/e0/e4 (plugin .data).

## (patch-target VC address → handler) pairs — FUN_10043c00 0.98e @ RVA 0x43c00
21 unconditional entries in `g_MvlStubHookSites` (15 JMP + 6 dword-slot):
- JMP: 0x426748→FUN_1003dff0; 0x4267DE→FUN_1003e010; 0x426AC6→FUN_1003e070; 0x426C80→FUN_1003e090; 0x427470→FUN_1003e0b0; 0x426820→FUN_1003e0d0; 0x426A4C→FUN_1003e100; 0x5BB40B→FUN_1003e120; 0x5BAC04→FUN_1003e140; 0x579290→FUN_1003e170; 0x579090→FUN_1003e1d0; 0x579190→FUN_1003e200; 0x5CF9F3→FUN_1003e2d0; 0x5C4341→FUN_1003e320; 0x408967→FUN_10043730.
- Dword write (FUN_10022590): 0x69ADC4→FUN_1003e230; 0x6D7B68→FUN_1003e250; 0x69B0E8→FUN_1003e270; 0x69ADC0→FUN_1003e290; 0x6D7B64→FUN_1003e2b0; 0x69ADB0→FUN_10043760.
- Conditional (not tabled, handled in `MvlInstallStubHooks`): 0x60048C→FUN_10043780 iff DAT_100b1791 (0.98e @ RVA 0xb1791).
- Other FUN_10043c00 pairs target non-batch handlers (FUN_1003e380..FUN_10043b50, 0x416770→FUN_10040d40, 0x64E18A→FUN_10043010, …) — decomp-chain.txt lines 54–247; extend table as ported.
- Continuation addresses used by handlers: 0x42674F, 0x4267E5, 0x426ACD, 0x426C87, 0x427477, 0x426A53, 0x4BB630, 0x5BAC09, 0x579295, 0x589790, 0x60AD50, 0x5A4DE0, 0x589AD0, 0x60AD90, 0x5CF9F9, 0x5C4348, 0x40896C, 0x6006B8, 0x60049C, 0x593030.

## Handler notes (stub batch)
- 19 stubs from stubs/: RVA 0x3DFF0..0x3E320; 3 from stubs2/: 0x43730, 0x43760, 0x43780.
- Globals written by handlers live at plugin .data 0x100Bxxxx/0x1013xxxx (kept as exact Ghidra names `DAT_...` in game_hooks.cpp with RVAs).
- 0x3E010 bypass table DAT_100b641e indexed 0x81..0xEC; 0x3E120 clears field +0x1D4; 0x3E170/0x3E290/0x3E2B0 share DAT_100b2450 across 0x3E230/250/270.
- Naked trampolines (asm): 0x3E2D0 (entry-ESP args +0x1B4/+0x1B8, EBP float*), 0x3E320 (arg +4, arg +0x40, own-frame scratch at +0x18..+0x40), 0x43780 (x87 ST0/ST1 args).
- Inert helper stubs (TODO replace with real port): FUN_1001b610, FUN_1003a450, FUN_1003a810, FUN_1003af20, FUN_1003b580, FUN_1003b5d0, FUN_1003b840, FUN_1003b9d0, FUN_1003bae0, FUN_1003de20, FUN_1004f7e0.

## Open questions / TODOs
- Who drives CLongModelId? FUN_10017390 (0.98e @ RVA 0x17390) runs the family when DAT_100b0c41!=0 (also calls 0x16ED0/0x16E70/0x17290/0x17370 — not yet ported). Integration order: MvlUnlockGameText → CaptureVcLimits → Replace* (driver FUN_10017820) → Register/Verify hooks.
- Writers of DAT_10124f14 / DAT_10124f24 / DAT_10124f30 are the model-directory hooks FUN_10017DB0..FUN_10018060 (installed by FUN_100180a0 @ RVA 0x180a0) — not ported; sizes of DAT_10124f24 (24) and DAT_10124f30 (16) are provisional.
- VerifyDirectoryCanAdd: which of +4/+8 is count vs limit is not visible in decomp; condition ported literally (field4 <= field8 → fail). Reporter return value masked to 0 (AL) in failure paths — exact EAX high bits use logger return in the original.
- RegisterModelForLookup: original used MSVC std::map<std::string,int> (DAT_1011c210); clone uses a 512-bucket FNV-1a registry (no exceptions policy). Duplicate check gated on DAT_100b17e9 as original.
- EAX-passthrough returns (0x3E010/0x3E120) use an inline-asm entry capture `mov in_eax, eax`; verify it stays the first statement under /O2.
- Naked shims assume Ghidra stack offsets are entry-ESP relative (0x1B4/0x1B8/0x40); confirm against original .text bytes before enabling those hooks.
- game_hooks.cpp `call` to `extern "C"` impls relies on MSVC inline-asm C-symbol resolution; if the linker wants the decorated `_name`, adjust.
- FUN_10043c00 also NOPs inline sites directly (e.g. `_DAT_0058c9d7=0x9090`, `_DAT_0063c52c`, `_DAT_004a404b`, `_DAT_00550c70`, `DAT_005ff260` … decomp lines 258–329) — out of scope here; log per version.
- Site tables target VC 1.0 US/retail as used by 0.98e; other VC 1.0 variants (exe base deltas) need a re-extraction check.
