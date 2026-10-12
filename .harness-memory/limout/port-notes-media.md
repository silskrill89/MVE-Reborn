# port-notes-media.md — media lane (sound / png / ifp / emlights)

Port of the decompiled MVL 0.98e media cluster into the clone. Sources:

| Dump | Range | Lane |
|------|-------|------|
| `.harness-memory/limout/audio/range-decomp.txt` | 0x33000-0x33f10 | CSoundHandler |
| `.harness-memory/limout/png/range-decomp.txt` | 0x1bdb0-0x1ca9a | CPngFuncs / CHelpers |
| `.harness-memory/limout/ifp/range-decomp.txt` | 0x1472-0x2666 | CCustomAnimFile |
| `.harness-memory/limout/emlights/range-decomp.txt` | 0x4e8f0-0x4f230 | CVehicleHandler (EmLights) |
| `.harness-memory/d3out/strings.txt` | — | verbatim string/name evidence |

All VAs are RVAs against vehmod_retail.dll 0.98e (image base 0x10000000).
Log strings in code are verbatim (including the original's typo
`Could not bytes from stream: %s.`). Naming per AGENTS.md: original
self-identifying names 1:1 (CSoundHandler::, CPngFuncs::,
CCustomAnimFile::, CVehicleHandler::LoadXML*EmLight*), provisional names for
unnamed functions, semantic `Mvl*` names with `DAT_` RVA comments for data.

## 1. Files

| File | Contents |
|------|----------|
| `src/audio/sound_handler.{h,cpp}` | CSoundHandler + `MvlSoundLoadFile` seam definition |
| `src/render/png_funcs.{h,cpp}` | CPngFuncs, MvlPngApi proc table, `PngFileToTexture` + image seams |
| `src/anim/custom_anim.{h,cpp}` | CCustomAnimFile, IFP helpers, stream seams + `MvlAnimFileLoad` seam definition |
| `src/vehicle/vehicle_handler.{h,cpp}` | CVehicleHandler EmLight trio (added to the existing class) |
| `src/archive/vehicle_archive.cpp` | 3 one-line forwards (EmLight) + see §5 Deviations |

## 2. Function table (name / RVA / status)

Status: DONE = logic + verbatim strings ported (engine seams may still log
HOOK POINT); STUB = body unrecoverable or wholly engine-side.

### CSoundHandler (src/audio/sound_handler.cpp)

| Function | RVA | Status |
|----------|-----|--------|
| `CSoundHandler::FillInfoFromName` | 0x338a0 | DONE (OQ-1) |
| `CSoundHandler::FindSlotForName` (prov., FUN_10033770) | 0x33770 | DONE |
| `CSoundHandler::FillSampleBufferFromData` | 0x33a00 | DONE |
| `CSoundHandler::LoadSoundFile` | 0x33e10 | DONE |
| `MvlSoundLoadFile` (archive seam) | 0x33e10 | DONE (forwarder) |

### CPngFuncs / CHelpers (src/render/png_funcs.cpp)

| Function | RVA | Status |
|----------|-----|--------|
| `CPngFuncs::LoadPngFuncs` | 0x1c3f0 | DONE (dynamic LoadLibrary + 16x GetProcAddress) |
| `CPngFuncs::EnsureLoaded` (prov., FUN_1001c5b0) | 0x1c5b0 | DONE |
| `CPngFuncs::PngReadCallback` (prov., FUN_1001c5f0) | 0x1c5f0 | DONE (OQ-5) |
| `CPngFuncs::FreeReadState` (prov., FUN_1001c650) | 0x1c650 | DONE |
| `CPngFuncs::RtPNGImageStreamRead` | 0x1c690 (catch 0x1c9aa) | DONE (OQ-6) |
| `CHelpers::PngFileToTexture` (free fn, body FUN_1001bed0) | 0x1bed0 (catch 0x1bfb3) | DONE (OQ-4) |
| `MvlRwImageCreate` (FUN_100327b0) | 0x327b0 | STUB TODO(media) |
| `MvlRwImageAllocatePixels` (FUN_100327d0) | 0x327d0 | STUB TODO(media) |
| `MvlRwImageReleasePixels` (FUN_100327e0) | 0x327e0 | STUB TODO(media) |
| `MvlRwImageDelete` (FUN_100327c0) | 0x327c0 | STUB TODO(media) |
| `MvlRwImageToTexture` (FUN_1001ca90) | 0x1ca90 | STUB TODO(media) |
| `MvlPngReadCleanup` (FUN_1001ca06) | 0x1ca06 | STUB TODO(media) |

### CCustomAnimFile (src/anim/custom_anim.cpp)

| Function | RVA | Status |
|----------|-----|--------|
| `CCustomAnimFile::Load(char*)` | catch 0x1472 | STUB TODO(media) (body 0x13b0-0x1472 outside dump; "Failed to open animation file." recovered) |
| `CCustomAnimFile::Load(void*, uint32_t)` | 0x14e0 (catch 0x1595) | DONE |
| `CCustomAnimFile::Load(RwStream*)` | 0x1600 (catch 0x16ed) | DONE |
| `CCustomAnimFile::LoadVersionOne` | 0x1740 (catch 0x1ea4) | DONE (structure; game seams STUB) |
| `CCustomAnimFile::LoadVersionTwo` | 0x1f90 (catch 0x25fb) | DONE (structure; game seams STUB) |
| `MvlAnimFileLoad` (archive seam) | 0x14e0 | DONE (forwarder) |
| `MvlRwStreamRead` (FUN_10032070) | 0x32070 | STUB HOOK POINT (shared with png lane) |
| `MvlRwStreamSkip` (FUN_10032080) | 0x32080 | STUB HOOK POINT |
| `MvlIfpReadRaw` (FUN_100027b0) | 0x27b0 | DONE (verbatim "Could not bytes from stream: %s.") |
| `MvlIfpReadName24` (FUN_10002810) | 0x2810 | DONE |
| `MvlIfpReadChunkId` (FUN_10002830) | 0x2830 | DONE (OQ-3) |
| `MvlIfpReadChunkHeader` (FUN_10002900) | 0x2900 | STUB TODO(media) (OQ-7; both strings recovered) |
| `MvlIfpReadString` (FUN_100029d0) | 0x29d0 | STUB TODO(media) (OQ-8; string recovered) |
| `MvlAnimAllocAnim` (FUN_10007360/0x7470) | 0x7360 | STUB TODO(media) |
| `MvlAnimAllocHierarchy` (FUN_100075b0) | 0x75b0 | STUB TODO(media) |
| `MvlAnimAllocSeqArray` (FUN_10058e9a+ctor) | 0x58e9a/0x7660 | STUB TODO(media) |
| `MvlAnimSeqSetInfo` (FUN_10007710) | 0x7710 | STUB TODO(media) |
| `MvlAnimSeqSetExtra` (FUN_10007700) | 0x7700 | STUB TODO(media) |
| `MvlAnimSeqAllocFrames` (FUN_100076f0) | 0x76f0 | STUB TODO(media) |
| `MvlAnimQuatInit` (FUN_100080f0) | 0x80f0 | STUB TODO(media) |
| `MvlAnimQuatDecode` (FUN_100084e0) | 0x84e0 | STUB TODO(media) |
| `MvlAnimQuatDecodeCompressed` (FUN_10008110) | 0x8110 | STUB TODO(media) |
| `MvlAnimVectorDecode` (FUN_10008c50) | 0x8c50 | STUB TODO(media) |
| `MvlAnimSeqFinalise` (FUN_10007630) | 0x7630 | STUB TODO(media) |
| `MvlAnimSeqPost` (FUN_10007620) | 0x7620 | STUB TODO(media) |
| `MvlAnimModelOneAvailable` | 0x1740 call site | STUB TODO(media) (DAT_100a6314+4 / DAT_100a631c+0x1c) |
| `MvlAnimModelOneGetKeyFrame` | 0x1740 vtable +0xc | STUB TODO(media) |
| `MvlAnimBindKeyFrame` (FUN_1002c1d0) | 0x2c1d0 | STUB TODO(media) |
| `MvlAnimHierarchyBindPrep` (FUN_100075a0) | 0x75a0 | STUB TODO(media) |
| `MvlAnimClumpIsReady` (FUN_10031240) | 0x31240 | STUB TODO(media) |
| `MvlAnimFrameClose` (FUN_10002ce0) | 0x2ce0 | STUB TODO(media) |
| `MvlAnimV2RotScale` | DAT_1008dd9c | STUB TODO(media) (value unrecovered) |
| `MvlAnimV2TimeScale` | DAT_1008ddb4 | STUB TODO(media) (value unrecovered) |

Reused (NOT duplicated) from `src/archive/vehicle_archive.h`:
`MvlRwMemoryStreamOpen` 0x320a0, `MvlRwMemoryStreamClose` 0x32090,
`MvlVectorZero` 0x8c30, `MvlVectorCopy` 0x8c90, `MvlRwClumpPostRead`
0x312c0, `MvlWeaponClumpRegister` 0x31450.

### CVehicleHandler EmLight trio (src/vehicle/vehicle_handler.cpp)

| Function | RVA | Status |
|----------|-----|--------|
| `CVehicleHandler::LoadXMLEmLightData` | 0x4e8f0 | DONE |
| `CVehicleHandler::LoadXMLSingleEmLightData` | 0x4edc0 | DONE |
| `CVehicleHandler::LoadXMLTaxiEmLightData` | 0x4f090 | DONE |

Note: the archive-side forwarders keep their earlier names
(`LoadXMLEmLightData` / `LoadXMLEmLightSingle` / `LoadXMLTaxiLight`, decls in
vehicle_archive.h:117-119) and one-line-forward to the recovered names.

## 3. Layout maps

### Sound slot record (0x14 bytes, `SMvlSoundSample`)

	+0x00 buffer ptr   +0x04 buffer bytes   +0x08 sample rate
	+0x0c loop start   +0x10 loop end (-1 => bufferBytes-2)

Stereo couple = two consecutive records (ch0 / ch1); the caller block is
`CMvlVehicle::m_abSoundSlots[2][20]` @ 0x2cf8. File-name grammar:
`<slotname>[_s<loopStart>e<loopEnd>].wav`; slot match on the base name up to
`_s`/`.`; an adjacent `<base>-r`/`<base>-l` name pair selects stereo.
Loop offsets are byte offsets into the channel-0 buffer.

### EmLight block (0x44 bytes, `SMvlEmLightData`, `m_abEmLightData` @ 0x27f4)

	+0x00 [3] enabled flags (0 emlights, 1 emlightsingle, 2 taxilight)
	+0x03 alwayson (shared)
	+0x04 posleft[3]     +0x10 posright[3]    (emlights)
	+0x1c position[3]                       (emlightsingle)
	+0x28 position[3]                       (taxilight)
	+0x34 colourone[3]   +0x38 colourtwo[3]  (emlights)
	+0x3c colour[3]                         (emlightsingle)
	+0x40 colour[3]                         (taxilight)

colourone named values: `primary` = (60,255,0), `secondary` = (255,0,175);
otherwise `r,g,b` with per-channel clamp to 255 (all three parsers).

### CCustomAnimFile / game anim objects

	CCustomAnimFile: +0x108 apAnims (SMvlAnimObject**), +0x10c nAnimCount (<=500)
	SMvlAnimHierarchy (0x28): +0x00 name[24], +0x18 pSeqArray, +0x1c nSeqCount (<=50)
	SMvlAnimObject: +0x0e/+0x2c/+0x2e u16 fields set 0xffff after bind, +0x14 pHierarchy
	SMvlAnimSequence (0x30): +0x08 name[24], +0x20 nFrameCount (<=10000),
	                         +0x28 pFrames (0x14-byte KR00 / 0x20-byte KRT0/KRTS frames)

Frame blocks: `KRTS` rot(0x10)+trans(0xc)+scale(skip 0xc), `KRT0`
rot(0x10)+trans(0xc), `KR00` rot(0x10) — each capped by a 4-byte rot/time
word (v1). ANP3 (v2) frames: 10-byte "Frame rotation and time" (+ 6-byte
"Frame translation" for bone type 4).

## 4. Open questions

- **OQ-1** FillInfoFromName: the decomp's "Loop offset end must be -1 or
  higher." test reads `param_2+0xc` (loop START) — unreachable after the
  earlier start>=0 check. Ported against `+0x10` (loop END) to match the
  message. Awaiting disasm confirmation.
- **OQ-2** `MvlSoundLoadFile` call site (vehicle_archive.cpp:249) passes the
  literal `"rcengine1"` where the original passes the name TABLE
  (PTR_s_rcengine1_100af104). The seam contract is the table; the call site
  is outside this lane's scope.
- **OQ-3** FUN_10002830 expected-fourcc selectors (DAT_1007a6e8/1007a6f0/
  1007a73c/1007a744) are 4-char .rdata literals NOT present in
  d3out/strings.txt (short-string filter). Ported as NULL (no validation);
  call identity kept in comments.
- **OQ-4** `PngFileToTexture` prototype is `(void)` in the decomp; the body
  opens a memory stream, so `(data, size)` is used (provisional).
- **OQ-5** PNG read callback: the decomp's FUN_10032070 call has dropped
  args; the stream handle is taken from the callback's first argument as
  recovered. The io_ptr passing convention of the vendored libpng15 build
  needs confirmation.
- **OQ-6** RtPNGImageStreamRead default case tests `local_c4`
  (interlace_type) against 0x1f — dead in the PNG color-type domain.
  Ported as recovered.
- **OQ-7** FUN_10002900 info-section wire layout partial: element count at
  hdr+8 recovered (FUN_10001740 use); the section size field feeding
  FUN_100029d0/reads is assumed at hdr+4. Strings recovered: "Animation
  info section contents must be at least 5 bytes, this is %d bytes." /
  "Failed to read element count from animation info section."
- **OQ-8** FUN_100029d0 string wire format (length prefix vs NUL) not
  recovered. String recovered: "Buffer is too small to load animation file
  string."
- **OQ-9** v1/v2 frame read targets (FUN_10032070/FUN_100027b0 dst args) are
  dropped in the decomp; recovered scratch layout used in the port.

## 5. Deviations (flagged to orchestrator)

1. `src/archive/vehicle_archive.cpp` gained `#include "../vehicle/vehicle_handler.h"`
   — required for the 3 one-line forwards to reach `CVehicleHandler::`.
2. The old stub BODIES of `MvlAnimFileLoad` / `MvlSoundLoadFile`
   (vehicle_archive.cpp ~864-883) were replaced by pointer comments: their
   definitions now live in custom_anim.cpp / sound_handler.cpp per the REUSE
   directive, and keeping both would be a duplicate-symbol link error
   (premake globs `src/**.*`). Everything else in the file is untouched.
3. Vector parsing reuses `MvlParseVector` (FUN_1001bdb0, defined in
   src/vehicle/vehicle_xml.cpp) — the exact original callee of the EmLight
   parsers — not `XmlParseVector` (same VA but comma-text sibling in
   src/config/xml_helpers.cpp). No new parser written.

## 6. Naming / evidence notes

- EmLight function names come from the functions' own error-log identities
  (strings.txt `CVehicleHandler::LoadXMLEmLightData` @ 0x1008b88c,
  `CVehicleHandler::LoadXMLSingleEmLightData` @ 0x1008bcb4,
  `CVehicleHandler::LoadXMLTaxiEmLightData` @ 0x1008bea8). The archive-side
  decl names (LoadXMLEmLightSingle/LoadXMLTaxiLight) predate this recovery.
- ANP3 field labels ("Animation block name", "Number of animations",
  "Animation name", "Number of sequences", "Frame data length", "Unknown
  animation field", "Animation sequence name", "Animation sequence type",
  "Number of frames in a sequence", "Bone index of a sequence", "Frame
  rotation and time", "Frame translation" — strings.txt 0x1007a918..
  0x1007aa6c) are used verbatim as the field-name args of the read helpers.
- Compiler-generated SEH epilogues (FUN_100014a7/0x15c7/0x171b/0x1f11,
  Catch@*) are not ported as functions; their cleanup behaviour is folded
  into the owning function's tail/catch (as in earlier lanes).
