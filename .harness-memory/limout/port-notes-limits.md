# port-notes — CMvlSettings limits processors (0.98e)

Source: `.harness-memory/limout/decomp-chain.txt` (Ghidra). Binary: `vehmod_retail.dll`
0.98e, image base 0x10000000 (RVA = VA - 0x10000000). Game-side VAs are VC 1.0 exe.

## Game address constants spotted (writes NOT ported yet)

- `ProcessStreamingMemLimit` @ RVA 0x2ba50: stores `streamingmem << 20` (MB->bytes) to
  VC 1.0 game VA 0x0041079f and mirrors it to game VA 0x0094dd54. Global source
  `DAT_100a6a8c` (-1 = unset). Bounds: <65 / >1900 MB are ignored (not clamped).
- `ProcessPoolLimits` @ RVA 0x2b4d0: engine pool-size pointer table @ DLL VA 0x100ae6a0
  (10 entries); min table @ VA 0x100ae70c; max table @ VA 0x100ae6e4. If a table slot is
  a negative pointer (0x80000000|addr), the original patches the code at addr to
  `6A 00 68 <limit-le32>` (word 0x006a, byte 0x68, dword limit) instead of writing a
  variable — i.e. push-immediate patch sites in the VC exe (addresses inside table,
  not dumped).
- `ProcessModelStoreLimits` @ RVA 0x2b5a0: min @ VA 0x100ae940, max @ VA 0x100ae978,
  element size @ VA 0x100ae76c, per-element ctor fn ptr @ VA 0x100ae750, store list
  heads @ VA 0x100ae788. Reallocs `_malloc(elemsize*limit + 4)` then calls the ctor
  `limit` times and relinks list heads.
- `ProcessIdLimits` @ RVA 0x2b8f0: caps 200000 (models) / 50000 (TXD) / 1000 (COL);
  long-model threshold 0x7fff on `texdicts + 100 + colfiles + models` (+100 = ANIM);
  dirty flag @ VA 0x100b178f; long-model flag @ VA 0x100b0c41; engine ID boundaries
  @ VA 0x100a6320 (model end) / 0x100a6328 (TXD end) / 0x100a6318 (COL end);
  calls FUN_100173d0 (RVA 0x173d0) -> FUN_10017390 (RVA 0x17390); rebuild
  FUN_10017820 (RVA 0x17820).

## Storage globals (now struct members)

- instances: VA 0x100b1794..0x100b17b8 (10 ints, order ptrnode, entryinfonode, peds,
  vehicles, buildings, treadables, objects, dummys, audioscriptobj, colmodel)
- modelstore: VA 0x100b17c0..0x100b17d8 (7 ints: simple, time, weapon, clump, vehicle,
  ped, fx2d)
- ids: models @ VA 0x100b17e4, texdicts @ 0x100b17e0, colfiles @ 0x100b17bc
- streamingmem: VA 0x100a6a8c

## Exact original log strings (mirrored)

- "Pool %s limit below minimum %d." / "Pool %s limit above maximum %d."
- "Model store %s limit below minimum %d." / "Model store %s limit above maximum %d."
- "Requested model limit over 200000, reducing back to 200000."
- "Requested TXD limit over 50000, reducing back to 50000."
- "Requested COL limit over 1000, reducing back to 1000."
- "Model, TXD, COL and ANIM limits totalling over 32767, switching to experimental
  long model ID mode."
- "Ignoring streaming memory limit below 65MB." / "... above 1900MB."
- "Could not load '%s' sector." (LoadMXMLSectionLimits @ RVA 0x2b030)

## Unportable / TODO(engine)

- Pool min/max descriptor tables (VA 0x100ae70c / 0x100ae6e4) and model-store
  min/max/elemsize/ctor tables: CONTENTS live in the DLL .data section, not in the
  decomp chain. Clone uses INT_MIN/INT_MAX placeholders (never clamps) until dumped.
- Engine pool writes / push-immediate patches (table VAs above; VC exe addresses
  inside the tables).
- Model-store realloc + ctor + list relink (VA 0x100ae788).
- Streaming-mem writes to VC 1.0 VAs 0x0041079f / 0x0094dd54.
- CLongModelId state calls (RVA 0x173d0/0x17390/0x17820).
- Engine ID boundaries (VA 0x100a6320/0x100a6328/0x100a6318) default 0 in clone until
  engine init fills them.

## Open questions

- modelstore last entry name string @ VA 0x1007eb20 not dumped: <fx2d> (samples) vs
  "2dfx" (version matrix). Clone accepts both, fx2d first.
- "peds" name string assumed @ VA 0x1007ea78 (content not dumped; order from sample XML).
- 10th pool name assumed "colmodel" (names table for %s not dumped).
- DAT_100a6320/28/18 semantics inferred as model/TXD/COL end boundaries from
  FUN_10017410 usage (DAT_100a6318 used as streaming-buffer count).
