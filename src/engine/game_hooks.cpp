// game_hooks.cpp - hook machinery trio + VC 1.0 hook-handler stub ports.
//
// Every handler below is a 1:1 port of the 0.98e handler with the same RVA in
// vehmod_retail.dll (Ghidra FUN_<VA> names kept per project naming policy).
// Handlers are reached by an E9 JMP (or by a function-pointer dword write) from
// the VC 1.0 exe; they emulate the displaced instructions and then call the
// continuation address, exactly like the original. Non-ported MVL helpers are
// stubbed at the bottom (TODO with RVA) so the TU links; do not install the
// corresponding hooks before those helpers are ported.
#include "../core/stdafx.h"
#include "game_hooks.h"
#include "engine_globals.h"

#if !defined(_M_IX86)
#error "MVL clone: game_hooks.cpp is x86-only (VC 1.0 plugin)."
#endif

// ---------------------------------------------------------------------------
// Shared plugin .data globals now live in engine_globals.cpp (single storage
// owner; see engine_globals.h for the Mvl* names + original DAT_ RVAs and
// globals-map.md for the census). This TU writes them from the handlers.
// ---------------------------------------------------------------------------

namespace {
// Direct call into a VC 1.0 exe function by absolute address.
template <typename FnT>
inline FnT VcFunc(uint32_t va)
{
	return reinterpret_cast<FnT>(static_cast<uintptr_t>(va));
}

// Original writes _DAT_100b0dac (= MvlLastHookContinuation) before each
// continuation call (crash-reporting breadcrumb); keep that behavior.
void SetContinuation(uint32_t va)
{
	MvlWriteDword(&MvlLastHookContinuation, va);
}
} // namespace

// ---------------------------------------------------------------------------
// Hook machinery (original trio, 0.98e @ RVA 0x22560 / 0x22590 / 0x225a0).
// ---------------------------------------------------------------------------

void
MvlWriteJmp(void *target, const void *destination)
{
	// FUN_10022560: *p = 0xE9; *(int*)(p+1) = (dest - p) - 5;
	uint8_t *p = static_cast<uint8_t*>(target);
	p[0] = 0xE9;
	const int32_t rel = static_cast<int32_t>(reinterpret_cast<intptr_t>(destination) -
	                                         reinterpret_cast<intptr_t>(target) - 5);
	memcpy(p + 1, &rel, sizeof(rel));
}

void
MvlWriteDword(void *target, uint32_t value)
{
	// FUN_10022590: *dst = value; (memcpy tolerates unaligned VC code sites)
	memcpy(target, &value, sizeof(value));
}

void
MvlUnlockGameText(void)
{
	// FUN_100225a0: VirtualProtect((LPVOID)0x401000, 0x27CE00,
	// PAGE_EXECUTE_READWRITE, &old) - the full VC 1.0 .text range.
	DWORD oldProtect = 0;
	VirtualProtect(reinterpret_cast<LPVOID>(static_cast<uintptr_t>(0x401000u)),
	               0x27CE00u, PAGE_EXECUTE_READWRITE, &oldProtect);
}

// ---------------------------------------------------------------------------
// Not-yet-ported MVL helpers referenced by the handlers.
// TODO(RE): each is a real plugin function; provide and link the real port
// before enabling the hooks that use it (stubs are intentionally inert).
// ---------------------------------------------------------------------------
static void FUN_1001b610(void);                                  // 0.98e @ RVA 0x1b610
static void FUN_1003a450(void);                                  // 0.98e @ RVA 0x3a450
static void FUN_1003a810(void);                                  // 0.98e @ RVA 0x3a810
static void FUN_1003af20(void);                                  // 0.98e @ RVA 0x3af20
static void FUN_1003b580(void);                                  // 0.98e @ RVA 0x3b580
static void FUN_1003b5d0(int *a, float *b, uint32_t c);          // 0.98e @ RVA 0x3b5d0
static void FUN_1003b840(int32_t a, int32_t b, int32_t c, int32_t d); // 0.98e @ RVA 0x3b840
static void FUN_1003b9d0(void);                                  // 0.98e @ RVA 0x3b9d0
static void FUN_1003bae0(void);                                  // 0.98e @ RVA 0x3bae0
static uint32_t FUN_1003de20(void);                              // 0.98e @ RVA 0x3de20
static void FUN_1004f7e0(int32_t a, int32_t b);                  // 0.98e @ RVA 0x4f7e0

// ---------------------------------------------------------------------------
// Handlers from stubs/range-decomp.txt (RVA order preserved).
// ---------------------------------------------------------------------------

void FUN_1003dff0(void)
{
	// 0.98e @ RVA 0x3dff0 (30 bytes). Hook target VC 0x426748; continues 0x42674f.
	SetContinuation(0x42674Fu);
	VcFunc<void (__cdecl *)(void)>(0x42674Fu)();
}

uint64_t __fastcall FUN_1003e010(uint32_t param_1, uint32_t param_2, uint32_t param_3,
                                  int32_t param_4)
{
	// 0.98e @ RVA 0x3e010 (86 bytes). Hook target VC 0x4267de; continues 0x4267e5.
	// Fast path returns EDX:EAX unchanged (Ghidra in_EAX/param_2); capture EAX
	// at entry before anything else runs.
	uint32_t in_eax = 0;
	__asm { mov in_eax, eax }
	(void)param_1;
	(void)param_3;

	MvlLastKeyClassParam = param_4;
	if (param_4 > 0x81 && param_4 < 0xED && MvlKeyClassBypassTable[param_4] != 0)
		return (static_cast<uint64_t>(param_2) << 32) | in_eax;

	SetContinuation(0x4267E5u);
	return VcFunc<uint64_t (__cdecl *)(void)>(0x4267E5u)();
}

void FUN_1003e070(void)
{
	// 0.98e @ RVA 0x3e070 (30 bytes). Hook target VC 0x426ac6; continues 0x426acd.
	SetContinuation(0x426ACDu);
	VcFunc<void (__cdecl *)(void)>(0x426ACDu)();
}

void FUN_1003e090(void)
{
	// 0.98e @ RVA 0x3e090 (30 bytes). Hook target VC 0x426c80; continues 0x426c87.
	SetContinuation(0x426C87u);
	VcFunc<void (__cdecl *)(void)>(0x426C87u)();
}

void FUN_1003e0b0(void)
{
	// 0.98e @ RVA 0x3e0b0 (30 bytes). Hook target VC 0x427470; continues 0x427477.
	SetContinuation(0x427477u);
	VcFunc<void (__cdecl *)(void)>(0x427477u)();
}

uint64_t __fastcall FUN_1003e0d0(uint32_t param_1, uint32_t param_2, int32_t param_3,
                                  int32_t param_4)
{
	// 0.98e @ RVA 0x3e0d0 (38 bytes). Hook target VC 0x426820.
	(void)param_1;
	MvlSetAiClassModelId = static_cast<int32_t>(param_3);
	MvlSetAiClassSlot = static_cast<int32_t>(param_4);
	FUN_1004f7e0(param_3, param_4);
	return (static_cast<uint64_t>(param_2) << 32) | static_cast<uint32_t>(param_4);
}

void FUN_1003e100(void)
{
	// 0.98e @ RVA 0x3e100 (30 bytes). Hook target VC 0x426a4c; continues 0x426a53.
	SetContinuation(0x426A53u);
	VcFunc<void (__cdecl *)(void)>(0x426A53u)();
}

uint64_t __fastcall FUN_1003e120(int32_t param_1, uint32_t param_2)
{
	// 0.98e @ RVA 0x3e120 (26 bytes). Hook target VC 0x5bb40b.
	// Returns EDX:EAX unchanged (in_EAX captured at entry).
	uint32_t in_eax = 0;
	__asm { mov in_eax, eax }

	MvlClearPartsEntity = static_cast<int32_t>(param_1);
	*reinterpret_cast<uint32_t*>(
	    static_cast<uintptr_t>(static_cast<uint32_t>(param_1) + 0x1D4u)) = 0u;
	return (static_cast<uint64_t>(param_2) << 32) | in_eax;
}

void __fastcall FUN_1003e140(uint32_t param_1)
{
	// 0.98e @ RVA 0x3e140 (42 bytes). Hook target VC 0x5bac04; continues 0x5bac09.
	MvlPartsTeardownEntity = static_cast<int32_t>(param_1);
	FUN_1003a810();
	VcFunc<void (__cdecl *)(void)>(0x4BB630u)();
	SetContinuation(0x5BAC09u);
	VcFunc<void (__cdecl *)(void)>(0x5BAC09u)();
}

void __fastcall FUN_1003e170(int32_t param_1, void* /*edx*/, uint32_t param_2, uint32_t param_3)
{
	// 0.98e @ RVA 0x3e170 (90 bytes). Hook target VC 0x579290; continues 0x579295.
	// The continuation consumes the live ESI/EBX as stack args (Ghidra
	// unaff_ESI/unaff_EBX) because the E9 JMP preserves all registers; capture
	// them before any call clobbers them.
	uint32_t saveEsi = 0;
	uint32_t saveEbx = 0;
	__asm {
		mov saveEsi, esi
		mov saveEbx, ebx
	}

	MvlRepaintColor1 = param_2 & 0xFFu;
	MvlRepaintColor2 = param_3 & 0xFFu;
	MvlRepaintEntity = static_cast<int32_t>(param_1);
	FUN_1003b840(param_1, static_cast<int32_t>(MvlRepaintColor1),
	             static_cast<int32_t>(MvlRepaintColor2),
	             static_cast<int32_t>(MvlActiveEntityTemp));
	SetContinuation(0x579295u);
	VcFunc<void (__cdecl *)(uint32_t, uint32_t)>(0x579295u)(saveEsi, saveEbx);
}

uint64_t __fastcall FUN_1003e1d0(uint32_t param_1, uint32_t param_2, uint32_t param_3,
                                  uint32_t param_4)
{
	// 0.98e @ RVA 0x3e1d0 (38 bytes). Hook target VC 0x579090.
	MvlPaintRandomByte1Ptr = reinterpret_cast<char *>(static_cast<uintptr_t>(param_3));
	MvlPaintRandomByte2Ptr = reinterpret_cast<char *>(static_cast<uintptr_t>(param_4));
	MvlPaintRandomEntity = static_cast<int32_t>(param_1);
	FUN_1003b9d0();
	return (static_cast<uint64_t>(param_2) << 32) | param_4;
}

uint64_t __fastcall FUN_1003e200(uint32_t param_1, uint32_t param_2, uint32_t param_3,
                                  uint32_t param_4)
{
	// 0.98e @ RVA 0x3e200 (38 bytes). Hook target VC 0x579190.
	MvlPaintVariantByte1Ptr = reinterpret_cast<char *>(static_cast<uintptr_t>(param_3));
	MvlPaintVariantByte2Ptr = reinterpret_cast<char *>(static_cast<uintptr_t>(param_4));
	MvlPaintVariantEntity = static_cast<int32_t>(param_1);
	FUN_1003bae0();
	return (static_cast<uint64_t>(param_2) << 32) | param_4;
}

void __fastcall FUN_1003e230(uint32_t param_1)
{
	// 0.98e @ RVA 0x3e230 (28 bytes). Dword-write hook target VC 0x69adc4.
	MvlActiveEntityTemp = static_cast<int32_t>(param_1);
	FUN_1003af20();
	VcFunc<void (__cdecl *)(void)>(0x589790u)();
	MvlActiveEntityTemp = 0;
}

void __fastcall FUN_1003e250(uint32_t param_1)
{
	// 0.98e @ RVA 0x3e250 (28 bytes). Dword-write hook target VC 0x6d7b68.
	MvlActiveEntityTemp = static_cast<int32_t>(param_1);
	FUN_1003af20();
	VcFunc<void (__cdecl *)(void)>(0x60AD50u)();
	MvlActiveEntityTemp = 0;
}

void __fastcall FUN_1003e270(uint32_t param_1)
{
	// 0.98e @ RVA 0x3e270 (28 bytes). Dword-write hook target VC 0x69b0e8.
	MvlActiveEntityTemp = static_cast<int32_t>(param_1);
	FUN_1003af20();
	VcFunc<void (__cdecl *)(void)>(0x5A4DE0u)();
	MvlActiveEntityTemp = 0;
}

void __fastcall FUN_1003e290(uint32_t param_1)
{
	// 0.98e @ RVA 0x3e290 (32 bytes). Dword-write hook target VC 0x69adc0.
	MvlWheelCacheEntity = static_cast<int32_t>(param_1);
	FUN_1003a450();
	FUN_1003b580();
	VcFunc<void (__cdecl *)(void)>(0x589AD0u)();
}

void __fastcall FUN_1003e2b0(uint32_t param_1)
{
	// 0.98e @ RVA 0x3e2b0 (27 bytes). Dword-write hook target VC 0x6d7b64.
	MvlPartFlagEntity = static_cast<int32_t>(param_1);
	FUN_1003b580();
	VcFunc<void (__cdecl *)(void)>(0x60AD90u)();
}

// Naked trampolines: Ghidra stack offsets are entry-ESP relative (assumption
// recorded in port-notes-engine.md).
extern "C" void FUN_1003e2d0_impl(int *param_1b8, float *param_ebp, uint32_t param_1b4);

__declspec(naked) void FUN_1003e2d0(void)
{
	// 0.98e @ RVA 0x3e2d0 (80 bytes). Hook target VC 0x5cf9f3; continues 0x5cf9f9.
	__asm {
		mov edx, [esp + 0x1b8] // in_stack_000001b8
		mov ecx, [esp + 0x1b4] // in_stack_000001b4
		push ecx               // 3rd arg
		push ebp               // 2nd arg (unaff_EBP)
		push edx               // 1st arg
		call FUN_1003e2d0_impl
		add esp, 0xc
		ret
	}
}

extern "C" void
FUN_1003e2d0_impl(int *param_1b8, float *param_ebp, uint32_t param_1b4)
{
	MvlFadePartArg = param_1b8;
	MvlFadeStackArg = param_1b4;
	MvlFadeVecArg = param_ebp;
	FUN_1003b5d0(param_1b8, param_ebp, param_1b4);
	SetContinuation(0x5CF9F9u);
	VcFunc<void (__cdecl *)(void)>(0x5CF9F9u)();
}

extern "C" void FUN_1003e320_impl(int32_t param_1, int32_t *param_40);

__declspec(naked) void FUN_1003e320(void)
{
	// 0.98e @ RVA 0x3e320 (81 bytes). Hook target VC 0x5c4341.
	// Original also consumed [esp+0x40] and passed a pointer to its own frame
	// slot [esp+0x18] (scratch, 0x28 bytes) - see impl.
	__asm {
		mov eax, [esp + 4]     // param_1
		mov edx, [esp + 0x40]  // in_stack_00000040
		push edx
		push eax
		call FUN_1003e320_impl
		add esp, 8
		ret
	}
}

extern "C" void
FUN_1003e320_impl(int32_t param_1, int32_t *param_40)
{
	float scratch[10]; // original: handler-frame bytes [esp+0x18 .. esp+0x40)
	MvlFadePartArgAlt = param_40;
	MvlFadeCallbackArgAlt = *reinterpret_cast<uint32_t*>(
	    static_cast<uintptr_t>(static_cast<uint32_t>(param_1) + 4u));
	MvlFadeScratchVec = scratch;
	FUN_1003b5d0(param_40, MvlFadeScratchVec, MvlFadeCallbackArgAlt);
	SetContinuation(0x5C4348u);
	VcFunc<void (__cdecl *)(void)>(0x5C4348u)();
}

// ---------------------------------------------------------------------------
// Handlers from stubs2/range-decomp.txt.
// ---------------------------------------------------------------------------

void FUN_10043730(void)
{
	// 0.98e @ RVA 0x43730 (34 bytes). Hook target VC 0x408967; continues 0x40896c.
	MvlGeneratedVcNamePtr = FUN_1003de20();
	SetContinuation(0x40896Cu);
	VcFunc<void (__cdecl *)(void)>(0x40896Cu)();
}

void __fastcall FUN_10043760(uint32_t param_1)
{
	// 0.98e @ RVA 0x43760 (21 bytes). Dword-write hook target VC 0x69adb0.
	MvlAttachFixupEntity = reinterpret_cast<void *>(static_cast<uintptr_t>(param_1));
	FUN_1001b610();
	VcFunc<void (__cdecl *)(void)>(0x593030u)();
}

extern "C" void FUN_10043780_impl(float st0, float st1);

__declspec(naked) void FUN_10043780(void)
{
	// 0.98e @ RVA 0x43780 (130 bytes). Conditional hook target VC 0x60048c;
	// continues 0x6006b8 (throttled) or 0x60049c. Original consumed x87
	// ST(0)/ST(1) arguments (Ghidra in_ST0/in_ST1).
	__asm {
		sub esp, 16
		fstp dword ptr [esp + 4] // ST(0) -> st1 (2nd arg)
		fstp dword ptr [esp]     // former ST(1) -> st0 (1st arg)
		call FUN_10043780_impl
		add esp, 16
		ret
	}
}

extern "C" void
FUN_10043780_impl(float st0, float st1)
{
	MvlFrameWaitTimeNow = st0;
	MvlFrameWaitTimePrev = st1;
	if (MvlFrameWaitTimePrev < MvlFrameWaitTimeNow) {
		MvlFrameWaitRemainingMs = static_cast<int32_t>(MvlFrameWaitTimeNow - MvlFrameWaitTimePrev) -
		               static_cast<int32_t>(MvlFrameWaitTimingConstant);
		if (MvlFrameWaitRemainingMs > 0)
			MsgWaitForMultipleObjects(0, NULL, FALSE,
			                          static_cast<DWORD>(MvlFrameWaitRemainingMs), 0x4BFu);
		SetContinuation(0x6006B8u);
		VcFunc<void (__cdecl *)(void)>(0x6006B8u)();
		return;
	}
	SetContinuation(0x60049Cu);
	VcFunc<void (__cdecl *)(void)>(0x60049Cu)();
}

// ---------------------------------------------------------------------------
// Patch table - (VC 1.0 target -> handler) pairs from FUN_10043c00
// (0.98e @ RVA 0x43c00, main init). Order matches the original install order.
// ---------------------------------------------------------------------------
const MvlHookSite g_MvlStubHookSites[] = {
	{ 0x00426748u, reinterpret_cast<uintptr_t>(&FUN_1003dff0), 0 }, // -> continues 0x42674f
	{ 0x004267DEu, reinterpret_cast<uintptr_t>(&FUN_1003e010), 0 }, // -> continues 0x4267e5
	{ 0x00426AC6u, reinterpret_cast<uintptr_t>(&FUN_1003e070), 0 }, // -> continues 0x426acd
	{ 0x00426C80u, reinterpret_cast<uintptr_t>(&FUN_1003e090), 0 }, // -> continues 0x426c87
	{ 0x00427470u, reinterpret_cast<uintptr_t>(&FUN_1003e0b0), 0 }, // -> continues 0x427477
	{ 0x00426820u, reinterpret_cast<uintptr_t>(&FUN_1003e0d0), 0 }, // -> no continuation
	{ 0x00426A4Cu, reinterpret_cast<uintptr_t>(&FUN_1003e100), 0 }, // -> continues 0x426a53
	{ 0x005BB40Bu, reinterpret_cast<uintptr_t>(&FUN_1003e120), 0 }, // -> no continuation
	{ 0x005BAC04u, reinterpret_cast<uintptr_t>(&FUN_1003e140), 0 }, // -> continues 0x5bac09
	{ 0x00579290u, reinterpret_cast<uintptr_t>(&FUN_1003e170), 0 }, // -> continues 0x579295
	{ 0x00579090u, reinterpret_cast<uintptr_t>(&FUN_1003e1d0), 0 }, // -> no continuation
	{ 0x00579190u, reinterpret_cast<uintptr_t>(&FUN_1003e200), 0 }, // -> no continuation
	{ 0x0069ADC4u, reinterpret_cast<uintptr_t>(&FUN_1003e230), 1 }, // dword slot
	{ 0x006D7B68u, reinterpret_cast<uintptr_t>(&FUN_1003e250), 1 }, // dword slot
	{ 0x0069B0E8u, reinterpret_cast<uintptr_t>(&FUN_1003e270), 1 }, // dword slot
	{ 0x0069ADC0u, reinterpret_cast<uintptr_t>(&FUN_1003e290), 1 }, // dword slot
	{ 0x006D7B64u, reinterpret_cast<uintptr_t>(&FUN_1003e2b0), 1 }, // dword slot
	{ 0x005CF9F3u, reinterpret_cast<uintptr_t>(&FUN_1003e2d0), 0 }, // -> continues 0x5cf9f9
	{ 0x005C4341u, reinterpret_cast<uintptr_t>(&FUN_1003e320), 0 }, // -> continues 0x5c4348
	{ 0x00408967u, reinterpret_cast<uintptr_t>(&FUN_10043730), 0 }, // -> continues 0x40896c
	{ 0x0069ADB0u, reinterpret_cast<uintptr_t>(&FUN_10043760), 1 }, // dword slot

	// Conditional site not tabled (applied by MvlInstallStubHooks):
	//   VC 0x60048c -> FUN_10043780, guarded by plugin flag MvlFrameWaitHookEnabled.
	// Other FUN_10043c00 pairs target handlers outside this port batch
	// (FUN_1003e380 .. FUN_10043b50); extend this table as they get ported.
};

const size_t g_MvlStubHookSiteCount =
	sizeof(g_MvlStubHookSites) / sizeof(g_MvlStubHookSites[0]);

void
MvlInstallStubHooks(void)
{
	MvlUnlockGameText();

	for (size_t i = 0; i < g_MvlStubHookSiteCount; ++i) {
		const MvlHookSite &site = g_MvlStubHookSites[i];
		if (site.dwordWrite != 0)
			MvlWriteDword(reinterpret_cast<void*>(static_cast<uintptr_t>(site.vcTarget)),
			              static_cast<uint32_t>(site.handler));
		else
			MvlWriteJmp(reinterpret_cast<void*>(static_cast<uintptr_t>(site.vcTarget)),
			            reinterpret_cast<const void*>(site.handler));
	}

	// FUN_10043c00: if (MvlFrameWaitHookEnabled != 0) hook VC 0x60048c -> FUN_10043780.
	if (MvlFrameWaitHookEnabled != 0)
		MvlWriteJmp(reinterpret_cast<void*>(static_cast<uintptr_t>(0x60048Cu)),
		            reinterpret_cast<const void*>(&FUN_10043780));
}

// ---------------------------------------------------------------------------
// Inert stubs for the not-yet-ported helpers declared above.
// TODO(RE): replace each with the real port (same RVA) and re-link.
// ---------------------------------------------------------------------------
static void FUN_1001b610(void) {}
static void FUN_1003a450(void) {}
static void FUN_1003a810(void) {}
static void FUN_1003af20(void) {}
static void FUN_1003b580(void) {}
static void FUN_1003b5d0(int *a, float *b, uint32_t c) { (void)a; (void)b; (void)c; }
static void FUN_1003b840(int32_t a, int32_t b, int32_t c, int32_t d)
{
	(void)a;
	(void)b;
	(void)c;
	(void)d;
}
static void FUN_1003b9d0(void) {}
static void FUN_1003bae0(void) {}
static uint32_t FUN_1003de20(void) { return 0; }
static void FUN_1004f7e0(int32_t a, int32_t b) { (void)a; (void)b; }
