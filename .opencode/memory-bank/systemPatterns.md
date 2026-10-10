# System Patterns — MVL-clone

## Recurring Patterns (expected — to be confirmed by RE)

### 1. Version-Diff Isolation
**Pattern**: `original_files/` contains MVL 0.9 → 0.98e. Diffing adjacent versions isolates the code added per feature.
**Impact**: Faster RE than analyzing 0.98e blind.
**Method**: See `.opencode/skills/mvl-re/SKILL.md`.

### 2. Config-Driven Loading
**Pattern**: XML files next to the plugin define vehicles; 7z archives carry assets.
**Impact**: XML schema is the contract — RE it first, document in docs/.

### 3. Embedded Scripting Runtime
**Pattern**: MVL hosts Python 3.3 (python33.dll) for per-vehicle scripts.
**Impact**: Script API surface must be RE'd (function names, calling conventions).

### 4. Engine Patching
**Pattern**: Direct code patches into VC 1.0 exe (x86, D3D8-era).
**Impact**: Patch sites are version-sensitive — cite binary + version for every claim; log per version in `.serena/memories/re-*.md`.

## Rules
- Never modify `original_files/`.
- Every RE claim cites binary + version.
