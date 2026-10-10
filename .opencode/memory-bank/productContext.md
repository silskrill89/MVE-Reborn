# Product Context — MVL-clone

## Product Vision
Clone **Maxo's Vehicle Loader (MVL)** — a GTA Vice City 1.0 DLL plugin — from binary reverse engineering. MVL provides an XML + 7z vehicle-archive configuration system, Python 3.3 scripting, and direct engine patching to load custom vehicles. Later target: port the clone to GTA San Andreas.

## Core Features (to be confirmed by RE)
1. **Vehicle loading** — custom vehicles from 7z archives
2. **XML configuration** — per-vehicle definitions (schema TBD by RE)
3. **extraflags bitmask** — per-vehicle feature flags
4. **Python 3.3 scripting** — per-vehicle scripts
5. **Engine patching** — direct VC 1.0 exe patches (D3D8-era, x86)

## User Requirements
- Clone must be functionally equivalent to MVL 0.98e (`vehmod_retail.dll`)
- Plugin target name: `vehmod`
- Deploy target: `%MVL_GAME_DIR%\mvl\vehmod_retail.dll` (game dir unknown — placeholder)
- `original_files/` is READ-ONLY — RE targets by version

## Known Constraints
- Host: GTA Vice City 1.0 (x86, D3D8)
- MVL hosts Python 3.3 — dependency on python33.dll
- 7z archive support — bundled 7z runtime in MVL distributions
