// custom_anim.cpp — CCustomAnimFile implementation (see custom_anim.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/ifp/range-decomp.txt; helper diagnostics from
// .harness-memory/d3out/strings.txt). Log strings are verbatim from the
// decomp. The archive seam MvlAnimFileLoad (declared in
// src/archive/vehicle_archive.h) is DEFINED at the bottom of this file —
// that is how CMvlVehicle::LoadAnimationFile hooks in.
//
// Exception idiom: the original throws through FUN_1001b8d0 +
// __CxxThrowException and each Load catch filter logs "fn" "%s"; the clone
// mirrors it with SMvlAnimError + AnimThrow (same idiom as
// src/archive/vehicle_archive.cpp).
#include "custom_anim.h"

#include "../archive/vehicle_archive.h"   // MvlRwMemoryStream* + vector/clump seams
#include "../loader/log.h"

#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// x86 target (VC 1.0 plugin).
// ---------------------------------------------------------------------------
#if !defined(_M_IX86)
#error "MVL clone: custom_anim.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// Log wrappers — FUN_1001cc00 msg / FUN_1001cb80 error (fn prefixed).
// ---------------------------------------------------------------------------
static void
AnimErr(const char *fn, const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "%s: %s", fn, msg);
	buf[sizeof(buf) - 1] = '\0';
	MvlErrorLog("%s", buf);
}

struct SMvlAnimError
{
	char msg[512];
};

static void
AnimThrow(const char *fmt, ...)
{
	SMvlAnimError e;
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(e.msg, sizeof(e.msg), fmt, ap);
	va_end(ap);
	e.msg[sizeof(e.msg) - 1] = '\0';
	throw e;
}

// Unrecoverable-callee stubs: log once via MvlErrorLog, benign return.
static void
MediaTodoOnce(bool *logged, const char *what)
{
	if(*logged)
		return;
	*logged = true;
	MvlErrorLog("%s", what);
}

// ===========================================================================
// Engine stream seams (shared with src/render/png_funcs.cpp).
// ===========================================================================
unsigned
MvlRwStreamRead(void *stream, void *dst, unsigned len)
{
	// FUN_10032070 @ 0x32070 — engine RwStream read. HOOK POINT: engine
	// lane owns the real stream; nothing is read or written here.
	MvlLog("HOOK POINT 0x32070 MvlRwStreamRead");
	(void)stream; (void)dst; (void)len;
	return 0;
}

unsigned
MvlRwStreamSkip(void *stream, unsigned len)
{
	// FUN_10032080 @ 0x32080 — engine RwStream skip. HOOK POINT.
	MvlLog("HOOK POINT 0x32080 MvlRwStreamSkip");
	(void)stream; (void)len;
	return 0;
}

// ===========================================================================
// IFP chunk helpers — FUN_100027b0 family (provisional names; diagnostics
// verbatim-recovered, see port-notes-media.md OQ-7/OQ-8).
// ===========================================================================

// FUN_100027b0 @ RVA 0x27b0 — raw field read. On short read the original
// throws "Could not bytes from stream: %s." (sic, verbatim). The field-name
// argument is a dropped immediate in the decomp; recovered from the format
// string's own use (ANP3 field labels).
unsigned
MvlIfpReadRaw(void *stream, void *dst, unsigned size, const char *fieldName)
{
	unsigned got = MvlRwStreamRead(stream, dst, size);
	if(got != size)
		AnimThrow("Could not bytes from stream: %s.",
		          fieldName ? fieldName : "");
	return got;
}

// FUN_10002810 @ RVA 0x2810 — 24-byte name-field read (all recovered call
// sites target 24-byte name slots).
unsigned
MvlIfpReadName24(void *stream, void *dst, const char *fieldName)
{
	return MvlIfpReadRaw(stream, dst, 0x18, fieldName);
}

// FUN_10002830 @ RVA 0x2830 — section-header fourcc read + validation.
// expectFourcc NULL = no expectation (the Load(char*)/Load(RwStream*)
// signature probe passes 0 in the decomp). Recovered diagnostics:
//   "Could not read animation section header named %s."
//   "Animation section header FOURCC is %s, should be %s."
unsigned
MvlIfpReadChunkId(void *stream, void *id4, const char *expectFourcc)
{
	char actual[5], expect[5];
	unsigned got;

	got = MvlRwStreamRead(stream, id4, 4);
	if(got != 4)
		AnimThrow("Could not read animation section header named %s.",
		          expectFourcc ? expectFourcc : "");

	if(expectFourcc != 0){
		memcpy(actual, id4, 4);
		actual[4] = '\0';
		memcpy(expect, expectFourcc, 4);
		expect[4] = '\0';
		if(strncmp(actual, expect, 4) != 0)
			AnimThrow("Animation section header FOURCC is %s, should be %s.",
			          actual, expect);
	}
	return got;
}

// FUN_10002900 @ RVA 0x2900 — "animation info section" read: validates the
// section length ("Animation info section contents must be at least 5
// bytes, this is %d bytes.") and extracts the element count
// ("Failed to read element count from animation info section.").
// STUB: the wire layout of the info section is only partially recovered
// (count lands at hdr+8 per the FUN_10001740 uses; see OQ-7) — TODO(media).
unsigned
MvlIfpReadChunkHeader(void *stream, void *hdr)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged,
	              "TODO(media): MvlIfpReadChunkHeader @ 0x2900 (info-section wire format unrecovered)");
	(void)stream;
	// Benign: write nothing; callers treat the count/size as 0 (zeroed
	// buffers) so the parse loops stay empty.
	(void)hdr;
	return 0;
}

// FUN_100029d0 @ RVA 0x29d0 — animation-file string read into a fixed
// buffer ("Buffer is too small to load animation file string.").
// STUB: length-prefix/NUL handling unrecovered (OQ-8) — TODO(media).
unsigned
MvlIfpReadString(void *stream, void *dst, unsigned bufSize,
                 unsigned sectionSize)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged,
	              "TODO(media): MvlIfpReadString @ 0x29d0 (string wire format unrecovered)");
	(void)stream;
	(void)dst;
	(void)bufSize;
	(void)sectionSize;
	return 0;
}

// ===========================================================================
// bool CCustomAnimFile::Load(char* pszFileName) — catch filter @ RVA 0x1472.
// BODY OUT OF DECOMP RANGE (only the catch + cleanup tail FUN_100014a7 @
// 0x14a7 were recovered; body diagnostics recoverable from strings.txt:
// "Failed to open animation file." @ 0x13b0). STUB.
// ===========================================================================
bool
CCustomAnimFile::Load(char *pszFileName)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged,
	              "TODO(media): CCustomAnimFile::Load(char*) body outside decomp range (0x13b0-0x1472; \"Failed to open animation file.\")");
	(void)pszFileName;
	return false;
}

// ===========================================================================
// bool CCustomAnimFile::Load(void* pBuffer, uint32_t uBufferLength) —
// FUN_100014e0 @ RVA 0x14e0, catch filter @ 0x1595.
// ===========================================================================
bool
CCustomAnimFile::Load(void *pBuffer, uint32_t uBufferLength)
{
	static const char *const fn =
		"bool CCustomAnimFile::Load(void* pBuffer, uint32_t uBufferLength)";
	void *stream = 0;
	bool ok = true;

	try{
		stream = MvlRwMemoryStreamOpen(pBuffer, uBufferLength);  // FUN_100320a0
		if(stream == 0)
			AnimThrow("Failed to open animation memory buffer.");
		if(!Load((RwStream *)stream))                           // FUN_10001600
			AnimThrow("Failed to read animation from memory.");
	}catch(const SMvlAnimError &e){
		AnimErr(fn, "%s", e.msg);
		ok = false;
	}

	if(stream != 0)
		MvlRwMemoryStreamClose(stream);                          // FUN_100015c7 tail
	return ok;
}

// ===========================================================================
// bool CCustomAnimFile::Load(RwStream* pStream) — FUN_10001600 @ RVA 0x1600,
// catch filter @ 0x16ed. Signature dispatch ANPK -> LoadVersionOne,
// ANP3 -> LoadVersionTwo.
// ===========================================================================
bool
CCustomAnimFile::Load(RwStream *pStream)
{
	static const char *const fn = "bool CCustomAnimFile::Load(RwStream* pStream)";
	char sig[8];
	bool ok = true;

	memset(sig, 0, sizeof(sig));
	try{
		MvlIfpReadChunkId(pStream, sig, 0);      // FUN_10002830 (no expectation)
		if(strncmp(sig, "ANPK", 4) == 0){
			if(!LoadVersionOne(pStream))          // FUN_10001740
				AnimThrow("Failed to load ANPK animation file.");
		}else{
			if(strncmp(sig, "ANP3", 4) != 0)
				AnimThrow("Unknown animation file signature.");
			if(!LoadVersionTwo(pStream))          // FUN_10001f90
				AnimThrow("Failed to load ANP3 animation file.");
		}
	}catch(const SMvlAnimError &e){
		AnimErr(fn, "%s", e.msg);
		ok = false;
	}
	return ok;
}

// ===========================================================================
// CCustomAnimFile::LoadVersionOne — FUN_10001740 @ RVA 0x1740, catch filter
// @ RVA 0x1ea4 ("CCustomAnimFile::LoadVersionOne" "%s").
// ANPK container: per anim a name section + info section (sequence count),
// per sequence an info block (name/frames/bone) and a KRTS/KRT0/KR00 frame
// block. Limits: 500 anims / 50 sequences / 10000 frames.
// ===========================================================================
bool
CCustomAnimFile::LoadVersionOne(RwStream *pStream)
{
	static const char *const fn = "CCustomAnimFile::LoadVersionOne";
	// Chunk-header view for MvlIfpReadChunkHeader: count @ +8 recovered
	// (FUN_10001740 reads local_1a8 = hdr+8); size @ +4 is the assumed
	// "section size" source (OQ-7) — the stub writes nothing, so both
	// read 0 today (benign).
	struct SIfpChunkHeader { uint32_t nFourCC; uint32_t nSize; uint32_t nCount; };

	SMvlAnimObject *anim = 0;
	unsigned nRead = 0;               // byte accounting (decomp running sums)
	char id4[4];
	unsigned char info[0x2c];         // sequence info block (0x2c max)
	float afTrans[3];

	try{
		SIfpChunkHeader hdr;
		uint32_t nAnims, i;

		memset(&hdr, 0, sizeof(hdr));
		nRead += MvlIfpReadChunkHeader(pStream, &hdr);
		nAnims = hdr.nCount;
		if(nAnims > 500)
			AnimThrow("Animation file claims to contain over 500 animations.");

		nAnimCount = (int32_t)nAnims;
		apAnims = (SMvlAnimObject **)malloc((size_t)nAnims * 4);  // FUN_10058e9a
		if(apAnims == 0)
			return false;             // CLONE-GUARD: allocation failure
		for(i = 0; i < nAnims; i++)
			apAnims[i] = 0;

		for(i = 0; i < (uint32_t)nAnimCount; i++){
			SMvlAnimHierarchy *hier;
			SIfpChunkHeader hdr2;
			uint32_t nSeqs, j;

			anim = (SMvlAnimObject *)MvlAnimAllocAnim();          // FUN_10007360/0x7470
			if(anim == 0)
				return false;         // CLONE-GUARD: TODO seam returns 0
			apAnims[i] = anim;
			hier = (SMvlAnimHierarchy *)MvlAnimAllocHierarchy();  // FUN_100075b0
			if(hier == 0)
				return false;         // CLONE-GUARD: TODO seam returns 0
			memset(hier, 0, sizeof(*hier));
			anim->pHierarchy = hier;

			// section header + hierarchy name + section header + info.
			// expect fourcc selectors = DAT_1007a6e8/DAT_1007a6f0 (4-char
			// literals not recovered from the decomp — OQ-3): NULL passed
			// (no validation), call identity preserved.
			nRead += MvlIfpReadChunkId(pStream, id4, 0);         // DAT_1007a6e8
			nRead += MvlIfpReadString(pStream, hier->szName, 0x18,
			                          hdr.nSize);                // FUN_100029d0
			nRead += MvlIfpReadChunkId(pStream, id4, 0);         // DAT_1007a6f0

			memset(&hdr2, 0, sizeof(hdr2));
			nRead += MvlIfpReadChunkHeader(pStream, &hdr2);
			nSeqs = hdr2.nCount;
			if(nSeqs > 0x32)
				AnimThrow("Animation hierarchy %d (%s) claims to contain over 50 sequences.",
				          (int)i, hier->szName);
			hier->nSeqCount = (int16_t)nSeqs;

			hier->pSeqArray = MvlAnimAllocSeqArray((int)nSeqs);  // FUN_10058e9a+0x7660
			if(hier->pSeqArray == 0)
				return false;         // CLONE-GUARD: TODO seam returns 0

			for(j = 0; j < (uint32_t)hier->nSeqCount; j++){
				SMvlAnimSequence *seq =
					(SMvlAnimSequence *)((unsigned char *)hier->pSeqArray +
					                     (size_t)j * 0x30);
				unsigned want, got, k;
				uint32_t nFrames;
				bool bKRTS = false, bKRT0 = false;

				// expect fourcc selectors = DAT_1007a73c/DAT_1007a744
				// (unrecovered literals — OQ-3).
				nRead += MvlIfpReadChunkId(pStream, id4, 0);     // DAT_1007a73c
				nRead += MvlIfpReadChunkId(pStream, id4, 0);     // DAT_1007a744

				// Info block: min(sectionSize, 0x2c) bytes, skip overhead.
				want = hdr2.nSize > 0x2c ? 0x2c : hdr2.nSize;
				got = MvlRwStreamRead(pStream, info, want);
				nRead += got;
				if(got != want)
					AnimThrow("Failed to read animation sequence %d,%d(%s) info section, req %d, read %d.",
					          (int)i, (int)j, (const char *)info, want, got);
				if(want < hdr2.nSize){
					if(MvlRwStreamSkip(pStream, hdr2.nSize - want) == 0)
						AnimThrow("Failed to skip struct overhead.");
					nRead += hdr2.nSize - want;
				}

				MvlAnimSeqSetInfo(seq, info);                     // FUN_10007710
				if(hdr2.nSize == 0x2c)
					MvlAnimSeqSetExtra(seq, (int16_t)*(uint32_t *)(info + 0x20));

				nFrames = *(uint32_t *)(info + 0x1c);
				if(nFrames != 0){
					if(nFrames > 10000)
						AnimThrow("Animation sequence %d,%d(%s) claims to have over 10000 frames.",
						          (int)i, (int)j, (const char *)info);

					nRead += MvlIfpReadChunkId(pStream, id4, 0); // frame-type fourcc
					if(strncmp(id4, "KRTS", 4) == 0)
						bKRTS = true;
					else if(strncmp(id4, "KRT0", 4) == 0)
						bKRT0 = true;
					else if(strncmp(id4, "KR00", 4) != 0)
						AnimThrow("Animation sequence frame type is invalid.");

					MvlAnimSeqAllocFrames(seq);                   // FUN_100076f0
					if(seq->pFrames == 0)
						return false;     // CLONE-GUARD: TODO seam leaves 0

					for(k = 0; k < (unsigned)seq->nFrameCount; k++){
						unsigned char abQuat[0x20];
						unsigned char *frame =
							(unsigned char *)seq->pFrames +
							(size_t)k * ((bKRTS || bKRT0) ? 0x20 : 0x14);
						unsigned n;

						// read targets for the frame blocks are dropped in
						// the decomp (FUN_10032070 args); scratch layout is
						// the recovered local block (rot 0x10 / trans 0xc /
						// time 4).
						MvlAnimQuatInit(abQuat);                  // FUN_100080f0
						MvlVectorZero(afTrans);                   // FUN_10008c30

						n = MvlRwStreamRead(pStream, abQuat, 0x10);
						nRead += n;
						if(n != 0x10)
							AnimThrow("Failed to read animation frame rotation.");
						if(bKRTS || bKRT0){
							n = MvlRwStreamRead(pStream, afTrans, 0xc);
							nRead += n;
							if(n != 0xc)
								AnimThrow("Failed to read animation frame translation.");
							if(bKRTS){
								if(MvlRwStreamSkip(pStream, 0xc) == 0)
									AnimThrow("Failed to skip animation frame scale.");
								nRead += 0xc;
							}
						}
						n = MvlRwStreamRead(pStream, abQuat + 0x10, 4);
						nRead += n;
						if(n != 4)
							AnimThrow("Failed to read animation frame rotation.");

						MvlAnimQuatDecode(abQuat);                // FUN_100084e0
						memcpy(frame, abQuat, 0x14);
						if(bKRTS || bKRT0)
							MvlVectorCopy((float *)(frame + 0x14), afTrans); // FUN_10008c90
					}
				}
			}

			MvlAnimSeqFinalise(hier);                             // FUN_10007630
			MvlAnimSeqPost();                                     // FUN_10007620

			if(!MvlAnimModelOneAvailable())
				AnimThrow("Model 1 used to initialise animations is not available.");
			MvlAnimBindKeyFrame(MvlAnimModelOneGetKeyFrame());     // vtable +0xc, 0x2c1d0
			MvlAnimHierarchyBindPrep();                           // FUN_100075a0
			anim->nField2c = 0xffff;
			anim->nField0e = 0xffff;
			anim->nField2e = 0xffff;
			if(MvlAnimClumpIsReady())                             // FUN_10031240
				MvlRwClumpPostRead(0);                            // FUN_100312c0
			MvlWeaponClumpRegister(0);                            // FUN_10031450
			MvlAnimFrameClose();                                  // FUN_10002ce0
		}
		return true;
	}catch(const SMvlAnimError &e){
		// Catch@10001ea4: clump teardown for the in-flight anim, then the
		// verbatim "%s" log (FUN_100012f0 partial dtor is engine lane).
		if(anim != 0){
			if(MvlAnimClumpIsReady())
				MvlRwClumpPostRead(0);
			MvlWeaponClumpRegister(0);
		}
		AnimErr(fn, "%s", e.msg);
		return false;
	}
}

// ===========================================================================
// CCustomAnimFile::LoadVersionTwo — FUN_10001f90 @ RVA 0x1f90, catch filter
// @ RVA 0x25fb ("CCustomAnimFile::LoadVersionTwo" "%s").
// ANP3 container: named fields (labels recovered verbatim from the DLL's
// own string table @ 0x1007a918..0x1007aa6c), bone type 3 (rot+time) or 4
// (rot+time+trans). Limits: 500 anims / 50 sequences / 10000 frames.
// ===========================================================================
bool
CCustomAnimFile::LoadVersionTwo(RwStream *pStream)
{
	static const char *const fn = "CCustomAnimFile::LoadVersionTwo";
	SMvlAnimObject *anim = 0;
	unsigned nRead = 0;
	unsigned char abBlock[16];
	float afTrans[3];

	try{
		int32_t i;

		nRead += MvlIfpReadName24(pStream, abBlock,
		                         "Animation block name");         // FUN_10002810
		nRead += MvlIfpReadRaw(pStream, &nAnimCount, 4,
		                       "Number of animations");           // FUN_100027b0
		if(nAnimCount > 500)
			AnimThrow("Animation file claims to contain over 500 animations.");

		apAnims = (SMvlAnimObject **)malloc((size_t)nAnimCount * 4);
		if(apAnims == 0)
			return false;             // CLONE-GUARD: allocation failure
		for(i = 0; i < nAnimCount; i++)
			apAnims[i] = 0;

		for(i = 0; i < nAnimCount; i++){
			SMvlAnimHierarchy *hier;
			unsigned char abFrame[16];
			uint32_t j;

			anim = (SMvlAnimObject *)MvlAnimAllocAnim();          // FUN_10007360/0x7470
			if(anim == 0)
				return false;         // CLONE-GUARD: TODO seam returns 0
			apAnims[i] = anim;
			hier = (SMvlAnimHierarchy *)MvlAnimAllocHierarchy();  // FUN_100075b0
			if(hier == 0)
				return false;         // CLONE-GUARD: TODO seam returns 0
			memset(hier, 0, sizeof(*hier));
			anim->pHierarchy = hier;

			nRead += MvlIfpReadName24(pStream, hier->szName,
			                         "Animation name");
			nRead += MvlIfpReadRaw(pStream, &hier->nSeqCount, 4,
			                       "Number of sequences");
			nRead += MvlIfpReadRaw(pStream, abBlock, 4,
			                       "Frame data length");
			nRead += MvlIfpReadRaw(pStream, abBlock, 4,
			                       "Unknown animation field");
			if(hier->nSeqCount > 0x32)
				AnimThrow("Animation hierarchy %d (%s) claims to contain over 50 sequences.",
				          (int)i, hier->szName);

			hier->pSeqArray = MvlAnimAllocSeqArray(hier->nSeqCount);
			if(hier->pSeqArray == 0)
				return false;         // CLONE-GUARD: TODO seam returns 0

			for(j = 0; j < (uint32_t)hier->nSeqCount; j++){
				SMvlAnimSequence *seq =
					(SMvlAnimSequence *)((unsigned char *)hier->pSeqArray +
					                     (size_t)j * 0x30);
				uint32_t nType = 0, nFrames = 0, k;
				unsigned char abBoneIdx[4];

				nRead += MvlIfpReadName24(pStream, seq->szName,
				                         "Animation sequence name");
				nRead += MvlIfpReadRaw(pStream, &nType, 4,
				                       "Animation sequence type");
				if(nType != 3 && nType != 4)
					AnimThrow("Animation hierarchy %d (%s) bone %d (%s) unknown type %d.",
					          (int)i, hier->szName, (int)j, seq->szName, (int)nType);
				nRead += MvlIfpReadRaw(pStream, &nFrames, 4,
				                       "Number of frames in a sequence");
				nRead += MvlIfpReadRaw(pStream, abBoneIdx, 4,
				                       "Bone index of a sequence");
				if(nFrames > 10000)
					AnimThrow("Animation sequence %d,%d(%s) claims to have over 10000 frames.",
					          (int)i, (int)j, seq->szName);

				MvlAnimSeqSetExtra(seq, (int16_t)*(uint32_t *)abBoneIdx); // FUN_10007700
				if(nFrames > 0)
					MvlAnimSeqAllocFrames(seq);                   // FUN_100076f0

				for(k = 0; k < nFrames; k++){
					unsigned char abRot[16];
					void *pQuat;
					float fTime;
					unsigned char *frame;

					// FUN_100080f0/FUN_10008c30 init targets dropped in
					// the decomp; scratch used (OQ-9).
					MvlAnimQuatInit(abRot);
					MvlVectorZero(afTrans);

					nRead += MvlIfpReadRaw(pStream, abFrame, 10,
					                       "Frame rotation and time");
					pQuat = MvlAnimQuatDecodeCompressed(
						abRot, (float)*(int16_t *)(abFrame + 6) *
						       MvlAnimV2RotScale());              // FUN_10008110
					fTime = (float)*(int16_t *)(abFrame + 8) *
					        MvlAnimV2TimeScale();

					if(nType == 4){
						nRead += MvlIfpReadRaw(pStream, abFrame, 6,
						                       "Frame translation");
						if(seq->pFrames == 0)
							return false; // CLONE-GUARD: TODO seam leaves 0
						frame = (unsigned char *)seq->pFrames + (size_t)k * 0x20;
						if(pQuat == 0)
							return false; // CLONE-GUARD: TODO seam returns 0
						memcpy(frame, pQuat, 0x10);
						*(float *)(frame + 0x10) = fTime;
						{
							void *pVec = MvlAnimVectorDecode(abFrame); // FUN_10008c50
							if(pVec)
								MvlVectorCopy((float *)(frame + 0x14), (const float *)pVec);
						}
					}else{
						if(seq->pFrames == 0)
							return false; // CLONE-GUARD: TODO seam leaves 0
						frame = (unsigned char *)seq->pFrames + (size_t)k * 0x14;
						if(pQuat == 0)
							return false; // CLONE-GUARD: TODO seam returns 0
						memcpy(frame, pQuat, 0x10);
						*(float *)(frame + 0x10) = fTime;
					}
				}
			}

			MvlAnimSeqFinalise(hier);                             // FUN_10007630
			MvlAnimSeqPost();                                     // FUN_10007620

			if(!MvlAnimModelOneAvailable())
				AnimThrow("Model 1 used to initialise animations is not available.");
			MvlAnimBindKeyFrame(MvlAnimModelOneGetKeyFrame());     // vtable +0xc, 0x2c1d0
			MvlAnimHierarchyBindPrep();                           // FUN_100075a0
			anim->nField2c = 0xffff;
			anim->nField0e = 0xffff;
			anim->nField2e = 0xffff;
			if(MvlAnimClumpIsReady())                             // FUN_10031240
				MvlRwClumpPostRead(0);                            // FUN_100312c0
			MvlWeaponClumpRegister(0);                            // FUN_10031450
			MvlAnimFrameClose();                                  // FUN_10002ce0
		}
		return true;
	}catch(const SMvlAnimError &e){
		// Catch@100025fb: same teardown + verbatim "%s" log as v1.
		if(anim != 0){
			if(MvlAnimClumpIsReady())
				MvlRwClumpPostRead(0);
			MvlWeaponClumpRegister(0);
		}
		AnimErr(fn, "%s", e.msg);
		return false;
	}
}

// ===========================================================================
// Archive seam — src/archive/vehicle_archive.h:83.
// ===========================================================================
bool
MvlAnimFileLoad(CCustomAnimFile *af, const void *data, unsigned size)
{
	if(af == 0)
		return false;
	return af->Load((void *)data, (uint32_t)size);   // FUN_100014e0 @ 0x14e0
}

// ===========================================================================
// Game anim-object seams — unrecoverable callees. TODO(media) stubs: log-once
// via MvlErrorLog, benign return, nothing real written.
// ===========================================================================
void *
MvlAnimAllocAnim(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimAllocAnim @ 0x7360/0x7470");
	return 0;
}

void *
MvlAnimAllocHierarchy(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimAllocHierarchy @ 0x75b0 (0x28-byte object)");
	return 0;
}

void *
MvlAnimAllocSeqArray(int)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimAllocSeqArray @ 0x58e9a+0x7660 (count-prefixed 0x30 array)");
	return 0;
}

void
MvlAnimSeqSetInfo(void *, const void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimSeqSetInfo @ 0x7710");
}

void
MvlAnimSeqSetExtra(void *, int16_t)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimSeqSetExtra @ 0x7700");
}

void
MvlAnimSeqAllocFrames(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimSeqAllocFrames @ 0x76f0");
}

void
MvlAnimQuatInit(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimQuatInit @ 0x80f0");
}

void
MvlAnimQuatDecode(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimQuatDecode @ 0x84e0");
}

void *
MvlAnimQuatDecodeCompressed(void *, float)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimQuatDecodeCompressed @ 0x8110");
	return 0;
}

void *
MvlAnimVectorDecode(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimVectorDecode @ 0x8c50");
	return 0;
}

void
MvlAnimSeqFinalise(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimSeqFinalise @ 0x7630");
}

void
MvlAnimSeqPost(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimSeqPost @ 0x7620");
}

bool
MvlAnimModelOneAvailable(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimModelOneAvailable @ 0x1740 (DAT_100a6314+4 / DAT_100a631c+0x1c)");
	return false;
}

void *
MvlAnimModelOneGetKeyFrame(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimModelOneGetKeyFrame @ 0x1740 (vtable +0xc)");
	return 0;
}

void
MvlAnimBindKeyFrame(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimBindKeyFrame @ 0x2c1d0");
}

void
MvlAnimHierarchyBindPrep(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimHierarchyBindPrep @ 0x75a0");
}

bool
MvlAnimClumpIsReady(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimClumpIsReady @ 0x31240");
	return false;
}

void
MvlAnimFrameClose(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimFrameClose @ 0x2ce0");
}

float
MvlAnimV2RotScale(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimV2RotScale @ DAT_1008dd9c (value unrecovered)");
	return 0.0f;
}

float
MvlAnimV2TimeScale(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlAnimV2TimeScale @ DAT_1008ddb4 (value unrecovered)");
	return 0.0f;
}
