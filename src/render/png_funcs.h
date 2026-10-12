// png_funcs.h — CPngFuncs: dynamic libpng15 host + PNG->RwImage->texture.
//
// Faithful recompilation from decompiled vehmod_retail.dll 0.98e (decomp
// source: .harness-memory/limout/png/range-decomp.txt, cluster
// 0x1bfb3-0x1ca9a). Every function carries its origin VA; original symbol
// names are reused 1:1 (error-string evidence: "CPngFuncs::LoadPngFuncs",
// "CPngFuncs::RtPNGImageStreamRead", "CHelpers::PngFileToTexture").
//
// Constraint (binding): NO vendored libpng headers. libpng15 is loaded
// DYNAMICALLY exactly like the original — LoadLibraryA("mvl/libpng15.dll")
// + GetProcAddress into the MvlPngApi proc table below. This is the same
// idiom as the Python host's MvlPyApi (src/script/python_host.h): named
// proc-table of function pointers, filled by LoadPngFuncs, one struct slot
// per original .data dword (slot VAs on each member).
#pragma once

#include "../core/stdafx.h"

#include <stdint.h>

// ---------------------------------------------------------------------------
// 1. MvlPngApi — the mvl/libpng15.dll proc table (16 exports; original
//    GetProcAddress targets FUN_1001c3f0 fills). Types are void* stand-ins
//    (no libpng headers); signatures widen when the render lane lands.
// ---------------------------------------------------------------------------
struct MvlPngApi
{
	void *(*png_create_read_struct_2)(const char *user_png_ver, void *err_ptr,
	                                  void *error_fn, void *warn_fn,
	                                  void *mem_ptr, void *malloc_fn,
	                                  void *free_fn);       // DAT_100b0d80 @ RVA 0xb0d80
	void *(*png_create_info_struct)(void *png_ptr);        // DAT_100b0d90 @ RVA 0xb0d90
	void  (*png_destroy_read_struct)(void *png_ptr_ptr,
	                                 void *info_ptr_ptr,
	                                 void *end_info_ptr_ptr); // DAT_100b0d68 @ RVA 0xb0d68
	void  (*png_set_read_fn)(void *png_ptr, void *io_ptr,
	                         void *read_fn);               // DAT_100b0d70 @ RVA 0xb0d70
	void  (*png_read_info)(void *png_ptr, void *info_ptr); // DAT_100b0d6c @ RVA 0xb0d6c
	void  (*png_get_IHDR)(void *png_ptr, void *info_ptr, void *width,
	                      void *height, void *bit_depth, void *color_type,
	                      void *interlace, void *compression,
	                      void *filter);                    // DAT_100b0d98 @ RVA 0xb0d98
	void  (*png_set_strip_16)(void *png_ptr);              // DAT_100b0d8c @ RVA 0xb0d8c
	void  (*png_set_packing)(void *png_ptr);               // DAT_100b0d88 @ RVA 0xb0d88
	void  (*png_set_expand_gray_1_2_4_to_8)(void *png_ptr); // DAT_100b0d74 @ RVA 0xb0d74
	void  (*png_set_gray_to_rgb)(void *png_ptr);           // DAT_100b0d7c @ RVA 0xb0d7c
	void  (*png_set_filler)(void *png_ptr, unsigned filler,
	                        int flags);                    // DAT_100b0d94 @ RVA 0xb0d94
	unsigned (*png_get_valid)(void *png_ptr, void *info_ptr,
	                          unsigned flag);              // DAT_100b0d9c @ RVA 0xb0d9c
	void  (*png_get_PLTE)(void *png_ptr, void *info_ptr, void *palette,
	                      void *num_palette);              // DAT_100b0da4 @ RVA 0xb0da4
	void  (*png_get_tRNS)(void *png_ptr, void *info_ptr, void *trans,
	                      void *num_trans,
	                      void *trans_values);             // DAT_100b0d78 @ RVA 0xb0d78
	void  (*png_read_image)(void *png_ptr, void *image);   // DAT_100b0da8 @ RVA 0xb0da8
	void  (*png_read_end)(void *png_ptr, void *info_ptr);  // DAT_100b0d84 @ RVA 0xb0d84
};

// Canonical storage (png_funcs.cpp) — the original keeps the same table as 16
// separate .data dwords; one struct keeps the mapping 1:1 (slot VAs are on
// each member). Do NOT redeclare per-TU.
extern MvlPngApi g_MvlPngApi;

// DAT_100b0cef @ RVA 0xb0cef — one-shot: LoadPngFuncs attempted (byte flag).
extern unsigned char MvlPngLibInitDone;

// DAT_100b0da0 @ RVA 0xb0da0 — one-shot: libpng15 load FAILED (byte flag).
extern unsigned char MvlPngLibFailed;

// ---------------------------------------------------------------------------
// 2. Engine / render seams. Stubs in png_funcs.cpp log "HOOK POINT" or
//    "TODO(media)" and return benign values; game behaviour is NOT faked.
//    MvlRwMemoryStreamOpen/Close are REUSED from
//    src/archive/vehicle_archive.h (FUN_100320a0/FUN_10032090).
// ---------------------------------------------------------------------------

// FUN_100327b0 @ RVA 0x327b0 — create the engine image object (RwImage-ish);
// argument list unrecovered (decomp call dropped args).
void *MvlRwImageCreate(void);                        // TODO(media): 0x327b0
// FUN_100327d0 @ RVA 0x327d0 — allocate the image pixel buffer (uses the
// IHDR dimensions captured earlier; args unrecovered).
bool  MvlRwImageAllocatePixels(void *image);         // TODO(media): 0x327d0
// FUN_100327e0 @ RVA 0x327e0 — release image pixels (catch-path teardown,
// called before MvlRwImageDelete in the RtPNGImageStreamRead catch).
void  MvlRwImageReleasePixels(void *image);          // TODO(media): 0x327e0
// FUN_100327c0 @ RVA 0x327c0 — tear the image object down (catch paths).
void  MvlRwImageDelete(void *image);                 // TODO(media): 0x327c0
// FUN_1001ca90 @ RVA 0x1ca90 — convert the decoded RwImage to a texture.
void *MvlRwImageToTexture(void *image);              // TODO(media): 0x1ca90
// FUN_1001ca06 @ RVA 0x1ca06 — success-path cleanup of RtPNGImageStreamRead
// (row-pointer array + read-struct teardown; body outside decomp range).
void  MvlPngReadCleanup(void);                       // TODO(media): 0x1ca06

// Image-object field view used by RtPNGImageStreamRead's row-pointer build:
// (provisional) — only +0x10 (row stride) and +0x14 (pixel buffer) are
// recovered from FUN_1001c690.
struct SMvlRwImageFields
{
	unsigned char pad00[0x10];      // @ 0x00..0x0f unrecovered
	uint32_t      nRowStride;       // @ 0x10 bytes per row
	void         *pPixels;          // @ 0x14 pixel buffer
};

// PNG read-state view (provisional) — FUN_1001c650 teardown layout.
struct SMvlPngReadState
{
	void *pPngPtr;                  // @ 0x00 png_structp
	void *pInfoPtr;                 // @ 0x04 png_infop
	void *pRowPtrs;                 // @ 0x08 row-pointer array (free'd first)
};

// ---------------------------------------------------------------------------
// 3. CPngFuncs — recovered method names 1:1.
// ---------------------------------------------------------------------------
class CPngFuncs
{
public:
	// CPngFuncs::LoadPngFuncs — 0.98e @ RVA 0x1c3f0.
	// LoadLibraryA("mvl/libpng15.dll") + 16x GetProcAddress. Any missing
	// export fails the whole load ("...%d functions are missing from
	// libpng15.dll" + FreeLibrary).
	static bool LoadPngFuncs(void);

	// FUN_1001c5b0 @ RVA 0x1c5b0 — (provisional) EnsureLoaded: one-shot
	// wrapper over LoadPngFuncs (DAT_100b0cef done / DAT_100b0da0 failed).
	static bool EnsureLoaded(void);

	// FUN_1001c5f0 @ RVA 0x1c5f0 — (provisional) PngReadCallback: the
	// png_set_read_fn read callback; pulls len bytes from the stream passed
	// as the first argument (decomp: FUN_10032070 with dropped args).
	static void PngReadCallback(void *stream, void *data, uint32_t len);

	// FUN_1001c650 @ RVA 0x1c650 — (provisional) FreeReadState: frees the
	// row-pointer array then png_destroy_read_struct.
	static void FreeReadState(SMvlPngReadState *state);

	// CPngFuncs::RtPNGImageStreamRead — 0.98e @ RVA 0x1c690, catch @ 0x1c9aa.
	// Decodes PNG -> engine image (throws on failure in the original; the
	// clone throws SMvlPngError and the catch logs verbatim).
	static void *RtPNGImageStreamRead(void *stream);
};

// CHelpers::PngFileToTexture — free function (CHelpers namespace concept,
// same convention as CHelpers::XmlParseVector in src/config/xml_helpers.h).
// Body FUN_1001bed0 @ RVA 0x1bed0, catch @ RVA 0x1bfb3. PROVISIONAL
// signature: the decomp prototype is (void) but the body opens a memory
// stream (FUN_100320a0(data,size)) — see port-notes-media.md OQ-4.
void *PngFileToTexture(const void *data, uint32_t size);
