// sound_handler.h — CSoundHandler: per-vehicle WAV sample loader.
//
// Faithful recompilation from decompiled vehmod_retail.dll 0.98e (decomp
// source: .harness-memory/limout/audio/range-decomp.txt, cluster
// 0x33770-0x33f10). Every function carries its origin VA. Original symbol
// names are reused 1:1 (self-identifying error strings recovered in the
// decomp: "CSoundHandler::FillInfoFromName" etc. — see
// .harness-memory/limout/port-notes-media.md).
//
// The archive pipeline reaches this module through the seam
// MvlSoundLoadFile (declared in src/archive/vehicle_archive.h, DEFINED in
// sound_handler.cpp): CMvlVehicle::RegisterArchiveFile routes every "*.wav"
// archive entry here. Sound samples come from 7z archive entries; the file
// name encodes frequency/loop semantics:
//
//   <slotname>[_s<loopStart>e<loopEnd>].wav      e.g. "rcengine1-r_s22050e44100.wav"
//
//   - the base name (up to "_s" or '.') selects the slot in the per-vehicle
//     name table (FUN_10033770 slot search);
//   - an adjacent "<base>-r"/"<base>-l" name pair selects a STEREO slot
//     pair (channel 0 -> record 0, channel 1 -> record 1);
//   - "_s<start>e<end>" are loop offsets in BYTES into the mono sample
//     buffer; end -1 means "to the end" (resolved to bufferBytes-2).
//
// Slot record layout (0x14 bytes per name-table index; a stereo pair uses
// two consecutive records — recovered from FUN_10033a00's 5-dword writes
// and the caller's `m_abSoundSlots[2][20]` block @ 0x2cf8):
//
//   +0x00 buffer ptr     +0x04 buffer bytes   +0x08 sample rate
//   +0x0c loop start     +0x10 loop end (-1 unresolved)
#pragma once

#include "../core/stdafx.h"

#include <stdint.h>

// ---------------------------------------------------------------------------
// Slot record — (provisional) name: the original writes it as raw dwords
// (param_2 + 0/4/8/c/10 in FUN_10033a00); no RTTI/recovered struct name.
// ---------------------------------------------------------------------------
struct SMvlSoundSample
{
	void    *pBuffer;        // @ +0x00 malloc'd 16-bit mono (or channel) samples
	uint32_t nBufferBytes;   // @ +0x04 sampleCount * 2
	uint32_t nSampleRate;    // @ +0x08 WAV sample rate (Hz)
	int32_t  nLoopStart;     // @ +0x0c byte offset (from "_s<start>")
	int32_t  nLoopEnd;       // @ +0x10 byte offset (-1 => nBufferBytes-2)
};

// ---------------------------------------------------------------------------
// CSoundHandler — recovered method names 1:1 (error-string evidence).
// All methods are register-called statics in the original (no `this`).
// ---------------------------------------------------------------------------
class CSoundHandler
{
public:
	// CSoundHandler::FillInfoFromName — 0.98e @ RVA 0x338a0.
	// Parses "_s<loopStart>e<loopEnd>." out of the file name into the slot
	// record. No marker => loopStart 0 / loopEnd -1. Errors are logged with
	// the verbatim original strings ("Failed to parse loop start offset
	// from file name." etc.) and return false.
	static bool FillInfoFromName(const char *fileName, SMvlSoundSample *info);

	// CSoundHandler::FillSampleBufferFromData — 0.98e @ RVA 0x33a00.
	// Validates the WAV header (PCM/16-bit/rate 100-384000/channels<=16,
	// block-align sanity) and deinterleaves channel 0 (and channel 1 when
	// stereo) into the slot record. Every diagnostic is verbatim.
	static bool FillSampleBufferFromData(const char *fileName,
	                                     SMvlSoundSample *info,
	                                     const void *data, uint32_t size,
	                                     bool stereo);

	// FUN_10033770 @ RVA 0x33770 — (provisional) FindSlotForName:
	// basename lookup in the NUL-terminated name table. Returns the slot
	// index or -1. *stereoOut is set when the matched entry is the "-r"
	// half of an adjacent "-r"/"-l" pair (channel-pair semantics).
	// FUN_1005abd0 is read as strrchr (last-occurrence; same helper used
	// for path stripping).
	static int  FindSlotForName(const char *const *names,
	                            const char *fileName, bool *stereoOut);

	// CSoundHandler::LoadSoundFile — 0.98e @ RVA 0x33e10.
	// names: per-vehicle name table (e.g. PTR_s_rcengine1_100af104);
	// slots: base of the 0x14-byte records (CMvlVehicle::m_abSoundSlots
	// @ 0x2cf8). Duplicate-slot and parse failures are verbatim errors.
	static bool LoadSoundFile(const char *const *names, void *slots,
	                          const char *fileName,
	                          const void *data, uint32_t size);
};

// ---------------------------------------------------------------------------
// Archive seam — DEFINED in sound_handler.cpp (declared in
// src/archive/vehicle_archive.h:88). Provided here for direct use; the
// prototype must stay identical to the vehicle_archive.h declaration.
// ---------------------------------------------------------------------------
// bool MvlSoundLoadFile(const void *name, void *slot, const char *fileName,
//                       const void *data, unsigned size);
