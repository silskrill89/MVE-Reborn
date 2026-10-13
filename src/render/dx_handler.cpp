// dx_handler.cpp — CDxHandler::DX9TextureFix port (see dx_handler.h).
//
// Ported 1:1 from vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/missing33/decomp-chain.txt, depth=0 FUN_1000d340)
// and verified against the raw disassembly of vehmod_retail.dll (branch
// order, constant 0x1003ffff, and the "a"-suffix word write are disasm-
// confirmed). Error strings are verbatim (string xrefs 00d340).
#include "dx_handler.h"

#include "../loader/log.h"

#include <string.h>

// ---------------------------------------------------------------------------
// Error log wrapper for FUN_1001cb80(fn, fmt, ...) — 0.98e @ RVA 0x1cb80
// ("Error in %s: %s\n" -> original errorlog.txt; clone -> MvlErrorLog).
// ---------------------------------------------------------------------------
static void
DxError(const char *fn, const char *fmt, ...)
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
// FUN_10032f30 (0.98e @ RVA 0x32f30) — RW library-ID -> comparable version
// word. Unnamed in the original (no error string); kept 1:1. Used to gate
// "Texture uses too high version." (valid window: <= 0x36003, fix needed
// when > 0x34003).
//   packed id (high 16 bits set): ((id >> 14) & 0x3FF00) + 0x30000 | (id >> 16) & 0x3F
//   plain id:                     id << 8
// ---------------------------------------------------------------------------
static unsigned MvlRwLibIdToVersion(unsigned id)     // FUN_10032f30 @ RVA 0x32f30
{
	if((id & 0xffff0000) != 0)
		return (((id >> 14) & 0x3ff00) + 0x30000) | ((id >> 16) & 0x3f);
	return id << 8;
}

// Library ID written over out-of-range entries — immediate 0x1003ffff in the
// original (`mov dword [ebx+8], 0x1003ffff`; Ghidra: &DAT_1003ffff).
static const unsigned kMvlRwLibIdFixed = 0x1003ffff;   // DAT_1003ffff @ RVA 0x3ffff

// Mask-name suffix: original copies the 2-byte word at 0x7b0d8 ("a\0") behind
// the copied mask name (asm @ 0xd49a: mov ax,[0x1007b0d8]; mov [edi],ax).
static const char kMvlMaskSuffixA[2] = { 'a', '\0' };  // @ RVA 0x1007b0d8

// ---------------------------------------------------------------------------
// CDxHandler::DX9TextureFix — 0.98e @ RVA 0xd340 (650 bytes).
// Original __fastcall(data in ECX, size in EDX); plain args here.
// Buffer layout used below (byte offsets from `data`, disasm-confirmed):
//   +0x00 main header id (0x16), +0x08 library id, +0x0C sub id (1),
//   +0x10 sub len (4), +0x14 sub library id, +0x18 texture count (u16),
//   +0x1C first texture entry:
//     +0x00 id (0x15), +0x04 len, +0x0C sub id (1), +0x18 dword <- 8,
//     +0x1C raster flags <- (flags != 0x1102) ? 0x1106 : 0x1102,
//     +0x20 name[32], +0x40 maskName[32], +0x60 raster format
//     (0x100/0x200/0x300/0x500/0x600), +0x64 dword, +0x6F byte.
// ---------------------------------------------------------------------------
bool
CDxHandler::DX9TextureFix(int *data, unsigned size)
{
	static const char *const fn = "CDxHandler::DX9TextureFix";

	if(size < 0x1c){                                        // cmp edx,0x1c / jae
		DxError(fn, "Texture file too small.");
		return false;
	}
	if(data[0] != 0x16){                                    // cmp [ebx],0x16
		DxError(fn, "Texture file main header missing.");
		return false;
	}
	unsigned ver = MvlRwLibIdToVersion((unsigned)data[2]);         // mov ecx,[ebx+8] / call 0x32f30
	if(ver <= 0x34003)                                      // cmp eax,0x34003 / ja
		return true;                                        // already fine — no fixup
	if(ver > 0x36003){                                      // cmp eax,0x36003 / jbe
		DxError(fn, "Texture uses too high version.");
		return false;
	}
	data[2] = (int)kMvlRwLibIdFixed;                        // mov [ebx+8],0x1003ffff
	if(data[3] != 1){                                       // cmp [ebx+0xc],1
		DxError(fn, "Texture dictionary sub-header missing.");
		return false;
	}
	if(data[4] != 4){                                       // cmp [ebx+0x10],4
		DxError(fn, "Texture dictionary sub-header size wrong.");
		return false;
	}

	unsigned numTex = (unsigned short)data[6];              // movzx eax,word [ebx+0x18]
	data[5] = (int)kMvlRwLibIdFixed;                        // mov [ebx+0x14],0x1003ffff
	data[6] = (int)numTex;                                  // dword write back (zero-extends u16)

	int *entry = data + 7;                                  // lea esi,[ebx+0x1c]
	unsigned i = 0;
	while(i < numTex){
		// bounds: (entry - data) + 0x58 must be < size  (asm @ 0xd426..0xd42f)
		if((unsigned)((char *)entry - (char *)data + 0x58) >= size){
			DxError(fn, "Texture offset is invalid (tex %u).", i);
			return false;
		}
		// BUGFIX(stable-1.0): the original check only guarantees 0x58 bytes
		// (decomp FUN_1000d340 line 1224) but the body reads/writes entry
		// fields up to +0x6F (entry[24] format dword, entry[25] +0x64, the
		// +0x6F byte, and the mask-name word at +0x60 for 31-char names) —
		// a truncated final texture entry would otherwise read/write past
		// the buffer. Require the full 0x70-byte extent; well-formed TXD
		// entries are >= 0x70, so only corrupt/truncated files are rejected.
		if((unsigned)((char *)entry - (char *)data + 0x70) > size){
			DxError(fn, "Texture offset is invalid (tex %u).", i);
			return false;
		}
		if(entry[0] != 0x15){                               // cmp [esi],0x15
			DxError(fn, "Texture header is missing (tex %u).", i);
			return false;
		}
		if(entry[3] != 1){                                  // cmp [esi+0xc],1
			DxError(fn, "Texture sub-header is missing (tex %u).", i);
			return false;
		}
		data[2] = (int)kMvlRwLibIdFixed;                    // re-assert both library IDs
		data[5] = (int)kMvlRwLibIdFixed;
		entry[6] = 8;                                       // mov [esi+0x18],8
		// raster flags: (flags != 0x1102) ? 0x1106 : 0x1102  (setne/lea)
		entry[7] = (entry[7] != 0x1102) ? (0x1102 + 4) : 0x1102;

		bool setFlag64 = true;                              // LAB_1000d52f: [esi+0x64]=1
		switch((unsigned)entry[24]){                         // cmp [esi+0x60] chain
		case 0x300: {                                       // 4444: mask name = name + 'a'
			char *mask = (char *)(entry + 16);              // lea edi,[esi+0x40]
			strncpy(mask, (const char *)(entry + 8), 0x1f);  // src [esi+0x20], n 0x1f
			mask[0x1f] = '\0';
			char *end = mask;
			while(*end) end++;                              // scan to NUL (asm @ 0xd490)
			end[0] = kMvlMaskSuffixA[0];                    // word write of 0x0061
			end[1] = kMvlMaskSuffixA[1];
			*((unsigned char *)entry + 0x6f) = 3;           // mov byte [esi+0x6f],3
			break;
		}
		case 0x200:                                         // 565 -> degrade to 1555
			entry[24] = 0x100;                              // mov [esi+0x60],0x100
			// fall through into the 0x100/0x200 common block (asm jmp 0xd4c3)
		case 0x100:                                         // 1555: clear mask name
			memset((char *)(entry + 16), 0, 32);            // 4x movq zeroing [esi+0x40..0x5c]
			entry[25] = 0;                                  // mov [esi+0x64],0
			*((unsigned char *)entry + 0x6f) = 1;           // mov byte [esi+0x6f],1
			setFlag64 = false;                              // -> LAB_1000d53c (skip +0x64=1)
			break;
		case 0x600:                                         // LUM8: flags only
			entry[7] = 0;                                   // mov [esi+0x1c],0
			setFlag64 = false;
			break;
		case 0x500: {                                       // RGBA8888: mask name = name + 'a'
			char *mask = (char *)(entry + 16);
			strncpy(mask, (const char *)(entry + 8), 0x1f);
			mask[0x1f] = '\0';
			char *end = mask;
			while(*end) end++;
			end[0] = kMvlMaskSuffixA[0];
			end[1] = kMvlMaskSuffixA[1];
			*((unsigned char *)entry + 0x6f) = 0;           // mov byte [esi+0x6f],0
			entry[7] = 0;                                   // mov [esi+0x1c],0
			break;
		}
		default:                                            // push i / error / false
			DxError(fn, "Texture type unknown (tex %u).", i);
			return false;
		}
		if(setFlag64)
			entry[25] = 1;                                  // LAB_1000d52f: mov [esi+0x64],1

		// advance: entry += len + 12  (mov eax,[esi+4]; add esi,0xc; add esi,eax)
		int len = entry[1];
		entry = (int *)((char *)entry + len + 0xc);
		i++;
	}
	return true;                                            // mov al,1
}
