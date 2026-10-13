// xmlgen.cpp — GenerateDefaultXML port (see xmlgen.h).
//
// Ported 1:1 from decompiled vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/missing33/decomp-chain.txt, range 0x40df0) and
// verified against the raw disassembly of original_files/maxovehmod098e/
// mvl/vehmod_retail.dll — the Ghidra dump elided the varargs of every
// fprintf call in this function, so each %s/%d/%f argument below was bound
// from the `push` sequences in the binary (cited inline with RVA 0x...).
// Log strings are verbatim from the decomp/strings table.
//
// Recovery aids used (all local, read-only):
//   - .harness-memory/d3out/strings.txt      — format-string table (xref 040df0)
//   - original_files/maxovehmod/samplexmls/  — shipped sample output
//   - plugin-sdk (E:\SDKs\plugin-sdk-master) — game struct layouts/addresses
//
// GAME/ENGINE SEAMS (never faked — HOOK POINT comments carry the target VA):
//   * CText::Get wide-name lookup   (game 0x584f30 via thunk 0x2ebb0)
//   * gta-vc.exe handling/audio/anim tables (placeholders until engine lane)
#include "xmlgen.h"

#include "../archive/vehicle_archive.h"   // MvlVehicleRegGet, MvlGameColourTable
#include "../loader/log.h"

#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

// ---------------------------------------------------------------------------
// Log wrappers matching the original three logging helpers used here:
//   FUN_1001cb80(fn, fmt, ...) -> error log  (0.98e @ RVA 0x1cb80)
//   FUN_1001cce0(fn, fmt, ...) -> warning    (0.98e @ RVA 0x1cce0)
// The original prefixes "Error in %s: %s\n" / "Warning in %s: %s\n"; the clone
// keeps fn+message through MvlErrorLog / MvlLog (vehicle_archive.cpp pattern).
// ---------------------------------------------------------------------------
static void
XmlgenError(const char *fn, const char *fmt, ...)
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

static void
XmlgenWarn(const char *fn, const char *fmt, ...)
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

// ---------------------------------------------------------------------------
// Globals (AGENTS.md naming rule: semantic Mvl* name in code, original
// Ghidra name + RVA kept as trailing comment).
// ---------------------------------------------------------------------------

int  MvlFetchedDataCount = 0;                 // DAT_100b2454 @ RVA 0xb2454
void **MvlModelInfoPtrs = 0;                  // DAT_100a6314 @ RVA 0xa6314 (init 0x92D4C8)
unsigned char MvlRgbColoursEnabled = 0;       // DAT_100b1a1f @ RVA 0xb1a1f

// FUN_1004f3a0 (0.98e @ RVA 0x4f3a0) registry record for a model id:
//   sub  ecx, [DAT_100a6d48]  ; DAT_100a6d48 @ RVA 0xa6d48, .data init 0x1770
//   call FUN_1004f380 (= MvlVehicleRegGet, vehicle_archive.h @ 0x4f380)
//   if (!rec || *(int*)rec == 0) return NULL;
// Default vehicles (ids 130..236) subtract to a negative index => NULL =>
// the Heli*/boat getters below fall through to their id-based defaults.
static int MvlRegistryIdOffset = 0x1770;      // DAT_100a6d48 @ RVA 0xa6d48 (.data init 6000)

static void *FUN_1004f3a0(int modelId)
{
	void *rec = MvlVehicleRegGet(modelId - MvlRegistryIdOffset);
	if(rec == 0 || *(const int *)rec == 0)
		return 0;
	return rec;
}

// FUN_1003cfe0 (0.98e @ RVA 0x3cfe0) — lazy init of the default-vehicle
// extraflags table (DAT_100b2438[id] for id 130..236). Body ported verbatim
// from the missing33 dump depth=1 section (memset via FUN_10063900 @ 0x63900
// -> CRT memset; literal stores are the original's default flag words).
static int MvlExtraFlagsByModelId[256];       // DAT_100b2438 @ RVA 0xb2438
static unsigned char MvlExtraFlagsInitDone;   // DAT_100b1a1e @ RVA 0xb1a1e

static void FUN_1003cfe0(void)
{
	int *t = MvlExtraFlagsByModelId;
	if(MvlExtraFlagsInitDone)
		return;
	// original: FUN_10063900(&DAT_100b2640, 0, 0x1b8) @ RVA 0x63900 —
	// DAT_100b2640 == &DAT_100b2438[130], so the wipe covers table[130..239]
	// (440 bytes); literal stores below are table[id] entries.
	memset(&t[130], 0, 0x1b8);
	// offsets below are the original absolute DVAs minus DAT_100b2438, /4.
	t[133] = 0x180000;  // @ RVA 0xb264c
	t[137] = 0x188081;  // @ RVA 0xb265c
	t[138] = 0x180000;  // @ RVA 0xb2660
	t[142] = 2;         // @ RVA 0xb2670
	t[146] = 0x10001;   // @ RVA 0xb2680
	t[147] = 1;         // @ RVA 0xb2684
	t[150] = 0x20000;   // @ RVA 0xb2690
	t[153] = 1;         // @ RVA 0xb269c
	t[154] = 8;         // @ RVA 0xb26a0
	t[155] = 0x101;     // @ RVA 0xb26a8
	t[156] = 0x201;     // @ RVA 0xb26ac
	t[160] = 0x401;     // @ RVA 0xb26b8
	t[161] = 0x180000;  // @ RVA 0xb26bc
	t[162] = 0x840;     // @ RVA 0xb26c0
	t[163] = 0x101000;  // @ RVA 0xb26c4
	t[164] = 0x10;      // @ RVA 0xb26c8
	t[167] = 0x180000;  // @ RVA 0xb26d4
	t[168] = 0x20000;   // @ RVA 0xb26d8
	t[171] = 0x40000;   // @ RVA 0xb26e4
	t[173] = 0x180000;  // @ RVA 0xb26ec
	t[185] = 0x180000;  // @ RVA 0xb271c
	t[188] = 0x20000;   // @ RVA 0xb2728
	t[201] = 0x40004;   // @ RVA 0xb2740
	t[214] = 0x20;      // @ RVA 0xb2774
	t[233] = 0x20000;   // @ RVA 0xb2798
	t[237] = 0x2001;    // @ RVA 0xb27a8
	t[243] = 0x4001;    // @ RVA 0xb27e8
	MvlExtraFlagsInitDone = 1;
}

// FUN_1003d120 (0.98e @ RVA 0x3d120) — <extraflags> getter, recovered name
// unknown (no error string): registry record dword @+0x10, else default table.
static int FUN_1003d120(int modelId)
{
	void *rec = FUN_1004f3a0(modelId);
	if(rec)
		return *(const int *)((const char *)rec + 0x10);   // asm: mov eax,[eax+0x10]
	if((unsigned)(modelId - 0x82) > 0x6a)                 // ids 130..236 only
		return 0;
	FUN_1003cfe0();
	return MvlExtraFlagsByModelId[modelId];
}

// ---------------------------------------------------------------------------
// Game-side data (gta-vc.exe). All tables below are READ by the original at
// fixed VAs; the clone keeps zero-filled placeholders (same pattern as
// MvlGameColourTable in vehicle_archive.cpp) until the engine lane wires the
// reads. GenerateDefaultXML is additionally gated by MvlFetchedDataCount, so
// these stay unreferenced until that lane lands.
// ---------------------------------------------------------------------------

// Game CVehicleModelInfo view (reVC x86 layout; the decomp reads these exact
// offsets — static_asserts below pin them). Pointed to by MvlModelInfoPtrs[id].
struct SMvlGameModelInfo
{
	unsigned char _base[0x30];      // CBaseModelInfo+CClumpModelInfo (not read here)
	unsigned char m_lastColour1;    // +0x30 (reVC m_lastColour1)
	unsigned char m_lastColour2;    // +0x31
	char          m_gameName[10];   // +0x32 reVC m_gameName — filename + CText key
	int           m_vehicleType;    // +0x3C eVehicleType: 0 car,1 boat,2 train,3 heli,4 plane,5 bike
	float         m_wheelScale;     // +0x40 <wheelscale>
	short         m_wheelId;        // +0x44 <wheelmodel> (type 0) / <lodmodel> (type 4; reVC union m_planeLodId)
	short         m_handlingId;     // +0x46 indexes the handling/flying/bike/boat tables
	char          m_numDoors;       // +0x48
	char          m_vehicleClass;   // +0x49 (-1 => "ignore") -> <class>
	char          m_level;          // +0x4A -> <level>
	char          m_numComps;       // +0x4B
	short         m_frequency;      // +0x4C -> <freq> (class != -1 path)
	unsigned char _pad1[2];         // +0x4E
	float         m_positions[5][3];// +0x50 reVC m_positions
	unsigned      m_compRules;      // +0x8C <comprules>
	float         m_bikeSteerAngle; // +0x90 <wheelrotangle> (type 5, cvttss2si)
	void         *m_materials1[24]; // +0x94
	void         *m_materials2[20]; // +0xF4
	unsigned char m_colours1[8];    // +0x144 <carcol> primary  (per colour byte)
	unsigned char m_colours2[8];    // +0x14C <carcol> secondary (edi+8 in the loop)
	unsigned char m_numColours;     // +0x154
	unsigned char m_lastColorVariation;
	unsigned char m_currentColour1;
	unsigned char m_currentColour2;
	void         *m_comps[6];       // +0x158
	int           m_animFileIndex;  // +0x170 -1 => "null", else anim-block name
};
static_assert(0x32 == offsetof(SMvlGameModelInfo, m_gameName), "layout");
static_assert(0x3C == offsetof(SMvlGameModelInfo, m_vehicleType), "layout");
static_assert(0x46 == offsetof(SMvlGameModelInfo, m_handlingId), "layout");
static_assert(0x8C == offsetof(SMvlGameModelInfo, m_compRules), "layout");
static_assert(0x144 == offsetof(SMvlGameModelInfo, m_colours1), "layout");
static_assert(0x170 == offsetof(SMvlGameModelInfo, m_animFileIndex), "layout");

// tHandlingData mirror — the original's per-handling record is 0xDC bytes and
// every read offset below matches plugin-sdk tHandlingData.h (VALIDATE_SIZE
// 0xDC). The DLL keeps its own copy in .data (DAT_1011e350) — "fetched" at
// early load ("DLL was loaded too late to fetch all data").
struct SMvlTransmissionData
{
	unsigned char _prefix[0x48];    // tTransmissionData fields not read here
	char          m_nDriveType;     // +0x7C (struct +0x48) <drivetype> %c
	char          m_nEngineType;    // +0x7D                <enginetype> %c
	char          m_nNumGears;      // +0x7E                <numofgears> %d
	char          _pad0;
	float         m_fAcceleration;  // +0x80 printed as value / 0.4f (const @ RVA 0x8ddfc)
	float         m_fMaxSpeed;      // +0x84 <maxspeed>
	unsigned char _pad1[0x0C];      // struct ends +0x94
};

struct SMvlHandlingRecord
{
	unsigned               m_dwVehicleId;                 // +0x00
	float                  m_fMass;                       // +0x04 <mass>
	float                  m_fMassInverted;               // +0x08
	float                  m_fTurnMass;                   // +0x0C
	float                  m_vDimensions[3];              // +0x10 <dimensions>
	float                  m_vCentreOfMass[3];            // +0x1C <centreofmass>
	int                    m_nPercentSubmerged;           // +0x28 <percentsubmerged> %d
	float                  m_fBuoyancyForce;              // +0x2C
	float                  m_fTractionMultiplier;         // +0x30 <traction><multiplier> / <boatsteering><bankforcemult>
	SMvlTransmissionData   m_transmission;                // +0x34 (0x60)
	float                  m_fBrakeDeceleration;          // +0x94 <brakes><deceleration> / <boatbrakes><verticalwavehitlimit>
	float                  m_fBrakeBias;                  // +0x98 <brakes><bias> / <boatbrakes><forwardwavehitbrake>
	unsigned char          m_bABS;                        // +0x9C <brakes><abs> %d
	unsigned char          _pad0[3];
	float                  m_fSteeringLock;               // +0xA0 <steeringlock>
	float                  m_fTractionLoss;               // +0xA4 <traction><loss> / <boatsteering><rudderturnforce>
	float                  m_fTractionBias;               // +0xA8 <traction><bias> / <boatsteering><speedsteerfalloff>
	float                  m_fABS;                        // +0xAC (not read)
	float                  m_fSuspensionForceLevel;       // +0xB0 <suspension><forcelevel> / <boatsuspension><waterresvolumemult>
	float                  m_fSuspensionDampingLevel;     // +0xB4 <dampening> / <waterdampingmult>
	float                  m_fSuspUpperLimit;             // +0xB8 <upperlimit> (both branches)
	float                  m_fSuspLowerLimit;             // +0xBC <lowerlimit> / <boatsuspension><handbrakedragmult>
	float                  m_fSuspBias;                   // +0xC0 <bias> / <sideslipforce>
	float                  m_fSuspAntiDiveMultiplier;     // +0xC4 <antidive> (both branches)
	float                  m_fCollisionDamageMultiplier;  // +0xC8 <damagemultiplier>
	unsigned               m_uFlags;                      // +0xCC <flags> %08X; 0x20000 => helidata, 0x60000 => aerohandling
	float                  m_fSeatOffsetDistance;         // +0xD0 <seatoffset>
	int                    m_nMonetaryValue;              // +0xD4 <value>
	unsigned char          m_bFrontLights;                // +0xD8 <lights><front> %d
	unsigned char          m_bRearLights;                 // +0xD9 <lights><rear> %d
	unsigned char          _pad1[2];
};
static_assert(0x94 == offsetof(SMvlHandlingRecord, m_fBrakeDeceleration), "layout");
static_assert(0xCC == offsetof(SMvlHandlingRecord, m_uFlags), "layout");
static_assert(sizeof(SMvlHandlingRecord) == 0xDC, "layout");

// tFlyingHandlingData mirror (0x4C) — <aerohandling>; gta-vc.exe @ VA 0x97CE44
// (code: handlingId * 0x4C + 0x97ce44), fields fThrust@0x04..vecSpeedRes@0x40.
struct SMvlFlyingRecord
{
	unsigned m_dwVehicleId;   // +0x00
	float    m_fThrust;       // +0x04 <thrust>
	float    m_fThrustFallOff;// +0x08 <thrustfalloff>
	float    m_fYaw;          // +0x0C <yaw>
	float    m_fYawStab;      // +0x10 <yawstab>
	float    m_fSideSlip;     // +0x14 <sideslip>
	float    m_fRoll;         // +0x18 <roll>
	float    m_fRollStab;     // +0x1C <rollstab>
	float    m_fPitch;        // +0x20 <pitch>
	float    m_fPitchStab;    // +0x24 <pitchstab>
	float    m_fFormLift;     // +0x28 <formlift>
	float    m_fAttackLift;   // +0x2C <attacklift>
	float    m_fMoveRes;      // +0x30 <moveres> (scalar)
	float    m_vTurnRes[3];   // +0x34 <turnres>
	float    m_vSpeedRes[3];  // +0x40 <speedres>
};
static_assert(sizeof(SMvlFlyingRecord) == 0x4C, "layout");

// tBikeHandlingData mirror (0x40) — gta-vc.exe @ VA 0x97D544 (code: id*0x40).
struct SMvlBikeRecord
{
	unsigned m_dwVehicleId;         // +0x00
	float    m_fLeanFwdCOM;         // +0x04 <leanfwdcom>
	float    m_fLeanFwdForce;       // +0x08 <leanfwdforce>
	float    m_fLeanBakCOM;         // +0x0C <leanbackcom>
	float    m_fLeanBakForce;       // +0x10 <leanbackforce>
	float    m_fMaxLean;            // +0x14 <maxlean>  (passes FUN_1006d1e0)
	float    m_fFullAnimLean;       // +0x18 <fullanimlean>
	float    m_fDesLean;            // +0x1C <deslean>
	float    m_fSpeedSteer;         // +0x20 <speedsteer>
	float    m_fSlipSteer;          // +0x24 <slipsteer>
	float    m_fNoPlayerCOMz;       // +0x28 <noplayercomz>
	float    m_fWheelieAng;         // +0x2C <wheelieang>  (passes FUN_1006d1e0)
	float    m_fStoppieAng;         // +0x30 <stoppieang>  (passes FUN_1006d1e0)
	float    m_fWheelieSteer;       // +0x34 <wheeliesteer>
	float    m_fWheelieStabMult;    // +0x38 <wheeliestabmult>
	float    m_fStoppieStabMult;    // +0x3C <stoppiestabmult>
};
static_assert(sizeof(SMvlBikeRecord) == 0x40, "layout");

// tBoatHandlingData mirror (0x3C) — gta-vc.exe @ VA 0x97D998 (code: id*15
// floats = id*0x3C; the code's base points at m_fThrustY, i.e. record+4, so
// <thrusty> is read at base+0).
struct SMvlBoatRecord
{
	unsigned m_dwVehicleId;      // +0x00
	float    m_fThrustY;         // +0x04 <thrusty>
	float    m_fThrustZ;         // +0x08 <thrustz>
	float    m_fThrustAppZ;      // +0x0C <thrustappz>
	float    m_fAqPlaneForce;    // +0x10 <aqplaneforce>
	float    m_fAqPlaneLimit;    // +0x14 <aqplanelimit>
	float    m_fAqPlaneOffset;   // +0x18 <aqplaneoffset>
	float    m_fWaveAudioMult;   // +0x1C <waveaudiomult>
	float    m_fLookLRBehindCamHeight; // +0x20 <looklrbcamheight>
	float    m_vMoveRes[3];      // +0x24 <moveres>
	float    m_vTurnRes[3];      // +0x30 <turnres>
};
static_assert(sizeof(SMvlBoatRecord) == 0x3C, "layout");

// Per-model vehicle audio stats — gta-vc.exe @ VA 0x6AC368 (code: modelId*7
// dword slots; slot 6 read as a byte => <doorsounds>). 7 dwords = stride 0x1C.
struct SMvlAudioStats
{
	unsigned m_anSamples[6];     // enginefarsample, enginenearsample, hornsample,
	                             // hornfreq, sirensample, sirenfreq
	unsigned char m_nDoorSounds; // <doorsounds> (byte read)
	unsigned char _pad[3];
};
static_assert(sizeof(SMvlAudioStats) == 0x1C, "layout");

// Placeholders (zero-filled) — origin VA in comment, contents wired by the
// engine lane later (open questions in the TRANCHE-misc port report).
static SMvlHandlingRecord s_mvlHandlingRecords[512]; // DAT_1011e350 @ RVA 0x11e350 (.data, "fetched" copy)
static SMvlFlyingRecord   s_mvlGameFlying[512];      // gta-vc.exe @ VA 0x97CE44
static SMvlBikeRecord     s_mvlGameBike[512];        // gta-vc.exe @ VA 0x97D544
static SMvlBoatRecord     s_mvlGameBoat[512];        // gta-vc.exe @ VA 0x97D998
static SMvlAudioStats     s_mvlGameAudio[0xED];      // gta-vc.exe @ VA 0x6AC368 (indexed by model id)
static char               s_mvlGameAnimBlocks[128][32]; // CAnimManager::ms_aAnimBlocks @ gta-vc.exe 0x7DB428 (CAnimBlock.m_name[32] @ +0)

// DLL .data pointer tables (contents recovered from the 0.98e binary — REAL
// values, not placeholders).
static const char *const MvlVehTypeNameTable[6] = {    // DAT_100aef34 @ RVA 0xaef34
	"car", "boat", "train", "heli", "plane", "bike"   // (reVC eVehicleType order)
};
static const char *const MvlAiClassTable[11] = {       // DAT_100aef54 @ RVA 0xaef54
	"normal", "poorfamily", "richfamily", "executive", "worker", "big",
	"taxi", "moped", "motorbike", "leisureboat", "workerboat"
};
static const char *const MvlHeliTypeNameTable[4] = {   // DAT_100af050 @ RVA 0xaf050
	"normal", "remote", "water", "hunter"
};
static const char *const MvlHeliWeaponNameTable[3] = { // DAT_100aef28 @ RVA 0xaef28
	"none", "hunter", "sparrow"
};
static const char kMvlAnimsNull[] = "null";            // @ RVA 0x10082088

// Division constant behind <acceleration>: value / 0.4f
static const float kMvlAccelDiv = 0.4f;                // @ RVA 0x8ddfc (0x3ECCCCCD)
// Bike deg/rad pair used as * 180.0 / pi for <maxlean>/<fullanimlean>/
// <wheelieang>/<stoppieang>:
static const float kMvlRadToDegNum = 180.0f;           // @ RVA 0x8df08 (0x43340000)
static const float kMvlRadToDegDen = 3.14159274f;      // @ RVA 0x8de7c (0x40490FDB)

// ---------------------------------------------------------------------------
// FUN_1002ebb0 (0.98e @ RVA 0x2ebb0) — tail thunk `mov eax,0x584f30; jmp eax`
// into gta-vc.exe: CText::Get(this=TheText @ 0x94B220, const char *key) ->
// wchar_t* (plugin-sdk plugin_vc/game_vc/CText.cpp). Used for <name>%ls with
// the model's game name (@ SMvlGameModelInfo::m_gameName).
// HOOK POINT: game text lookup not wired in the clone — returns an empty wide
// string after logging (never fakes a translated name).
// ---------------------------------------------------------------------------
static const wchar_t *
MvlTextGet(const char *key)
{
	static bool s_logged = false;
	if(!s_logged){
		MvlErrorLog("HOOK POINT 0x584f30 CText::Get (via FUN_1002ebb0 @ 0x2ebb0; "
		            "TheText @ 0x94b220) — wide vehicle name lookup not wired");
		s_logged = true;
	}
	(void)key;
	return L"";
}

// ---------------------------------------------------------------------------
// FUN_1006d1e0 (0.98e @ RVA 0x6d1e0) — double helper called on <maxlean>,
// <wheelieang>, <stoppieang> before the 180/pi scale. Identity on the entire
// exercised input domain: shipped samples give maxlean=44.999996 for raw
// 0.785398 rad (pcj600), stoppieang=-40.0 for raw -0.698131 — i.e. the input
// passes through unchanged (0.785398*180/pi = 44.99999). The fast path checks
// the default rounding modes then early-returns for |x| < ~1; the
// ___libm_error_support path (0x6d70c) handles Inf/NaN — identity there is
// unobservable from shipped output. OPEN QUESTION in port-notes.
// ---------------------------------------------------------------------------
static double FUN_1006d1e0(double x)
{
	return x;
}

// ---------------------------------------------------------------------------
// Heli getters (0.98e @ RVA 0x3ce30 / 0x3cea0 / 0x3cf00) — names recovered
// from their own error strings: "HeliGetType", "HeliGetWeaponType",
// "HeliGetRotorSettings". Originals are __fastcall with the model id in ECX;
// HeliGetRotorSettings also writes a second float through EDX (rotor radius)
// and one on the stack (rotor damage multiplier) — all three params are plain
// arguments here.
// ---------------------------------------------------------------------------

// FUN_1003ce30 @ 0x3ce30 — "Checks for model %d, which has not been loaded."
static int HeliGetType(int modelId)
{
	void *rec = FUN_1004f3a0(modelId);
	if(rec){
		if(*(const int *)rec != modelId){
			XmlgenError("HeliGetType", "Checks for model %d, which has not been loaded.", modelId);
			return 0;
		}
		return *(const int *)((const char *)rec + 0x76 * 4);  // piVar1[0x76] @ +0x1D8
	}
	// Defaults (asm): 0x9b -> 3, 0xc3/0xe7 -> 1, 0xb1 -> 2, else 0.
	if(modelId == 0x9b)
		return 3;                       // hunter  -> helitype table[3] = "hunter"
	if(modelId == 0xc3 || modelId == 0xe7)
		return 1;                       // rc helis -> table[1] = "remote"
	if(modelId == 0xb1)
		return 2;                       // seasparrow -> table[2] = "water"
	return 0;                           // table[0] = "normal"
}

// FUN_1003cea0 @ 0x3cea0 — "HeliGetWeaponType"
static int HeliGetWeaponType(int modelId)
{
	void *rec = FUN_1004f3a0(modelId);
	if(rec){
		if(*(const int *)rec == modelId){
			if((*(const unsigned *)((const char *)rec + 0x39 * 4) & 0x20000U) != 0)
				return *(const int *)((const char *)rec + 0x77 * 4);  // piVar1[0x77]
		}
		else{
			XmlgenError("HeliGetWeaponType", "Checks for model %d, which has not been loaded.", modelId);
		}
		return 0;
	}
	if(modelId == 0x9b)
		return 1;                       // hunter -> weapons table[1] = "hunter"
	return (modelId == 0xb1) ? 2 : 0;   // seasparrow -> [2] = "sparrow", else [0] = "none"
}

// FUN_1003cf00 @ 0x3cf00 — "HeliGetRotorSettings": writes rotor radius (EDX
// out) + rotor damage multiplier (stack out). Defaults (asm) quoted as float
// immediates: 6.15f/0.5f, 6.15f/0.72f, 5.15f/0.8f.
static void HeliGetRotorSettings(int modelId, float *rotorRadiusOut, float *rotorDamMultOut)
{
	void *rec = FUN_1004f3a0(modelId);
	if(rec){
		if(*(const int *)rec != modelId){
			*rotorRadiusOut = 6.15f;    // 0x40c4cccd
			*rotorDamMultOut = 1.0f;    // 0x3f800000
			XmlgenError("HeliGetRotorSettings", "Checks for model %d, which has not been loaded.", modelId);
			return;
		}
		*rotorRadiusOut = *(const float *)((const char *)rec + 0x78 * 4);    // piVar1[0x78]
		*rotorDamMultOut = *(const float *)((const char *)rec + 0x79 * 4);   // piVar1[0x79]
		return;
	}
	if(modelId == 0x9b){                 // hunter
		*rotorRadiusOut = 6.15f;         // 0x40c4cccd
		*rotorDamMultOut = 0.5f;         // 0x3f000000
		return;
	}
	if(modelId == 0xc3 || modelId == 0xe7){
		*rotorRadiusOut = 0.9f;          // 0x3f666666
		*rotorDamMultOut = 0.72f;        // 0x3f3851ec
		return;
	}
	if(modelId != 0xb1 && modelId != 0xc7){
		*rotorRadiusOut = 6.15f;         // 0x40c4cccd
		*rotorDamMultOut = 1.0f;         // 0x3f800000
		return;
	}
	*rotorRadiusOut = 5.15f;             // 0x40a4cccd
	*rotorDamMultOut = 0.8f;             // 0x3f4ccccd
}

// ---------------------------------------------------------------------------
// FUN_1003cc70 (0.98e @ RVA 0x3cc70) — boat-engine settings getter (recovered
// provisional: feeds <boatengine>; no error string). __fastcall(id in ECX,
// output struct in EDX). Output struct layout (from GenerateDefaultXML's
// fprintf order, disasm-verified):
//   +0x00 nType %d, +0x04 nBaseVolume %d, +0x08 nBaseFrequency %d,
//   +0x0C fVolumeIncrease %f, +0x10 fFrequencyIncrease %f.
// ---------------------------------------------------------------------------
struct SMvlBoatEngine
{
	int   nType;
	int   nBaseVolume;
	int   nBaseFrequency;
	float fVolumeIncrease;
	float fFrequencyIncrease;
};

static void FUN_1003cc70(int modelId, SMvlBoatEngine *out)
{
	// The original leaves `out` untouched on the no-match path (returns 0
	// without writing); clone zeroes first to avoid reading uninitialised
	// memory — logged deviation, see port-notes.
	memset(out, 0, sizeof(*out));

	void *rec = FUN_1004f3a0(modelId);
	if(rec == 0){
		// Defaults (asm, exact field order +0x00/+0x08/+0x10/+0x04/+0x0C):
		if(modelId == 0x88){
			out->nType = 0; out->nBaseVolume = 0x14; out->nBaseFrequency = 0x760;
			out->fVolumeIncrease = 60.0f;      // 0x42700000 @ +0x0C
			out->fFrequencyIncrease = 490.0f;  // 0x43f50000 @ +0x10
			return;
		}
		if(modelId == 0xa0 || modelId == 0xb0 || modelId == 0xb6 ||
		   modelId == 0xca || modelId == 0xcb || modelId == 0xdf){
			out->nType = 1; out->nBaseVolume = 0x14; out->nBaseFrequency = 9000;
			out->fVolumeIncrease = 60.0f;      // 0x42700000 @ +0x0C
			out->fFrequencyIncrease = 6000.0f; // 0x45bb8000 @ +0x10
			return;
		}
		if(modelId == 0xb7){
			out->nType = 0; out->nBaseVolume = 0; out->nBaseFrequency = 0xebf;
			out->fVolumeIncrease = 80.0f;      // 0x42a00000 @ +0x0C
			out->fFrequencyIncrease = 715.0f;  // 0x4432c000 @ +0x10
			return;
		}
		if(modelId == 0xb8 || modelId == 0xd6){
			out->nType = 0; out->nBaseVolume = 0x14; out->nBaseFrequency = 0x6f6;
			out->fVolumeIncrease = 60.0f;      // 0x42700000 @ +0x0C
			out->fFrequencyIncrease = 463.0f;  // 0x43e78000 @ +0x10
			return;
		}
		return;
	}
	if(*(const int *)((const char *)rec + 1 * 4) == 1){
		// Record copy: 5 dwords from rec+0xaf4*4 (= rec+0x2BD0).
		const int *src = (const int *)((const char *)rec + 0xaf4 * 4);
		int *dst = (int *)out;
		dst[0] = src[0]; dst[1] = src[1]; dst[2] = src[2];
		dst[3] = src[3]; dst[4] = src[4];
	}
	// else: return value has low byte cleared, struct stays zeroed (see above).
}

// ---------------------------------------------------------------------------
// GenerateDefaultXML — vehmod_retail.dll 0.98e @ RVA 0x40df0 (4682 bytes).
// Loop model ids 0x82..0xEC (130..236), one "mvl\vlist\<name>.xml" each.
// Structure follows the disassembly (see header); format strings verbatim
// from the 0.98e string table (xref 040df0, .harness-memory/d3out/strings.txt).
// ---------------------------------------------------------------------------
void
GenerateDefaultXML(void)
{
	static const char *const fn = "GenerateDefaultXML";
	char path[260];
	bool warnNoFolder = false;
	bool warnNoModelInfo = false;

	// cmp [DAT_100b2454], 0x64 / jge  (asm @ RVA 0x40e03)
	if(MvlFetchedDataCount < 100){
		XmlgenError(fn, "DLL was loaded too late to fetch all data - use a launcher.");
		return;
	}
	// GetFileAttributesA("mvl\\vlist") == -1 -> CreateDirectoryA (asm @ 0x40e32)
	if(GetFileAttributesA("mvl\\vlist") == INVALID_FILE_ATTRIBUTES &&
	   CreateDirectoryA("mvl\\vlist", NULL) == 0){
		XmlgenError(fn, "Failed to create mvl/vlist directory.");
		return;
	}
	// Clone safety guard (AGENTS.md null-pointer rule): the original reads
	// [DAT_100a6314][id] without a check; the clone refuses to run unwired.
	if(MvlModelInfoPtrs == 0){
		XmlgenError(fn, "Game model-info table not wired (DAT_100a6314 @ RVA 0xa6314).");
		return;
	}

	for(int modelId = 0x82; modelId <= 0xEC; modelId++){
		// mov esi, [eax + ebx*4]  (asm @ RVA 0x40e88)
		SMvlGameModelInfo *mi = (SMvlGameModelInfo *)MvlModelInfoPtrs[modelId];
		if(mi == 0){
			if(!warnNoModelInfo){
				XmlgenWarn(fn, "Model info for id %d not registered - skipping.", modelId);
				warnNoModelInfo = true;
			}
			continue;
		}

		// Loop-top locals (asm @ 0x40e8e..0x40f0d):
		int handlingId = mi->m_handlingId;                 // movzx ecx, word [esi+0x46]
		int vehType    = mi->m_vehicleType;                // mov eax, [esi+0x3c]
		int extraFlags = FUN_1003d120(modelId);            // -> [ebp-0x60], <extraflags>
		int heliType   = HeliGetType(modelId);             // -> [ebp-0x68]
		int heliWeapon = HeliGetWeaponType(modelId);       // -> [ebp-0x7c]
		float rotorRadius = 0.0f, rotorDamMult = 0.0f;     // HeliGetRotorSettings -> EDX/stack (asm @ 0x40efc)
		HeliGetRotorSettings(modelId, &rotorRadius, &rotorDamMult);

		if(vehType < 0 || vehType > 5){
			XmlgenError(fn, "Model %d has vehicle type %d out of range.", modelId, vehType);
			continue;
		}
		if(handlingId < 0 || handlingId >= 512){
			XmlgenError(fn, "Model %d handling id %d out of range.", modelId, handlingId);
			continue;
		}
		const SMvlHandlingRecord *hrec = &s_mvlHandlingRecords[handlingId];

		int maxPassengers = -1;                            // local_5c, init -1
		const char *nameSrc = mi->m_gameName;              // +0x32 (asm @ 0x40f23)
		if(modelId == 0x89)
			maxPassengers = 2;
		else if(modelId == 0x9b)
			maxPassengers = 1;
		else if(modelId == 0xab || modelId == 0xb2 || modelId == 0xd7)
			maxPassengers = 0;
		else if(modelId == 0xa7)
			maxPassengers = 8;
		if(modelId == 0xec)
			nameSrc = "vicechee";                          // @ RVA 0x10081fb8

		// sprintf("mvl\\vlist\\%s.xml", name) -> lowercase in place (asm 0x40f26..0x40f55).
		// Clone uses snprintf (AGENTS buffer-safety rule; original used _sprintf).
		snprintf(path, sizeof(path), "mvl\\vlist\\%s.xml", nameSrc);
		for(char *p = path; *p; p++)
			*p = (char)tolower((unsigned char)*p);        // _tolower @ RVA 0x5a141

		FILE *f = fopen(path, "w");                        // _fopen @ RVA 0x5a70f ("w")
		if(f == 0){
			if(!warnNoFolder){                            // warn-once local_51
				XmlgenWarn(fn, "Unable to generate file mvl\\vlist\\%s.xml, does folder mvl\\vlist exist?", nameSrc);
				warnNoFolder = true;
			}
			continue;
		}

		fprintf(f, "<?xml version=\"1.0\" encoding=\"ASCII\"?>\n");
		fprintf(f, "<vehicle>\n");
		fprintf(f, "\t<basic>\n");
		fprintf(f, "\t\t<type>%s</type>\n", MvlVehTypeNameTable[vehType]);
		// <name>%ls: TheText.Get(gameName) — HOOK POINT seam, see MvlTextGet.
		fprintf(f, "\t\t<name>%ls</name>\n", MvlTextGet(mi->m_gameName));
		// anims: m_animFileIndex == -1 -> "null" (@ RVA 0x10082088), else
		// CAnimManager::ms_aAnimBlocks[idx].m_name (gta-vc.exe 0x7DB428).
		if(mi->m_animFileIndex == -1){
			fprintf(f, "\t\t<anims>%s</anims>\n", kMvlAnimsNull);
		}
		else if(mi->m_animFileIndex >= 0 && mi->m_animFileIndex < 128){
			fprintf(f, "\t\t<anims>%s</anims>\n", s_mvlGameAnimBlocks[mi->m_animFileIndex]);
		}
		else{
			XmlgenError(fn, "Model %d anim file index %d out of range.", modelId, mi->m_animFileIndex);
			fprintf(f, "\t\t<anims>%s</anims>\n", "");
		}
		fprintf(f, "\t\t<comprules>%d</comprules>\n", (int)mi->m_compRules);   // [+0x8c]
		if(maxPassengers != -1)                                        // local_5c gate
			fprintf(f, "\t\t<maxpassengers>%d</maxpassengers>\n", maxPassengers);
		fprintf(f, "\t\t<extraflags>%04X</extraflags>\n", extraFlags);
		if(vehType == 0){
			// wheelmodel (int16 +0x44, movsx) + wheelscale (float +0x40)
			fprintf(f, "\t\t<wheelmodel>%d</wheelmodel>\n", (int)mi->m_wheelId);
			fprintf(f, "\t\t<wheelscale>%f</wheelscale>\n", (double)mi->m_wheelScale);
		}
		else if(vehType == 5){
			// wheelrotangle = (int)m_bikeSteerAngle (cvttss2si float +0x90)
			fprintf(f, "\t\t<wheelrotangle>%d</wheelrotangle>\n", (int)mi->m_bikeSteerAngle);
			fprintf(f, "\t\t<wheelscale>%f</wheelscale>\n", (double)mi->m_wheelScale);
		}
		else if(vehType == 4){
			// lodmodel = m_planeLodId (same int16 +0x44), NO wheelscale
			fprintf(f, "\t\t<lodmodel>%d</lodmodel>\n", (int)mi->m_wheelId);
		}
		// immunity = (modelId == 0xa2) ? 5 : 0  (cmove, asm @ 0x4112f)
		fprintf(f, "\t\t<immunity>%d</immunity>\n", modelId == 0xa2 ? 5 : 0);
		fprintf(f, "\t</basic>\n\n");

		fprintf(f, "\t<aidata>\n");
		// class byte +0x49: -1 -> "ignore" (@ RVA 0x100821cc), else table[]
		{
			int cls = (signed char)mi->m_vehicleClass;
			if(cls == -1){
				fprintf(f, "\t\t<class>%s</class>\n", "ignore");
			}
			else if(cls >= 0 && cls < 11){
				fprintf(f, "\t\t<class>%s</class>\n", MvlAiClassTable[cls]);
			}
			else{
				XmlgenError(fn, "Model %d vehicle class %d out of range.", modelId, cls);
				fprintf(f, "\t\t<class>%s</class>\n", "ignore");   // clone guard (see port-notes)
			}
			// freq: class==-1 -> 10, else int16 +0x4C (movsx)
			fprintf(f, "\t\t<freq>%d</freq>\n", cls == -1 ? 10 : (int)mi->m_frequency);
		}
		fprintf(f, "\t\t<level>%d</level>\n", (int)mi->m_level);  // movsx byte +0x4A
		fprintf(f, "\t</aidata>\n\n");

		fprintf(f, "\t<colors>\n");
		{
			unsigned numCol = mi->m_numColours;             // byte +0x154 (jbe guard)
			if(numCol > 8){
				XmlgenError(fn, "Model %d has %u car colours (max 8).", modelId, numCol);
				numCol = 8;
			}
			for(unsigned k = 0; k < numCol; k++){
				unsigned prim = mi->m_colours1[k];          // edi+0  (+0x144)
				unsigned sec  = mi->m_colours2[k];          // edi+8  (+0x14C)
				if(MvlRgbColoursEnabled){
					// RGB via game palette (gta-vc.exe 0x939228 == reVC
					// CVehicleModelInfo::ms_colourTextureTable; clone =
					// MvlGameColourTable, vehicle_archive.cpp, zero-filled).
					const unsigned char *pal = (const unsigned char *)MvlGameColourTable;
					fprintf(f, "\t\t<rgbcol>%d,%d,%d,%d,%d,%d</rgbcol>\n",
					        (int)pal[prim * 4 + 0], (int)pal[prim * 4 + 1], (int)pal[prim * 4 + 2],
					        (int)pal[sec * 4 + 0],  (int)pal[sec * 4 + 1],  (int)pal[sec * 4 + 2]);
				}
				else{
					fprintf(f, "\t\t<carcol>%d,%d</carcol>\n", prim, sec);
				}
			}
		}
		fprintf(f, "\t</colors>\n\n");

		fprintf(f, "\t<audio>\n");
		{
			// 7 slots, stride 0x1C, indexed by model id (asm @ 0x4129f, table
			// @ gta-vc.exe 0x6AC368); <doorsounds> is the byte of slot 6.
			const SMvlAudioStats *au = &s_mvlGameAudio[modelId];
			fprintf(f, "\t\t<enginefarsample>%u</enginefarsample>\n",   au->m_anSamples[0]);
			fprintf(f, "\t\t<enginenearsample>%u</enginenearsample>\n", au->m_anSamples[1]);
			fprintf(f, "\t\t<hornsample>%u</hornsample>\n",              au->m_anSamples[2]);
			fprintf(f, "\t\t<hornfreq>%u</hornfreq>\n",                  au->m_anSamples[3]);
			fprintf(f, "\t\t<sirensample>%u</sirensample>\n",            au->m_anSamples[4]);
			fprintf(f, "\t\t<sirenfreq>%u</sirenfreq>\n",                au->m_anSamples[5]);
			fprintf(f, "\t\t<doorsounds>%u</doorsounds>\n",              (unsigned)au->m_nDoorSounds);
		}
		if(vehType == 1){
			SMvlBoatEngine be;
			FUN_1003cc70(modelId, &be);                  // output via EDX (asm @ 0x41353)
			fprintf(f, "\n\t\t<boatengine>\n");
			fprintf(f, "\t\t\t<type>%d</type>\n",                 be.nType);
			fprintf(f, "\t\t\t<basevolume>%d</basevolume>\n",     be.nBaseVolume);
			fprintf(f, "\t\t\t<basefrequency>%d</basefrequency>\n", be.nBaseFrequency);
			fprintf(f, "\t\t\t<volumeincrease>%f</volumeincrease>\n",     (double)be.fVolumeIncrease);
			fprintf(f, "\t\t\t<frequencyincrease>%f</frequencyincrease>\n", (double)be.fFrequencyIncrease);
			fprintf(f, "\t\t</boatengine>\n");
		}
		fprintf(f, "\t</audio>\n\n");

		if((hrec->m_uFlags & 0x20000) != 0){              // test dword [edi+0xcc], 0x20000
			if(heliType < 0 || heliType > 3){
				XmlgenError(fn, "Model %d heli type %d out of range.", modelId, heliType);
				heliType = 0;
			}
			if(heliWeapon < 0 || heliWeapon > 2){
				XmlgenError(fn, "Model %d heli weapon type %d out of range.", modelId, heliWeapon);
				heliWeapon = 0;
			}
			fprintf(f, "\t<helidata>\n");
			fprintf(f, "\t\t<helitype>%s</helitype>\n", MvlHeliTypeNameTable[heliType]);
			fprintf(f, "\t\t<weapons>%s</weapons>\n",   MvlHeliWeaponNameTable[heliWeapon]);
			fprintf(f, "\t\t<rotorradius>%f</rotorradius>\n",     (double)rotorRadius);
			fprintf(f, "\t\t<rotordammult>%f</rotordammult>\n",   (double)rotorDamMult);
			fprintf(f, "\t</helidata>\n\n");
		}

		fprintf(f, "\t<handling>\n");
		fprintf(f, "\t\t<mass>%f</mass>\n",                     (double)hrec->m_fMass);
		fprintf(f, "\t\t<percentsubmerged>%d</percentsubmerged>\n", hrec->m_nPercentSubmerged);
		fprintf(f, "\t\t<steeringlock>%f</steeringlock>\n",     (double)hrec->m_fSteeringLock);
		fprintf(f, "\t\t<seatoffset>%f</seatoffset>\n",         (double)hrec->m_fSeatOffsetDistance);
		fprintf(f, "\t\t<damagemultiplier>%f</damagemultiplier>\n", (double)hrec->m_fCollisionDamageMultiplier);
		fprintf(f, "\t\t<value>%d</value>\n",                   hrec->m_nMonetaryValue);
		fprintf(f, "\t\t<flags>%08X</flags>\n\n",               hrec->m_uFlags);
		fprintf(f, "\t\t<dimensions>\n\t\t\t<x>%f</x>\n\t\t\t<y>%f</y>\n\t\t\t<z>%f</z>\n\t\t</dimensions>\n\n",
		        (double)hrec->m_vDimensions[0], (double)hrec->m_vDimensions[1], (double)hrec->m_vDimensions[2]);
		fprintf(f, "\t\t<centreofmass>\n\t\t\t<x>%f</x>\n\t\t\t<y>%f</y>\n\t\t\t<z>%f</z>\n\t\t</centreofmass>\n\n",
		        (double)hrec->m_vCentreOfMass[0], (double)hrec->m_vCentreOfMass[1], (double)hrec->m_vCentreOfMass[2]);
		if(vehType == 1){
			// boatsteering reuses traction slots (disasm @ 0x415b2)
			fprintf(f, "\t\t<boatsteering>\n");
			fprintf(f, "\t\t\t<bankforcemult>%f</bankforcemult>\n",       (double)hrec->m_fTractionMultiplier);
			fprintf(f, "\t\t\t<rudderturnforce>%f</rudderturnforce>\n",   (double)hrec->m_fTractionLoss);
			fprintf(f, "\t\t\t<speedsteerfalloff>%f</speedsteerfalloff>\n", (double)hrec->m_fTractionBias);
			fprintf(f, "\t\t</boatsteering>\n\n");
		}
		else{
			fprintf(f, "\t\t<traction>\n");
			fprintf(f, "\t\t\t<multiplier>%f</multiplier>\n", (double)hrec->m_fTractionMultiplier);
			fprintf(f, "\t\t\t<loss>%f</loss>\n",             (double)hrec->m_fTractionLoss);
			fprintf(f, "\t\t\t<bias>%f</bias>\n",             (double)hrec->m_fTractionBias);
			fprintf(f, "\t\t</traction>\n\n");
		}
		fprintf(f, "\t\t<transmission>\n");
		fprintf(f, "\t\t\t<numofgears>%d</numofgears>\n", (int)hrec->m_transmission.m_nNumGears);
		fprintf(f, "\t\t\t<maxspeed>%f</maxspeed>\n",     (double)hrec->m_transmission.m_fMaxSpeed);
		// acceleration = raw / 0.4f (divss [0x8ddfc], asm @ 0x416be)
		fprintf(f, "\t\t\t<acceleration>%f</acceleration>\n",
		        (double)(hrec->m_transmission.m_fAcceleration / kMvlAccelDiv));
		fprintf(f, "\t\t\t<drivetype>%c</drivetype>\n",   (int)hrec->m_transmission.m_nDriveType);
		fprintf(f, "\t\t\t<enginetype>%c</enginetype>\n", (int)hrec->m_transmission.m_nEngineType);
		fprintf(f, "\t\t</transmission>\n\n");
		if(vehType == 1){
			fprintf(f, "\t\t<boatbrakes>\n");
			fprintf(f, "\t\t\t<verticalwavehitlimit>%f</verticalwavehitlimit>\n", (double)hrec->m_fBrakeDeceleration);
			fprintf(f, "\t\t\t<forwardwavehitbrake>%f</forwardwavehitbrake>\n",   (double)hrec->m_fBrakeBias);
			fprintf(f, "\t\t</boatbrakes>\n\n");
			fprintf(f, "\t\t<boatsuspension>\n");
			fprintf(f, "\t\t\t<waterresvolumemult>%f</waterresvolumemult>\n", (double)hrec->m_fSuspensionForceLevel);
			fprintf(f, "\t\t\t<waterdampingmult>%f</waterdampingmult>\n",     (double)hrec->m_fSuspensionDampingLevel);
			fprintf(f, "\t\t\t<upperlimit>%f</upperlimit>\n",                 (double)hrec->m_fSuspUpperLimit);
			fprintf(f, "\t\t\t<handbrakedragmult>%f</handbrakedragmult>\n",   (double)hrec->m_fSuspLowerLimit);
			fprintf(f, "\t\t\t<sideslipforce>%f</sideslipforce>\n",           (double)hrec->m_fSuspBias);
			fprintf(f, "\t\t\t<antidive>%f</antidive>\n",                     (double)hrec->m_fSuspAntiDiveMultiplier);
			fprintf(f, "\t\t</boatsuspension>\n\n");
		}
		else{
			fprintf(f, "\t\t<brakes>\n");
			fprintf(f, "\t\t\t<deceleration>%f</deceleration>\n", (double)hrec->m_fBrakeDeceleration);
			fprintf(f, "\t\t\t<bias>%f</bias>\n",                 (double)hrec->m_fBrakeBias);
			fprintf(f, "\t\t\t<abs>%d</abs>\n",                   (int)hrec->m_bABS);
			fprintf(f, "\t\t</brakes>\n\n");
			fprintf(f, "\t\t<suspension>\n");
			fprintf(f, "\t\t\t<forcelevel>%f</forcelevel>\n",     (double)hrec->m_fSuspensionForceLevel);
			fprintf(f, "\t\t\t<dampening>%f</dampening>\n",       (double)hrec->m_fSuspensionDampingLevel);
			fprintf(f, "\t\t\t<upperlimit>%f</upperlimit>\n",     (double)hrec->m_fSuspUpperLimit);
			fprintf(f, "\t\t\t<lowerlimit>%f</lowerlimit>\n",     (double)hrec->m_fSuspLowerLimit);
			fprintf(f, "\t\t\t<bias>%f</bias>\n",                 (double)hrec->m_fSuspBias);
			fprintf(f, "\t\t\t<antidive>%f</antidive>\n",         (double)hrec->m_fSuspAntiDiveMultiplier);
			fprintf(f, "\t\t</suspension>\n\n");
		}
		fprintf(f, "\t\t<lights>\n");
		fprintf(f, "\t\t\t<front>%d</front>\n", (int)hrec->m_bFrontLights);
		fprintf(f, "\t\t\t<rear>%d</rear>\n",   (int)hrec->m_bRearLights);
		fprintf(f, "\t\t</lights>\n");
		fprintf(f, "\t</handling>\n\n");

		if((hrec->m_uFlags & 0x60000) != 0){              // test dword [edi+0xcc], 0x60000
			const SMvlFlyingRecord *fly = &s_mvlGameFlying[handlingId];
			fprintf(f, "\t<aerohandling>\n");
			fprintf(f, "\t\t<thrust>%f</thrust>\n",                 (double)fly->m_fThrust);
			fprintf(f, "\t\t<thrustfalloff>%f</thrustfalloff>\n",   (double)fly->m_fThrustFallOff);
			fprintf(f, "\t\t<yaw>%f</yaw>\n",                       (double)fly->m_fYaw);
			fprintf(f, "\t\t<yawstab>%f</yawstab>\n",               (double)fly->m_fYawStab);
			fprintf(f, "\t\t<sideslip>%f</sideslip>\n",             (double)fly->m_fSideSlip);
			fprintf(f, "\t\t<roll>%f</roll>\n",                     (double)fly->m_fRoll);
			fprintf(f, "\t\t<rollstab>%f</rollstab>\n",             (double)fly->m_fRollStab);
			fprintf(f, "\t\t<pitch>%f</pitch>\n",                   (double)fly->m_fPitch);
			fprintf(f, "\t\t<pitchstab>%f</pitchstab>\n",           (double)fly->m_fPitchStab);
			fprintf(f, "\t\t<formlift>%f</formlift>\n",             (double)fly->m_fFormLift);
			fprintf(f, "\t\t<attacklift>%f</attacklift>\n",         (double)fly->m_fAttackLift);
			fprintf(f, "\t\t<moveres>%f</moveres>\n\n",             (double)fly->m_fMoveRes);
			fprintf(f, "\t\t<turnres>\n\t\t\t<x>%f</x>\n\t\t\t<y>%f</y>\n\t\t\t<z>%f</z>\n\t\t</turnres>\n\n",
			        (double)fly->m_vTurnRes[0], (double)fly->m_vTurnRes[1], (double)fly->m_vTurnRes[2]);
			fprintf(f, "\t\t<speedres>\n\t\t\t<x>%f</x>\n\t\t\t<y>%f</y>\n\t\t\t<z>%f</z>\n\t\t</speedres>\n",
			        (double)fly->m_vSpeedRes[0], (double)fly->m_vSpeedRes[1], (double)fly->m_vSpeedRes[2]);
			fprintf(f, "\t</aerohandling>\n\n");
		}

		if(vehType == 1){
			const SMvlBoatRecord *boat = &s_mvlGameBoat[handlingId];
			fprintf(f, "\t<boathandling>\n");
			fprintf(f, "\t\t<thrusty>%f</thrusty>\n",                 (double)boat->m_fThrustY);
			fprintf(f, "\t\t<thrustz>%f</thrustz>\n",                 (double)boat->m_fThrustZ);
			fprintf(f, "\t\t<thrustappz>%f</thrustappz>\n",           (double)boat->m_fThrustAppZ);
			fprintf(f, "\t\t<aqplaneforce>%f</aqplaneforce>\n",       (double)boat->m_fAqPlaneForce);
			fprintf(f, "\t\t<aqplanelimit>%f</aqplanelimit>\n",       (double)boat->m_fAqPlaneLimit);
			fprintf(f, "\t\t<aqplaneoffset>%f</aqplaneoffset>\n",     (double)boat->m_fAqPlaneOffset);
			fprintf(f, "\t\t<waveaudiomult>%f</waveaudiomult>\n\n",   (double)boat->m_fWaveAudioMult);
			fprintf(f, "\t\t<moveres>\n\t\t\t<x>%f</x>\n\t\t\t<y>%f</y>\n\t\t\t<z>%f</z>\n\t\t</moveres>\n\n",
			        (double)boat->m_vMoveRes[0], (double)boat->m_vMoveRes[1], (double)boat->m_vMoveRes[2]);
			fprintf(f, "\t\t<turnres>\n\t\t\t<x>%f</x>\n\t\t\t<y>%f</y>\n\t\t\t<z>%f</z>\n\t\t</turnres>\n",
			        (double)boat->m_vTurnRes[0], (double)boat->m_vTurnRes[1], (double)boat->m_vTurnRes[2]);
			fprintf(f, "\t\t<looklrbcamheight>%f</looklrbcamheight>\n", (double)boat->m_fLookLRBehindCamHeight);
			fprintf(f, "\t</boathandling>\n\n");
		}
		else if(vehType == 5){
			const SMvlBikeRecord *bike = &s_mvlGameBike[handlingId];
			// FUN_1006d1e0 round-trip (identity, see helper) feeds maxlean/
			// wheelieang/stoppieang: float(raw)*180/pi (asm @ 0x41db6..0x41dd2);
			// fullanimlean skips the helper (no call, asm @ 0x41dee).
			#define MVL_BIKE_DEG(v) ((float)((float)FUN_1006d1e0((double)(v)) * kMvlRadToDegNum / kMvlRadToDegDen))
			fprintf(f, "\t<bikehandling>\n");
			fprintf(f, "\t\t<leanfwdcom>%f</leanfwdcom>\n",         (double)bike->m_fLeanFwdCOM);
			fprintf(f, "\t\t<leanfwdforce>%f</leanfwdforce>\n",     (double)bike->m_fLeanFwdForce);
			fprintf(f, "\t\t<leanbackcom>%f</leanbackcom>\n",       (double)bike->m_fLeanBakCOM);
			fprintf(f, "\t\t<leanbackforce>%f</leanbackforce>\n",   (double)bike->m_fLeanBakForce);
			fprintf(f, "\t\t<maxlean>%f</maxlean>\n",               (double)MVL_BIKE_DEG(bike->m_fMaxLean));
			// fullanimlean: float math only (no FUN_1006d1e0 call — asm @ 0x41dee
			// does mulss/divss then cvtps2pd, so keep the scale in float precision)
			fprintf(f, "\t\t<fullanimlean>%f</fullanimlean>\n",
			        (double)(bike->m_fFullAnimLean * kMvlRadToDegNum / kMvlRadToDegDen));
			fprintf(f, "\t\t<deslean>%f</deslean>\n",               (double)bike->m_fDesLean);
			fprintf(f, "\t\t<speedsteer>%f</speedsteer>\n",         (double)bike->m_fSpeedSteer);
			fprintf(f, "\t\t<slipsteer>%f</slipsteer>\n",           (double)bike->m_fSlipSteer);
			fprintf(f, "\t\t<noplayercomz>%f</noplayercomz>\n",     (double)bike->m_fNoPlayerCOMz);
			fprintf(f, "\t\t<wheelieang>%f</wheelieang>\n",         (double)MVL_BIKE_DEG(bike->m_fWheelieAng));
			fprintf(f, "\t\t<stoppieang>%f</stoppieang>\n",         (double)MVL_BIKE_DEG(bike->m_fStoppieAng));
			fprintf(f, "\t\t<wheeliesteer>%f</wheeliesteer>\n",     (double)bike->m_fWheelieSteer);
			fprintf(f, "\t\t<wheeliestabmult>%f</wheeliestabmult>\n", (double)bike->m_fWheelieStabMult);
			fprintf(f, "\t\t<stoppiestabmult>%f</stoppiestabmult>\n", (double)bike->m_fStoppieStabMult);
			fprintf(f, "\t</bikehandling>\n\n");
			#undef MVL_BIKE_DEG
		}

		// <specials> presets — verbatim literals (string xrefs 040df0).
		const char *specials = 0;
		if(modelId == 0x92)
			specials =
			"\t<specials>\n\t\t<emlights>\n\t\t\t<alwayson>false</alwayson>\n\t\t\t<colourone>255,0,0</colourone>\n\t\t\t<colourtwo>255,255,255</colourtwo>\n\t\t\t\n\t\t\t<posleft>\n\t\t\t\t<x>-1.1</x>\n\t\t\t\t<y>0.9</y>\n\t\t\t\t<z>1.6</z>\n\t\t\t</posleft>\n\t\t\t\n\t\t\t<posright>\n\t\t\t\t<x>1.1</x>\n\t\t\t\t<y>0.9</y>\n\t\t\t\t<z>1.6</z>\n\t\t\t</posright>\n\t\t</emlights>\n\t</specials>\n\n";
		else if(modelId == 0xa8 || modelId == 0xd8 || modelId == 0x96 || modelId == 0xbc)
			specials =
			"\t<specials>\n\t\t<taxilight>\n\t\t\t<alwayson>false</alwayson>\n\t\t\t<colour>128,128,0</colour>\n\n\t\t\t<position>\n\t\t\t\t<x>0.0</x>\n\t\t\t\t<y>0.0</y>\n\t\t\t\t<z>0.95</z>\n\t\t\t</position>\n\t\t</taxilight>\n\t</specials>\n\n";
		else if(modelId == 0xec)
			specials =
			"\t<specials>\n\t\t<emlightsingle>\n\t\t\t<alwayson>false</alwayson>\n\t\t\t<colour>70,70,255</colour>\n\n\t\t\t<position>\n\t\t\t\t<x>0.4</x>\n\t\t\t\t<y>0.6</y>\n\t\t\t\t<z>0.3</z>\n\t\t\t</position>\n\t\t</emlightsingle>\n\t</specials>\n\n";
		else if(modelId == 0x9d)
			specials =
			"\t<specials>\n\t\t<emlights>\n\t\t\t<colourone>255,0,0</colourone>\n\t\t\t<colourtwo>0,0,255</colourtwo>\n\t\t\t<posleft>\n\t\t\t\t<x>-1.1</x>\n\t\t\t\t<y>0.8</y>\n\t\t\t\t<z>1.2</z>\n\t\t\t</posleft>\n\t\t\t<posright>\n\t\t\t\t<x>1.1</x>\n\t\t\t\t<y>0.8</y>\n\t\t\t\t<z>1.2</z>\n\t\t\t</posright>\n\t\t</emlights>\n\t</specials>\n\n";
		else if(modelId == 0xdc)
			specials =
			"\t<specials>\n\t\t<emlightsingle>\n\t\t\t<alwayson>false</alwayson>\n\t\t\t<colour>70,70,255</colour>\n\n\t\t\t<position>\n\t\t\t\t<x>0.4</x>\n\t\t\t\t<y>0.6</y>\n\t\t\t\t<z>0.3</z>\n\t\t\t</position>\n\t\t</emlightsingle>\n\t</specials>\n\n";
		else if(modelId == 0x89)
			specials =
			"\t<specials>\n\t\t<emlights>\n\t\t\t<alwayson>false</alwayson>\n\t\t\t<colourone>255,0,0</colourone>\n\t\t\t<colourtwo>255,255,0</colourtwo>\n\t\t\t\n\t\t\t<posleft>\n\t\t\t\t<x>-1.1</x>\n\t\t\t\t<y>1.7</y>\n\t\t\t\t<z>2.0</z>\n\t\t\t</posleft>\n\t\t\t\n\t\t\t<posright>\n\t\t\t\t<x>1.1</x>\n\t\t\t\t<y>1.7</y>\n\t\t\t\t<z>2.0</z>\n\t\t\t</posright>\n\t\t</emlights>\n\t</specials>\n\n";
		else if(modelId == 0x9b)
			specials =
			"\t<specials>\n\t\t<heliweps>\n\t\t\t<missilepos>\n\t\t\t\t<x>2.5</x>\n\t\t\t\t<y>1.0</y>\n\t\t\t\t<z>-0.5</z>\n\t\t\t</missilepos>\n\t\t\t\n\t\t\t<machinegunpos>\n\t\t\t\t<x>0.0</x>\n\t\t\t\t<y>4.8</y>\n\t\t\t\t<z>-1.3</z>\n\t\t\t</machinegunpos>\n\t\t</heliweps>\n\t</specials>\n\n";
		else if(modelId == 0x9c)
			specials =
			"\t<specials>\n\t\t<emlights>\n\t\t\t<alwayson>false</alwayson>\n\t\t\t<colourone>255,0,0</colourone>\n\t\t\t<colourtwo>0,0,255</colourtwo>\n\t\t\t\n\t\t\t<posleft>\n\t\t\t\t<x>-0.7</x>\n\t\t\t\t<y>-0.4</y>\n\t\t\t\t<z>1.0</z>\n\t\t\t</posleft>\n\t\t\t\n\t\t\t<posright>\n\t\t\t\t<x>0.7</x>\n\t\t\t\t<y>-0.4</y>\n\t\t\t\t<z>1.0</z>\n\t\t\t</posright>\n\t\t</emlights>\n\t</specials>\n\n";
		else if(modelId == 0xb1)
			specials =
			"\t<specials>\n\t\t<heliweps>\n\t\t\t<machinegunpos>\n\t\t\t\t<x>0.0</x>\n\t\t\t\t<y>4.8</y>\n\t\t\t\t<z>-1.3</z>\n\t\t\t</machinegunpos>\n\t\t</heliweps>\n\t</specials>\n\n";
		if(specials)
			fprintf(f, "%s", specials);

		fprintf(f, "</vehicle>\n");
		fclose(f);                                          // _fclose @ RVA 0x5aabc
	}
}
