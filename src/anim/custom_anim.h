// custom_anim.h — CCustomAnimFile: GTA .ifp (ANPK/ANP3) custom anim loader.
//
// Faithful recompilation from decompiled vehmod_retail.dll 0.98e (decomp
// source: .harness-memory/limout/ifp/range-decomp.txt, cluster
// 0x1472-0x2666). Every function carries its origin VA; original symbol
// names are reused 1:1 (error-string evidence: "bool CCustomAnimFile::
// Load(char* pszFileName)", "CCustomAnimFile::LoadVersionOne", ...).
//
// The archive pipeline reaches this module through the seam MvlAnimFileLoad
// (declared in src/archive/vehicle_archive.h, DEFINED in custom_anim.cpp):
// CMvlVehicle::LoadAnimationFile hands it the ".ifp" archive entry bytes.
//
// Format (recovered from the loaders' fourcc dispatch):
//   "ANPK" -> LoadVersionOne (GTA III/VC-era container)
//   "ANP3" -> LoadVersionTwo (compressed keyframe container)
//   per-sequence frame blocks: "KRTS" rot+trans+scale / "KRT0" rot+trans /
//   "KR00" rot-only. Limits: 500 anims, 50 sequences per hierarchy,
//   10000 frames per sequence.
#pragma once

#include "../core/stdafx.h"

#include <stdint.h>

struct RwStream;   // engine stream type (opaque; engine lane)

// ---------------------------------------------------------------------------
// Game anim-object field views — (provisional) names; only the offsets the
// loaders touch are recovered (FUN_10001740/FUN_10001f90 accesses).
// ---------------------------------------------------------------------------
struct SMvlAnimHierarchy   // FUN_100075b0 @ 0x75b0 — 0x28-byte game object
{
	char          szName[24];     // @ 0x00 (MvlIfpReadName24/MvlIfpReadString
	                              //        target — hierarchy name)
	void         *pSeqArray;      // @ 0x18 (count-prefixed 0x30-byte records)
	int16_t       nSeqCount;      // @ 0x1c
	unsigned char pad1e[0x0a];    // @ 0x1e..0x27 unrecovered
};

struct SMvlAnimObject      // FUN_10007360 @ 0x7360 / FUN_10007470 @ 0x7470
{
	unsigned char      pad00[0x0e]; // @ 0x00..0x0d unrecovered
	uint16_t           nField0e;    // @ 0x0e set to 0xffff after bind
	unsigned char      pad10[0x04]; // @ 0x10..0x13 unrecovered
	SMvlAnimHierarchy  *pHierarchy; // @ 0x14
	unsigned char      pad18[0x14]; // @ 0x18..0x2b unrecovered
	uint16_t           nField2c;    // @ 0x2c set to 0xffff after bind
	uint16_t           nField2e;    // @ 0x2e set to 0xffff after bind
};

struct SMvlAnimSequence    // (provisional) — 0x30-byte sequence record
{
	unsigned char pad00[0x08];      // @ 0x00..0x07 unrecovered
	char          szName[24];       // @ 0x08 (MvlIfpReadName24 target)
	int32_t       nFrameCount;      // @ 0x20 (loop bound in both loaders)
	unsigned char pad24[0x04];      // @ 0x24..0x27 unrecovered
	void         *pFrames;          // @ 0x28 (0x14-byte KR00 / 0x20-byte
	                                //        KRT0/KRTS frames)
	unsigned char pad2c[0x04];      // @ 0x2c..0x2f (FUN_10007700 target)
};

// ---------------------------------------------------------------------------
// CCustomAnimFile — recovered names 1:1. Object layout: only +0x108/+0x10c
// are recovered from the loaders (FUN_100012b0 ctor is the engine lane).
// ---------------------------------------------------------------------------
class CCustomAnimFile
{
public:
	// bool CCustomAnimFile::Load(char* pszFileName) — catch @ RVA 0x1472.
	// BODY OUT OF DECOMP RANGE (only the catch filter + cleanup tail
	// FUN_100014a7 @ 0x14a7 were recovered) — STUB.
	bool Load(char *pszFileName);

	// bool CCustomAnimFile::Load(void* pBuffer, uint32_t uBufferLength) —
	// FUN_100014e0 @ RVA 0x14e0, catch @ RVA 0x1595.
	bool Load(void *pBuffer, uint32_t uBufferLength);

	// bool CCustomAnimFile::Load(RwStream* pStream) — FUN_10001600 @ RVA
	// 0x1600, catch @ RVA 0x16ed. Signature dispatch ANPK/ANP3.
	bool Load(RwStream *pStream);

	// CCustomAnimFile::LoadVersionOne — FUN_10001740 @ RVA 0x1740, catch
	// @ RVA 0x1ea4. ANPK container parser.
	bool LoadVersionOne(RwStream *pStream);

	// CCustomAnimFile::LoadVersionTwo — FUN_10001f90 @ RVA 0x1f90, catch
	// @ RVA 0x25fb. ANP3 container parser.
	bool LoadVersionTwo(RwStream *pStream);

	unsigned char   pad00[0x108];   // @ 0x00..0x107 ctor-owned state
	SMvlAnimObject **apAnims;       // @ 0x108 anim object ptr array
	int32_t         nAnimCount;     // @ 0x10c anim count (<= 500)
};

// ---------------------------------------------------------------------------
// Engine / IFP seams. Stubs log "TODO(media)" once via MvlErrorLog and
// return benign values; game behaviour is NOT faked. MvlRwMemoryStreamOpen/
// Close, MvlVectorZero/Copy, MvlRwClumpPostRead and
// MvlWeaponClumpRegister are REUSED from src/archive/vehicle_archive.h.
// ---------------------------------------------------------------------------

// Shared engine stream seams (used by this lane and by
// src/render/png_funcs.cpp). DEFINED in custom_anim.cpp.
unsigned MvlRwStreamRead(void *stream, void *dst, unsigned len);  // FUN_10032070 @ 0x32070
unsigned MvlRwStreamSkip(void *stream, unsigned len);             // FUN_10032080 @ 0x32080

// IFP chunk helpers (provisional names — FUN_100027b0 family). Diagnostics
// are verbatim-recovered from .harness-memory/d3out/strings.txt; the wire
// formats behind FUN_10002900/FUN_100029d0 are only partially recovered
// (see port-notes-media.md OQ-7/OQ-8):
unsigned MvlIfpReadRaw(void *stream, void *dst, unsigned size,
                       const char *fieldName);                  // FUN_100027b0 @ 0x27b0
unsigned MvlIfpReadName24(void *stream, void *dst,
                          const char *fieldName);               // FUN_10002810 @ 0x2810
unsigned MvlIfpReadChunkId(void *stream, void *id4,
                           const char *expectFourcc);           // FUN_10002830 @ 0x2830
unsigned MvlIfpReadChunkHeader(void *stream, void *hdr);        // FUN_10002900 @ 0x2900
unsigned MvlIfpReadString(void *stream, void *dst, unsigned bufSize,
                          unsigned sectionSize);                // FUN_100029d0 @ 0x29d0

// Game anim-object model (engine lane; FUN_10007360 cluster):
void *MvlAnimAllocAnim(void);                                     // TODO(media): 0x7360/0x7470
void *MvlAnimAllocHierarchy(void);                                // TODO(media): 0x75b0
void *MvlAnimAllocSeqArray(int count);                            // TODO(media): 0x58e9a+0x7660
void  MvlAnimSeqSetInfo(void *seq, const void *info);             // TODO(media): 0x7710
void  MvlAnimSeqSetExtra(void *seq, int16_t extra);               // TODO(media): 0x7700
void  MvlAnimSeqAllocFrames(void *seq);                           // TODO(media): 0x76f0
void  MvlAnimQuatInit(void *quat);                                // TODO(media): 0x80f0
void  MvlAnimQuatDecode(void *quat);                              // TODO(media): 0x84e0
void *MvlAnimQuatDecodeCompressed(void *dst, float f);            // TODO(media): 0x8110
void *MvlAnimVectorDecode(void *dst);                             // TODO(media): 0x8c50
void  MvlAnimSeqFinalise(void *hier);                             // TODO(media): 0x7630
void  MvlAnimSeqPost(void);                                       // TODO(media): 0x7620
bool  MvlAnimModelOneAvailable(void);                             // TODO(media): 0x1740 (DAT_100a6314+4)
void *MvlAnimModelOneGetKeyFrame(void);                           // TODO(media): 0x1740 (vtable +0xc)
void  MvlAnimBindKeyFrame(void *kf);                              // TODO(media): 0x2c1d0
void  MvlAnimHierarchyBindPrep(void);                             // TODO(media): 0x75a0
bool  MvlAnimClumpIsReady(void);                                  // TODO(media): 0x31240
void  MvlAnimFrameClose(void);                                    // TODO(media): 0x2ce0
float MvlAnimV2RotScale(void);                                    // TODO(media): DAT_1008dd9c
float MvlAnimV2TimeScale(void);                                   // TODO(media): DAT_1008ddb4

// Archive seam — DEFINED in custom_anim.cpp (declared in
// src/archive/vehicle_archive.h:83); forwards to CCustomAnimFile::Load.
// bool MvlAnimFileLoad(CCustomAnimFile *af, const void *data, unsigned size);
