# MVL Patch-Site Log — vehmod_retail.dll 0.98e

Format: `vehmod_retail.dll 0.98e @ RVA 0xXXXX` patches VC 1.0 @ 0xYYYYY (purpose, confidence, evidence).
Image base 0x10000000. VC 1.0 retail exe anchor: `H:\games\VC\gta-vc.exe` (Q3.6).
Steam variant: same code, retargeted addresses (marker strings dump640FE5h vs dump63FF95h @ 0x1007bba4).

## Hook installer machinery
| RVA | Role | Evidence |
|---|---|---|
| 0x22560 | Hook writer: writes jump/hook at a VC address | decomp-chain.txt depth-1; called repeatedly from init FUN_10043c00 |
| 0x22590 | Hook writer variant (VirtualProtect path) | decomp-chain.txt |
| 0x225a0 | VirtualProtect wrapper (region incl. 0x401000) | `PUSH 0x401000` @ 0x100225af (vcconsts.txt) |
| 0x43c00 | Main init — orchestrates all patch installs | decomp-chain.txt depth-0 |

## Hook stub bodies (MVL-side targets of installed patches)
19 stubs created 0x3dff0–0x3e320 + 0x40d40, 0x43010, 0x43730–0x43780 (mkfuncs-xrefs.txt F-CREATE).
Decompile pending → each maps to one VC patch site.

## Patch sites (to be filled from FUN_10022560/10022590 call-site enumeration)
| # | RVA in dll | VC target | Purpose | Confidence | Evidence |
|---|---|---|---|---|---|
| 1 | (pending) | 0x401000-region | code-region unlock/first patch | high | PUSH 0x401000 @ 0x100225af |
| 2 | (pending) | pool new-vehicle ctor | CMvlSettings::HookPoolNewVehicle — pool extension ("RAN OUT OF VEHICLE SLOTS!!!") | high | string @ 0x1007ef64 |
| 3 | (pending) | pool new-colmodel ctor | CMvlSettings::HookPoolNewColModel | high | string @ 0x1007f10c |
| 4 | (pending) | model-index sites | CModelIndexLimit::Replace*Refs (streaming/model-ptr/tex/col/anim start-index) | high | strings @ 0x1007b43c–0x1007b4f8 |
| 5 | (pending) | entity model-id reads | CLongModelId::Extend*/Relocate* (16→32-bit model id widening) | high | strings @ 0x1007b38c–0x1007b418 |
| 6 | (pending) | SCM opcode dispatch | CVehicleSCM::SetNextTextForVehicleId (opcode 0800) / ProcessNextTextOpcode | high | strings @ 0x1008c988/0x1008ca90 |
| 7 | (pending) | weapon-pickup opcode | CWeaponPickups::ProcessNewWeaponPickupOpcode (opcode 0810) | high | string @ 0x1008dbb0 |
| 8 | (pending) | D3D8 texture path | CDxHandler::DX9TextureFix | medium | string @ 0x1007afb4 |
| 9 | (pending) | streaming buffers | CMvlSettings::ProcessStreamingMemLimit | high | string @ 0x1007f290 |
| 10 | (pending) | vehicle-path init | "Initialise vehicle paths" @ 0x1007ae74 (FUN_0x77b0) | medium | string xref |

## Known VC 1.0 address constants (vcconsts.txt, 2,755 hits — top clusters)
0x401000-region (code unlock), 0x401770/0x401820/0x4018F0 (early MOV EAX,imm sites),
0x4019F7/0x401A40/0x401B10/0x401B18 (MOV ECX,imm + global write 0x100b0dac),
0x402A20/0x402AF0/0x402CF6/0x402D0D/0x402DDB, 0x4055E0–0x405BF0, 0x407440/0x407890/0x407E10–0x407EF0…
Full list: `.harness-memory/d3out/vcconsts.txt` (each row: constant, referencing func, instruction).

## Next actions
1. Ghidra script: enumerate all calls to 0x10022560/0x10022590 across init chain → extract (VC target, hook stub) pairs.
2. Decompile 19 hook stubs → fill purpose column.
3. Byte-diff retail vs steam dll → confirm address-table deltas match vcconsts entries.
