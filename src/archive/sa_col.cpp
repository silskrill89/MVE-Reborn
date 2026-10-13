// sa_col.cpp — LoadSAColSection (see sa_col.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e
// (.harness-memory/limout/missing33/decomp-chain.txt depth-0 root 0x100388a0
// + dumped depth-1 callee 0x1002c290 (colmodel init)). Log strings verbatim.
// The col-model field writes below are RAW byte offsets taken 1:1 from the
// decomp (the CColModel layout belongs to the engine lane); math/log helpers
// are the archive lane's vehicle_archive.h seams (FUN_1002c240/2c290/8c90/
// 0x45d60). Undumped engine calls are HOOK POINT seams — nothing is faked.
#include "sa_col.h"

#include "../archive/vehicle_archive.h"   // CMvlVehicle + col/vector seams
#include "../loader/log.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>     // malloc/free
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrapper (lane convention: FUN_1001cb80 error; context verbatim
// "LoadSAColSection").
// ---------------------------------------------------------------------------
static void
SaColErr(const char *fmt, ...)
{
	char msg[896], buf[1024];
	va_list ap;
	va_start(ap, fmt);
	_vsnprintf(msg, sizeof(msg), fmt, ap);
	va_end(ap);
	msg[sizeof(msg) - 1] = '\0';
	_snprintf(buf, sizeof(buf), "%s: %s", "LoadSAColSection", msg);
	buf[sizeof(buf) - 1] = '\0';
	MvlErrorLog("%s", buf);
}

// ---------------------------------------------------------------------------
// Loading context — DAT_100b77d0 (the CMvlVehicle being loaded; its
// m_pInfo->pColModel receives the parsed model). See sa_col.h for the
// MvlSetLoadingContext routing note.
// ---------------------------------------------------------------------------
static void *s_pLoadingVehicle;   // DAT_100b77d0 @ RVA 0xb77d0

void
MvlSaColSetLoadingVehicle(void *veh)
{
	s_pLoadingVehicle = veh;
}

// Vehicle-side scratch slots written by this loader (inside the CMvlVehicle
// region the archive lane's struct does not model; dword-indexed as puVar5
// [0xa16]/[0xa17] in the decomp = byte offsets 0x2858/0x285c).
static void *&SaColSectionBuf(void *veh)
{
	return *(void **)((unsigned char *)veh + 0x2858);   // puVar5[0xa16] @ +0x2858
}

static void *&SaColVertexBuf(void *veh)
{
	return *(void **)((unsigned char *)veh + 0x285c);   // puVar5[0xa17] @ +0x285c
}

// ---------------------------------------------------------------------------
// FUN_10032070 @ RVA 0x32070 — HOOK POINT. The decomp renders it as an
// arg-less thunk into gta-vc.exe 0x6454b0, yet the very next statement
// reads the freshly malloc'd buffer (magic/size check) — so it must copy
// the section into it. Operands RECONSTRUCTED as (dst, src, len); the
// engine lane recovers the true signature. Until wired it does NOT copy,
// so the length check below fails and the loader bails (no fabricated data).
// ---------------------------------------------------------------------------
static void
MvlCopySaColSection(void *dst, const void *src, unsigned len)
{
	MvlErrorLog("HOOK POINT 0x32070 MvlCopySaColSection (game 0x6454b0; operands RECONSTRUCTED)");
	(void)dst; (void)src; (void)len;
}

// ---------------------------------------------------------------------------
// SA .col container layout (raw offsets; names = engine lane):
//   +0x00 magic "COL2"/"COL3"/"COLL"; +0x04 dword = payload length (size-8).
// COL2/COL3: counts at +0x48 (spheres, u16) / +0x4A (meshes, u16) /
//   +0x4C (groups, u16); vectors at +0x20/+0x2C/+0x38, dword +0x44;
//   array offsets at +0x54 (spheres) / +0x58 (meshes) / +0x60 (int16 verts,
//   /128 scale) / +0x64 (groups) — all relative to (payload = sec+4).
// COLL: counts at +0x48 (spheres) / +0x4A (meshes) / +0x4C (boxes, byte)
//   with inline arrays following; vectors at +0x24/+0x30/+0x3C, dword +0x20.
// ---------------------------------------------------------------------------

// Stamps the shared tail (version byte 6 @ col+0x30, info+0x1b flag, and the
// info->pColModel attach). Both branches end here.
static void
AttachColToInfo(CMvlVehicle *veh, unsigned char *col)
{
	col[0x30] = 6;                                        // *(byte*)(puVar9 + 0xc) = 6
	if(veh->m_pInfo){
		// decomp: *(info + 0x1b) = 1 — byte just before pColModel; the clone's
		// SMvlVehicleInfo models the neighbouring dword @ +0x20 (pad020).
		*((unsigned char *)veh->m_pInfo + 0x1B) = 1;
		veh->m_pInfo->pColModel = col;
	}
}

void *
LoadSAColSection(const void *data, unsigned size)
{
	// decomp guard: DAT_100b77d0 != 0 && *(info + 0x1c) == 0 (no col yet).
	if(s_pLoadingVehicle == 0)
		return (void *)data;
	CMvlVehicle *veh = (CMvlVehicle *)s_pLoadingVehicle;
	if(veh->m_pInfo == 0 || veh->m_pInfo->pColModel != 0)
		return (void *)data;

	if(size > 0x100000){
		SaColErr("SA collision section length longer than 1MB.");
		return (void *)data;
	}
	// BUGFIX(stable-1.0): the header dword at +4 and every count/offset
	// below are read out of `sec` — a section smaller than the fixed 0x68
	// header would read past the allocation. (The length-match check would
	// usually catch it first, but only after the OOB read already happened.)
	if(size < 0x68){
		SaColErr("SA collision section length does not match length specified in header.");
		return (void *)data;
	}

	unsigned char *sec = (unsigned char *)malloc(size);
	if(sec == 0)
		return (void *)data;   // clone guard (the original assumes malloc)
	// BUGFIX(stable-1.0): MvlCopySaColSection (0x32070 hook) is not wired
	// yet and copies nothing — zero the buffer so the length-match read
	// below is deterministic instead of chewing on uninitialized heap.
	memset(sec, 0, size);
	MvlCopySaColSection(sec, data, size);              // FUN_10032070 @ 0x32070 (HOOK POINT)

	if(*(int *)(sec + 4) != (int)size - 8){
		// NOTE: the original returns WITHOUT freeing sec (faithful leak).
		SaColErr("SA collision section length does not match length specified in header.");
		return (void *)data;
	}
	// Cursor limit for every count-driven walk below: all counts/offsets
	// come from the (untrusted) section header, so each loop must stay
	// inside [sec, secEnd). BUGFIX(stable-1.0).
	const unsigned char *secEnd = sec + size;

	void *raw = MvlColModelAlloc();                    // thunk_FUN_1002c240 @ 0x2c240
	unsigned char *col = raw ? (unsigned char *)MvlColModelInit(raw) : 0;   // FUN_1002c290 @ 0x2c290
	if(col == 0){
		// The original does not guard this (assumes the col pool never
		// exhausts) and would write through NULL — the clone bails.
		if(raw)
			free(raw);                                 // decomp FID_conflict__free on later paths
		SaColErr("Could not allocate collision model.");
		return (void *)data;
	}
	MvlColModelSetVehicleId(col, (unsigned short)veh->m_nId);   // col+0x31/+0x32 @ 0x45d60

	if(strncmp((char *)sec, "COL2", 4) != 0 &&
	   strncmp((char *)sec, "COL3", 4) != 0){
		if(strncmp((char *)sec, "COLL", 4) != 0){
			free(raw);                                 // FID_conflict__free(puVar9)
			SaColSectionBuf(veh) = 0;                 // puVar5[0xa16] = 0
			// NOTE: the original leaks sec here too (faithful).
			SaColErr("SA collision section magic code not COLL nor COL3.");
			return (void *)data;
		}

		// ---------------------- COLL branch (deep copy) ----------------------
		unsigned char *p;   // pbVar18 cursor
		unsigned short *q;  // puVar19 cursor

		*(unsigned short *)(col + 0x2C) = *(unsigned short *)(sec + 0x4C);  // puVar9[0xb]
		*(unsigned short *)(col + 0x28) = *(unsigned short *)(sec + 0x48);  // puVar9[10]
		*(unsigned short *)(col + 0x2A) = *(unsigned short *)(sec + 0x4A);
		col[0x2E] = 0;                                                     // box count byte
		*(int *)(col + 0x48) = 0;                                          // puVar9[0x12]
		// decomp lines 100-103 verbatim order: dword scalar first, then the
		// three 12-byte vector copies (COLL layout: +0x00, +0x10, +0x1C).
		*(int *)(col + 0x0C) = *(int *)(sec + 0x20);                      // puVar9[3]
		MvlVectorCopy((float *)(col + 0x00), (const float *)(sec + 0x24)); // puVar9
		MvlVectorCopy((float *)(col + 0x10), (const float *)(sec + 0x30)); // puVar9+4
		MvlVectorCopy((float *)(col + 0x1C), (const float *)(sec + 0x3C)); // puVar9+7

		p = sec + 0x4C;
		{
			unsigned nSph = *(unsigned short *)(sec + 0x48);
			// BUGFIX(stable-1.0): nSph comes from the untrusted header —
			// the p-walk below would run nSph*0x14 bytes past secEnd.
			if((size_t)(secEnd - p) < (size_t)nSph * 0x14){
				SaColErr("SA collision section truncated (spheres).");
				free(raw);
				return (void *)data;
			}
			void *sph = nSph ? malloc((size_t)nSph * 0x14) : 0;
			*(void **)(col + 0x34) = sph;                 // puVar9[0xd]
			if(nSph && sph == 0){ SaColErr("Out of memory (spheres)."); return (void *)data; }
			for(unsigned i = 0; i < nSph; i++){
				unsigned char *e = (unsigned char *)sph + (size_t)i * 0x14;
				*(int *)(e + 0x0C) = *(int *)p;           // *(dword @ +0xc)
				MvlVectorCopy((float *)e, (const float *)(p + 4));
				*(int *)(e + 0x10) = *(int *)(p + 0x10);
				p += 0x14;
			}
		}
		{
			// BUGFIX(stable-1.0): box-count byte + q walk bounds. The count
			// byte at p and the 0x18-byte-per-box q stride are header-driven.
			if(p >= secEnd || (size_t)(secEnd - (p + 4)) < (size_t)*p * 0x18){
				SaColErr("SA collision section truncated (boxes).");
				free(raw);
				return (void *)data;
			}
			unsigned char nBox = *p;                      // byte count (pbVar18)
			q = (unsigned short *)(p + 4);                // puVar19
			void *box = nBox ? malloc((size_t)nBox << 5) : 0;
			*(void **)(col + 0x38) = box;                 // puVar9[0xe]
			if(nBox && box == 0){ SaColErr("Out of memory (boxes)."); return (void *)data; }
			for(unsigned i = 0; i < nBox; i++){
				unsigned char *e = (unsigned char *)box + (size_t)i * 0x20;
				MvlVectorCopy((float *)e, (const float *)q);
				MvlVectorCopy((float *)(e + 0x10), (const float *)(q + 6));
				q += 0x0C;
			}
		}
		{
			// BUGFIX(stable-1.0): mesh count + 0x1C-byte q stride bounds.
			if((const unsigned char *)(q + 2) > secEnd ||
			   (size_t)(secEnd - (const unsigned char *)(q + 2)) < (size_t)*q * 0x1C){
				SaColErr("SA collision section truncated (meshes).");
				free(raw);
				return (void *)data;
			}
			unsigned nMesh = *q;                          // puVar19 count -> col+0x2A
			q += 2;
			*(unsigned short *)(col + 0x2A) = (unsigned short)nMesh;
			void *mesh = nMesh ? malloc((size_t)nMesh * 0x1C) : 0;
			*(void **)(col + 0x3C) = mesh;                // puVar9[0xf]
			if(nMesh && mesh == 0){ SaColErr("Out of memory (meshes)."); return (void *)data; }
			for(unsigned i = 0; i < nMesh; i++){
				unsigned char *e = (unsigned char *)mesh + (size_t)i * 0x1C;
				MvlVectorCopy((float *)e, (const float *)q);
				MvlVectorCopy((float *)(e + 0x0C), (const float *)(q + 6));
				*(int *)(e + 0x18) = *(int *)(q + 0x0C);
				q += 0x0E;
			}
		}
		{
			// BUGFIX(stable-1.0): vertex count + 12-byte q stride bounds.
			if((const unsigned char *)(q + 2) > secEnd ||
			   (size_t)(secEnd - (const unsigned char *)(q + 2)) < (size_t)*q * 0x0C){
				SaColErr("SA collision section truncated (vertices).");
				free(raw);
				return (void *)data;
			}
			// Vertices: int16 triples copied AS floats? no — the COLL container
			// stores them as raw 12-byte rows (decomp copies 12 B per entry).
			unsigned nVert = *q;
			q += 2;
			void *verts = nVert ? malloc((size_t)nVert * 0x0C) : 0;
			*(void **)(col + 0x40) = verts;               // puVar9[0x10]
			if(nVert && verts == 0){ SaColErr("Out of memory (vertices)."); return (void *)data; }
			for(unsigned i = 0; i < nVert; i++){
				MvlVectorCopy((float *)((unsigned char *)verts + (size_t)i * 0x0C),
				              (const float *)q);
				q += 6;                                   // 12 bytes
			}
		}
		{
			// BUGFIX(stable-1.0): group count + 16-byte-per-group q stride
			// (8 words read per entry in the loop below).
			if((const unsigned char *)(q + 2) > secEnd ||
			   (size_t)(secEnd - (const unsigned char *)(q + 2)) < (size_t)*q * 0x10){
				SaColErr("SA collision section truncated (groups).");
				free(raw);
				return (void *)data;
			}
			unsigned nGrp = *q;                           // -> col+0x2C (puVar9+0xb)
			q += 2;
			*(unsigned short *)(col + 0x2C) = (unsigned short)nGrp;
			void *grp = nGrp ? malloc((size_t)nGrp << 3) : 0;
			*(void **)(col + 0x44) = grp;                 // puVar9[0x11]
			if(nGrp && grp == 0){ SaColErr("Out of memory (groups)."); return (void *)data; }
			for(unsigned i = 0; i < nGrp; i++){
				unsigned char *e = (unsigned char *)grp + (size_t)i * 8;
				*(unsigned short *)(e + 0) = q[0];
				*(unsigned short *)(e + 2) = q[2];
				*(unsigned short *)(e + 4) = q[4];
				{
					unsigned dw = *(int *)(q + 6);        // dword at byte +12
					e[6] = (unsigned char)(dw & 0xFF);            // (char)uVar4
					e[7] = (unsigned char)((dw >> 24) & 0xFF);    // (char)(uVar4 >> 0x18)
				}
				q += 8;
			}
		}
		// NOTE: the original does NOT store sec (puVar5[0xa16]) on this branch
		// and never frees it (faithful leak — the deep copies own everything).
		AttachColToInfo(veh, col);
		return (void *)data;
	}

	// ---------------------- COL2 / COL3 branch (in-place pointers) ----------
	SaColSectionBuf(veh) = sec;                          // puVar5[0xa16] = _Str1 (veh+0x2858)
	*(unsigned short *)(col + 0x2C) = *(unsigned short *)(sec + 0x4C);  // puVar9[0xb]
	*(unsigned short *)(col + 0x28) = *(unsigned short *)(sec + 0x48);  // puVar9[10]
	*(unsigned short *)(col + 0x2A) = *(unsigned short *)(sec + 0x4A);
	col[0x2E] = 0;
	*(int *)(col + 0x48) = 0;                            // puVar9[0x12]
	MvlVectorCopy((float *)(col + 0x10), (const float *)(sec + 0x20));  // puVar9+4
	MvlVectorCopy((float *)(col + 0x1C), (const float *)(sec + 0x2C));  // extraout_ECX+0xc =>
	                                                                    // RECONSTRUCTED col+0x1C
	                                                                    // (matches the COLL
	                                                                    // branch's third vector)
	MvlVectorCopy((float *)(col + 0x00), (const float *)(sec + 0x38));  // puVar9
	*(int *)(col + 0x0C) = *(int *)(sec + 0x44);         // puVar9[3]

	// Array pointers INTO the stored section (payload base = sec + 4).
	// BUGFIX(stable-1.0): the four dword offsets at +0x54/+0x58/+0x60/+0x64
	// are signed ints from the untrusted header. A negative or oversized
	// offset turns each pointer below into an out-of-bounds address that the
	// game later dereferences (and the group scan + vertex conversion read
	// through them right here). Validate against the payload bounds.
	{
		const int offSph  = *(int *)(sec + 0x54);
		const int offMesh = *(int *)(sec + 0x58);
		const int offVert = *(int *)(sec + 0x60);
		const int offGrp  = *(int *)(sec + 0x64);
		const int payload = (int)size - 4;               // bytes after the +4 base
		if(offSph  < 0 || offSph  > payload ||
		   offMesh < 0 || offMesh > payload ||
		   offVert < 0 || offVert > payload ||
		   offGrp  < 0 || offGrp  > payload){
			SaColErr("SA collision section has invalid array offsets.");
			free(raw);
			return (void *)data;
		}
		*(void **)(col + 0x34) = (void *)(sec + 4 + offSph);   // puVar9[0xd]
		*(int *)(col + 0x38) = 0;                              // puVar9[0xe]
		*(void **)(col + 0x3C) = (void *)(sec + 4 + offMesh);  // puVar9[0xf]
		{
			unsigned char *groups = sec + 4 + offGrp;          // puVar9[0x11]
			unsigned nGrp = *(unsigned short *)(col + 0x2C);
			// BUGFIX(stable-1.0): the group scan reads nGrp*8 bytes.
			if((size_t)(secEnd - groups) < (size_t)nGrp * 8){
				SaColErr("SA collision section truncated (groups).");
				free(raw);
				return (void *)data;
			}
			unsigned maxIdx = 0xFFFFFFFFu;                  // decomp uVar17 = -1
			for(unsigned i = 0; i < nGrp; i++){
				unsigned a = *(unsigned short *)(groups + i * 8 + 0);
				unsigned b = *(unsigned short *)(groups + i * 8 + 2);
				unsigned c = *(unsigned short *)(groups + i * 8 + 4);
				// decomp compares SIGNED (int)uVar17 < (int)uVar13
				if((int)maxIdx < (int)a) maxIdx = a;
				if((int)maxIdx < (int)b) maxIdx = b;
				if((int)maxIdx < (int)c) maxIdx = c;
			}
			{
				// Vertex conversion: int16 triples -> float (/128 = *0.0078125f),
				// cursor base = sec + 4 + off + 4 with the first triple read at
				// [-2,-1,0] shorts (decomp psVar15).
				unsigned n = maxIdx + 1;                     // (uVar17 + 1) * 0xC bytes
				// BUGFIX(stable-1.0): the conversion reads n triples of 3
				// shorts starting at (sec+4+offVert); maxIdx comes from the
				// untrusted group table, so bound the read.
				if((int)maxIdx >= 0 &&
				   (size_t)(secEnd - (sec + 4 + offVert)) < (size_t)n * 6){
					SaColErr("SA collision section truncated (vertices).");
					free(raw);
					return (void *)data;
				}
				float *verts = (float *)malloc((size_t)n * 0x0C);
				SaColVertexBuf(veh) = verts;                 // puVar5[0xa17] (veh+0x285c)
				if(verts == 0 && n != 0){ SaColErr("Out of memory (vertices)."); return (void *)data; }
				if((int)maxIdx >= 0){
					short *sv = (short *)(sec + 4 + offVert + 4);
					for(unsigned i = n; i != 0; i--){
						verts[0] = (float)sv[-2] * 0.0078125f;
						verts[1] = (float)sv[-1] * 0.0078125f;
						verts[2] = (float)sv[0]  * 0.0078125f;
						verts += 3;
						sv += 3;
					}
				}
				*(void **)(col + 0x40) = SaColVertexBuf(veh);   // puVar9[0x10] = puVar5[0xa17]
			}
		}
	}
	AttachColToInfo(veh, col);
	return (void *)data;
}
