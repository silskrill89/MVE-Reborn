// png_funcs.cpp — CPngFuncs implementation (see png_funcs.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/png/range-decomp.txt). Log strings are verbatim
// from the decomp. libpng15 is loaded dynamically (no vendored headers).
#include "png_funcs.h"

#include "../archive/vehicle_archive.h"   // MvlRwMemoryStreamOpen/Close seams
#include "../loader/log.h"

#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// x86 target (VC 1.0 plugin).
// ---------------------------------------------------------------------------
#if !defined(_M_IX86)
#error "MVL clone: png_funcs.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// Log wrappers — FUN_1001cc00 msg / FUN_1001cce0 warn / FUN_1001cb80 error
// (this module only emits error-log lines in the recovered range).
// ---------------------------------------------------------------------------
static void
PngErr(const char *fn, const char *fmt, ...)
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

// Clone-local throw mechanism (same idiom as src/archive/vehicle_archive.cpp:
// the original throws through FUN_1001b8d0 + __CxxThrowException and the
// catch filter logs "fn" "%s").
struct SMvlPngError
{
	char msg[512];
};

static void
PngThrow(const char *fmt, ...)
{
	SMvlPngError e;
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

// ---------------------------------------------------------------------------
// Globals — canonical storage (original .data dwords; DAT_ comments per the
// globals naming rule).
// ---------------------------------------------------------------------------
MvlPngApi g_MvlPngApi;                    // DAT_100b0d68..DAT_100b0da8 (16 slots)
unsigned char MvlPngLibInitDone;          // DAT_100b0cef @ RVA 0xb0cef
unsigned char MvlPngLibFailed;            // DAT_100b0da0 @ RVA 0xb0da0

// MvlRwStreamRead is the shared engine stream seam (FUN_10032070 @ 0x32070)
// used by the png read callback; it is DEFINED in src/anim/custom_anim.cpp
// (the IFP lane is its primary consumer) — see port-notes-media.md.
extern unsigned MvlRwStreamRead(void *stream, void *dst, unsigned len);

// ===========================================================================
// CPngFuncs::LoadPngFuncs — vehmod_retail.dll 0.98e @ RVA 0x1c3f0
// ===========================================================================
bool
CPngFuncs::LoadPngFuncs(void)
{
	static const char *const fn = "CPngFuncs::LoadPngFuncs";
	HMODULE hModule;
	int nMissing;

	hModule = LoadLibraryA("mvl/libpng15.dll");
	if(hModule == 0){
		DWORD err = GetLastError();
		PngErr(fn, "Could not load 'mvl/libpng15.dll', LoadLibrary failed with error %d.",
		       (int)err);
		return false;
	}

	nMissing = 0;
	g_MvlPngApi.png_create_read_struct_2 =
		(void *(*)(const char *, void *, void *, void *, void *, void *, void *))
		GetProcAddress(hModule, "png_create_read_struct_2");
	if(g_MvlPngApi.png_create_read_struct_2 == 0) nMissing++;
	g_MvlPngApi.png_create_info_struct =
		(void *(*)(void *))GetProcAddress(hModule, "png_create_info_struct");
	if(g_MvlPngApi.png_create_info_struct == 0) nMissing++;
	g_MvlPngApi.png_destroy_read_struct =
		(void (*)(void *, void *, void *))GetProcAddress(hModule, "png_destroy_read_struct");
	if(g_MvlPngApi.png_destroy_read_struct == 0) nMissing++;
	g_MvlPngApi.png_set_read_fn =
		(void (*)(void *, void *, void *))GetProcAddress(hModule, "png_set_read_fn");
	if(g_MvlPngApi.png_set_read_fn == 0) nMissing++;
	g_MvlPngApi.png_read_info =
		(void (*)(void *, void *))GetProcAddress(hModule, "png_read_info");
	if(g_MvlPngApi.png_read_info == 0) nMissing++;
	g_MvlPngApi.png_get_IHDR =
		(void (*)(void *, void *, void *, void *, void *, void *, void *, void *, void *))
		GetProcAddress(hModule, "png_get_IHDR");
	if(g_MvlPngApi.png_get_IHDR == 0) nMissing++;
	g_MvlPngApi.png_set_strip_16 =
		(void (*)(void *))GetProcAddress(hModule, "png_set_strip_16");
	if(g_MvlPngApi.png_set_strip_16 == 0) nMissing++;
	g_MvlPngApi.png_set_packing =
		(void (*)(void *))GetProcAddress(hModule, "png_set_packing");
	if(g_MvlPngApi.png_set_packing == 0) nMissing++;
	g_MvlPngApi.png_set_expand_gray_1_2_4_to_8 =
		(void (*)(void *))GetProcAddress(hModule, "png_set_expand_gray_1_2_4_to_8");
	if(g_MvlPngApi.png_set_expand_gray_1_2_4_to_8 == 0) nMissing++;
	g_MvlPngApi.png_set_gray_to_rgb =
		(void (*)(void *))GetProcAddress(hModule, "png_set_gray_to_rgb");
	if(g_MvlPngApi.png_set_gray_to_rgb == 0) nMissing++;
	g_MvlPngApi.png_set_filler =
		(void (*)(void *, unsigned, int))GetProcAddress(hModule, "png_set_filler");
	if(g_MvlPngApi.png_set_filler == 0) nMissing++;
	g_MvlPngApi.png_get_valid =
		(unsigned (*)(void *, void *, unsigned))GetProcAddress(hModule, "png_get_valid");
	if(g_MvlPngApi.png_get_valid == 0) nMissing++;
	g_MvlPngApi.png_get_PLTE =
		(void (*)(void *, void *, void *, void *))GetProcAddress(hModule, "png_get_PLTE");
	if(g_MvlPngApi.png_get_PLTE == 0) nMissing++;
	g_MvlPngApi.png_get_tRNS =
		(void (*)(void *, void *, void *, void *, void *))GetProcAddress(hModule, "png_get_tRNS");
	if(g_MvlPngApi.png_get_tRNS == 0) nMissing++;
	g_MvlPngApi.png_read_image =
		(void (*)(void *, void *))GetProcAddress(hModule, "png_read_image");
	if(g_MvlPngApi.png_read_image == 0) nMissing++;
	g_MvlPngApi.png_read_end =
		(void (*)(void *, void *))GetProcAddress(hModule, "png_read_end");
	if(g_MvlPngApi.png_read_end == 0) nMissing++;

	if(nMissing != 0){
		PngErr(fn, "%d functions are missing from libpng15.dll", nMissing);
		FreeLibrary(hModule);
		return false;
	}
	return true;
}

// ===========================================================================
// FUN_1001c5b0 @ RVA 0x1c5b0 — (provisional) CPngFuncs::EnsureLoaded
// ===========================================================================
bool
CPngFuncs::EnsureLoaded(void)
{
	if(MvlPngLibInitDone == 0){
		bool ok = LoadPngFuncs();
		MvlPngLibFailed = ok ? 0 : 1;
		MvlPngLibInitDone = 1;
		return MvlPngLibFailed == 0;
	}
	return MvlPngLibFailed == 0;
}

// ===========================================================================
// FUN_1001c5f0 @ RVA 0x1c5f0 — (provisional) CPngFuncs::PngReadCallback
// The png_set_read_fn read callback. The decomp's FUN_10032070 call has its
// arguments dropped; the stream handle is the first callback argument as
// recovered (see port-notes-media.md OQ-5).
// ===========================================================================
void
CPngFuncs::PngReadCallback(void *stream, void *data, uint32_t len)
{
	if(len == 0)
		return;
	if(MvlRwStreamRead(stream, data, len) == 0)
		PngThrow("Failed to read data from PNG file via stream.");
}

// ===========================================================================
// FUN_1001c650 @ RVA 0x1c650 — (provisional) CPngFuncs::FreeReadState
// ===========================================================================
void
CPngFuncs::FreeReadState(SMvlPngReadState *state)
{
	if(!state)
		return;
	if(state->pRowPtrs != 0)
		free(state->pRowPtrs);
	if(state->pPngPtr != 0){
		void *pInfoPtrPtr = state->pInfoPtr != 0 ? (void *)&state->pInfoPtr : 0;
		if(g_MvlPngApi.png_destroy_read_struct != 0)
			g_MvlPngApi.png_destroy_read_struct(&state->pPngPtr, pInfoPtrPtr, 0);
	}
}

// ===========================================================================
// CPngFuncs::RtPNGImageStreamRead — vehmod_retail.dll 0.98e @ RVA 0x1c690
// catch filter 0.98e @ RVA 0x1c9aa ("CPngFuncs::RtPNGImageStreamRead" "%s").
// ===========================================================================
void *
CPngFuncs::RtPNGImageStreamRead(void *stream)
{
	static const char *const fn = "CPngFuncs::RtPNGImageStreamRead";
	void *pngPtr = 0;
	void *infoPtr = 0;
	void *image = 0;
	void *rows = 0;
	uint32_t width = 0, height = 0;
	int bitDepth = 0, colorType = 0, interlaceType = 0;
	int compressionType = 0, filterType = 0;
	uint32_t i;

	if(!EnsureLoaded()){
		// Direct error log + NULL return (NOT thrown in the original).
		PngErr(fn, "PNG library not loaded.");
		return 0;
	}

	try{
		pngPtr = g_MvlPngApi.png_create_read_struct_2("1.5.13", 0, 0, 0, 0, 0, 0);
		if(pngPtr == 0)
			PngThrow("Failed to create read struct.");

		infoPtr = g_MvlPngApi.png_create_info_struct(pngPtr);
		if(infoPtr == 0)
			PngThrow("Failed to create info struct.");

		g_MvlPngApi.png_set_read_fn(pngPtr, stream,
		                            (void *)(void (*)(void *, void *, uint32_t))
		                                &CPngFuncs::PngReadCallback);
		g_MvlPngApi.png_read_info(pngPtr, infoPtr);
		g_MvlPngApi.png_get_IHDR(pngPtr, infoPtr, &width, &height, &bitDepth,
		                         &colorType, &interlaceType, &compressionType,
		                         &filterType);
		g_MvlPngApi.png_set_strip_16(pngPtr);

		switch(colorType){
		case 0:                                   // grayscale
			if(bitDepth < 8)
				g_MvlPngApi.png_set_expand_gray_1_2_4_to_8(pngPtr);
			break;
		case 2:                                   // RGB -> RGBA
			g_MvlPngApi.png_set_filler(pngPtr, 0xff, 1);
			goto create_image;
		case 3:                                   // palette
			if(bitDepth < 8)
				g_MvlPngApi.png_set_packing(pngPtr);
			break;
		case 4:                                   // gray+alpha -> RGBA
			g_MvlPngApi.png_set_gray_to_rgb(pngPtr);
			goto create_image;
		case 6:                                   // RGBA
		create_image:
			image = MvlRwImageCreate();
			if(image == 0)
				PngThrow("Failed to create image.");
			if(!MvlRwImageAllocatePixels(image))
				PngThrow("Failed to allocate image pixels.");

			rows = malloc((size_t)height * 4);
			if(rows == 0)
				PngThrow("Failed to allocate image pixels."); // CLONE-GUARD
			for(i = 0; i < height; i++){
				SMvlRwImageFields *im = (SMvlRwImageFields *)image;
				((void **)rows)[i] = (char *)im->pPixels + im->nRowStride * i;
			}
			g_MvlPngApi.png_read_image(pngPtr, rows);
			g_MvlPngApi.png_read_end(pngPtr, infoPtr);
			MvlPngReadCleanup();                  // FUN_1001ca06 (out of range)
			return image;
		default:
			// Recovered as-is: the decomp tests local_c4 (interlace_type)
			// against 0x1f here — a dead branch in the PNG color-type
			// domain (see port-notes-media.md OQ-6).
			if(0x1f < interlaceType)
				goto create_image;
			break;
		}

		PngThrow("Only 32-bit PNG is currently supported!");
	}catch(const SMvlPngError &e){
		// Catch@1001c9aa: image teardown, then the verbatim "%s" log.
		if(image != 0){
			MvlRwImageReleasePixels(image);       // FUN_100327e0
			MvlRwImageDelete(image);              // FUN_100327c0
			image = 0;
		}
		PngErr(fn, "%s", e.msg);
		return 0;
	}
	return 0;
}

// ===========================================================================
// CHelpers::PngFileToTexture — vehmod_retail.dll 0.98e: body FUN_1001bed0
// @ RVA 0x1bed0, catch filter @ 0x1bfb3 ("CHelpers::PngFileToTexture" "%s").
// ===========================================================================
void *
PngFileToTexture(const void *data, uint32_t size)
{
	void *stream = 0;
	void *image = 0;
	void *texture = 0;

	try{
		stream = MvlRwMemoryStreamOpen(data, size);   // FUN_100320a0
		if(stream == 0)
			PngThrow("Could not open memory stream for PNG file.");
		image = CPngFuncs::RtPNGImageStreamRead(stream);   // FUN_1001c690
		if(image == 0)
			PngThrow("Could not read RwImage from PNG file.");
		texture = MvlRwImageToTexture(image);             // FUN_1001ca90
		if(texture == 0)
			PngThrow("Could not get texture from RwImage.");
	}catch(const SMvlPngError &e){
		// Catch@1001bfb3: teardown of the half-built image, then the
		// verbatim "%s" log.
		if(image != 0){
			MvlRwImageDelete(image);              // FUN_100327c0
			image = 0;
		}
		PngErr("CHelpers::PngFileToTexture", "%s", e.msg);
		texture = 0;
	}

	if(stream != 0)
		MvlRwMemoryStreamClose(stream);            // FUN_10032090
	return texture;
}

// ===========================================================================
// Engine / render seams — unrecoverable callees. TODO(media) stubs: log-once
// via MvlErrorLog, benign return, nothing real written.
// ===========================================================================
void *
MvlRwImageCreate(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlRwImageCreate @ 0x327b0 (args unrecovered)");
	return 0;
}

bool
MvlRwImageAllocatePixels(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlRwImageAllocatePixels @ 0x327d0 (args unrecovered)");
	return false;
}

void
MvlRwImageReleasePixels(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlRwImageReleasePixels @ 0x327e0");
}

void
MvlRwImageDelete(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlRwImageDelete @ 0x327c0");
}

void *
MvlRwImageToTexture(void *)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlRwImageToTexture @ 0x1ca90");
	return 0;
}

void
MvlPngReadCleanup(void)
{
	static bool s_logged;
	MediaTodoOnce(&s_logged, "TODO(media): MvlPngReadCleanup @ 0x1ca06 (row array + read struct)");
}
