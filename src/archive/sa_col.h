// sa_col.h — LoadSAColSection: San-Andreas-format collision section loader.
//
// vehmod_retail.dll 0.98e, free function (name from its own error-log
// identity "LoadSAColSection"; re-098e-map §3). Decomp source:
// .harness-memory/limout/missing33/decomp-chain.txt depth-0 root
// 0x100388a0 (1279 B; its 4 callees are exported depth-1 sections).
//
// Parses an SA-wrapped collision section (COL2 / COL3 / COLL magic) into the
// game CColModel attached to the loading vehicle's info object
// (CMvlVehicle::m_pInfo->pColModel @ +0x1c; the +0x1b "SA col loaded" byte
// is stamped alongside). Dispatch point in the original is not in this dump —
// the archive lane's RegisterArchiveFile / LoadCollisionFile should route SA
// magic here (orchestrator note; see port report).
#pragma once

#include "../core/stdafx.h"

// LoadSAColSection — 0.98e @ RVA 0x388a0. Returns its data argument (the
// decomp returns param_1 on every path). Size cap: > 1MB is rejected with
// "SA collision section length longer than 1MB.".
void *LoadSAColSection(const void *data, unsigned size);

// DAT_100b77d0 - 0.98e @ RVA 0xb77d0 - archive loading context: the
// CMvlVehicle whose colmodel the SA section attaches to. NOTE: the archive
// lane's MvlSetLoadingContext @ 0x45af0 (vehicle_archive.cpp) is still a
// hook-point stub and does not route here — the orchestrator wires the two
// (MvlSetLoadingContext(veh, ...) -> MvlSaColSetLoadingVehicle(veh)).
void MvlSaColSetLoadingVehicle(void *veh);
