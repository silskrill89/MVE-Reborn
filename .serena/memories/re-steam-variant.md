# re-steam-variant.md - M8b steam address-map recovery (vehmod_steam.dll)

Binary pair: original_files/maxovehmod098e/mvl/vehmod_retail.dll vs
vehmod_steam.dll (both 760,832 B PE32 x86, image base 0x10000000, IDENTICAL
section raw layout: .text va 0x1000 raw 0x400/0x73A00, .rdata va 0x75000 raw
0x73A00/0x2F200, .data va 0xA5000 raw 0xA2C00/0xAE00, .rsrc, .reloc; VirtualSize
differs by a few bytes: .text 0x7392D vs 0x73A05, .rdata 0x2F14A vs 0x2F14E).

Method (tools/steam_addr_diff.py, pure stdlib, READ-ONLY): the two DLLs are
builds of the same MVL source with every embedded gta-vc.exe address
substituted; a retail VA immediate at file offset X in the retail DLL maps to
the steam VA at the same offset X in the steam DLL. Methods tried per field:
imm (absolute dword) -> rvaimm (VA-0x400000) -> rel32. Confidence: high =
context bytes around the immediate identical in both DLLs (same instruction,
different immediate) or >=2 agreeing matches; medium = single match, context
differs; identical = steam value == retail value at every match.

Cluster verification (post-pass, added after first run): ctx_identical masks
only the field's own immediate (+/-6 B), so fields packed next to OTHER
substituted addresses fail it (e.g. texCountRef2@0x17675 / texCountRef3@0x17670
are 5 B apart - each one's context window contains the other's changed bytes).
cluster_verify() re-checks with a +/-24 B window where EVERY differing byte
must be covered by a dword pair that is itself a plausible address
substitution (both dwords in the game-VA window, or pair delta in the global
census). All 7 initially-medium fields passed at every evidence offset ->
upgraded to high (method suffix "+cluster" in the output tables).

Result: 83/83 fields resolved, ALL HIGH - 69 recovered-diff high (incl. 7
cluster-verified) + 14 identical high; 0 medium, 0 low/ambiguous, 0 NOT FOUND.
No Ghidra fallback needed. Full evidence table (incl. cluster_verified flags):
tools/steam_addr_diff_output.md / .json.

Delta census (retail-steam): 48x13, 272x8, 288x2, 304x1, 464x14, 560x2,
832x5, 928x3, 992x2, 4086x1, 4087x1, 4088x7, 4096x4, 4144x2, 4176x3, 4200x1 -
region-consistent shifts (steam exe .text layout differs slightly per region),
used as a cross-check per cluster.

Hook-site installer verification (PLAN.md 10.4 risk): the original stores each
install as inline code BA <handler> B9 <target> E8 <rel32-installer>; for all
21 sites the handler dword, BA/B9/E8 opcodes and installer rel32 are
BYTE-IDENTICAL between the DLLs (only the target dword differs) -> same
installer per site -> the clone dwordWrite flags carry over unchanged.

Detection fingerprint (re-loader-shim.md, reused for M8c): dword @ exe VA
0x61A7F8: 0x9753C405 = retail, 0xE8535004 = steam.

| field | retail RVA | steam RVA | method | conf | evidence (dll rva/section/ctx) |
|---|---|---|---|---|---|
| textUnlockBase | 0x001000 | 0x001000 | identical | high | imm-same@0x225B0/.text/ctx=1; imm-same@0xA2062/.rdata/ctx=1 |
| textUnlockSize | 0x-183200 | 0x-183200 | identical | high | imm-same@0x225AB/.text/ctx=1 |
| modelPtrBaseImm | 0x15F636 | 0x15F526 | imm | high | imm@0x18181/.text/ctx=1; imm@0xAE06C/.data/ctx=0 |
| streamingBaseImm | 0x00AF00 | 0x00AF00 | identical | high | imm-same@0x1818B/.text/ctx=1; imm-same@0xACF34/.data/ctx=1 |
| texStartImm | 0x00C417 | 0x00C417 | identical | high | imm-same@0x18195/.text/ctx=1; imm-same@0xAE4DC/.data/ctx=1 |
| colStartImm | 0x00C507 | 0x00C507 | identical | high | imm-same@0x1819F/.text/ctx=1; imm-same@0xAD9A8/.data/ctx=1 |
| animStartImm | 0x00C582 | 0x00C582 | identical | high | imm-same@0x181A9/.text/ctx=1; imm-same@0xAD8D0/.data/ctx=1 |
| streamingCapacity | 0x0102B2 | 0x0102B2 | identical | high | imm-same@0x17501/.text/ctx=1 |
| texStartRef1 | 0x00BD2D | 0x00BD2D | identical | high | imm-same@0x174AC/.text/ctx=1; imm-same@0x17659/.text/ctx=1; imm-same@0xAD89C/.data/ctx=0 |
| texStartRef2 | 0x010651 | 0x010651 | identical+cluster | high | imm-same@0x174B3/.text/ctx=0; imm-same@0x17668/.text/ctx=0; imm-same@0xAD8A0/.data/ctx=0 |
| texCountRef1 | 0x01065F | 0x01065F | identical+cluster | high | imm-same@0x1767A/.text/ctx=0 |
| texStartRef3 | 0x15FB1C | 0x15FA0C | imm | high | imm@0x17644/.text/ctx=1 |
| texCountRef2 | 0x180FD6 | 0x180E06 | imm+cluster | high | imm@0x17675/.text/ctx=0 |
| texCountRef3 | 0x18102D | 0x180E5D | imm+cluster | high | imm@0x17670/.text/ctx=0 |
| texPoolGlobal | 0x5B6A4C | 0x5B5A54 | imm | high | imm@0x1767F/.text/ctx=1; imm@0x1768C/.text/ctx=0; imm@0x176AF/.text/ctx=1; imm@0x176EE/.text/ctx=0 |
| texPoolRebuild1 | 0x181010 | 0x180E40 | imm | high | imm@0x17695/.text/ctx=1 |
| texPoolRebuild2 | 0x180FF0 | 0x180E20 | imm+cluster | high | imm@0x176F3/.text/ctx=0 |
| colStartRef1 | 0x22A01B | 0x229CDB | imm | high | imm@0x174C1/.text/ctx=0; imm@0x17789/.text/ctx=0; imm@0xAD8A8/.data/ctx=0 |
| colStartRef2 | 0x22A309 | 0x229FC9 | imm | high | imm@0x174BA/.text/ctx=0; imm@0x17791/.text/ctx=1; imm@0xAD8A4/.data/ctx=0 |
| colExtraRef1 | 0x22AA50 | 0x22A710 | imm | high | imm@0x1779C/.text/ctx=1 |
| colExtraRef2 | 0x22AA52 | 0x22A712 | imm+cluster | high | imm@0x177AB/.text/ctx=0 |
| colExtraRef3 | 0x22AA53 | 0x22A713 | imm+cluster | high | imm@0x177B1/.text/ctx=0 |
| streamingSkipSite1 | 0x00BB6B | 0x00BB6B | identical | high | imm-same@0x17480/.text/ctx=1; imm-same@0xAD3A4/.data/ctx=0 |
| streamingSkipSite2 | 0x00FDC6 | 0x00FDC6 | identical | high | imm-same@0x17487/.text/ctx=1; imm-same@0xAD3A8/.data/ctx=1 |
| modelPtrSkipSite | 0x178A77 | 0x178967 | imm | high | imm@0x17568/.text/ctx=1; imm@0xAE0B4/.data/ctx=0 |
| modelPtrSpecialSite | 0x05E866 | 0x05E746 | imm | high | imm@0x17588/.text/ctx=0; imm@0xADCA4/.data/ctx=0 |
| modelPtrSpecialValue | 0x05E865 | 0x05E745 | imm | high | imm@0x17590/.text/ctx=1; imm@0x175A8/.text/ctx=1 |
| wideModelIdSkipSite | 0x1B8520 | 0x1B8350 | imm | high | imm@0x171E7/.text/ctx=1; imm@0x4459B/.text/ctx=1; imm@0xACAE8/.data/ctx=0 |
| paintByteGlobal1 | 0x610B26 | 0x60FB2F | imm | high | imm@0x3BA08/.text/ctx=1; imm@0x3BB16/.text/ctx=1; imm@0xA6BD4/.data/ctx=0 |
| paintByteGlobal2 | 0x610B82 | 0x60FB8C | imm | high | imm@0x3B9FB/.text/ctx=1; imm@0x3BB09/.text/ctx=1; imm@0xA6BDC/.data/ctx=0 |
| partFlagByte | 0x171820 | 0x171710 | imm | high | imm@0x3B5C3/.text/ctx=1 |
| liveryColorTable | 0x539228 | 0x538230 | imm | high | imm@0x3B867/.text/ctx=0; imm@0x3B86E/.text/ctx=0; imm@0x3DA84/.text/ctx=1; imm@0x3DACB/.text/ctx=1 |
| speedCheckObject | 0x54AD28 | 0x549D30 | imm | high | imm@0x798E/.text/ctx=1; imm@0x7998/.text/ctx=1; imm@0x1253E/.text/ctx=1; imm@0x1296C/.text/ctx=1 |
| frameCounter | 0x574B2C | 0x573B34 | imm | high | imm@0xA9AD/.text/ctx=1; imm@0xF214/.text/ctx=1; imm@0xF275/.text/ctx=1; imm@0xF2C5/.text/ctx=1 |
| classCounterBase | 0x5785A8 | 0x5775B0 | imm | high | imm@0x4F74F/.text/ctx=1; imm@0x4F7FF/.text/ctx=0; imm@0x4F807/.text/ctx=1; imm@0xA6D4C/.data/ctx=1 |
| cacheByte1 | 0x23C6F2 | 0x23B6A2 | imm | high | imm@0xC2B2/.text/ctx=0; imm@0x3A3AC/.text/ctx=0; imm@0x3A3D8/.text/ctx=0; imm@0x3DECE/.text/ctx=0 |
| cacheByte2 | 0x23C71D | 0x23B6CD | imm | high | imm@0xC2BE/.text/ctx=0; imm@0x3A3B3/.text/ctx=0; imm@0x3A3E4/.text/ctx=0; imm@0x3DED5/.text/ctx=0 |
| cacheByte3 | 0x23CE27 | 0x23BDD7 | imm | high | imm@0xC2CA/.text/ctx=0; imm@0x3A3BA/.text/ctx=0; imm@0x3A3F0/.text/ctx=0; imm@0x3DEDC/.text/ctx=0 |
| cacheRestoreByte | 0x2DDE20 | 0x2DCDB8 | imm | high | imm@0xC27B/.text/ctx=1; imm@0xC293/.text/ctx=0; imm@0xC299/.text/ctx=0; imm@0xC2A2/.text/ctx=1 |
| classSlotBase | 0x3DB888 | 0x3DA890 | imm | high | imm@0x13B4C/.text/ctx=0; imm@0x3AA6A/.text/ctx=0; imm@0xA6BEC/.data/ctx=0 |
| classObjTable | 0x54B708 | 0x54A710 | imm | high | imm@0x13B31/.text/ctx=1; imm@0x3A95A/.text/ctx=1; imm@0x3B6D5/.text/ctx=1; imm@0xA6BE8/.data/ctx=0 |
| frameWaitHookTarget | 0x20048C | 0x2000EC | imm | high | imm@0x44802/.text/ctx=1 |
| contTrampoline426748 | 0x02674F | 0x02671F | imm | high | imm@0x3E004/.text/ctx=1 |
| contKeyClassBypass | 0x0267E5 | 0x0267B5 | imm | high | imm@0x3E05C/.text/ctx=1 |
| contTrampoline426ac6 | 0x026ACD | 0x026A9D | imm | high | imm@0x3E084/.text/ctx=1 |
| contTrampoline426c80 | 0x026C87 | 0x026C57 | imm | high | imm@0x3E0A4/.text/ctx=1 |
| contTrampoline427470 | 0x027477 | 0x027447 | imm | high | imm@0x3E0C4/.text/ctx=1 |
| contTrampoline426a4c | 0x026A53 | 0x026A23 | imm | high | imm@0x3E114/.text/ctx=1 |
| teardownPartsCallee | 0x0BB630 | 0x0BB500 | imm | high | imm@0x3E154/.text/ctx=1 |
| contTeardownParts | 0x1BAC09 | 0x1BAA39 | imm | high | imm@0x3E160/.text/ctx=1 |
| contRepaint | 0x179295 | 0x179185 | imm | high | imm@0x3E1C0/.text/ctx=1 |
| partUpdateCallee69adc4 | 0x189790 | 0x1895C0 | imm | high | imm@0x33331/.text/ctx=1; imm@0x3E23E/.text/ctx=1 |
| partUpdateCallee6d7b68 | 0x20AD50 | 0x20A970 | imm | high | imm@0x33511/.text/ctx=1; imm@0x3E25E/.text/ctx=1 |
| partUpdateCallee69b0e8 | 0x1A4DE0 | 0x1A4C10 | imm | high | imm@0x336A1/.text/ctx=1; imm@0x3E27E/.text/ctx=1 |
| wheelCacheCallee | 0x189AD0 | 0x189900 | imm | high | imm@0x16C74/.text/ctx=1; imm@0x33321/.text/ctx=1; imm@0x3E2A9/.text/ctx=1 |
| partFlagCallee | 0x20AD90 | 0x20A9B0 | imm | high | imm@0x33501/.text/ctx=1; imm@0x3E2C4/.text/ctx=1 |
| contFadePart | 0x1CF9F9 | 0x1CF7C9 | imm | high | imm@0x3E316/.text/ctx=1 |
| contFadePartAlt | 0x1C4348 | 0x1C4178 | imm | high | imm@0x3E367/.text/ctx=1 |
| contGenerateVcName | 0x00896C | 0x00896C | identical | high | imm-same@0x43748/.text/ctx=1 |
| attachFixupCallee | 0x193030 | 0x192E60 | imm | high | imm@0x33301/.text/ctx=1; imm@0x4376E/.text/ctx=1 |
| contFrameWaitThrottled | 0x2006B8 | 0x200318 | imm | high | imm@0x437E7/.text/ctx=1 |
| contFrameWaitNormal | 0x20049C | 0x2000FC | imm | high | imm@0x437F8/.text/ctx=1 |
| hook:MvlHookTrampoline426748 | 0x026748 | 0x026718 | imm | high | imm@0x43C4A/.text/ctx=1 |
| hook:MvlHookKeyClassBypass | 0x0267DE | 0x0267AE | imm | high | imm@0x43C59/.text/ctx=1 |
| hook:MvlHookTrampoline426ac6 | 0x026AC6 | 0x026A96 | imm | high | imm@0x43C68/.text/ctx=1 |
| hook:MvlHookTrampoline426c80 | 0x026C80 | 0x026C50 | imm | high | imm@0x43C77/.text/ctx=1 |
| hook:MvlHookTrampoline427470 | 0x027470 | 0x027440 | imm | high | imm@0x43C86/.text/ctx=1 |
| hook:MvlHookSetAiClass | 0x026820 | 0x0267F0 | imm | high | imm@0x43C95/.text/ctx=1 |
| hook:MvlHookTrampoline426a4c | 0x026A4C | 0x026A1C | imm | high | imm@0x43CA4/.text/ctx=1 |
| hook:MvlHookClearParts | 0x1BB40B | 0x1BB23B | imm | high | imm@0x43CB3/.text/ctx=1 |
| hook:MvlHookTeardownParts | 0x1BAC04 | 0x1BAA34 | imm | high | imm@0x43CC2/.text/ctx=1 |
| hook:MvlHookRepaint | 0x179290 | 0x179180 | imm | high | imm@0x43CD1/.text/ctx=1 |
| hook:MvlHookPaintRandom | 0x179090 | 0x178F80 | imm | high | imm@0x43CE0/.text/ctx=1 |
| hook:MvlHookPaintVariant | 0x179190 | 0x179080 | imm | high | imm@0x43CEF/.text/ctx=1 |
| hook:MvlHookPartUpdate69adc4 | 0x29ADC4 | 0x299DC4 | imm | high | imm@0x43CFE/.text/ctx=1 |
| hook:MvlHookPartUpdate6d7b68 | 0x2D7B68 | 0x2D6B38 | imm | high | imm@0x43D0D/.text/ctx=1 |
| hook:MvlHookPartUpdate69b0e8 | 0x29B0E8 | 0x29A0E8 | imm | high | imm@0x43D1C/.text/ctx=1 |
| hook:MvlHookWheelCache69adc0 | 0x29ADC0 | 0x299DC0 | imm | high | imm@0x43D2B/.text/ctx=1 |
| hook:MvlHookPartFlag6d7b64 | 0x2D7B64 | 0x2D6B34 | imm | high | imm@0x43D3A/.text/ctx=1 |
| hook:MvlHookFadePart | 0x1CF9F3 | 0x1CF7C3 | imm | high | imm@0x43D49/.text/ctx=1 |
| hook:MvlHookFadePartAlt | 0x1C4341 | 0x1C4171 | imm | high | imm@0x43D58/.text/ctx=1 |
| hook:MvlHookGenerateVcName | 0x008967 | 0x008967 | identical | high | imm-same@0x4477B/.text/ctx=1 |
| hook:MvlHookAttachFixup | 0x29ADB0 | 0x299DB0 | imm | high | imm@0x4478A/.text/ctx=1 |
