# steam_addr_diff output (M8b)

Retail: `original_files/maxovehmod098e/mvl/vehmod_retail.dll`  |  Steam: `original_files/maxovehmod098e/mvl/vehmod_steam.dll`

Stats: 83 fields | recovered-diff high 69 | recovered-diff medium 0 | identical high 14 | identical medium 0 | low/ambiguous 0 | NOT FOUND 0 (buckets sum to total)

| field | kind | retail VA | retail RVA | steam VA | steam RVA | method | conf | evidence (dll rva/section/ctx) |
|---|---|---|---|---|---|---|---|---|
| `textUnlockBase` | map | 0x401000 | 0x001000 | 0x401000 | 0x001000 | identical | high | imm-same@0x225B0/.text/ctx=1; imm-same@0xA2062/.rdata/ctx=1 |
| `textUnlockSize` | map | 0x27CE00 | 0x-183200 | 0x27CE00 | 0x-183200 | identical | high | imm-same@0x225AB/.text/ctx=1 |
| `modelPtrBaseImm` | map | 0x55F636 | 0x15F636 | 0x55F526 | 0x15F526 | imm | high | imm@0x18181/.text/ctx=1; imm@0xAE06C/.data/ctx=0 |
| `streamingBaseImm` | map | 0x40AF00 | 0x00AF00 | 0x40AF00 | 0x00AF00 | identical | high | imm-same@0x1818B/.text/ctx=1; imm-same@0xACF34/.data/ctx=1 |
| `texStartImm` | map | 0x40C417 | 0x00C417 | 0x40C417 | 0x00C417 | identical | high | imm-same@0x18195/.text/ctx=1; imm-same@0xAE4DC/.data/ctx=1 |
| `colStartImm` | map | 0x40C507 | 0x00C507 | 0x40C507 | 0x00C507 | identical | high | imm-same@0x1819F/.text/ctx=1; imm-same@0xAD9A8/.data/ctx=1 |
| `animStartImm` | map | 0x40C582 | 0x00C582 | 0x40C582 | 0x00C582 | identical | high | imm-same@0x181A9/.text/ctx=1; imm-same@0xAD8D0/.data/ctx=1 |
| `streamingCapacity` | map | 0x4102B2 | 0x0102B2 | 0x4102B2 | 0x0102B2 | identical | high | imm-same@0x17501/.text/ctx=1 |
| `texStartRef1` | map | 0x40BD2D | 0x00BD2D | 0x40BD2D | 0x00BD2D | identical | high | imm-same@0x174AC/.text/ctx=1; imm-same@0x17659/.text/ctx=1; imm-same@0xAD89C/.data/ctx=0 |
| `texStartRef2` | map | 0x410651 | 0x010651 | 0x410651 | 0x010651 | identical+cluster | high | imm-same@0x174B3/.text/ctx=0; imm-same@0x17668/.text/ctx=0; imm-same@0xAD8A0/.data/ctx=0 |
| `texCountRef1` | map | 0x41065F | 0x01065F | 0x41065F | 0x01065F | identical+cluster | high | imm-same@0x1767A/.text/ctx=0 |
| `texStartRef3` | map | 0x55FB1C | 0x15FB1C | 0x55FA0C | 0x15FA0C | imm | high | imm@0x17644/.text/ctx=1 |
| `texCountRef2` | map | 0x580FD6 | 0x180FD6 | 0x580E06 | 0x180E06 | imm+cluster | high | imm@0x17675/.text/ctx=0 |
| `texCountRef3` | map | 0x58102D | 0x18102D | 0x580E5D | 0x180E5D | imm+cluster | high | imm@0x17670/.text/ctx=0 |
| `texPoolGlobal` | map | 0x9B6A4C | 0x5B6A4C | 0x9B5A54 | 0x5B5A54 | imm | high | imm@0x1767F/.text/ctx=1; imm@0x1768C/.text/ctx=0; imm@0x176AF/.text/ctx=1; imm@0x176EE/.text/ctx=0 |
| `texPoolRebuild1` | map | 0x581010 | 0x181010 | 0x580E40 | 0x180E40 | imm | high | imm@0x17695/.text/ctx=1 |
| `texPoolRebuild2` | map | 0x580FF0 | 0x180FF0 | 0x580E20 | 0x180E20 | imm+cluster | high | imm@0x176F3/.text/ctx=0 |
| `colStartRef1` | map | 0x62A01B | 0x22A01B | 0x629CDB | 0x229CDB | imm | high | imm@0x174C1/.text/ctx=0; imm@0x17789/.text/ctx=0; imm@0xAD8A8/.data/ctx=0 |
| `colStartRef2` | map | 0x62A309 | 0x22A309 | 0x629FC9 | 0x229FC9 | imm | high | imm@0x174BA/.text/ctx=0; imm@0x17791/.text/ctx=1; imm@0xAD8A4/.data/ctx=0 |
| `colExtraRef1` | map | 0x62AA50 | 0x22AA50 | 0x62A710 | 0x22A710 | imm | high | imm@0x1779C/.text/ctx=1 |
| `colExtraRef2` | map | 0x62AA52 | 0x22AA52 | 0x62A712 | 0x22A712 | imm+cluster | high | imm@0x177AB/.text/ctx=0 |
| `colExtraRef3` | map | 0x62AA53 | 0x22AA53 | 0x62A713 | 0x22A713 | imm+cluster | high | imm@0x177B1/.text/ctx=0 |
| `streamingSkipSite1` | map | 0x40BB6B | 0x00BB6B | 0x40BB6B | 0x00BB6B | identical | high | imm-same@0x17480/.text/ctx=1; imm-same@0xAD3A4/.data/ctx=0 |
| `streamingSkipSite2` | map | 0x40FDC6 | 0x00FDC6 | 0x40FDC6 | 0x00FDC6 | identical | high | imm-same@0x17487/.text/ctx=1; imm-same@0xAD3A8/.data/ctx=1 |
| `modelPtrSkipSite` | map | 0x578A77 | 0x178A77 | 0x578967 | 0x178967 | imm | high | imm@0x17568/.text/ctx=1; imm@0xAE0B4/.data/ctx=0 |
| `modelPtrSpecialSite` | map | 0x45E866 | 0x05E866 | 0x45E746 | 0x05E746 | imm | high | imm@0x17588/.text/ctx=0; imm@0xADCA4/.data/ctx=0 |
| `modelPtrSpecialValue` | map | 0x45E865 | 0x05E865 | 0x45E745 | 0x05E745 | imm | high | imm@0x17590/.text/ctx=1; imm@0x175A8/.text/ctx=1 |
| `wideModelIdSkipSite` | map | 0x5B8520 | 0x1B8520 | 0x5B8350 | 0x1B8350 | imm | high | imm@0x171E7/.text/ctx=1; imm@0x4459B/.text/ctx=1; imm@0xACAE8/.data/ctx=0 |
| `paintByteGlobal1` | map | 0xA10B26 | 0x610B26 | 0xA0FB2F | 0x60FB2F | imm | high | imm@0x3BA08/.text/ctx=1; imm@0x3BB16/.text/ctx=1; imm@0xA6BD4/.data/ctx=0 |
| `paintByteGlobal2` | map | 0xA10B82 | 0x610B82 | 0xA0FB8C | 0x60FB8C | imm | high | imm@0x3B9FB/.text/ctx=1; imm@0x3BB09/.text/ctx=1; imm@0xA6BDC/.data/ctx=0 |
| `partFlagByte` | map | 0x571820 | 0x171820 | 0x571710 | 0x171710 | imm | high | imm@0x3B5C3/.text/ctx=1 |
| `liveryColorTable` | map | 0x939228 | 0x539228 | 0x938230 | 0x538230 | imm | high | imm@0x3B867/.text/ctx=0; imm@0x3B86E/.text/ctx=0; imm@0x3DA84/.text/ctx=1; imm@0x3DACB/.text/ctx=1 |
| `speedCheckObject` | map | 0x94AD28 | 0x54AD28 | 0x949D30 | 0x549D30 | imm | high | imm@0x798E/.text/ctx=1; imm@0x7998/.text/ctx=1; imm@0x1253E/.text/ctx=1; imm@0x1296C/.text/ctx=1 |
| `frameCounter` | map | 0x974B2C | 0x574B2C | 0x973B34 | 0x573B34 | imm | high | imm@0xA9AD/.text/ctx=1; imm@0xF214/.text/ctx=1; imm@0xF275/.text/ctx=1; imm@0xF2C5/.text/ctx=1 |
| `classCounterBase` | map | 0x9785A8 | 0x5785A8 | 0x9775B0 | 0x5775B0 | imm | high | imm@0x4F74F/.text/ctx=1; imm@0x4F7FF/.text/ctx=0; imm@0x4F807/.text/ctx=1; imm@0xA6D4C/.data/ctx=1 |
| `cacheByte1` | map | 0x63C6F2 | 0x23C6F2 | 0x63B6A2 | 0x23B6A2 | imm | high | imm@0xC2B2/.text/ctx=0; imm@0x3A3AC/.text/ctx=0; imm@0x3A3D8/.text/ctx=0; imm@0x3DECE/.text/ctx=0 |
| `cacheByte2` | map | 0x63C71D | 0x23C71D | 0x63B6CD | 0x23B6CD | imm | high | imm@0xC2BE/.text/ctx=0; imm@0x3A3B3/.text/ctx=0; imm@0x3A3E4/.text/ctx=0; imm@0x3DED5/.text/ctx=0 |
| `cacheByte3` | map | 0x63CE27 | 0x23CE27 | 0x63BDD7 | 0x23BDD7 | imm | high | imm@0xC2CA/.text/ctx=0; imm@0x3A3BA/.text/ctx=0; imm@0x3A3F0/.text/ctx=0; imm@0x3DEDC/.text/ctx=0 |
| `cacheRestoreByte` | map | 0x6DDE20 | 0x2DDE20 | 0x6DCDB8 | 0x2DCDB8 | imm | high | imm@0xC27B/.text/ctx=1; imm@0xC293/.text/ctx=0; imm@0xC299/.text/ctx=0; imm@0xC2A2/.text/ctx=1 |
| `classSlotBase` | map | 0x7DB888 | 0x3DB888 | 0x7DA890 | 0x3DA890 | imm | high | imm@0x13B4C/.text/ctx=0; imm@0x3AA6A/.text/ctx=0; imm@0xA6BEC/.data/ctx=0 |
| `classObjTable` | map | 0x94B708 | 0x54B708 | 0x94A710 | 0x54A710 | imm | high | imm@0x13B31/.text/ctx=1; imm@0x3A95A/.text/ctx=1; imm@0x3B6D5/.text/ctx=1; imm@0xA6BE8/.data/ctx=0 |
| `frameWaitHookTarget` | map | 0x60048C | 0x20048C | 0x6000EC | 0x2000EC | imm | high | imm@0x44802/.text/ctx=1 |
| `contTrampoline426748` | map | 0x42674F | 0x02674F | 0x42671F | 0x02671F | imm | high | imm@0x3E004/.text/ctx=1 |
| `contKeyClassBypass` | map | 0x4267E5 | 0x0267E5 | 0x4267B5 | 0x0267B5 | imm | high | imm@0x3E05C/.text/ctx=1 |
| `contTrampoline426ac6` | map | 0x426ACD | 0x026ACD | 0x426A9D | 0x026A9D | imm | high | imm@0x3E084/.text/ctx=1 |
| `contTrampoline426c80` | map | 0x426C87 | 0x026C87 | 0x426C57 | 0x026C57 | imm | high | imm@0x3E0A4/.text/ctx=1 |
| `contTrampoline427470` | map | 0x427477 | 0x027477 | 0x427447 | 0x027447 | imm | high | imm@0x3E0C4/.text/ctx=1 |
| `contTrampoline426a4c` | map | 0x426A53 | 0x026A53 | 0x426A23 | 0x026A23 | imm | high | imm@0x3E114/.text/ctx=1 |
| `teardownPartsCallee` | map | 0x4BB630 | 0x0BB630 | 0x4BB500 | 0x0BB500 | imm | high | imm@0x3E154/.text/ctx=1 |
| `contTeardownParts` | map | 0x5BAC09 | 0x1BAC09 | 0x5BAA39 | 0x1BAA39 | imm | high | imm@0x3E160/.text/ctx=1 |
| `contRepaint` | map | 0x579295 | 0x179295 | 0x579185 | 0x179185 | imm | high | imm@0x3E1C0/.text/ctx=1 |
| `partUpdateCallee69adc4` | map | 0x589790 | 0x189790 | 0x5895C0 | 0x1895C0 | imm | high | imm@0x33331/.text/ctx=1; imm@0x3E23E/.text/ctx=1 |
| `partUpdateCallee6d7b68` | map | 0x60AD50 | 0x20AD50 | 0x60A970 | 0x20A970 | imm | high | imm@0x33511/.text/ctx=1; imm@0x3E25E/.text/ctx=1 |
| `partUpdateCallee69b0e8` | map | 0x5A4DE0 | 0x1A4DE0 | 0x5A4C10 | 0x1A4C10 | imm | high | imm@0x336A1/.text/ctx=1; imm@0x3E27E/.text/ctx=1 |
| `wheelCacheCallee` | map | 0x589AD0 | 0x189AD0 | 0x589900 | 0x189900 | imm | high | imm@0x16C74/.text/ctx=1; imm@0x33321/.text/ctx=1; imm@0x3E2A9/.text/ctx=1 |
| `partFlagCallee` | map | 0x60AD90 | 0x20AD90 | 0x60A9B0 | 0x20A9B0 | imm | high | imm@0x33501/.text/ctx=1; imm@0x3E2C4/.text/ctx=1 |
| `contFadePart` | map | 0x5CF9F9 | 0x1CF9F9 | 0x5CF7C9 | 0x1CF7C9 | imm | high | imm@0x3E316/.text/ctx=1 |
| `contFadePartAlt` | map | 0x5C4348 | 0x1C4348 | 0x5C4178 | 0x1C4178 | imm | high | imm@0x3E367/.text/ctx=1 |
| `contGenerateVcName` | map | 0x40896C | 0x00896C | 0x40896C | 0x00896C | identical | high | imm-same@0x43748/.text/ctx=1 |
| `attachFixupCallee` | map | 0x593030 | 0x193030 | 0x592E60 | 0x192E60 | imm | high | imm@0x33301/.text/ctx=1; imm@0x4376E/.text/ctx=1 |
| `contFrameWaitThrottled` | map | 0x6006B8 | 0x2006B8 | 0x600318 | 0x200318 | imm | high | imm@0x437E7/.text/ctx=1 |
| `contFrameWaitNormal` | map | 0x60049C | 0x20049C | 0x6000FC | 0x2000FC | imm | high | imm@0x437F8/.text/ctx=1 |
| `hook:MvlHookTrampoline426748` | site:0 | 0x426748 | 0x026748 | 0x426718 | 0x026718 | imm | high | imm@0x43C4A/.text/ctx=1 |
| `hook:MvlHookKeyClassBypass` | site:0 | 0x4267DE | 0x0267DE | 0x4267AE | 0x0267AE | imm | high | imm@0x43C59/.text/ctx=1 |
| `hook:MvlHookTrampoline426ac6` | site:0 | 0x426AC6 | 0x026AC6 | 0x426A96 | 0x026A96 | imm | high | imm@0x43C68/.text/ctx=1 |
| `hook:MvlHookTrampoline426c80` | site:0 | 0x426C80 | 0x026C80 | 0x426C50 | 0x026C50 | imm | high | imm@0x43C77/.text/ctx=1 |
| `hook:MvlHookTrampoline427470` | site:0 | 0x427470 | 0x027470 | 0x427440 | 0x027440 | imm | high | imm@0x43C86/.text/ctx=1 |
| `hook:MvlHookSetAiClass` | site:0 | 0x426820 | 0x026820 | 0x4267F0 | 0x0267F0 | imm | high | imm@0x43C95/.text/ctx=1 |
| `hook:MvlHookTrampoline426a4c` | site:0 | 0x426A4C | 0x026A4C | 0x426A1C | 0x026A1C | imm | high | imm@0x43CA4/.text/ctx=1 |
| `hook:MvlHookClearParts` | site:0 | 0x5BB40B | 0x1BB40B | 0x5BB23B | 0x1BB23B | imm | high | imm@0x43CB3/.text/ctx=1 |
| `hook:MvlHookTeardownParts` | site:0 | 0x5BAC04 | 0x1BAC04 | 0x5BAA34 | 0x1BAA34 | imm | high | imm@0x43CC2/.text/ctx=1 |
| `hook:MvlHookRepaint` | site:0 | 0x579290 | 0x179290 | 0x579180 | 0x179180 | imm | high | imm@0x43CD1/.text/ctx=1 |
| `hook:MvlHookPaintRandom` | site:0 | 0x579090 | 0x179090 | 0x578F80 | 0x178F80 | imm | high | imm@0x43CE0/.text/ctx=1 |
| `hook:MvlHookPaintVariant` | site:0 | 0x579190 | 0x179190 | 0x579080 | 0x179080 | imm | high | imm@0x43CEF/.text/ctx=1 |
| `hook:MvlHookPartUpdate69adc4` | site:1 | 0x69ADC4 | 0x29ADC4 | 0x699DC4 | 0x299DC4 | imm | high | imm@0x43CFE/.text/ctx=1 |
| `hook:MvlHookPartUpdate6d7b68` | site:1 | 0x6D7B68 | 0x2D7B68 | 0x6D6B38 | 0x2D6B38 | imm | high | imm@0x43D0D/.text/ctx=1 |
| `hook:MvlHookPartUpdate69b0e8` | site:1 | 0x69B0E8 | 0x29B0E8 | 0x69A0E8 | 0x29A0E8 | imm | high | imm@0x43D1C/.text/ctx=1 |
| `hook:MvlHookWheelCache69adc0` | site:1 | 0x69ADC0 | 0x29ADC0 | 0x699DC0 | 0x299DC0 | imm | high | imm@0x43D2B/.text/ctx=1 |
| `hook:MvlHookPartFlag6d7b64` | site:1 | 0x6D7B64 | 0x2D7B64 | 0x6D6B34 | 0x2D6B34 | imm | high | imm@0x43D3A/.text/ctx=1 |
| `hook:MvlHookFadePart` | site:0 | 0x5CF9F3 | 0x1CF9F3 | 0x5CF7C3 | 0x1CF7C3 | imm | high | imm@0x43D49/.text/ctx=1 |
| `hook:MvlHookFadePartAlt` | site:0 | 0x5C4341 | 0x1C4341 | 0x5C4171 | 0x1C4171 | imm | high | imm@0x43D58/.text/ctx=1 |
| `hook:MvlHookGenerateVcName` | site:0 | 0x408967 | 0x008967 | 0x408967 | 0x008967 | identical | high | imm-same@0x4477B/.text/ctx=1 |
| `hook:MvlHookAttachFixup` | site:1 | 0x69ADB0 | 0x29ADB0 | 0x699DB0 | 0x299DB0 | imm | high | imm@0x4478A/.text/ctx=1 |
