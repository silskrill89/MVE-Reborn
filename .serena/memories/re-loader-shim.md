# RE: vehmod_chooser.flt — loader shim (0.98c–0.98e)

**Binary**: `original_files/maxovehmod098e/mss/vehmod_chooser.flt` (532 B, PE32, image base 0x10000000, EP RVA 0x154). MD5 `ba45b291…` byte-identical across 0.98c/d/e (see version-matrix.md).
**Method**: full disassembly via pefile+capstone (Ghidra decomp failed — 64-byte .text, bad-instruction truncation; dump at `E:\dev(dave)\tools\ghidra-projects\dumps\chooser\`).

## Complete logic (entire .text = 0x40 bytes, DllMain stdcall ret 0xC)

```
10000154  push ebx
10000155  call $+5          ; get own EIP
1000015a  pop  ebx
1000015b  add  ebx, 0x3a    ; ebx = 0x10000194 = .rdata start (IAT)
1000015e  cmp  [esp+0xC], 1 ; reason == DLL_PROCESS_ATTACH
10000163  jne  ret
10000165  mov  eax, [0x61A7F8]      ; read dword from gta-vc.exe (fixed VA)
1000016a  cmp  eax, 0x9753C405      ; fingerprint A
1000016f  jne  checkB
10000171  lea  eax, [ebx+0x55]      ; -> 0x100001E9 "mvl\vehmod_retail.dll"
10000174  jmp  load
10000176  cmp  eax, 0xE8535004      ; fingerprint B
1000017b  jne  ret                  ; neither -> do nothing
1000017d  lea  eax, [ebx+0x6B]      ; -> 0x100001FF "mvl\vehmod_steam.dll"
10000180  push eax
10000181  call [ebx]                ; IAT[0] = LoadLibraryA
10000183  pop  ebx
10000184  ret  0xC
```

## Findings
- **Exe-variant detection**: reads one dword at fixed gta-vc.exe VA `0x61A7F8` and compares against two magic constants `0x9753C405` (→ retail) / `0xE8535004` (→ steam). No pattern scanning, no checksum — single 4-byte fingerprint. **Open item**: verify which exe bytes sit at 0x61A7F8 in `H:\games\VC\gta-vc.exe` (retail 1.0) to confirm fingerprint A and identify what code/data it points at.
- **Load target**: `LoadLibraryA("mvl\vehmod_retail.dll")` or `"mvl\vehmod_steam.dll"` — paths relative to game dir (CWD). Strings at 0x100001E9/0x100001FF; IAT has exactly one import (KERNEL32!LoadLibraryA).
- **Silent failure**: unknown exe → shim loads nothing (no log, no error). Explains "why is your game loading mss/vehmod.flt" confusion in forum reports.
- **Clone implication**: our `vehmod_chooser` equivalent = 30 lines of C: read dword at 0x61A7F8, pick DLL name, LoadLibraryA. Must keep the same fingerprint constants for retail/steam 1.0 exes (or extend with pattern matching per WindowedMode doctrine — but original is fingerprint-only).

## Clone mapping
| Original | Clone module |
|---|---|
| DllMain fingerprint check @0x154 | `src/loader/chooser.cpp` (or fold into main DLL init) |
| LoadLibraryA of mvl DLL | same |

*Status: COMPLETE (shim fully understood).*
