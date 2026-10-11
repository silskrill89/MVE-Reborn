// vehicle_archive.h — CMvlVehicle: archive pipeline + vehicle-XML parser cluster.
//
// Faithful recompilation of the CMvlVehicle load pipeline from decompiled
// vehmod_retail.dll 0.98e (decomp source:
// .harness-memory/limout/vehpipe/range-decomp.txt, range 0x44f20-0x4e790).
// Every method carries its origin VA. Original symbol names are reused 1:1
// per the naming policy (RTTI/error-string recovered names; see
// .serena/memories/re-098e-map.md). Methods without a recovered original name
// carry a provisional name + "(provisional)" in the comment and are logged in
// .harness-memory/limout/port-notes-veh.md.
//
// Layout note: the original CMvlVehicle is a single flat x86 object whose
// member offsets are cited throughout ("@ 0x..."). The clone keeps ONE
// coherent CMvlVehicle class; grouping structs below mirror the original
// field blocks and each carries its absolute offset in the comment. See
// port-notes-veh.md for the full offset map and open questions.
#pragma once

#include "../core/stdafx.h"
#include "seven_walker.h"

#include <string>
#include <vector>

class TiXmlElement;   // external/tinyxml — the original parsers use TinyXML

// ---------------------------------------------------------------------------
// Engine / game hooks. These wrap engine internals NOT yet ported (RW stream
// IO, clump/txd loading, col pool, anim files, sound handler, vehicle
// registry). Each declaration carries the target VA of the original routine.
// Stubs live at the bottom of vehicle_archive.cpp and log "HOOK POINT".
// DO NOT fake game behaviour here — the engine lane replaces these.
// ---------------------------------------------------------------------------

struct SMvlVehicleInfo;      // game/model-info wrapper (vtable object)
struct CCustomAnimFile;      // custom .ifp loader (0x12b0 ctor / 0x14e0 load)
class  CMvlVehicle;          // defined below

// CSevenWalker is real (src/archive/seven_walker.cpp wraps external/lzma).
// Additional archive-registry helpers referenced by ParseArchive (the
// registry holds CMvlVehicle objects — decomp compares against `this` and
// reads id @ 0 / totalSize @ 0x330):
int   MvlVehicleRegCount(void);                        // FUN_1004f3f0 @ 0x4f3f0
CMvlVehicle *MvlVehicleRegGet(int i);                  // FUN_1004f380 @ 0x4f380

// RenderWare-era stream/chunk helpers (engine lane). Names are behaviour-
// neutral where the exact semantics were not recovered; the VA is the truth.
void *MvlRwMemoryStreamOpen(const void *data, unsigned size);  // FUN_100320a0 @ 0x320a0
void  MvlRwMemoryStreamClose(void *stream);                    // FUN_10032090 @ 0x32090
unsigned MvlRwStreamFindChunkHeader(void *stream);             // FUN_10032770 @ 0x32770
                                     // returns chunk version (0 = not found);
                                     // used for .txd and .dff header finds
void  MvlRwClumpReadPrep(void);                                // FUN_10032760 @ 0x32760
void  MvlRwTexDictReadPrep(void);                              // FUN_10032750 @ 0x32750
void *MvlRwReadTextureDictionary(void *stream);                // FUN_10031290 @ 0x31290
void *MvlRwReadClump(void *stream);                            // FUN_10031670 @ 0x31670
void  MvlRwClumpPostRead(void *clump);                         // FUN_100312c0 @ 0x312c0
void  MvlRwClumpSetup(void *clump);                            // FUN_10031ca0 @ 0x31ca0
void  MvlRwClumpFinalise(void);                                // FUN_10031f10 @ 0x31f10
void  MvlRwDestroyTextureDictionary(void *txd);                // FUN_10032710 @ 0x32710
int   MvlRecognizeTextureData(const void *data, unsigned size);// FUN_1000d340 @ 0xd340
void  MvlProcessLoadedClump(void *vehicle, void *clump);       // FUN_1003c700 @ 0x3c700
void  MvlWeaponClumpRegister(void *clump);                     // FUN_10031450 @ 0x31450
void  MvlSpecialModelPrep(void);                               // FUN_10008b90/08b40 @ 0x8b90
void  MvlSetLoadingContext(void *vehicle, bool isSa);          // DAT_100b77d0/100b3f08 @ 0x45af0
void  MvlVehicleInfoSetClump(void *info, void *clump);         // vtable +0x24 @ 0x45af0
void  MvlVehicleInfoSetAnimsName(void *info, const char *name);// vtable +0x18 @ 0x46420
void  MvlVehicleInfoPostAnimsName(void *info);                 // vtable +0x1c @ 0x46420
void  MvlPostColModelSetup(void *vehicle);                     // FUN_10039ec0 @ 0x39ec0
void  MvlRegisterVehicleSlot(int id, void *info);              // DAT_100a6314/100a631c @ 0x45110
bool  MvlSaTexDictAvailable(void);                             // DAT_100b51d0 @ 0x44f20

// Collision (game col pool):
void *MvlColModelAlloc(void);                                  // thunk_FUN_1002c240 @ 0x2c240
void *MvlColModelInit(void *raw);                              // FUN_1002c290 @ 0x2c290
void  MvlColModelReadBuffer(void *col, const void *data, unsigned size); // FUN_1002e610 @ 0x2e610
void  MvlColModelSetVehicleId(void *col, unsigned short id);   // col +0x31/+0x32 @ 0x45d60
const float *MvlColModelGetBBox(void *col);                    // col +0x10..0x24 @ 0x45030
void *MvlColModelGetField0C(void *col);                        // col +0x0c @ 0x2e610

// Animation files (CCustomAnimFile lineage):
CCustomAnimFile *MvlAnimFileNew(void);                         // FUN_10059d7f new + FUN_100012b0 ctor
bool  MvlAnimFileLoad(CCustomAnimFile *af, const void *data, unsigned size); // FUN_100014e0 @ 0x14e0
int   MvlAnimFilePostProcess(CCustomAnimFile *af);             // FUN_100027a0 @ 0x27a0 (returns anim count)
void  MvlAnimFileFree(CCustomAnimFile *af);                    // thunk_FUN_100012f0 dtor + free

// Sound handler (CSoundHandler::LoadSoundFile @ 0x33e10 and friends):
bool  MvlSoundLoadFile(const void *name, void *slot, const char *fileName,
                       const void *data, unsigned size);       // FUN_10033e10 @ 0x33e10
void  MvlSoundNotifyLoaded(void);                              // FUN_100336f0 @ 0x336f0
void  MvlSoundNotifyAiClass(void);                             // FUN_10032f80 @ 0x32f80

// CVehicleHandler / engine glue referenced from InitialiseLoaded/LoadFromArchive:
void  MvlHandlerSetAiClass(int id, int aiClass);               // FUN_1004f7e0 @ 0x4f7e0
void  MvlHandlerApplyGang(unsigned gang, int id);              // FUN_1004f820 @ 0x4f820 (arg = this+0x2cf4)
void  MvlEngineRegisterVehicle(int id);                        // FUN_1003d790 @ 0x3d790
void  MvlVehicleSetNameW(wchar_t *dst, const char *src);       // FUN_1004f360 @ 0x4f360 (swprintf %hs)

// Colour helpers (game palette at 0x939228):
unsigned MvlPackRgb(int r, int g, int b);                      // FUN_10032f10 @ 0x32f10
extern const unsigned MvlGameColourTable[256];                 // DAT_0x939228 (engine lane)

// Math helpers used by the weapon parsers:
void  MvlVectorZero(float *v);                                 // FUN_10008c30 @ 0x8c30
void  MvlVectorCopy(float *dst, const float *src);             // FUN_10008c90 @ 0x8c90
void  MvlEulerToQuat(float *quat, const float *euler);         // FUN_10008250 @ 0x8250
void  MvlQuatMid(float *dst, const float *a, const float *b);  // FUN_10008590 @ 0x8590
void  MvlWeaponMatrixBuild(void *entry, const float *pos);     // FUN_1002e1e0/2e240/2e210 @ 0x2e1e0

// Anim-group name table (DAT_0x6857b0, stride 24, ~60 entries, bound
// 0x685d67): resolve an <anims> name to its table index.
int   MvlLookupAnimGroupIndex(const char *name);               // FUN_10044fc0 table @ 0x6857b0
const char *MvlAnimGroupTableName(int index);                  // row name accessor @ 0x6857b0

// EM-light sector parsers (CVehicleHandler — OUT OF SCOPE for this lane, see
// port-notes-veh.md; called from CMvlVehicle::LoadXMLSpecialData):
bool  LoadXMLEmLightData(TiXmlElement *e, void *dest);         // FUN_1004e8f0 @ 0x4e8f0
bool  LoadXMLEmLightSingle(TiXmlElement *e, void *dest);       // FUN_1004edc0 @ 0x4edc0
bool  LoadXMLTaxiLight(TiXmlElement *e, void *dest);           // FUN_1004f090 @ 0x4f090

// ---------------------------------------------------------------------------
// Data blocks. Field offsets are the absolute offsets inside the original
// CMvlVehicle object (x86); the structs group them for readability. Padding
// members mark gaps whose fields were not recovered from this decomp range.
// ---------------------------------------------------------------------------

// Game model-info wrapper (CMvlVehicle::m_pInfo, pointer @ 0x14). Virtual
// slots used by the load pipeline: +0x18 (set anims name, called with the
// anims text), +0x1c (post-set hook), +0x24 (install clump). Exact class
// definition belongs to the engine lane; fields below are the ones the XML
// and archive parsers touch.
struct SMvlVehicleInfo
{
	void  *pVtable;              // @ 0x00
	void  *pad04[5];             // @ 0x04..0x17 unrecovered
	int    nField018;            // @ 0x18 unrecovered
	void  *pColModel;            // @ 0x1c  *(info+0x1c) colmodel (LoadCollisionFile)
	int    pad020;               // @ 0x20
	int    nFlags028;            // @ 0x28  "already has clump" test in LoadModelFile
	int    pad02c[3];            // @ 0x2c..0x37
	int    nVehicleType;         // @ 0x3c  eVehicleType (LoadXMLBasic "type")
	float  fWheelScale;          // @ 0x40  (1.0f forced for planes)
	short  nWheelModelId;        // @ 0x44  wheelmodel / lodmodel
	short  nVehicleId;           // @ 0x46  id (LoadFromArchive)
	unsigned char nComprulesSrc; // @ 0x48  comprules default source (ResolveComprules)
	char   nAiClass;             // @ 0x49  aidata class (0-10, -1 ignore)
	char   nAiLevel;             // @ 0x4a  aidata level
	short  nAiFreq;              // @ 0x4c  aidata freq
	void  *pad04e[15];           // @ 0x4e..0x8b unrecovered
	int    nImmunity;            // @ 0x8c  basic table slot i=3 ("immunity" PROVISIONAL)
	float  fWheelRotAngle;       // @ 0x90  wheelrotangle (bikes)
	void  *pad094[43];           // @ 0x94..0x143 unrecovered
	char   bCarColInit1;         // @ 0x144 (LoadXMLCarCol default 1)
	char   pad145[7];
	char   bCarColInit2;         // @ 0x14c (LoadXMLCarCol default 1)
	char   pad14d[6];
	char   bCarColInit3;         // @ 0x154 (LoadXMLCarCol default 1)
	char   pad155[26];
	int    nSpecialModelId;      // @ 0x170  (LoadModelFile: != -1 triggers FUN_10008b90/08b40)
	// ... object continues; remaining fields unrecovered (engine lane owns it)
};

// Handling block. Parsed by the LoadXMLHandling* cluster into CMvlVehicle
// starting at @ 0x1c. Boat/boatsteering fields SHARE the traction/brake
// slots (the original overwrites them; the sectors are mutually exclusive).
struct SMvlVehicleHandling
{
	float  fMass;                // @ 0x1c  handling/mass
	unsigned pad020[2];          // @ 0x20..0x27 unrecovered
	float  fDimensions[3];       // @ 0x28  handling/dimensions (x,y,z); fallback from col
	float  fCentreOfMass[3];     // @ 0x34  handling/centreofmass
	int    nPercentSubmerged;    // @ 0x40  handling/percentsubmerged (%i; float convert at apply)
	float  fTractionMultiplier;  // @ 0x48  traction/multiplier | boatsteering/bankforcemult
	unsigned char pad04c[0x48];  // @ 0x4c..0x93 unrecovered
	unsigned char nDriveType;    // @ 0x94  transmission/drivetype (%c)
	unsigned char nEngineType;   // @ 0x95  transmission/enginetype (%c)
	unsigned char nNumGears;     // @ 0x96  transmission/numofgears (%i)
	unsigned char nFlagsLowByte; // @ 0x97  low byte of nHandlingFlags (dup @ 0xe4)
	float  fAcceleration;        // @ 0x98  transmission/acceleration * DAT_1008ddfc
	float  fMaxVelocity;         // @ 0x9c  transmission/maxspeed
	unsigned pad0a0[3];          // @ 0xa0..0xab unrecovered
	float  fBrakeDeceleration;   // @ 0xac  brakes/deceleration | boatbrakes/verticalwavehitlimit
	float  fBrakeBias;           // @ 0xb0  brakes/bias | boatbrakes/forwardwavehitbrake
	unsigned char nBrakeAbs;     // @ 0xb4  brakes/abs (0/1; 0 for boats)
	unsigned char pad0b5[3];
	float  fSteeringLock;        // @ 0xb8  handling/steeringlock
	float  fTractionLoss;        // @ 0xbc  traction/loss | boatsteering/rudderturnforce
	float  fTractionBias;        // @ 0xc0  traction/bias | boatsteering/speedsteerfalloff
	unsigned pad0c4;
	float  fSusp1;               // @ 0xc8  suspension/forcelevel | boatsuspension/waterresvolumemult
	float  fSusp2;               // @ 0xcc  suspension/dampening | boatsuspension/waterdampingmult
	float  fSusp3;               // @ 0xd0  suspension/upperlimit | boatsuspension/upperlimit
	float  fSusp4;               // @ 0xd4  suspension/lowerlimit | boatsuspension/handbrakedragmult
	float  fSusp5;               // @ 0xd8  suspension/bias | boatsuspension/sideslipforce
	float  fSusp6;               // @ 0xdc  suspension/antidive | boatsuspension/antidive
	float  fDamageMultiplier;    // @ 0xe0  handling/damagemultiplier
	unsigned nHandlingFlags;     // @ 0xe4  handling/flags (%x)
	float  fSeatOffset;          // @ 0xe8  handling/seatoffset
	int    nMonetaryValue;       // @ 0xec  handling/value (%i)
	unsigned char nLightsFront;  // @ 0xf0  lights/front
	unsigned char nLightsRear;   // @ 0xf1  lights/rear
	unsigned char pad0f2[2];
	int    nBoatSectionId;       // @ 0xf4  set to vehicle id when boathandling parsed
	// boathandling block @ 0xf8..0x12c
	float  fBoatThrustY;         // @ 0xf8  boathandling/thrusty
	float  fBoatThrustZ;         // @ 0xfc  boathandling/thrustz
	float  fBoatThrustAppZ;      // @ 0x100 boathandling/thrustappz
	float  fBoatAqPlaneForce;    // @ 0x104 boathandling/aqplaneforce
	float  fBoatAqPlaneLimit;    // @ 0x108 boathandling/aqplanelimit
	float  fBoatAqPlaneOffset;   // @ 0x10c boathandling/aqplaneoffset
	float  fBoatWaveAudioMult;   // @ 0x110 boathandling/waveaudiomult
	float  fBoatLookLrbCamHeight;// @ 0x114 boathandling/looklrbcamheight
	float  fBoatMoveRes[3];      // @ 0x118 boathandling/moveres (vector)
	float  fBoatTurnRes[3];      // @ 0x124 boathandling/turnres (vector)
	int    nAeroSectionId;       // @ 0x130 set to vehicle id when aerohandling parsed
	// aerohandling block @ 0x134..0x17c
	float  fAeroThrust;          // @ 0x134 aerohandling/thrust
	float  fAeroThrustFalloff;   // @ 0x138 aerohandling/thrustfalloff
	float  fAeroYaw;             // @ 0x13c aerohandling/yaw
	float  fAeroYawStab;         // @ 0x140 aerohandling/yawstab
	float  fAeroSideSlip;        // @ 0x144 aerohandling/sideslip
	float  fAeroRoll;            // @ 0x148 aerohandling/roll
	float  fAeroRollStab;        // @ 0x14c aerohandling/rollstab
	float  fAeroPitch;           // @ 0x150 aerohandling/pitch
	float  fAeroPitchStab;       // @ 0x154 aerohandling/pitchstab
	float  fAeroFormLift;        // @ 0x158 aerohandling/formlift
	float  fAeroAttackLift;      // @ 0x15c aerohandling/attacklift
	float  fAeroMoveRes;         // @ 0x160 aerohandling/moveres (scalar here!)
	float  fAeroTurnRes[3];      // @ 0x164 aerohandling/turnres (vector)
	float  fAeroSpeedRes[3];     // @ 0x170 aerohandling/speedres (vector)
	int    nBikeSectionId;       // @ 0x17c set to vehicle id when bikehandling parsed
	// bikehandling block @ 0x180..0x1bc
	float  fBikeLeanFwdCom;      // @ 0x180 bikehandling/leanfwdcom
	float  fBikeLeanFwdForce;    // @ 0x184 bikehandling/leanfwdforce
	float  fBikeLeanBackCom;     // @ 0x188 bikehandling/leanbackcom
	float  fBikeLeanBackForce;   // @ 0x18c bikehandling/leanbackforce
	float  fBikeMaxLean;         // @ 0x190 bikehandling/maxlean (deg -> rad)
	float  fBikeFullAnimLean;    // @ 0x194 bikehandling/fullanimlean (deg -> rad)
	float  fBikeDesLean;         // @ 0x198 bikehandling/deslean
	float  fBikeSpeedSteer;      // @ 0x19c bikehandling/speedsteer
	float  fBikeSlipSteer;       // @ 0x1a0 bikehandling/slipsteer
	float  fBikeNoPlayerComZ;    // @ 0x1a4 bikehandling/noplayercomz
	float  fBikeWheelieAng;      // @ 0x1a8 bikehandling/wheelieang (deg -> rad)
	float  fBikeStopPieAng;      // @ 0x1ac bikehandling/stoppieang (deg -> rad)
	float  fBikeWheelieSteer;    // @ 0x1b0 bikehandling/wheeliesteer
	float  fBikeWheelieStabMult; // @ 0x1b4 bikehandling/wheeliestabmult
	float  fBikeStopPieStabMult; // @ 0x1b8 bikehandling/stoppiestabmult
};

// Weapon slot (one <weapon> of <weaponlist>), 0xC0 bytes each,
// CMvlVehicle::m_aWeapons[4] @ 0x2864, count @ 0x2860.
struct SMvlVehicleWeaponData
{
	unsigned nType;              // @ 0x00 1=missile, 2=machinegun
	unsigned nModel;             // @ 0x04 0=none 1=rocketlauncher 2=m4 3..6=custom1..4
	unsigned char bHasDocked;    // @ 0x08 1 when <dockedinfo> parsed
	unsigned char bGravity;      // @ 0x09 missileinfo/gravity ("on"=1)
	unsigned char pad0a[2];
	unsigned nMissileTimeout;    // @ 0x0c missileinfo/timeout (default 1000)
	float  fMissileAddSpeedMul;  // @ 0x10 missileinfo/addspeedmul (default 1.0)
	int    nMachineGunSound;     // @ 0x14 machineguninfo/sound (default 55)
	float  fMachineGunRange;     // @ 0x18 machineguninfo/range (default 100; neg->1.0)
	float  fMachineGunAccuracy;  // @ 0x1c machineguninfo/accuracy (deg -> rad, clamp 0..45)
	unsigned nMachineGunTimeout; // @ 0x20 machineguninfo/timeout (default 30, 0->25)
	float  fDockedPosition[3];   // @ 0x24 dockedinfo/position
	float  fDockedRotation[4];   // @ 0x30 dockedinfo/rotation (euler -> quat)
	unsigned nDockSpeed;         // @ 0x40 dockedinfo/dockspeed
	unsigned nMoveSpeed;         // @ 0x44 armedinfo/movespeed
	float  fMiddlePos;           // @ 0x48 armedinfo/middlepos (default 0.5, clamp 0..1)
	float  fMinPos[3];           // @ 0x4c armedinfo/minpos
	float  fMinRot[4];           // @ 0x58 armedinfo/minrot (euler -> quat)
	float  fMaxPos[3];           // @ 0x68 armedinfo/maxpos (defaults to minpos)
	float  fMaxRot[4];           // @ 0x74 armedinfo/maxrot (defaults to minrot)
	float  fMidRot[4];           // @ 0x84 mid rotation quat (FUN_10008590)
	float  fMaxPosCopy[3];       // @ 0x94 copy of maxpos (FUN_10008c90)
	float  fFireOffset[3];       // @ 0xa0 weapon/fireoffset
	unsigned char aHotKeys[5][4];// @ 0xac hotkeys/{dockkey,moveupkey,movedownkey,
	                             //           restorekey,shootkey} x 3 key bytes + pad
};

// <audio> sector int fields (6 sample ids) + boat engine block. The original
// stores the sample ids in a 6-int run whose exact base offset was not
// recoverable from this decomp range (decompiler left it as an opaque EDX);
// layout below is the only placement consistent with the known neighbours
// (lightbox byte @ 0x2bcc, boatengine @ 0x2bd0). See port-notes-veh.md.
struct SMvlVehicleAudioData
{
	unsigned nEngineFarSample;   // @ ~0x2bb4 enginefarsample (range 268-457)
	unsigned nEngineNearSample;  // @ ~0x2bb8 enginenearsample (range 0-35)
	unsigned nHornSample;        // @ ~0x2bbc hornsample (range 0-35)
	unsigned nHornFreq;          // @ ~0x2bc0 hornfreq (range 5-1000000)
	unsigned nSirenSample;       // @ ~0x2bc4 sirensample (range 0-3)
	unsigned nSirenFreq;         // @ ~0x2bc8 sirenfreq (range 5-1000000)
	unsigned char bHasLightBox;  // @ 0x2bcc lights/hasboxes ("lightbox" in errors)
	unsigned char pad2bd[3];
	unsigned nBoatEngineType;              // @ 0x2bd0 boatengine/type (0-1)
	unsigned nBoatEngineBaseVolume;        // @ 0x2bd4 boatengine/basevolume (0-100)
	unsigned nBoatEngineBaseFrequency;     // @ 0x2bd8 boatengine/basefrequency (5-1000000)
	float  fBoatEngineVolumeIncrease;      // @ 0x2bdc boatengine/volumeincrease
	float  fBoatEngineFrequencyIncrease;   // @ 0x2be0 boatengine/frequencyincrease
};

// Per-archive-file slot ids — RegisterArchiveFile classification
// (vehmod_retail.dll 0.98e @ RVA 0x455d0):
enum EMvlArchiveFileSlot
{
	MVL_SLOT_XML = 0,            // ".xml"
	MVL_SLOT_DFF = 1,            // ".dff"
	MVL_SLOT_TXD = 2,            // ".txd"
	MVL_SLOT_COL = 3,            // ".col"
	MVL_SLOT_TUNING1 = 4,        // "tuning1.dff"
	MVL_SLOT_TUNING2 = 5,
	MVL_SLOT_TUNING3 = 6,
	MVL_SLOT_TUNING4 = 7,
	MVL_SLOT_TUNING5 = 8,
	MVL_SLOT_TUNING6 = 9,
	MVL_SLOT_TUNING7 = 10,
	MVL_SLOT_TUNING8 = 11,
	MVL_SLOT_TUNING9 = 12,
	MVL_SLOT_WEAPONS_TXD = 13,   // "weapons.txd"
	MVL_SLOT_WEAPONS_DFF = 14,   // "weapons.dff"
	MVL_SLOT_IFP = 15,           // ".ifp"
	MVL_SLOT_COUNT = 16
};

// Archive-file entry size cap: "File '%s' is larger than 12582912 bytes."
#define MVL_MAX_ARCHIVE_FILE_SIZE  0xC00000u
#define MVL_MAX_COLOURS            16
#define MVL_MAX_WEAPONS            4

// ---------------------------------------------------------------------------
// CMvlVehicle — one vehicle load unit (archive + XML + registration).
// ---------------------------------------------------------------------------
class CMvlVehicle
{
public:
	CMvlVehicle();

	// === Archive pipeline ================================================
	// Original call order (LoadFromArchive body, 0.98e @ 0x45110):
	//   ParseArchive -> [check dup] -> RegisterArchiveFile (per entry)
	//   -> VerifyRequiredFiles -> LoadFromArchive continues:
	//   LoadWeaponFiles -> LoadXMLFile -> ResolveAnimIndex -> LoadTextureFile
	//   -> LoadModelFile -> LoadCollisionFile -> LoadAnimationFile
	//   -> InitialiseLoaded

	// CMvlVehicle::LoadFromArchive — vehmod_retail.dll 0.98e @ RVA 0x45110
	// (exception filter / catch handler logged at @ 0x45318). Throws a
	// format-string exception on any stage failure; the catch block logs
	// "CMvlVehicle::LoadFromArchive" + message and rethrows.
	void LoadFromArchive(const char *archivePath, int id);

	// CMvlVehicle::ParseArchive — 0.98e @ RVA 0x45360. Opens the 7z via
	// CSevenWalker, rejects duplicate archives (by total uncompressed size),
	// extracts every entry and feeds it to RegisterArchiveFile, then runs
	// VerifyRequiredFiles. Errors are logged under "CMvlVehicle::ParseArchive".
	bool ParseArchive(const char *archivePath);

	// CMvlVehicle::RegisterArchiveFile — 0.98e @ RVA 0x455d0. Classifies one
	// extracted file (tuning1-9.dff, weapons.txd/dff exact names; then
	// extension .xml/.dff/.txd/.col/.ifp/.wav). ".wav" goes to the sound
	// handler. Returns false on hard failure (size cap, duplicate type).
	bool RegisterArchiveFile(const char *name, void *data, unsigned size);

	// CMvlVehicle::VerifyRequiredFiles — 0.98e @ RVA 0x45870. Requires XML,
	// DFF, TXD; COL required unless the DFF is an SA model (IsSaDff result is
	// cached in m_bIsSaModel @ 0x8).
	bool VerifyRequiredFiles(void);

	// CMvlVehicle::LoadTextureFile — 0.98e @ RVA 0x45900 (catch @ 0x45a5a).
	// Loads the main .txd into a RwTexDictionary (m_pTxd @ 0x1e8). The
	// original's AL return is checked by LoadFromArchive.
	bool LoadTextureFile(void);

	// CMvlVehicle::LoadModelFile — 0.98e @ RVA 0x45af0 (catch @ 0x45ccb).
	// Loads the main .dff clump and installs it into the model info.
	bool LoadModelFile(void);

	// CMvlVehicle::LoadCollisionFile — 0.98e @ RVA 0x45d60 (catch @ 0x45ed7).
	// Validates "COLL" header and reads a CColModel from the .col buffer.
	bool LoadCollisionFile(void);

	// CMvlVehicle::LoadAnimationFile — 0.98e @ RVA 0x45f50. Loads the .ifp
	// via CCustomAnimFile (m_pAnimFile @ 0x2be4). Returns false on read fail
	// (logged, archive load continues — matches original control flow).
	bool LoadAnimationFile(void);

	// CMvlVehicle::LoadWeaponFiles — 0.98e @ RVA 0x46010 (catch @ 0x462fe).
	// Loads weapons.txd (m_pWeaponsTxd @ 0x2b64) and weapons.dff clump.
	bool LoadWeaponFiles(void);

	// CMvlVehicle::InitialiseLoaded — 0.98e @ RVA 0x44f20. Final registration:
	// dimensions fallback, SA txd warning, AI class/anim group push, engine
	// registration, success log "ID %d: '%ls' from archive '%s'".
	void InitialiseLoaded(void);

	// === Unnamed helpers (provisional names; see port-notes-veh.md) =======
	// FUN_10044fc0 @ 0x44fc0 — resolve m_szAnimsName against the anim-group
	// name table (DAT_0x6857b0) into m_nAnimsIndex @ 0x7dc ("null" -> -1).
	void ResolveAnimIndex(void);

	// FUN_10045030 @ 0x45030 — fill m_handling.fDimensions from the collision
	// box when <dimensions> was absent ("Dimensions set to %f %f %f.").
	void SetDimensionsFromCollision(void);

	// FUN_100450d0 @ 0x450d0 — resolve m_nCompRules @ 0xc when left at -1
	// (from info comprules byte for cars; 1 for bikes, else 0).
	void ResolveComprules(void);

	// FUN_10046370 @ 0x46370 — peek the DFF clump chunk version; returns true
	// when the model is a San Andreas RW version ((ver-0x34004)<0x2000).
	static bool IsSaDff(const void *data, unsigned size);

	// FUN_100463f0 @ 0x463f0 — return &m_aSoundSlots[i] for i<=1 when the
	// slot is initialised, else NULL.
	void *GetWeaponSoundSlot(unsigned i);

	// === Vehicle XML parser cluster ======================================
	// All implemented in src/vehicle/vehicle_xml.cpp; exact element names
	// and log strings recovered from the decomp + sample XMLs. Signatures
	// take the sector element; return false on parse error (the exact
	// error text was already logged).

	// CMvlVehicle::LoadXMLBasic          — 0.98e @ RVA 0x46420
	bool LoadXMLBasic(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLAIData         — 0.98e @ RVA 0x46960
	bool LoadXMLAIData(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLCarCol         — 0.98e @ RVA 0x46e90
	bool LoadXMLCarCol(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLAudioBoatEngine— 0.98e @ RVA 0x474d0
	bool LoadXMLAudioBoatEngine(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLAudio          — 0.98e @ RVA 0x478a0
	bool LoadXMLAudio(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandlingTraction    — 0.98e @ RVA 0x47c40
	bool LoadXMLHandlingTraction(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandlingTransmission— 0.98e @ RVA 0x47f10
	bool LoadXMLHandlingTransmission(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandlingBrakes      — 0.98e @ RVA 0x483b0
	bool LoadXMLHandlingBrakes(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandlingBoatBrakes  — 0.98e @ RVA 0x486c0
	bool LoadXMLHandlingBoatBrakes(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandlingSuspension  — 0.98e @ RVA 0x488d0
	bool LoadXMLHandlingSuspension(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandlingBoatSuspension — 0.98e @ RVA 0x48e20
	bool LoadXMLHandlingBoatSuspension(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandlingBoatSteering— 0.98e @ RVA 0x49370
	bool LoadXMLHandlingBoatSteering(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandlingLights      — 0.98e @ RVA 0x49640
	bool LoadXMLHandlingLights(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHandling       — 0.98e @ RVA 0x49930
	bool LoadXMLHandling(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLAeroHandling   — 0.98e @ RVA 0x4a610
	bool LoadXMLAeroHandling(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLBikeHandling   — 0.98e @ RVA 0x4b1a0
	bool LoadXMLBikeHandling(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLBoatHandling   — 0.98e @ RVA 0x4bee0
	bool LoadXMLBoatHandling(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHeliData       — 0.98e @ RVA 0x4c700
	bool LoadXMLHeliData(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLHeliWepsData   — 0.98e @ RVA 0x4cb80
	bool LoadXMLHeliWepsData(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLSpecialData    — 0.98e @ RVA 0x4ccd0
	bool LoadXMLSpecialData(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLWeaponKeyData  — 0.98e @ RVA 0x4cec0
	static bool LoadXMLWeaponKeyData(TiXmlElement *sector, SMvlVehicleWeaponData *w);
	// CMvlVehicle::LoadXMLWeaponMissileInfo    — 0.98e @ RVA 0x4d0d0
	static bool LoadXMLWeaponMissileInfo(TiXmlElement *sector, SMvlVehicleWeaponData *w);
	// CMvlVehicle::LoadXMLWeaponMachineGunInfo — 0.98e @ RVA 0x4d380
	static bool LoadXMLWeaponMachineGunInfo(TiXmlElement *sector, SMvlVehicleWeaponData *w);
	// CMvlVehicle::LoadXMLWeaponDockedInfo     — 0.98e @ RVA 0x4d610
	static bool LoadXMLWeaponDockedInfo(TiXmlElement *sector, SMvlVehicleWeaponData *w);
	// CMvlVehicle::LoadXMLWeaponArmedInfo      — 0.98e @ RVA 0x4d890
	static bool LoadXMLWeaponArmedInfo(TiXmlElement *sector, SMvlVehicleWeaponData *w);
	// CMvlVehicle::LoadXMLExtraWeaponData      — 0.98e @ RVA 0x4dd20
	bool LoadXMLExtraWeaponData(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLWeaponList     — 0.98e @ RVA 0x4e430
	bool LoadXMLWeaponList(TiXmlElement *sector);
	// CMvlVehicle::LoadXMLSectionVehicle — 0.98e @ RVA 0x4e4f0
	bool LoadXMLSectionVehicle(TiXmlElement *root);
	// CMvlVehicle::LoadXMLFile           — 0.98e @ RVA 0x4e790
	bool LoadXMLFile(void);

	// === Data members (original offsets in comments) =====================
	int    m_nId;                        // @ 0x00
	int    m_nType;                      // @ 0x04  eVehicleType (0 car,1 boat,2 train,3 heli,4 plane,5 bike)
	char   m_bIsSaModel;                 // @ 0x08
	int    m_nCompRules;                 // @ 0x0c  -1 = unset (ResolveComprules)
	unsigned m_nExtraFlags;              // @ 0x10
	SMvlVehicleInfo *m_pInfo;            // @ 0x14  game model info (vtable object)
	int    m_nIdCopy;                    // @ 0x18  id duplicate

	SMvlVehicleHandling m_handling;      // @ 0x1c .. 0x1bc+

	unsigned char m_nDoorSounds;         // @ 0x1d4 audio/doorsounds
	int    m_nHeliType;                  // @ 0x1d8 helidata/helitype (0 normal,1 remote,2 water,3 hunter)
	int    m_nHeliWeapons;               // @ 0x1dc helidata/weapons (0 none,1 hunter,2 sparrow)
	float  m_fRotorRadius;               // @ 0x1e0 helidata/rotorradius
	float  m_fRotorDamageMult;           // @ 0x1e4 helidata/rotordammult
	void  *m_pTxd;                       // @ 0x1e8 RwTexDictionary* (main .txd)
	wchar_t m_wsName[32];                // @ 0x1ec display name (wide)
	char   m_szArchivePath[260];         // @ 0x22c archive path (0x104 bytes)

	unsigned m_nArchiveTotalSize;        // @ 0x330 sum of entry sizes (dup-archive key)
	void    *m_apFiles[MVL_SLOT_COUNT];  // @ 0x334 extracted buffers
	unsigned m_anFileSizes[MVL_SLOT_COUNT]; // @ 0x374
	char    m_aszFileNames[4][260];      // @ 0x3b4 names for slots 0..3

	char   m_szAnimsName[24];            // @ 0x7c4 basic/anims ("null" default)
	int    m_nAnimsIndex;                // @ 0x7dc resolved anim group index (-1 = default)

	unsigned char m_abEmLightData[0x44]; // @ 0x27f4 specials/emlights|emlightsingle|taxilight
	                                     //        (parsed by LoadXMLEmLightData @ 0x4e8f0)
	float  m_fBasicUnk10;                // @ 0x2838 pre-default DAT_1008de28 (SA: DAT_1008de40)
	                                     //        basic table slot i=10 — name unrecovered
	unsigned char m_bHasHeliMissilePos;  // @ 0x283c
	unsigned char m_bHasHeliGunPos;      // @ 0x283d
	unsigned char pad283e[2];
	float  m_fHeliMissilePos[3];         // @ 0x283e heliweps/missilepos
	float  m_fHeliGunPos[3];             // @ 0x284a heliweps/machinegunpos

	int    m_nWeaponCount;               // @ 0x2860
	SMvlVehicleWeaponData m_aWeapons[MVL_MAX_WEAPONS]; // @ 0x2864 (4 x 0xC0)

	void  *m_pWeaponsTxd;                // @ 0x2b64 weapons.txd texdict
	unsigned m_anCustomModelLoaded[4];   // @ 0x2b68 custom1-4 model loaded flags
	SMvlVehicleAudioData m_audio;        // @ ~0x2bb4 (see struct comment)

	void  *m_pAnimFile;                  // @ 0x2be4 CCustomAnimFile* (.ifp)
	unsigned m_nMaxPassengers;           // @ 0x2bec basic/maxpassengers PROVISIONAL

	unsigned m_anColours[MVL_MAX_COLOURS][4]; // @ 0x2bf0 16 x 4 colours
	int    m_nColourCount;               // @ 0x2cf0
	unsigned m_nGang;                    // @ 0x2cf4 aidata/gang (0-7)

	// Sound slot block @ 0x2cf8: 2 slots x 20 bytes (FUN_100463f0 indexes
	// base + i*20 + 0x2cf8). Layout inside a slot unrecovered — raw bytes.
	unsigned char m_abSoundSlots[2][20]; // @ 0x2cf8
	void  *m_pColModelDerived;           // @ 0x2d28 colModel->field_0xc mirror
};

// Shared XML parse helpers (vehicle_xml.cpp):
// FUN_1001bdb0 @ 0x1bdb0 — parse <x><y><z> child vector into 3 floats.
bool MvlParseVector(TiXmlElement *e, float *dest);
// FUN_1001ba00 @ 0x1ba00 — <type> text -> eVehicleType (-1 unknown).
int  MvlLookupVehicleType(const char *text);
// FUN_10032f10 @ 0x32f10 — pack r,g,b into the game colour dword.
unsigned MvlPackRgb(int r, int g, int b);
