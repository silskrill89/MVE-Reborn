// model_helpers.cpp - ports of the MVL 0.98e hook-helper functions.
//
// Source: .harness-memory/limout/helpers/decomp-chain.txt (Ghidra decomp of
// vehmod_retail.dll 0.98e, image base 0x10000000). Every function carries its
// original RVA; Ghidra FUN_<VA> names kept per the binding naming policy.
//
// Layout of this TU:
//   1. plugin .data globals (file-local, provisional types; see port notes)
//   2. helpers (log-once stub machinery, game-memory pokes)
//   3. depth-1 stubs  - marked `// TODO(helpers): <VA> unported`
//   4. depth-1 ports  - self-contained leaf/helper implementations
//   5. depth-0 roots  - the 11 helpers + the driver FUN_10017820
//
// Conventions:
//   * VC 1.0 game addresses are 0x00xxxxxx VAs and are accessed through
//     reinterpret_cast (only valid when reached from an installed hook);
//     plugin-internal VAs are 0x100xxxxx and become file-local globals here.
//   * MSVC C3865: no __thiscall on free functions - Ghidra __thiscall reads
//     are lowered to __fastcall + dummy `void* /*edx*/` (see model_helpers.h).
//   * Unrecoverable callees are inert stubs that log once via MvlErrorLog and
//     return the decomp's default; marked TODO(helpers) with the VA.
#include "../core/stdafx.h"
#include "model_helpers.h"
#include "engine_globals.h"
#include "model_id.h"
#include "dirmgr.h"
#include "game_hooks.h"
#include "../loader/log.h"

#include <stdlib.h> // malloc / free
#include <stdio.h>  // snprintf
#include <math.h>

#if !defined(_M_IX86)
#error "MVL clone: model_helpers.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// Section 1: plugin-side .data globals now live in engine_globals.cpp (single
// storage owner; see engine_globals.h for the Mvl* names + original DAT_ RVAs
// and globals-map.md for the census). This TU reads/writes them in the ports.
// ---------------------------------------------------------------------------

// ---------------------------------------------------------------------------
// VC 1.0 game addresses referenced below (RVA = VA - 0x400000).
// ---------------------------------------------------------------------------
static constexpr uint32_t kVC_PaintByteGlobal1   = 0x00A10B26u; // 0.98e @ RVA 0x610B26
static constexpr uint32_t kVC_PaintByteGlobal2   = 0x00A10B82u; // 0.98e @ RVA 0x610B82
static constexpr uint32_t kVC_PartFlagByte       = 0x00571820u; // 0.98e @ RVA 0x171820
static constexpr uint32_t kVC_LiveryColorTable   = 0x00939228u; // 0.98e @ RVA 0x539228
static constexpr uint32_t kVC_SpeedCheckObject   = 0x0094AD28u; // 0.98e @ RVA 0x54AD28
static constexpr uint32_t kVC_FrameCounter       = 0x00974B2Cu; // 0.98e @ RVA 0x574B2C
static constexpr uint32_t kVC_ClassCounterBase   = 0x009785A8u; // 0.98e @ RVA 0x5785A8 (int per slot)
static constexpr uint32_t kVC_CacheByte1         = 0x0063C6F2u; // 0.98e @ RVA 0x23C6F2 (.text byte)
static constexpr uint32_t kVC_CacheByte2         = 0x0063C71Du; // 0.98e @ RVA 0x23C71D
static constexpr uint32_t kVC_CacheByte3         = 0x0063CE27u; // 0.98e @ RVA 0x23CE27
static constexpr uint32_t kVC_CacheRestoreByte   = 0x006DDE20u; // 0.98e @ RVA 0x2DDE20 (.data byte)
static constexpr uint32_t kVC_ClassSlotBase      = 0x007DB888u; // 0.98e @ RVA 0x3DB888 (stride 0x1C)
static constexpr uint32_t kVC_ClassObjTable      = 0x0094B708u; // 0.98e @ RVA 0x54B708

// ---------------------------------------------------------------------------
// Section 2: port helpers.
// ---------------------------------------------------------------------------
namespace {

// Unported-callee stub: log once per stub site (hot per-frame paths would
// otherwise flood the log), keep an inert default.
#define MVL_HELPERS_TODO(name)                                                \
	do {                                                                      \
		static bool s_logged = false;                                         \
		if (!s_logged) {                                                      \
			s_logged = true;                                                  \
			MvlErrorLog("helpers: TODO(helpers): " #name " unported - inert");\
		}                                                                     \
	} while (0)

// Byte poke into the (unlocked) VC 1.0 .text / .data image.
inline void PokeByte(uint32_t va, uint8_t value)
{
	*reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(va)) = value;
}

inline uint8_t PeekByte(uint32_t va)
{
	return *reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(va));
}

inline uint32_t PeekU32(uint32_t va)
{
	return *reinterpret_cast<const uint32_t *>(static_cast<uintptr_t>(va));
}

} // namespace

// ===========================================================================
// Section 3: depth-1 stubs (unrecoverable from the decomp alone).
// Each is marked: TODO(helpers): <VA> unported.
// ===========================================================================

// TODO(helpers): 0x1001af70 unported - entity -> attachment lookup.
static int32_t __fastcall FUN_1001af70(int32_t entity)
{
	MVL_HELPERS_TODO(FUN_1001af70);
	(void)entity;
	return 0;
}

// TODO(helpers): 0x1001b280 unported - attachment world-position build.
static void __fastcall FUN_1001b280(void *out, void *entity, float a, float b)
{
	MVL_HELPERS_TODO(FUN_1001b280);
	(void)out; (void)entity; (void)a; (void)b;
}

// TODO(helpers): 0x1001b640 unported - attach/detach transform fixup
// (implicit XMM inputs lost by the decompiler; not recoverable 1:1).
static void __fastcall FUN_1001b640(void *param_1, char param_2)
{
	MVL_HELPERS_TODO(FUN_1001b640);
	(void)param_1; (void)param_2;
}

// TODO(helpers): 0x1002d880 unported - "current model/directory index"
// (decomp depends on an ECX value produced by FUN_100173d0).
static int32_t FUN_1002d880(void)
{
	MVL_HELPERS_TODO(FUN_1002d880);
	return 0;
}

// TODO(helpers): 0x1003d120 unported - current model extra-flags fetch.
static uint32_t __fastcall FUN_1003d120(int32_t modelIndex)
{
	MVL_HELPERS_TODO(FUN_1003d120);
	(void)modelIndex;
	return 0;
}

// TODO(helpers): 0x1004f380 unported - directory slot array indexer.
static int *__fastcall FUN_1004f380(int32_t index)
{
	MVL_HELPERS_TODO(FUN_1004f380);
	(void)index;
	return NULL;
}

// TODO(helpers): 0x1002faa0 unported - cached-parts destructor thunk.
static void thunk_FUN_1002faa0(void *mem)
{
	MVL_HELPERS_TODO(thunk_FUN_1002faa0);
	(void)mem;
}

// TODO(helpers): 0x10059d7f unported - plugin-CRT operator new(size);
// falls back to plain malloc (same static-CRT heap the original used).
static void *FUN_10059d7f(size_t size)
{
	MVL_HELPERS_TODO(FUN_10059d7f);
	void *p = malloc(size);
	if (!p)
		MvlErrorLog("helpers: FUN_10059d7f: out of memory (%u)", (unsigned)size);
	return p;
}

// TODO(helpers): 0x1002f690 unported - parts-object constructor.
static void *__fastcall FUN_1002f690(void *obj)
{
	MVL_HELPERS_TODO(FUN_1002f690);
	return obj;
}

// TODO(helpers): 0x1002f8b0 unported - parts-object data fill.
static void __fastcall FUN_1002f8b0(void *obj, const uint8_t *src)
{
	MVL_HELPERS_TODO(FUN_1002f8b0);
	(void)obj; (void)src;
}

// TODO(helpers): 0x1002f8a0 unported - parts-object sub-init.
static void __fastcall FUN_1002f8a0(void *obj)
{
	MVL_HELPERS_TODO(FUN_1002f8a0);
	(void)obj;
}

// TODO(helpers): 0x1002f880 unported - parts-object sub-init.
static void __fastcall FUN_1002f880(int32_t obj)
{
	MVL_HELPERS_TODO(FUN_1002f880);
	(void)obj;
}

// TODO(helpers): 0x1002e8d0 unported - wheel-cache flush body.
static void FUN_1002e8d0(void)
{
	MVL_HELPERS_TODO(FUN_1002e8d0);
}

// ---------------------------------------------------------------------------
// RenderWare/game-frame thunk family: each is a tail call into VC 1.0 code
// (decomp shows `(*(code *)0x64xxxx)()`), not recoverable as C. TODO stubs.
// ---------------------------------------------------------------------------
// TODO(helpers): 0x10031ec0 unported - RW frame sync (VC 0x644AD0).
static void FUN_10031ec0(void) { MVL_HELPERS_TODO(FUN_10031ec0); }
// TODO(helpers): 0x10031da0 unported - RW frame update (VC 0x6445F0).
static void FUN_10031da0(void) { MVL_HELPERS_TODO(FUN_10031da0); }
// TODO(helpers): 0x10031d90 unported - RW frame update (VC 0x644470).
static void FUN_10031d90(void) { MVL_HELPERS_TODO(FUN_10031d90); }
// TODO(helpers): 0x10031ee0 unported - RW frame update (VC 0x644D00).
static void FUN_10031ee0(void) { MVL_HELPERS_TODO(FUN_10031ee0); }
// TODO(helpers): 0x10031d40 unported - RW matrix push (VC 0x6437B0).
static void FUN_10031d40(void) { MVL_HELPERS_TODO(FUN_10031d40); }
// TODO(helpers): 0x10032140 unported - RW matrix pop (VC 0x647160).
static void FUN_10032140(void) { MVL_HELPERS_TODO(FUN_10032140); }
// TODO(helpers): 0x100092f0 unported - quaternion normalize chain.
static void FUN_100092f0(void) { MVL_HELPERS_TODO(FUN_100092f0); }
// TODO(helpers): 0x10009300 unported (called by FUN_100092f0).
static void FUN_10009300(void) { MVL_HELPERS_TODO(FUN_10009300); }
// TODO(helpers): 0x100080d0 unported (called by FUN_100092f0 / FUN_10008590).
static void FUN_100080d0(void *p) { MVL_HELPERS_TODO(FUN_100080d0); (void)p; }

// TODO(helpers): 0x10031380 unported - part frame teardown (needs the RW
// vtable at _DAT_007870c0; returns 1 in EAX per decomp).
static uint32_t __fastcall FUN_10031380(int32_t part)
{
	MVL_HELPERS_TODO(FUN_10031380);
	(void)part;
	return 1;
}

// TODO(helpers): 0x1003ab00 unported - projectile/fire-effect emitter
// (heavy FPU + lost-ECX chain; not recoverable 1:1 from the decomp).
static void __fastcall FUN_1003ab00(int32_t a, int32_t b, int32_t c, int32_t d)
{
	MVL_HELPERS_TODO(FUN_1003ab00);
	(void)a; (void)b; (void)c; (void)d;
}

// TODO(helpers): 0x1002dff0 unported - game call into VC 0x4BC1E0
// (returns the local player ped; default 0 = none).
static int32_t __fastcall FUN_1002dff0(int32_t unused)
{
	MVL_HELPERS_TODO(FUN_1002dff0);
	(void)unused;
	return 0;
}

// ---------------------------------------------------------------------------
// CVector / CQuaternion leaf family (pure math, but bodies live in sections
// the decomp did not emit; pass-through defaults keep callers linkable).
// ---------------------------------------------------------------------------
// TODO(helpers): 0x10008f50 unported - CVector product (result written to ret).
static float *__fastcall FUN_10008f50(float *a, float *b)
{
	MVL_HELPERS_TODO(FUN_10008f50);
	(void)b;
	return a;
}
// TODO(helpers): 0x10008fc0 unported - CQuaternion Normalise body.
static float *__fastcall FUN_10008fc0(float *q)
{
	MVL_HELPERS_TODO(FUN_10008fc0);
	return q;
}
// TODO(helpers): 0x10008ee0 unported - CVector product (2nd variant).
static float *__fastcall FUN_10008ee0(float *a, float *b)
{
	MVL_HELPERS_TODO(FUN_10008ee0);
	(void)b;
	return a;
}
// TODO(helpers): 0x10008590 unported - CQuaternion slerp (implicit XMM args).
static void __fastcall FUN_10008590(void *this_, void *a, void *b)
{
	MVL_HELPERS_TODO(FUN_10008590);
	(void)this_; (void)a; (void)b;
}
// TODO(helpers): 0x10008ce0 unported - CVector ctor pass-through.
static void *__fastcall FUN_10008ce0(void *ret) { MVL_HELPERS_TODO(FUN_10008ce0); return ret; }
// TODO(helpers): 0x10009060 unported - CQuaternion Get pass-through.
static void *__fastcall FUN_10009060(void *ret) { MVL_HELPERS_TODO(FUN_10009060); return ret; }
// TODO(helpers): 0x10009190 unported - CQuaternion Get pass-through.
static void *__fastcall FUN_10009190(void *ret) { MVL_HELPERS_TODO(FUN_10009190); return ret; }
// TODO(helpers): 0x10009330 unported - CVector length helper (pass-through).
static float *__fastcall FUN_10009330(float *v) { MVL_HELPERS_TODO(FUN_10009330); return v; }
// TODO(helpers): 0x10008c30 unported - CVector zero-init.
static void *__fastcall FUN_10008c30(void *v)
{
	MVL_HELPERS_TODO(FUN_10008c30);
	memset(v, 0, 12);
	return v;
}
// TODO(helpers): 0x10008c50 unported - CVector zero-init variant.
static void *__fastcall FUN_10008c50(void *v)
{
	MVL_HELPERS_TODO(FUN_10008c50);
	memset(v, 0, 12);
	return v;
}
// TODO(helpers): 0x10008250 unported - CQuaternion Set pass-through.
static void *__fastcall FUN_10008250(void *q, void *src)
{
	MVL_HELPERS_TODO(FUN_10008250);
	(void)src;
	return q;
}

// ---------------------------------------------------------------------------
// Fire-effect / game-object family used by FUN_1003a900 / FUN_1003af20.
// ---------------------------------------------------------------------------
// TODO(helpers): 0x1002e750 unported - effect object alloc (default 0 = none).
static int32_t FUN_1002e750(void) { MVL_HELPERS_TODO(FUN_1002e750); return 0; }
// TODO(helpers): 0x1002e780 unported - effect object flag setup.
static void __fastcall FUN_1002e780(int32_t obj, uint32_t flags)
{ MVL_HELPERS_TODO(FUN_1002e780); (void)obj; (void)flags; }
// TODO(helpers): 0x1002e170 unported - effect object matrix fixup.
static void __fastcall FUN_1002e170(int32_t m) { MVL_HELPERS_TODO(FUN_1002e170); (void)m; }
// TODO(helpers): 0x1000b170 unported - effect registration (returns 0).
static uint32_t FUN_1000b170(void) { MVL_HELPERS_TODO(FUN_1000b170); return 0; }

// ---------------------------------------------------------------------------
// FUN_1003af20 support family (render state + per-ped lookups).
// ---------------------------------------------------------------------------
// TODO(helpers): 0x10037a60 unported - RNG/seed fetch used by the emitter.
static void FUN_10037a60(void) { MVL_HELPERS_TODO(FUN_10037a60); }
// TODO(helpers): 0x1000b130 unported - render alpha state.
static void __fastcall FUN_1000b130(int a, int b, int c, int d)
{ MVL_HELPERS_TODO(FUN_1000b130); (void)a; (void)b; (void)c; (void)d; }
// TODO(helpers): 0x1000b120 unported - render state flush.
static void FUN_1000b120(void) { MVL_HELPERS_TODO(FUN_1000b120); }
// TODO(helpers): 0x1000b070 unported - render state reset.
static void FUN_1000b070(void) { MVL_HELPERS_TODO(FUN_1000b070); }
// TODO(helpers): 0x1000b090 unported - render state toggle.
static void FUN_1000b090(void) { MVL_HELPERS_TODO(FUN_1000b090); }
// TODO(helpers): 0x1000b080 unported - render state toggle.
static void FUN_1000b080(void) { MVL_HELPERS_TODO(FUN_1000b080); }
// TODO(helpers): 0x1002dfd0 unported - player ped lookup (default 0).
static int32_t __fastcall FUN_1002dfd0(int32_t unused)
{ MVL_HELPERS_TODO(FUN_1002dfd0); (void)unused; return 0; }
// TODO(helpers): 0x1006cc5a unported - float10 trig (returns 0.0).
static double FUN_1006cc5a(void) { MVL_HELPERS_TODO(FUN_1006cc5a); return 0.0; }
// TODO(helpers): 0x1006d730 unported - float->double widen.
static double FUN_1006d730(float f) { MVL_HELPERS_TODO(FUN_1006d730); return (double)f; }
// TODO(helpers): 0x1006e460 unported - double->float narrow (returns input).
static float FUN_1006e460(double d) { MVL_HELPERS_TODO(FUN_1006e460); return (float)d; }
// TODO(helpers): 0x1006ccc0 unported - trig helper.
static void FUN_1006ccc0(void *p) { MVL_HELPERS_TODO(FUN_1006ccc0); (void)p; }
// TODO(helpers): 0x1002e340 unported - effect matrix combine.
static void __fastcall FUN_1002e340(void *a, void *b, void *c)
{ MVL_HELPERS_TODO(FUN_1002e340); (void)a; (void)b; (void)c; }
// TODO(helpers): 0x10009380 unported - CVector store pass-through.
static void *__fastcall FUN_10009380(void *v) { MVL_HELPERS_TODO(FUN_10009380); return v; }
// TODO(helpers): 0x10009490 unported - effect position update.
static void FUN_10009490(void) { MVL_HELPERS_TODO(FUN_10009490); }
// TODO(helpers): 0x10009480 unported - effect frame update.
static void FUN_10009480(void) { MVL_HELPERS_TODO(FUN_10009480); }
// TODO(helpers): 0x1002ec60 unported - effect light attach.
static void FUN_1002ec60(void) { MVL_HELPERS_TODO(FUN_1002ec60); }
// TODO(helpers): 0x1001b100 unported - attachment fixup apply.
static void __fastcall FUN_1001b100(int32_t entity)
{ MVL_HELPERS_TODO(FUN_1001b100); (void)entity; }
// TODO(helpers): 0x1001b240 unported - attachment failure path.
static void __fastcall FUN_1001b240(int32_t entity)
{ MVL_HELPERS_TODO(FUN_1001b240); (void)entity; }
// TODO(helpers): 0x1001af90 unported - attachment slot query (returns 0).
static int32_t __fastcall FUN_1001af90(int32_t a, int32_t b, void *c)
{ MVL_HELPERS_TODO(FUN_1001af90); (void)a; (void)b; (void)c; return 0; }
// TODO(helpers): 0x1001b080 unported - attachment offset query (returns 0).
static int32_t __fastcall FUN_1001b080(void *a, void *b)
{ MVL_HELPERS_TODO(FUN_1001b080); (void)a; (void)b; return 0; }
// TODO(helpers): 0x1001b1a0 unported - attachment world matrix fetch.
static void __fastcall FUN_1001b1a0(int32_t entity, void *out)
{ MVL_HELPERS_TODO(FUN_1001b1a0); (void)entity; (void)out; }
// TODO(helpers): 0x10032bf0 unported - part frame step 1.
static void FUN_10032bf0(void) { MVL_HELPERS_TODO(FUN_10032bf0); }
// TODO(helpers): 0x10032b70 unported - part frame step 2.
static void __fastcall FUN_10032b70(int32_t *p)
{ MVL_HELPERS_TODO(FUN_10032b70); (void)p; }
// TODO(helpers): 0x10032300 unported - part frame step 3.
static void FUN_10032300(void) { MVL_HELPERS_TODO(FUN_10032300); }
// TODO(helpers): 0x10032810 unported - part frame step 4 (returns 0).
static int32_t FUN_10032810(void) { MVL_HELPERS_TODO(FUN_10032810); return 0; }
// TODO(helpers): 0x10032b30 unported - part frame step 5.
static void FUN_10032b30(void) { MVL_HELPERS_TODO(FUN_10032b30); }
// TODO(helpers): 0x10031cc0 unported - part frame fetch (returns 0).
static int32_t FUN_10031cc0(void) { MVL_HELPERS_TODO(FUN_10031cc0); return 0; }

// FUN_1001cce0 (0.98e @ RVA 0x1cce0) - second logger flavor (context + fmt).
static void FUN_1001cce0(const char *context, const char *format, ...)
{
	char msg[512];
	va_list args;

	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);
	MvlErrorLog("%s: %s", context, msg);
}

// FUN_1001cb80 (0.98e @ RVA 0x1cb80) - primary plugin logger; same shape as
// ReportProblem, kept under the original name for call sites in the decomp.
static void FUN_1001cb80(const char *context, const char *format, ...)
{
	char msg[512];
	va_list args;

	va_start(args, format);
	vsnprintf(msg, sizeof(msg), format, args);
	va_end(args);
	MvlErrorLog("%s: %s", context, msg);
}

// ===========================================================================
// Section 4: depth-1 ports (self-contained implementations).
// ===========================================================================

// 0.98e @ RVA 0x4f400 - parts-block accessor: *(entity + 0x1D4).
static int32_t __fastcall FUN_1004f400(int32_t entity)
{
	return *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(entity + 0x1D4));
}

// 0.98e @ RVA 0x2fb00 - "has geometry": *(obj + 0x2C) != 0.
static bool __fastcall FUN_1002fb00(int32_t obj)
{
	return *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(obj + 0x2C)) != 0;
}

// 0.98e @ RVA 0x31ca0 - part -> RW frame: *(part + 4).
static int32_t __fastcall FUN_10031ca0(int32_t part)
{
	return *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(part + 4));
}

// 0.98e @ RVA 0x80f0 - CQuaternion::SetIdentity.
static uint32_t *__fastcall FUN_100080f0(uint32_t *q)
{
	q[0] = 0;
	q[1] = 0;
	q[2] = 0;
	q[3] = 0x3F800000u;
	return q;
}

// 0.98e @ RVA 0x8c90 - CVector copy (original __thiscall; C3865-lowered).
static void *__fastcall FUN_10008c90(void *this_, void * /*edx*/, uint32_t *src)
{
	uint32_t *dst = static_cast<uint32_t *>(this_);
	dst[0] = src[0];
	dst[1] = src[1];
	dst[2] = src[2];
	return this_;
}

// 0.98e @ RVA 0x8990 - CQuaternion::Get (quaternion -> 3x3 matrix rows).
// Original __thiscall; C3865-lowered to __fastcall + dummy EDX.
static void __fastcall FUN_10008990(void *this_, void * /*edx*/, float *m)
{
	const float x = reinterpret_cast<float *>(this_)[0];
	const float y = reinterpret_cast<float *>(this_)[1];
	const float z = reinterpret_cast<float *>(this_)[2];
	const float w = reinterpret_cast<float *>(this_)[3];

	m[0]  = (1.0f - y * 2.0f * y) - z * 2.0f * z;
	m[1]  = x * 2.0f * y + z * 2.0f * w;
	m[2]  = x * 2.0f * z - y * 2.0f * w;
	m[4]  = x * 2.0f * y - z * 2.0f * w;
	m[5]  = (1.0f - x * 2.0f * x) - z * 2.0f * z;
	m[6]  = x * 2.0f * w + z * 2.0f * y;
	m[8]  = y * 2.0f * w + x * 2.0f * z;
	m[9]  = z * 2.0f * y - x * 2.0f * w;
	m[10] = (1.0f - x * 2.0f * x) - y * 2.0f * y;
	// m[3]/m[7]/m[11] untouched (original writes only the 3x3 core).
}

// 0.98e @ RVA 0x4f3a0 - directory entry lookup by model index.
static int *__fastcall FUN_1004f3a0(int32_t modelIndex)
{
	int *entry = FUN_1004f380(modelIndex - MvlModelIndexBaseOffset);
	if (entry == NULL || entry[0] == 0)
		return NULL;
	return entry;
}

// 0.98e @ RVA 0x3cfc0 - "directory entry exists for model index".
static bool __fastcall FUN_1003cfc0(int32_t modelIndex)
{
	return FUN_1004f3a0(modelIndex) != NULL;
}

// 0.98e @ RVA 0x59f55 - MSVC CRT rand() (per-thread _holdrand LCG).
static uint32_t FUN_10059f55(void)
{
	// TODO(helpers): original seeds _holdrand per-thread via __getptid();
	// the clone uses one process-wide LCG until TLS is introduced.
	static uint32_t s_holdrand = 1u;
	s_holdrand = s_holdrand * 0x343FDu + 0x269EC3u;
	return (s_holdrand >> 16) & 0x7FFFu;
}

// 0.98e @ RVA 0x3a410 - frees the cached parts block of the entity.
static void __fastcall FUN_1003a410(int32_t entity)
{
	int32_t parts = FUN_1004f400(entity);
	if (parts == 0)
		return;
	int32_t block = *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(parts + 0xE4));
	if (block == 0)
		return;
	thunk_FUN_1002faa0(reinterpret_cast<void *>(static_cast<uintptr_t>(block)));
	free(reinterpret_cast<void *>(static_cast<uintptr_t>(block)));
	*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(parts + 0xE4)) = 0;
}

// 0.98e @ RVA 0x3a350 - (re)builds the wheel-parts cache for the entity.
static void __fastcall FUN_1003a350(int32_t entity)
{
	int32_t parts = FUN_1004f400(entity);
	int32_t model = *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(entity + 0x120));
	if (parts == 0 || model == 0 || MvlWheelCacheFeatureFlag == 0)
		return;
	const uint32_t flags = *reinterpret_cast<uint32_t *>(
	    static_cast<uintptr_t>(model + 0xCC));
	if ((flags & 0x60000u) == 0)
		return;

	void *obj = FUN_10059d7f(0x58);
	obj = (obj != NULL) ? FUN_1002f690(obj) : NULL;
	*reinterpret_cast<void **>(static_cast<uintptr_t>(parts + 0xE4)) = obj;

	// Original zeroes 3 VC .text bytes around the fill call, then restores
	// them from the single .data byte at 0x6DDE20 (code-site patching).
	MvlUnlockGameText();
	PokeByte(kVC_CacheByte1, 0);
	PokeByte(kVC_CacheByte2, 0);
	PokeByte(kVC_CacheByte3, 0);
	FUN_1002f8b0(obj, reinterpret_cast<const uint8_t *>(
	                  static_cast<uintptr_t>(*reinterpret_cast<int32_t *>(
	                      static_cast<uintptr_t>(entity + 0x4C)))));
	const uint8_t restore = PeekByte(kVC_CacheRestoreByte);
	PokeByte(kVC_CacheByte1, restore);
	PokeByte(kVC_CacheByte2, restore);
	PokeByte(kVC_CacheByte3, restore);
	*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(parts + 0xE8)) =
	    MvlWheelCacheGenerationStamp;
}

// 0.98e @ RVA 0x2f890 - parts-object init pair (two unported sub-inits).
static void __fastcall FUN_1002f890(void *obj)
{
	FUN_1002f8a0(obj);
	FUN_1002f880(static_cast<int32_t>(reinterpret_cast<intptr_t>(obj)));
}

// 0.98e @ RVA 0x2e8e0 - wheel-cache flush (delegates to FUN_1002e8d0).
static void FUN_1002e8e0(void)
{
	FUN_1002e8d0();
}

// 0.98e @ RVA 0x8d90 - CVector transform helper (original __thiscall).
static void *__fastcall FUN_10008d90(void *this_, void * /*edx*/,
                                      uint32_t *out, float *v)
{
	uint32_t tmp[3];
	tmp[0] = reinterpret_cast<uint32_t *>(this_)[0];
	tmp[1] = reinterpret_cast<uint32_t *>(this_)[1];
	tmp[2] = reinterpret_cast<uint32_t *>(this_)[2];
	float *r = FUN_10008f50(reinterpret_cast<float *>(tmp), v);
	const float z = r[2];
	out[0] = reinterpret_cast<uint32_t *>(r)[0];
	out[1] = reinterpret_cast<uint32_t *>(r)[1];
	out[2] = reinterpret_cast<uint32_t *>(r)[2];
	(void)z;
	return out;
}

// 0.98e @ RVA 0x8e30 - CQuaternion::Normalise wrapper (original __thiscall).
static void *__fastcall FUN_10008e30(void *this_, void * /*edx*/, uint32_t *out)
{
	uint32_t tmp[3];
	tmp[0] = reinterpret_cast<uint32_t *>(this_)[0];
	tmp[1] = reinterpret_cast<uint32_t *>(this_)[1];
	tmp[2] = reinterpret_cast<uint32_t *>(this_)[2];
	float *r = FUN_10008fc0(reinterpret_cast<float *>(tmp));
	out[0] = reinterpret_cast<uint32_t *>(r)[0];
	out[1] = reinterpret_cast<uint32_t *>(r)[1];
	out[2] = reinterpret_cast<uint32_t *>(r)[2];
	return out;
}

// 0.98e @ RVA 0x8d10 - CVector product helper (original __thiscall).
static void *__fastcall FUN_10008d10(void *this_, void * /*edx*/,
                                      uint32_t *out, float *v)
{
	uint32_t tmp[3];
	tmp[0] = reinterpret_cast<uint32_t *>(this_)[0];
	tmp[1] = reinterpret_cast<uint32_t *>(this_)[1];
	tmp[2] = reinterpret_cast<uint32_t *>(this_)[2];
	float *r = FUN_10008ee0(reinterpret_cast<float *>(tmp), v);
	out[0] = reinterpret_cast<uint32_t *>(r)[0];
	out[1] = reinterpret_cast<uint32_t *>(r)[1];
	out[2] = reinterpret_cast<uint32_t *>(r)[2];
	return out;
}

// 0.98e @ RVA 0x1b5b0 - attachment post-fixup when extra-flag 0x400000 set.
static void __fastcall FUN_1001b5b0(void *param_1)
{
	int32_t attach = FUN_1001af70(static_cast<int32_t>(
	    reinterpret_cast<intptr_t>(param_1)));
	if (attach == 0)
		return;
	const int32_t modelIndex = FUN_1002d880();
	const uint32_t flags = FUN_1003d120(modelIndex);
	if ((flags & 0x400000u) != 0 &&
	    *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(attach + 4)) == 0) {
		// Original reads a CVector at entity+0x34 as two floats (Ghidra
		// undefined8 split); keep both halves.
		const uint64_t posBits = *reinterpret_cast<const uint64_t *>(
		    reinterpret_cast<const uint8_t *>(param_1) + 0x34);
		float out[16]; // decomp used a stack slot below ESP; size is ample
		FUN_1001b280(out, param_1,
		             static_cast<float>(posBits),
		             static_cast<float>(posBits >> 32));
	}
}

// 0.98e @ RVA 0x3a900 - spawns a fire effect on a free class slot.
static uint32_t __fastcall FUN_1003a900(int32_t param_1, int32_t param_2,
                                        uint32_t *param_3, char param_4)
{
	constexpr int32_t kSlotCount = 32; // (0x7DBC14 - 0x7DB894) / 0x1C

	for (int32_t i = 0; i < kSlotCount; ++i) {
		uint8_t *entry = reinterpret_cast<uint8_t *>(
		    static_cast<uintptr_t>(kVC_ClassSlotBase + static_cast<uint32_t>(i) * 0x1Cu));
		if (entry[0x0C] != 0) // &DAT_007db894 + i*0x1C - occupied
			continue;

		const int32_t obj = FUN_1002e750();
		if (obj == 0)
			return 0;
		*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(
		    kVC_ClassObjTable + static_cast<uint32_t>(i) * 4u)) =
		    static_cast<uint32_t>(obj);
		FUN_1002e780(obj, 0x111);

		uint32_t local_28[3] = {}; // CVector
		uint32_t local_38[4] = {}; // CQuaternion
		uint32_t local_78[3] = {}; // CMatrix rows scratch
		uint32_t local_6c = 0;
		uint32_t local_68[4] = {};
		uint32_t local_58[4] = {};
		uint32_t local_48[4] = {};
		uint32_t local_1c[3] = {};

		*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(param_2 + 0x0C)) = 0;
		(void)local_6c;
		FUN_10008250(local_38, local_28);
		FUN_10008990(local_38, NULL, reinterpret_cast<float *>(local_78));
		FUN_10008c90(local_48, NULL, param_3);
		FUN_10031d90();
		FUN_10008c50(local_1c);
		FUN_10032140();
		FUN_10008f50(reinterpret_cast<float *>(local_1c),
		             reinterpret_cast<float *>(static_cast<uintptr_t>(param_2 + 0x30)));
		uint32_t *pfVar3 = static_cast<uint32_t *>(FUN_10008e30(
		    reinterpret_cast<void *>(static_cast<uintptr_t>(param_1 + 0x70)),
		    NULL, local_28));
		FUN_10008ee0(reinterpret_cast<float *>(local_1c),
		             reinterpret_cast<float *>(pfVar3));

		entry[0x0C] = 1; // (&DAT_007db894)[i*0x1C] = 1
		*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(
		    kVC_ClassSlotBase + static_cast<uint32_t>(i) * 0x1Cu + 8u)) =
		    PeekU32(kVC_FrameCounter) + 4000u;
		*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(
		    kVC_ClassSlotBase + static_cast<uint32_t>(i) * 0x1Cu + 4u)) =
		    static_cast<uint32_t>(param_1);
		*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(
		    kVC_ClassSlotBase + static_cast<uint32_t>(i) * 0x1Cu)) = 0x10u;
		FUN_10008c90(reinterpret_cast<void *>(static_cast<uintptr_t>(
		                 kVC_ClassSlotBase + static_cast<uint32_t>(i) * 0x1Cu + 0x10u)),
		             NULL, local_48);
		FUN_10008c90(reinterpret_cast<void *>(static_cast<uintptr_t>(obj + 0x14)),
		             NULL, local_68);
		FUN_10008c90(reinterpret_cast<void *>(static_cast<uintptr_t>(obj + 0x24)),
		             NULL, local_58);
		FUN_10008c90(reinterpret_cast<void *>(static_cast<uintptr_t>(obj + 4)),
		             NULL, local_78);
		FUN_10008c90(reinterpret_cast<void *>(static_cast<uintptr_t>(obj + 0x34)),
		             NULL, local_48);
		FUN_1002e170(obj + 4);
		FUN_10008c90(reinterpret_cast<void *>(static_cast<uintptr_t>(obj + 0x70)),
		             NULL, local_1c);

		uint8_t *objFlags = reinterpret_cast<uint8_t *>(
		    static_cast<uintptr_t>(obj + 0x11A));
		*objFlags = static_cast<uint8_t>(((param_4 != 0) ? 0x02u : 0u) |
		                                 (*objFlags & 0xFDu));
		*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(obj + 200)) =
		    0x3F400000u;
		*reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(obj + 0x179)) = 0;
		*reinterpret_cast<int8_t *>(static_cast<uintptr_t>(obj + 0x192)) =
		    static_cast<int8_t>(i);
		FUN_1000b170();
		return 1;
	}
	return 0;
}

// 0.98e @ RVA 0x17290 - grows the anim pool to MvlColIdBoundary + 100 entries
// (0xFFFFFFFF-filled), only while the long-model-id flag MvlLongModelIdsEnabled is set.
static void FUN_10017290(void)
{
	int32_t capacity = MvlColIdBoundary;
	if (MvlLongModelIdsEnabled == 0)
		return;
	if (MvlAnimPoolBuffer != NULL) {
		if (MvlColIdBoundary + 100 <= MvlAnimPoolCapacity)
			return; // still big enough
		free(MvlAnimPoolBuffer);
		MvlAnimPoolBuffer = NULL;
	}
	capacity += 100;
	MvlAnimPoolCapacity = capacity;
	MvlAnimPoolBuffer = malloc(static_cast<size_t>(capacity) * 4u);
	if (MvlAnimPoolBuffer == NULL) {
		MvlErrorLog("helpers: FUN_10017290: anim pool alloc failed (%d entries)",
		            capacity);
		return;
	}
	uint32_t *p = static_cast<uint32_t *>(MvlAnimPoolBuffer);
	for (int32_t i = 0; i < capacity; ++i)
		p[i] = 0xFFFFFFFFu;
}

// 0.98e @ RVA 0x17940 - col/anim code-site patches (guarded by MvlColAnimPatchesInstalled).
static void FUN_10017940(void)
{
	if (MvlColAnimPatchesInstalled != 0)
		return;

	// Original installs 5 E9 JMPs via FUN_10022560 (MvlWriteJmp) from VC
	// sites 0x62A127 / 0x62A20A / 0x62A357 / 0x62A66E / 0x62A9F5 to plugin
	// handlers 0x10017890 / 0x100178B0 / 0x100178D0 / 0x100178F0 /
	// 0x10017920. Those handler bodies are absent from the limout dumps, so
	// the JMPs are NOT installed yet.
	// TODO(helpers): 0x10017890 unported;  TODO(helpers): 0x100178b0 unported;
	// TODO(helpers): 0x100178d0 unported;  TODO(helpers): 0x100178f0 unported;
	// TODO(helpers): 0x10017920 unported.
	MvlErrorLog("helpers: TODO(helpers): FUN_10017940 - col/anim JMP sites not "
	            "installed (handlers 0x10017890..0x10017920 unported)");

	// Data patches are portable (word + byte into the unlocked .text):
	MvlUnlockGameText();
	const uint16_t patchCmpEbx = 0x5E3B; // bytes 3B 5E
	const uint16_t patchCmpEsi = 0x5F3B; // bytes 3B 5F
	const uint8_t  patchImm8   = 0x08;
	memcpy(reinterpret_cast<void *>(0x0048DE71u), &patchCmpEbx, 2);
	PokeByte(0x0048DE73u, patchImm8);
	memcpy(reinterpret_cast<void *>(0x00629FCCu), &patchCmpEbx, 2);
	PokeByte(0x00629FCEu, patchImm8);
	memcpy(reinterpret_cast<void *>(0x0062A038u), &patchCmpEsi, 2);
	PokeByte(0x0062A03Au, patchImm8);

	MvlColAnimPatchesInstalled = 1;
}

// ===========================================================================
// Section 5: depth-0 roots (the 11 helpers + driver).
// ===========================================================================

// 0.98e @ RVA 0x1b610 - attach/detach fixup pair on MvlAttachFixupEntity.
void FUN_1001b610(void)
{
	void *entity = MvlAttachFixupEntity;
	FUN_1001b640(MvlAttachFixupEntity, '\0');
	FUN_1001b640(entity, '\x01');
	FUN_1001b5b0(entity);
}

// 0.98e @ RVA 0x3a450 - per-frame wheel-cache refresh for MvlWheelCacheEntity.
void FUN_1003a450(void)
{
	const int32_t entity = MvlWheelCacheEntity;
	int32_t parts = FUN_1004f400(entity);
	if (parts == 0)
		return;

	if (*reinterpret_cast<int32_t *>(static_cast<uintptr_t>(parts + 0xE4)) != 0 &&
	    static_cast<uint32_t>(*reinterpret_cast<int32_t *>(
	        static_cast<uintptr_t>(parts + 0xE8))) != MvlWheelCacheGenerationStamp) {
		// Cache generation mismatch: rebuild.
		FUN_1003a410(entity);
		FUN_1003a350(entity);
	}

	const int32_t cache =
	    *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(parts + 0xE4));
	if (cache != 0 && FUN_1002fb00(cache)) {
		// TODO(helpers): decompiler lost the ECX arg of FUN_1002f890; the
		// cache object (parts+0xE4) is the only candidate - verify.
		FUN_1002f890(reinterpret_cast<void *>(static_cast<uintptr_t>(cache)));
		const uint32_t saved =
		    *reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(entity + 0x194));
		*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(entity + 0x194)) =
		    static_cast<uint32_t>(cache);
		FUN_1002e8e0();
		*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(entity + 0x194)) =
		    saved;
	}
}

// 0.98e @ RVA 0x3a810 - teardown of the cached parts block of MvlPartsTeardownEntity.
// Returns 1 (AL) on success / nothing-to-do, 0 on failure (original EAX
// semantics preserved: the decomp returns `x & 0xffffff00` for 0).
uint32_t FUN_1003a810(void)
{
	const int32_t entity = MvlPartsTeardownEntity;
	const int32_t modelIndex = FUN_1002d880(); // TODO(helpers): stub -> 0
	int *dirEntry = FUN_1004f3a0(modelIndex);
	if (dirEntry == NULL)
		return 1;

	int32_t parts = FUN_1004f400(entity);
	if (parts == 0)
		return 1;

	// Original guards the whole teardown with IsBadReadPtr(parts, 0xF4).
	if (IsBadReadPtr(reinterpret_cast<void *>(static_cast<uintptr_t>(parts)),
	                 0xF4) != 0)
		return 0;

	FUN_1003a410(entity); // frees *(parts + 0xE4) sub-block first
	if (*reinterpret_cast<int32_t *>(static_cast<uintptr_t>(parts)) != dirEntry[0])
		return 0; // model ID mismatch - bail (AL = 0)

	if (MvlSkipPartsFreeFlag == '\0') {
		const int32_t count = dirEntry[0xA18];
		for (int32_t i = 0; i < count; ++i) {
			// Records at byte 0x60 of the block, stride 0x24 ints? No: the
			// decomp walks int* (base + 0x18) with stride 9 ints = 0x24 bytes.
			int32_t *part = reinterpret_cast<int32_t *>(
			    static_cast<uintptr_t>(parts)) + 0x18 + i * 9;
			if (part[0] != 0) {
				const int32_t frame = FUN_10031ca0(part[0]);
				FUN_10031380(frame);
				FUN_10031ec0();
				FUN_10031da0();
				part[4] = 0;
			}
		}
		free(reinterpret_cast<void *>(static_cast<uintptr_t>(parts)));
	}

	*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(entity + 0x1D4)) = 0;
	return 1;
}

// 0.98e @ RVA 0x3af20 - per-frame part (door/window/extra) state machine.
// Known decompiler losses (all marked TODO at the use sites):
//   * the entity base for FUN_1004f400 came back as extraout registers; the
//     active-entity temp MvlActiveEntityTemp is the only candidate,
//   * the per-part record stride (0x24) is inconsistent with fields up to
//     +0x74 in the same record - transcribed literally, verify vs disasm.
void FUN_1003af20(void)
{
	// TODO(helpers): entity base = MvlActiveEntityTemp is a decompiler-loss guess.
	const int32_t entity = MvlActiveEntityTemp;
	const int32_t modelIndex = FUN_1002d880(); // TODO(helpers): stub -> 0
	int *dirEntry = FUN_1004f3a0(modelIndex);
	if (dirEntry == NULL)
		return;

	int32_t *parts = reinterpret_cast<int32_t *>(
	    static_cast<uintptr_t>(FUN_1004f400(entity)));
	const uint32_t frameCounter = PeekU32(kVC_FrameCounter);
	const int32_t partCount = dirEntry[0xA18];

	// Locals matching the Ghidra frame (sizes are generous, layout not ABI):
	uint32_t local_60[4] = {}; // CQuaternion (reset every iteration)
	uint32_t local_50[2] = {};
	uint32_t local_44[2] = {};
	uint32_t local_38[2] = {};
	uint32_t local_2c[2] = {};
	uint32_t local_20[2] = {};
	uint32_t local_14[4] = {};

	for (int32_t i = 0; i < partCount; ++i) {
		uint8_t *st = reinterpret_cast<uint8_t *>(parts) + i * 0x24;
		int32_t *cfg = dirEntry + i * 0x30 + 0xA19;

		int32_t  *pMode   = reinterpret_cast<int32_t  *>(st + 0x54);
		float    *pValue  = reinterpret_cast<float    *>(st + 0x58);
		float    *pTarget = reinterpret_cast<float    *>(st + 0x5C);
		int32_t  *pPart   = reinterpret_cast<int32_t  *>(st + 0x60);
		uint32_t *pCool   = reinterpret_cast<uint32_t *>(st + 0x64); // +100
		uint32_t *pDelay  = reinterpret_cast<uint32_t *>(st + 0x68);
		uint32_t *pMatrix = reinterpret_cast<uint32_t *>(st + 0x70);
		uint8_t  *pDirty  = st + 0x74;

		// --- key polling (all 5 groups run only when the vehicle is the
		//     "speed object" at game 0x94AD28 and its +0x354 float > 0).
		bool fire = false;
		const int32_t speedObj =
		    *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(entity + 0x1A8));
		if (PeekU32(kVC_SpeedCheckObject) == static_cast<uint32_t>(speedObj) &&
		    speedObj != 0 &&
		    *reinterpret_cast<float *>(static_cast<uintptr_t>(speedObj + 0x354)) > 0.0f) {
			auto keyDown = [](int key) {
				return (GetKeyState(static_cast<BYTE>(key)) < 0);
			};
			const uint8_t *cfgBytes = reinterpret_cast<const uint8_t *>(cfg);
			// Groups 1-4: original queried the alt keys but kept no result.
			if (keyDown(cfg[0x2B]) && keyDown(cfgBytes[0xAD]))
				(void)keyDown(cfgBytes[0xAE]);
			if (keyDown(cfg[0x2C]) && keyDown(cfgBytes[0xB1]))
				(void)keyDown(cfgBytes[0xB2]);
			if (keyDown(cfg[0x2D]) && keyDown(cfgBytes[0xB5]))
				(void)keyDown(cfgBytes[0xB6]);
			if (keyDown(cfg[0x2E]) && keyDown(cfgBytes[0xB9]))
				(void)keyDown(cfgBytes[0xBA]);
			// Group 5 decides `fire` (all three keys).
			if (keyDown(cfg[0x2F]) && keyDown(cfgBytes[0xBD])) {
				const bool last = keyDown(cfgBytes[0xBE]);
				fire = true;
				if (!last)
					fire = false;
			}
		}

		// --- per-frame step scales (decomp: 30 / (double)frames with the
		//     MSVC int->double negative-fix constant at plugin 0x8E010).
		auto stepScale = [](int32_t frames) -> float {
			if (frames == 0)
				return 1.0f;
			double d = static_cast<double>(frames);
			if (frames < 0)
				d += 4294967296.0; // TODO(helpers): DAT_1008e010 table
			return 30.0f / static_cast<float>(d);
		};
		const float kUp   = stepScale(cfg[0x11]);
		const float kDown = stepScale(cfg[0x10]);

		if (*pPart == 0)
			continue;

		const int32_t frame = FUN_10031ca0(*pPart);
		(void)frame;

		// --- animation state machine (first switch; only modes 2/3/4/6/7
		//     mutate and set the dirty flag).
		switch (*pMode) {
		case 2:
			*pValue += kDown;
			if (*pValue >= 1.0f) {
				*reinterpret_cast<int32_t *>(st + 0x58) = cfg[0x12];
				*pMode = 4;
			}
			*pDirty = 1;
			break;
		case 3:
			*pValue -= kDown;
			if (*pValue <= 0.0f) {
				*pMode = 1;
				*reinterpret_cast<uint32_t *>(st + 0x58) = 0;
			}
			*pDirty = 1;
			break;
		case 4: {
			float v = *pValue;
			const float t = *pTarget;
			bool done;
			if (t < v) {
				v -= kUp;
				done = t < v;
			} else {
				v += kUp;
				done = v < t;
			}
			*pValue = v;
			if (!done) {
				*pMode = 5;
				*pValue = t;
			}
			*pDirty = 1;
			break;
		}
		case 6:
		case 7: {
			float v = *pValue;
			const float target = static_cast<float>(cfg[0x12]);
			bool done;
			if (target < v) {
				v -= kUp;
				*pValue = v;
				done = target < v;
			} else {
				v += kUp;
				*pValue = v;
				done = v < target;
			}
			if (!done) {
				if (*pMode == 6) {
					*pMode = 5;
					*reinterpret_cast<int32_t *>(st + 0x58) = cfg[0x12];
				} else {
					*reinterpret_cast<uint32_t *>(st + 0x58) = 0x3F800000u;
					*pMode = 3;
				}
			}
			*pDirty = 1;
			break;
		}
		default: // modes 1 and 5 fall straight through
			break;
		}

		// --- keyframe interpolation (second switch).
		FUN_100080f0(local_60);
		switch (*pMode) {
		case 1:
		case 2:
		case 3:
			if (*pDirty != 0) {
				FUN_10008590(local_60, cfg + 0x0C,
				             reinterpret_cast<uint32_t *>(cfg + 0x21));
				FUN_10008990(local_60, NULL,
				              reinterpret_cast<float *>(pMatrix));
				float *kf = reinterpret_cast<float *>(cfg + 9);
				FUN_10008d90(cfg + 0x25, NULL, local_50, kf);
				FUN_10008e30(local_50, NULL, local_14);
				FUN_10008d10(local_14, NULL, local_38, kf);
				FUN_10008c90(reinterpret_cast<uint8_t *>(
				                 static_cast<uintptr_t>(*pMatrix)) + 0x30,
				             NULL, local_38);
				FUN_10031d40();
				*pDirty = 0;
			}
			break;
		case 4:
		case 5:
		case 6:
		case 7:
			if (*pDirty != 0) {
				FUN_10008590(local_60, cfg + 0x16,
				             reinterpret_cast<uint32_t *>(cfg + 0x1D));
				FUN_10008990(local_60, NULL,
				              reinterpret_cast<float *>(pMatrix));
				float *kf = reinterpret_cast<float *>(cfg + 0x13);
				FUN_10008d90(cfg + 0x1A, NULL, local_2c, kf);
				FUN_10008e30(local_2c, NULL, local_44);
				FUN_10008d10(local_44, NULL, local_20, kf);
				FUN_10008c90(reinterpret_cast<uint8_t *>(
				                 static_cast<uintptr_t>(*pMatrix)) + 0x30,
				             NULL, local_20);
				FUN_10031d40();
				*pDirty = 0;
			}
			break;
		default:
			break;
		}

		// --- write the part matrix into the RW frame.
		// TODO(helpers): decomp writes m[0..2] to frame+0x10 and then m[4..6]
		// to frame+0x10 again (lost-ECX artifact); transcribed literally -
		// verify against the disassembly before trusting the first store.
		uint32_t *m = *reinterpret_cast<uint32_t **>(
		    static_cast<uintptr_t>(*pMatrix));
		uint8_t *fr = reinterpret_cast<uint8_t *>(
		    static_cast<uintptr_t>(frame));
		FUN_10008c90(fr + 0x10, NULL, m + 0);
		*reinterpret_cast<uint32_t *>(fr + 0x0C) = m[3];
		FUN_10008c90(fr + 0x10, NULL, m + 4);
		*reinterpret_cast<uint32_t *>(fr + 0x2C) = m[7];
		FUN_10008c90(fr + 0x30, NULL, m + 8);
		*reinterpret_cast<uint32_t *>(fr + 0x3C) = m[0xB];
		FUN_10008c90(fr + 0x40, NULL, m + 0xC);
		*reinterpret_cast<uint32_t *>(fr + 0x4C) = m[0xF];		FUN_10031d90();
		FUN_10031d40();
		FUN_10031ee0();

		// --- fire / periodic action logic.
		if (!fire)
			*pDelay = 0;
		if (*pMode == 5 && fire) {
			if (cfg[0] == 1) {
				if (cfg[3] + *pCool < frameCounter) {
					FUN_1003a900(entity, frame + 0x10,
					              reinterpret_cast<uint32_t *>(cfg + 0x28),
					              *(reinterpret_cast<const char *>(cfg) + 9));
					*pCool = frameCounter;
				}
			} else if (cfg[0] == 2) {
				if (*pDelay == 0) {
					if (static_cast<uint32_t>(cfg[8]) < frameCounter)
						*pDelay = frameCounter - static_cast<uint32_t>(cfg[8]);
					else
						*pDelay = 1;
				}
				const uint32_t n =
				    (frameCounter - *pDelay) / static_cast<uint32_t>(cfg[8]);
				for (uint32_t k = n; k != 0; --k)
					FUN_1003ab00(entity, frame + 0x10,
					              reinterpret_cast<intptr_t>(cfg),
					              reinterpret_cast<intptr_t>(pMode));
				*pDelay += static_cast<uint32_t>(cfg[8]) * n;
			}
		}

		// --- optional per-part callback through vtable slot +0x48.
		if (cfg[1] != 0) {
			typedef void (__cdecl *PartFn)(int32_t *);
			const int32_t partObj = *pPart;
			const int32_t vtable =
			    *reinterpret_cast<int32_t *>(static_cast<uintptr_t>(partObj));
			PartFn fn = *reinterpret_cast<PartFn *>(
			    static_cast<uintptr_t>(vtable + 0x48));
			fn(reinterpret_cast<int32_t *>(static_cast<uintptr_t>(partObj)));
		}
	}
}

// 0.98e @ RVA 0x3b580 - writes the single-byte patch at VC 0x571820.
void FUN_1003b580(void)
{
	const int32_t modelIndex = FUN_1002d880(); // TODO(helpers): stub -> 0
	int *dirEntry = FUN_1004f3a0(modelIndex);

	bool patch = false;
	if (dirEntry == NULL) {
		patch = (MvlPartFlagDisable == '\0');
	} else {
		patch = (MvlPartFlagDisable == '\0') &&
		        (*reinterpret_cast<char *>(
		             reinterpret_cast<uint8_t *>(dirEntry) + 0xAF3 * 4) != 0);
		// NOTE: decomp indexes piVar2[0xaf3] (int units) then reads it as char.
	}

	MvlUnlockGameText();
	PokeByte(kVC_PartFlagByte, 0xC3);
	if (patch)
		PokeByte(kVC_PartFlagByte, 0xD9);
}

// 0.98e @ RVA 0x3b5d0 - part fade/visibility update.
void __fastcall FUN_1003b5d0(int *param_1, float *param_2, uint32_t param_3)
{
	const int32_t modelIndex = FUN_1002d880(); // TODO(helpers): stub -> 0
	int *dirEntry = FUN_1004f3a0(modelIndex);
	if (dirEntry == NULL)
		return;
	if (*reinterpret_cast<char *>(
	         reinterpret_cast<uint8_t *>(dirEntry) + 0xADE * 4) == 0)
		return;
	uint8_t *partBytes = reinterpret_cast<uint8_t *>(param_1);
	if ((partBytes[0x53] & 2) != 0)
		return;

	const float fade = *reinterpret_cast<const float *>(
	    reinterpret_cast<const uint8_t *>(dirEntry) + 0x2B79);

	// Ghidra frame layout preserved (local_60 @ -0x60, out @ -0x54,
	// int-copy @ -0x48): build the part matrix in place.
	uint8_t buf[0x60] = {};
	float *local_60 = reinterpret_cast<float *>(buf + 0x00);
	uint32_t *local_54 = reinterpret_cast<uint32_t *>(buf + 0x0C);
	int32_t *local_48 = reinterpret_cast<int32_t *>(buf + 0x18);

	local_60[0] = fade;
	*reinterpret_cast<float *>(buf + 0x04) = *reinterpret_cast<const float *>(
	    reinterpret_cast<const uint8_t *>(dirEntry) + 0x2B7D);
	*reinterpret_cast<float *>(buf + 0x08) = *reinterpret_cast<const float *>(
	    reinterpret_cast<const uint8_t *>(dirEntry) + 0x2B81);

	// 16 ints copied from param_1 + 4 (original increments before storing).
	memcpy(local_48, partBytes + 4, 0x10 * sizeof(int32_t));

	FUN_10031d40();
	FUN_10032140();
	FUN_10008d90(local_60, NULL, local_54, param_2);
	FUN_100092f0();

	if (fade < 0.2f) {
		typedef void (__cdecl *PartFn)(uint32_t);
		const int32_t vtable = *param_1;
		PartFn fn = *reinterpret_cast<PartFn *>(
		    static_cast<uintptr_t>(vtable + 0x80));
		fn(param_3);
	}
}

// 0.98e @ RVA 0x3b840 - repaint: push resolved colours into the 4 part groups.
void __fastcall FUN_1003b840(int32_t param_1, int32_t param_2, int32_t param_3,
                             int32_t param_4)
{
	int *dirEntry = FUN_1004f3a0(
	    static_cast<int32_t>(*reinterpret_cast<const uint16_t *>(
	        static_cast<uintptr_t>(param_1 + 0x46))));
	if (dirEntry == NULL)
		return;

	uint32_t col1 = PeekU32(kVC_LiveryColorTable +
	                         static_cast<uint32_t>(param_2) * 4u);
	uint32_t col2 = PeekU32(kVC_LiveryColorTable +
	                         static_cast<uint32_t>(param_3) * 4u);
	uint32_t col3 = 0xFFFFFFFFu;
	uint32_t col4 = 0xFFFFFFFFu;

	if (param_2 == 0xFF && param_3 < dirEntry[0xB3C]) {
		// Directory-entry override: 16-byte paint record (4 colours).
		// TODO(helpers): decomp stride arithmetic is garbled; the 16-byte
		// read below matches its 4 dword loads - verify vs disasm.
		const uint8_t *rec = reinterpret_cast<const uint8_t *>(dirEntry) +
		                     static_cast<uint32_t>(param_3 + 0x2BF) * 16u;
		col1 = *reinterpret_cast<const uint32_t *>(rec + 0);
		col2 = *reinterpret_cast<const uint32_t *>(rec + 4);
		col3 = *reinterpret_cast<const uint32_t *>(rec + 8);
		col4 = *reinterpret_cast<const uint32_t *>(rec + 12);
	}

	auto paintGroup = [](int *entry, int countIdx, int baseIdx, uint32_t colour) {
		const int32_t count = entry[countIdx];
		for (int32_t n = 0; n < count; ++n) {
			const int32_t obj = entry[baseIdx + n];
			if (obj == 0)
				continue;
			*reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(obj + 4)) = colour;
			*reinterpret_cast<uint8_t *>(static_cast<uintptr_t>(obj + 7)) = 0xFF;
		}
	};
	paintGroup(dirEntry, 0x9F9, 0x8C5, col1);
	paintGroup(dirEntry, 0x9FA, 0x941, col2);
	paintGroup(dirEntry, 0x9FB, 0x9B9, col3);
	paintGroup(dirEntry, 0x9FC, 0x9D7, col4);

	// Livery group: 4 slots (iVar3 steps 2 while < 8).
	for (int32_t k = 0; k < 8; k += 2) {
		const int32_t obj = dirEntry[0x9F5 + k / 2];
		if (param_4 == 0 || MvlPrimaryLiveryColor == 0 || MvlAlternateLiveryColor == 0 || obj == 0)
			continue;
		const uint8_t mask = PeekByte(static_cast<uint32_t>(param_4) + 0x1F9);
		const uint32_t bits = PeekU32(static_cast<uint32_t>(param_4) + 0x2B0);
		uint32_t *slot = reinterpret_cast<uint32_t *>(static_cast<uintptr_t>(obj));
		if ((mask & 0x40u) == 0u || ((bits >> (k & 0x1F)) & 3u) != 0u)
			*slot = static_cast<uint32_t>(MvlPrimaryLiveryColor);
		else
			*slot = static_cast<uint32_t>(MvlAlternateLiveryColor);
	}
}

// 0.98e @ RVA 0x3b9d0 - random paint for MvlPaintRandomEntity.
void FUN_1003b9d0(void)
{
	const int32_t entity = MvlPaintRandomEntity;
	char *paint1 = MvlPaintRandomByte1Ptr;
	char *paint2 = MvlPaintRandomByte2Ptr;

	if (*reinterpret_cast<const char *>(static_cast<uintptr_t>(entity + 0x154)) == 0 ||
	    PeekByte(kVC_PaintByteGlobal2) != 0) {
		*paint1 = '\0';
		*paint2 = '\0';
		return;
	}
	if (PeekByte(kVC_PaintByteGlobal1) == 1) {
		*paint1 = 'D';
		*paint2 = 'D';
		return;
	}

	int *dirEntry = FUN_1004f3a0(
	    static_cast<int32_t>(*reinterpret_cast<const uint16_t *>(
	        static_cast<uintptr_t>(entity + 0x46))));
	uint32_t count = 0;
	if (dirEntry != NULL)
		count = static_cast<uint32_t>(dirEntry[0xB3C]);

	if (count > 1) {
		for (int32_t attempt = 0; attempt < 8; ++attempt) {
			if (*paint1 != *reinterpret_cast<const char *>(
			                   static_cast<uintptr_t>(entity + 0x30)) ||
			    *paint2 != *reinterpret_cast<const char *>(
			                   static_cast<uintptr_t>(entity + 0x31)))
				break;
			const uint32_t r = FUN_10059f55();
			if (FUN_1003cfc0(static_cast<int32_t>(
			        *reinterpret_cast<const uint16_t *>(
			            static_cast<uintptr_t>(entity + 0x46))))) {
				*paint1 = static_cast<char>(-1);
				// TODO(helpers): decompiler lost the EDX result; the fresh
				// random value is the only candidate for the variant byte.
				*paint2 = static_cast<char>(r);
			} else {
				const uint8_t idx = static_cast<uint8_t>(r % count);
				*paint1 = *reinterpret_cast<const char *>(
				    static_cast<uintptr_t>(entity + 0x144 + idx));
				*paint2 = *reinterpret_cast<const char *>(
				    static_cast<uintptr_t>(entity + 0x14C + idx));
			}
		}
		*reinterpret_cast<char *>(static_cast<uintptr_t>(entity + 0x30)) = *paint1;
		*reinterpret_cast<char *>(static_cast<uintptr_t>(entity + 0x31)) = *paint2;
	}
}

// 0.98e @ RVA 0x3bae0 - paint-variant round-robin for MvlPaintVariantEntity.
// Reads the shared model-lookup registry (MvlModelLookupRegistry /
// MvlModelLookupRegistry, now single-storage in engine_globals.cpp); the lookup stays
// guarded until CModelIndexLimit::CaptureVcLimits has filled it.

void FUN_1003bae0(void)
{
	char *paint2 = MvlPaintVariantByte2Ptr;
	const int32_t entity = MvlPaintVariantEntity;
	char *paint1 = MvlPaintVariantByte1Ptr;

	if (*reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(entity + 0x154)) == 0 ||
	    PeekByte(kVC_PaintByteGlobal2) != 0) {
		*paint1 = '\0';
		*paint2 = '\0';
		return;
	}
	if (PeekByte(kVC_PaintByteGlobal1) != 0) {
		*paint1 = 'D';
		*paint2 = 'D';
		return;
	}

	uint32_t count = static_cast<uint8_t>(
	    *reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(entity + 0x154)));
	int *dirEntry = FUN_1004f3a0(
	    static_cast<int32_t>(*reinterpret_cast<const uint16_t *>(
	        static_cast<uintptr_t>(entity + 0x46))));
	if (dirEntry != NULL)
		count = static_cast<uint32_t>(dirEntry[0xB3C]);
	if (count == 0)
		return; // clone guard (original trusted the count > 0)

	uint8_t *variant = reinterpret_cast<uint8_t *>(
	    static_cast<uintptr_t>(entity + 0x155));
	*variant = static_cast<uint8_t>((*variant + 1u) % count);

	const uint16_t modelIndex = *reinterpret_cast<const uint16_t *>(
	    static_cast<uintptr_t>(entity + 0x46));
	char next2;
	if (FUN_1003cfc0(static_cast<int32_t>(modelIndex))) {
		*paint1 = static_cast<char>(-1);
		next2 = static_cast<char>(*variant);
	} else {
		*paint1 = *reinterpret_cast<const char *>(
		    static_cast<uintptr_t>(entity + 0x144 + *variant));
		next2 = *reinterpret_cast<const char *>(
		    static_cast<uintptr_t>(entity + 0x14C + *variant));
	}
	*paint2 = next2;

	if (*reinterpret_cast<const uint8_t *>(static_cast<uintptr_t>(entity + 0x154)) > 1) {
		const int32_t player = FUN_1002dff0(0);
		if (player != 0 &&
		    *reinterpret_cast<const char *>(static_cast<uintptr_t>(player + 0x1A0)) == *paint1 &&
		    *reinterpret_cast<const char *>(static_cast<uintptr_t>(player + 0x1A1)) == *paint2) {
			const int32_t idx = FUN_1002d880(); // TODO(helpers): stub -> 0
			if (MvlModelLookupRegistry == NULL ||
			    MvlModelLookupRegistry[static_cast<uint32_t>(idx)] !=
	                static_cast<uint32_t>(entity)) {
				*variant = static_cast<uint8_t>((*variant + 1u) % count);
				if (FUN_1003cfc0(static_cast<int32_t>(modelIndex))) {
					*paint1 = static_cast<char>(-1);
					*paint2 = static_cast<char>(*variant);
					return;
				}
				// TODO(helpers): decompiler lost EDX for the first index;
				// the advancing variant byte is the only candidate.
				*paint1 = *reinterpret_cast<const char *>(
				    static_cast<uintptr_t>(entity + 0x144 + *variant));
				*paint2 = *reinterpret_cast<const char *>(
				    static_cast<uintptr_t>(entity + 0x14C + *variant));
			}
		}
	}
}

// 0.98e @ RVA 0x3de20 - formats "vc%u" (GetTickCount) into MvlGeneratedVcNameBuffer.
char *FUN_1003de20(void)
{
	const DWORD tick = GetTickCount();
	snprintf(MvlGeneratedVcNameBuffer, sizeof(MvlGeneratedVcNameBuffer), "vc%u",
	         static_cast<unsigned>(tick));
	return MvlGeneratedVcNameBuffer;
}

// 0.98e @ RVA 0x4f7e0 - set AI class (slot table or per-slot 0x61 list).
void __fastcall FUN_1004f7e0(int32_t modelId, int32_t slot)
{
	const uint32_t id = static_cast<uint32_t>(modelId);
	if (id - 0x82u < 0x6Bu && MvlKeyClassBypassTable[id] != 0) {
		MvlAiClassSlotTable[slot] = modelId;
		return;
	}
	// Per-slot entry counter lives in the VC .data table at 0x9785A8.
	uint32_t *count = reinterpret_cast<uint32_t *>(
	    static_cast<uintptr_t>(kVC_ClassCounterBase)) + slot;
	if (*count < 0x61u) {
		MvlAiClassSlotList[slot * 0x61 + static_cast<int32_t>(*count)] = modelId;
		++(*count);
	}
}

// 0.98e @ RVA 0x17820 - DRIVER for the CModelIndexLimit relocation chain.
// Callees map 1:1 onto already-ported original symbols:
//   FUN_10018180 -> CModelIndexLimit::CaptureVcLimits        (model_id.h)
//   FUN_10017410 -> CModelIndexLimit::ReplaceStreamingBufferRefs
//   FUN_10017510 -> CModelIndexLimit::ReplaceModelPointerBufferRefs
//   FUN_100175f0 -> CModelIndexLimit::ReplaceTexStartIndexRefs
//   FUN_10017720 -> CModelIndexLimit::ReplaceColStartIndexRefs
//   FUN_100177c0 -> CModelIndexLimit::ReplaceAnimStartIndexRefs
//   FUN_10017940 -> col/anim code patches (this TU, above)
//   FUN_100180a0 -> directory-manager hook installer (dirmgr.h)
//   (the decomp's extraout_ECX on the FUN_10017410 call is a lost register;
//    the call carries exactly one int argument = streamingMax + 100.)
void FUN_10017820(void)
{
	const int32_t modelCount = MvlConfiguredModelLimit;
	const uint32_t texCount = static_cast<uint32_t>(MvlConfiguredTexDictLimit);
	const int32_t colCount = MvlConfiguredColFileLimit;

	CModelIndexLimit::CaptureVcLimits();

	const int32_t streamingMax =
	    modelCount + static_cast<int32_t>(texCount);
	CModelIndexLimit::ReplaceStreamingBufferRefs(streamingMax + 100);
	CModelIndexLimit::ReplaceModelPointerBufferRefs(modelCount);
	CModelIndexLimit::ReplaceTexStartIndexRefs(modelCount, texCount);
	CModelIndexLimit::ReplaceColStartIndexRefs(streamingMax, colCount);
	CModelIndexLimit::ReplaceAnimStartIndexRefs(streamingMax + colCount);

	FUN_10017940();
	FUN_100180a0();
	FUN_10017290();
}
