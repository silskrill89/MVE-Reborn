// sound_handler.cpp — CSoundHandler implementation (see sound_handler.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/audio/range-decomp.txt). Log strings are verbatim
// from the decomp. The archive seam MvlSoundLoadFile (declared in
// src/archive/vehicle_archive.h) is DEFINED at the bottom of this file —
// that is how CMvlVehicle::RegisterArchiveFile's "*.wav" route hooks in.
#include "sound_handler.h"

#include "../loader/log.h"

#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// x86 target (VC 1.0 plugin).
// ---------------------------------------------------------------------------
#if !defined(_M_IX86)
#error "MVL clone: sound_handler.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// Log wrappers — same mapping as the archive lane:
//   FUN_1001cc00(fmt,...)     -> message log
//   FUN_1001cce0(fn,fmt,...)  -> warning (message log, fn prefixed)
//   FUN_1001cb80(fn,fmt,...)  -> error log (fn prefixed)
// ---------------------------------------------------------------------------
static void
SndMsg(const char *fmt, ...)
{
	char buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	buf[sizeof(buf) - 1] = '\0';
	MvlLog("%s", buf);
}

static void
SndWarn(const char *fn, const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "%s: %s", fn, msg);
	buf[sizeof(buf) - 1] = '\0';
	MvlLog("%s", buf);
}

static void
SndErr(const char *fn, const char *fmt, ...)
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

// ---------------------------------------------------------------------------
// WAV field view — offsets match the decomp's param_3 dword/word accesses
// exactly (RIFF 0x00/0x04, WAVE 0x08, 'fmt ' 0x0c, fmt size 0x10,
// format 0x14, channels 0x16, rate 0x18, block align 0x20, bits 0x22,
// 'data' 0x24, data size 0x28, samples 0x2c).
// ---------------------------------------------------------------------------
struct SWavHeader
{
	uint32_t nRiffFourCC;        // +0x00 'RIFF' (0x46464952)
	uint32_t nRiffSize;          // +0x04 file size - 8
	uint32_t nWaveFourCC;        // +0x08 'WAVE' (0x45564157)
	uint32_t nFmtFourCC;         // +0x0c 'fmt ' (0x20746d66)
	uint32_t nFmtSize;           // +0x10 16 (PCM)
	uint16_t nFormat;            // +0x14 1 = PCM
	uint16_t nChannels;          // +0x16
	uint32_t nSampleRate;        // +0x18 Hz
	uint32_t nByteRate;          // +0x1c
	uint16_t nBlockAlign;        // +0x20
	uint16_t nBitsPerSample;     // +0x22 16
	uint32_t nDataFourCC;        // +0x24 'data' (0x61746164)
	uint32_t nDataSize;          // +0x28
	// samples at +0x2c
};

// ===========================================================================
// CSoundHandler::FillInfoFromName — vehmod_retail.dll 0.98e @ RVA 0x338a0
// ===========================================================================
bool
CSoundHandler::FillInfoFromName(const char *fileName, SMvlSoundSample *info)
{
	static const char *const fn = "CSoundHandler::FillInfoFromName";
	const char *base;
	const char *mark;
	char *end;
	long v;

	if(!fileName || !info)
		return false;

	// Strip directories (last '\' then last '/').
	base = fileName;
	mark = strrchr(base, '\\');
	if(mark)
		base = mark + 1;
	mark = strrchr(base, '/');
	if(mark)
		base = mark + 1;

	// Loop marker is the last '_' when followed by 's' ("_s<start>e<end>.").
	mark = strrchr(base, '_');
	if(mark == 0 || mark[1] != 's'){
		info->nLoopStart = 0;
		info->nLoopEnd = -1;
		return true;
	}

	v = strtol(mark + 2, &end, 10);
	info->nLoopStart = (int32_t)v;
	if(end == mark + 2 || end == 0){
		SndErr(fn, "Failed to parse loop start offset from file name.");
		return false;
	}
	if(v < 0){
		SndErr(fn, "Loop offset start may not be negative.");
		return false;
	}
	if(*end != 'e'){
		SndErr(fn, "Failed to find loop end offset indicator 'e' from file name.");
		return false;
	}

	mark = end;
	v = strtol(mark + 1, &end, 10);
	info->nLoopEnd = (int32_t)v;
	if(end == mark + 1 || end == 0){
		SndErr(fn, "Failed to parse loop end offset from file name.");
		return false;
	}
	// The decomp tests *(int*)(param_2 + 0xc) (loop START) here — unreachable
	// (start >= 0 already enforced) while the message says "Loop offset end";
	// the semantically consistent target is +0x10 (loop END). See
	// port-notes-media.md open question OQ-1.
	if(info->nLoopEnd < -1){
		SndErr(fn, "Loop offset end must be -1 or higher.");
		return false;
	}
	if(*end != '.'){
		SndErr(fn, "After loop offset end there should be a dot.");
		return false;
	}
	return true;
}

// ===========================================================================
// FUN_10033770 @ RVA 0x33770 — (provisional) CSoundHandler::FindSlotForName
// ===========================================================================
int
CSoundHandler::FindSlotForName(const char *const *names, const char *fileName,
                               bool *stereoOut)
{
	const char *base;
	const char *mark;
	const char *split;
	int prefixLen;
	int i;

	if(!names || !fileName || !stereoOut)
		return -1;

	*stereoOut = false;

	// Strip directories.
	base = fileName;
	mark = strrchr(base, '\\');
	if(mark)
		base = mark + 1;
	mark = strrchr(base, '/');
	if(mark)
		base = mark + 1;

	// Split point: last '_' followed by 's', else last '.'.
	split = strrchr(base, '_');
	if(split == 0 || split[1] != 's')
		split = strrchr(base, '.');
	if(split == 0)
		return -1;

	prefixLen = (int)(split - base);

	for(i = 0; names[i] != 0; i++){
		const char *n = names[i];
		size_t nlen = strlen(n);

		if((int)nlen < prefixLen)
			continue;
		if(_strnicmp(n, base, prefixLen) != 0)
			continue;
		if(n[prefixLen] == '\0')
			return i;                      // exact mono slot
		// "-r" half of an adjacent "-r"/"-l" pair => stereo couple.
		if(_stricmp(n + prefixLen, "-r") == 0 && names[i + 1] != 0 &&
		   _strnicmp(n, names[i + 1], prefixLen) == 0 &&
		   _stricmp(names[i + 1] + prefixLen, "-l") == 0){
			*stereoOut = true;
			return i;
		}
	}
	return -1;
}

// ===========================================================================
// CSoundHandler::FillSampleBufferFromData — vehmod_retail.dll 0.98e @ RVA 0x33a00
// ===========================================================================
bool
CSoundHandler::FillSampleBufferFromData(const char *fileName,
                                        SMvlSoundSample *info,
                                        const void *data, uint32_t size,
                                        bool stereo)
{
	static const char *const fn = "CSoundHandler::FillSampleBufferFromData";
	const SWavHeader *h = (const SWavHeader *)data;
	const unsigned char *samples;
	uint32_t frames;
	uint32_t bufBytes;
	uint32_t i;

	if(!info || !data)
		return false;

	// ---- header validation (exact decomp order, verbatim strings) --------
	if(size < 0x0c){
		SndErr(fn, "Unexpected file end in the middle of RIFF header.");
		return false;
	}
	if(h->nRiffFourCC != 0x46464952){
		SndErr(fn, "Did not find a RIFF fourCC in the WAV file.");
		return false;
	}
	if(h->nRiffSize != size - 8){
		SndErr(fn, "File size specified in RIFF header does not match actual size.");
		return false;
	}
	if(h->nWaveFourCC != 0x45564157){
		SndErr(fn, "RIFF header does not contain a WAVE fourCC.");
		return false;
	}
	if(size < 0x14){
		SndErr(fn, "Unexpected file end in the middle of 'fmt ' subchunk header.");
		return false;
	}
	if(h->nFmtFourCC != 0x20746d66){
		SndErr(fn, "First subchunk is not with a 'fmt ' fourCC.");
		return false;
	}
	if(h->nFmtSize != 0x10){
		SndErr(fn, "First subchunk size should be 16, probably file is not in PCM format.");
		return false;
	}
	if(size < 0x24){
		SndErr(fn, "Unexpected file end in the middle of 'fmt ' subchunk.");
		return false;
	}
	if(h->nFormat != 1){
		SndErr(fn, "Format chunk has audio format other than 1 (PCM).");
		return false;
	}
	if(h->nChannels == 0){
		SndErr(fn, "Format chunk specifies 0 channels.");
		return false;
	}
	if(h->nChannels > 16){
		SndErr(fn, "Format chunk specifies over 16 channels.");
		return false;
	}
	if(h->nSampleRate < 100 || h->nSampleRate > 0x5dc00){
		SndErr(fn, "Format chunk specifies a sample rate out of range 100-384000.");
		return false;
	}
	if(h->nBitsPerSample != 0x10){
		SndErr(fn, "Format chunk specifies bits per sample other than 16.");
		return false;
	}
	if(h->nBlockAlign < (uint32_t)h->nChannels * 2){
		SndErr(fn, "Block align in format chunk is smaller than channels*bitspersample.");
		return false;
	}
	if(h->nBlockAlign > (uint32_t)h->nChannels * 2 + 0x20){
		SndErr(fn, "Block align in format chunk specifies padding more than 32 bytes.");
		return false;
	}
	if(size < 0x2c){
		SndErr(fn, "Unexpected file end in the middle of 'data' subchunk header.");
		return false;
	}
	if(h->nDataFourCC != 0x61746164){
		SndErr(fn, "Second subchunk header is not with a 'data' fourCC.");
		return false;
	}
	// BUGFIX(stable-1.0): the original form `size < h->nDataSize + 0x2cU`
	// (decomp 0x98e @ 0x1417) WRAPS for nDataSize >= 0xFFFFFFE4: the check
	// passes, frames = nDataSize/blockAlign stays large, malloc(frames*2)
	// can succeed (e.g. nDataSize=0xFFFFFFE0, blockAlign=0x20 -> 16MB) and
	// the deinterleave loop below then reads gigabytes past the archive
	// buffer. Compare without the addition (size >= 0x2c guaranteed above).
	if(h->nDataSize > size - 0x2cU){
		SndErr(fn, "Second subchunk length too large - goes beyond end of file.");
		return false;
	}
	if(h->nDataSize % h->nBlockAlign != 0){
		SndErr(fn, "Second subchunk length is not a multiple of block align.");
		return false;
	}

	// ---- channel-count warnings ----------------------------------------
	if(!stereo){
		if(h->nChannels >= 2)
			SndWarn(fn, "More than one channel in %s, using only the first one.",
			        fileName);
	}else{
		if(h->nChannels >= 3)
			SndWarn(fn, "More than two channels in %s, using only the first two.",
			        fileName);
	}

	// ---- sample buffer fill --------------------------------------------
	frames = h->nDataSize / h->nBlockAlign;
	bufBytes = frames * 2;
	info->nBufferBytes = bufBytes;
	info->nSampleRate = h->nSampleRate;

	if(frames == 0){
		SndErr(fn, "File contains no samples.");
		return false;
	}

	// Loop offset validation (byte offsets into the channel-0 buffer).
	if((int32_t)info->nLoopEnd >= 0 && bufBytes < (uint32_t)info->nLoopEnd){
		SndErr(fn, "Loop end offset points past the end of sample buffer.");
		return false;
	}
	if(bufBytes < (uint32_t)info->nLoopStart){
		SndErr(fn, "Loop start offset points past the end of sample buffer.");
		return false;
	}
	if((int32_t)info->nLoopEnd >= 0 && info->nLoopEnd <= info->nLoopStart){
		SndErr(fn, "Loop start offset is not before end offset");
		return false;
	}
	if(info->nLoopEnd == -1)
		info->nLoopEnd = (int32_t)(bufBytes - 2);

	// Deinterleave channel 0 (first 2 bytes of every block-aligned frame).
	samples = (const unsigned char *)data + 0x2c;
	info->pBuffer = malloc(bufBytes);
	if(info->pBuffer == 0)
		return false;   // CLONE-GUARD: allocation failure (original unchecked)
	for(i = 0; i < frames; i++){
		uint16_t s = *(const uint16_t *)(samples + h->nBlockAlign * i);
		((uint16_t *)info->pBuffer)[i] = s;
	}

	// ---- stereo pair: second record = channel 1 -------------------------
	if(stereo){
		SMvlSoundSample *right = info + 1;   // param_2[5..9] = the 0x14-byte
		                                     // record of the "-l" slot

		right->nBufferBytes = info->nBufferBytes;
		right->nSampleRate = info->nSampleRate;
		right->nLoopStart = info->nLoopStart;
		right->nLoopEnd = info->nLoopEnd;

		right->pBuffer = malloc(right->nBufferBytes);
		if(right->pBuffer == 0){
			// CLONE-GUARD: allocation failure (original unchecked).
			// BUGFIX(stable-1.0): roll back the channel-0 buffer — otherwise
			// the slot is left half-loaded (info->pBuffer non-NULL with no
			// usable right channel): the caller reports failure, the orphaned
			// buffer is never freed, and any retry hits "Duplicate file".
			free(info->pBuffer);
			info->pBuffer = 0;
			return false;
		}

		if(h->nChannels < 2){
			SndMsg("Wave file %s - using channel 0 for both left and right samples.",
			       fileName);
			memcpy(right->pBuffer, info->pBuffer, right->nBufferBytes);
		}else{
			SndMsg("Wave file %s - using channels 0 and 1 as left and right samples.",
			       fileName);
			for(i = 0; i < frames; i++){
				uint16_t s = *(const uint16_t *)(samples + h->nBlockAlign * i + 2);
				((uint16_t *)right->pBuffer)[i] = s;
			}
		}
	}
	return true;
}

// ===========================================================================
// CSoundHandler::LoadSoundFile — vehmod_retail.dll 0.98e @ RVA 0x33e10
// ===========================================================================
bool
CSoundHandler::LoadSoundFile(const char *const *names, void *slots,
                             const char *fileName,
                             const void *data, uint32_t size)
{
	static const char *const fn = "CSoundHandler::LoadSoundFile";
	bool stereo = false;
	int idx;
	SMvlSoundSample *slot;

	idx = FindSlotForName(names, fileName, &stereo);
	if(idx == -1){
		SndErr(fn, "Could not find slot for sound file %s.", fileName);
		return false;
	}

	// Slot record at slots + idx*0x14 (a stereo couple spans two records).
	slot = (SMvlSoundSample *)((unsigned char *)slots +
	                           (size_t)idx * sizeof(SMvlSoundSample));
	if(slot->pBuffer != 0){
		SndErr(fn, "Duplicate file on the same slot, %s.", fileName);
		return false;
	}
	if(!FillInfoFromName(fileName, slot)){
		SndErr(fn, "Could not get sound info from audio file name %s.", fileName);
		return false;
	}
	if(!FillSampleBufferFromData(fileName, slot, data, size, stereo)){
		SndErr(fn, "Could not read sample buffer from file %s.", fileName);
		return false;
	}
	return true;
}

// ===========================================================================
// Archive seam — src/archive/vehicle_archive.h:88.
// The original signature is FUN_10033e10(int *nameTable, int slots,
// char *fileName, int *data, uint size) @ 0x33e10; the clone seam carries
// the same five parameters (name = the per-vehicle name table whose first
// entry is the PTR_s_rcengine1_100af104 string; see port-notes-media.md
// open question OQ-2 regarding the call site's literal).
// ===========================================================================
bool
MvlSoundLoadFile(const void *name, void *slot, const char *fileName,
                 const void *data, unsigned size)
{
	return CSoundHandler::LoadSoundFile((const char *const *)name, slot,
	                                    fileName, data, size);
}
