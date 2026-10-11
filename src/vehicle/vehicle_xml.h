// vehicle_xml.h — CMvlVehicle vehicle-XML parser cluster (LoadXML*).
//
// Implementation lives in vehicle_xml.cpp; the methods themselves are
// declared on CMvlVehicle in ../archive/vehicle_archive.h (ONE coherent
// class, per the task). This header carries the cluster documentation and
// the shared parse helpers.
//
// Function inventory (vehmod_retail.dll 0.98e, names from decomp error
// strings — authoritative):
//   CMvlVehicle::LoadXMLBasic                @ RVA 0x46420
//   CMvlVehicle::LoadXMLAIData               @ RVA 0x46960
//   CMvlVehicle::LoadXMLCarCol               @ RVA 0x46e90
//   CMvlVehicle::LoadXMLAudioBoatEngine      @ RVA 0x474d0
//   CMvlVehicle::LoadXMLAudio                @ RVA 0x478a0
//   CMvlVehicle::LoadXMLHandlingTraction     @ RVA 0x47c40
//   CMvlVehicle::LoadXMLHandlingTransmission @ RVA 0x47f10
//   CMvlVehicle::LoadXMLHandlingBrakes       @ RVA 0x483b0
//   CMvlVehicle::LoadXMLHandlingBoatBrakes   @ RVA 0x486c0
//   CMvlVehicle::LoadXMLHandlingSuspension   @ RVA 0x488d0
//   CMvlVehicle::LoadXMLHandlingBoatSuspension @ RVA 0x48e20
//   CMvlVehicle::LoadXMLHandlingBoatSteering @ RVA 0x49370
//   CMvlVehicle::LoadXMLHandlingLights       @ RVA 0x49640
//   CMvlVehicle::LoadXMLHandling             @ RVA 0x49930
//   CMvlVehicle::LoadXMLAeroHandling         @ RVA 0x4a610
//   CMvlVehicle::LoadXMLBikeHandling         @ RVA 0x4b1a0
//   CMvlVehicle::LoadXMLBoatHandling         @ RVA 0x4bee0
//   CMvlVehicle::LoadXMLHeliData             @ RVA 0x4c700
//   CMvlVehicle::LoadXMLHeliWepsData         @ RVA 0x4cb80
//   CMvlVehicle::LoadXMLSpecialData          @ RVA 0x4ccd0
//   CMvlVehicle::LoadXMLWeaponKeyData        @ RVA 0x4cec0
//   CMvlVehicle::LoadXMLWeaponMissileInfo    @ RVA 0x4d0d0
//   CMvlVehicle::LoadXMLWeaponMachineGunInfo @ RVA 0x4d380
//   CMvlVehicle::LoadXMLWeaponDockedInfo     @ RVA 0x4d610
//   CMvlVehicle::LoadXMLWeaponArmedInfo      @ RVA 0x4d890
//   CMvlVehicle::LoadXMLExtraWeaponData      @ RVA 0x4dd20
//   CMvlVehicle::LoadXMLWeaponList           @ RVA 0x4e430
//   CMvlVehicle::LoadXMLSectionVehicle       @ RVA 0x4e4f0
//   CMvlVehicle::LoadXMLFile                 @ RVA 0x4e790
//
// Element names are 1:1 with the decomp error strings and the sample XMLs
// (original_files/maxovehmod098e/mvl/help/samplexmls/), including the
// original's quirks (e.g. 'lightbox' error text for the 'hasboxes' element,
// the "'gamg'" typo in LoadXMLAIData, "'missileinfo'" in the machinegun
// parser's unknown-element message, 'heliweps' in the weapon fireoffset
// dupe message, and the transmission missing-element messages naming
// 'multiplier'/'loss'/'bias').
#pragma once

#include "../archive/vehicle_archive.h"

class TiXmlElement;

// Shared helpers (implemented in vehicle_xml.cpp):
//   MvlParseVector       — FUN_1001bdb0 @ 0x1bdb0 (<x><y><z> -> float[3])
//   MvlLookupVehicleType — FUN_1001ba00 @ 0x1ba00 (<type> text -> enum)
// Declared in vehicle_archive.h (used by both translation units).
