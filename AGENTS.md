# AGENTS.md — MVL-clone (Maxos_vehicle_revnres)

## Project

Reverse-engineering rebuild of **Maxo's Vehicle Loader (MVL)** — a GTA Vice City 1.0 DLL plugin (`vehmod_retail.dll`, also distributed as `.flt`). MVL implements an XML + 7z vehicle-archive configuration system, hosts Python 3.3 for per-vehicle scripting, and applies direct engine patches to the VC 1.0 exe (x86, D3D8-era).

**Goal**: clone MVL 0.98e functionality from binary RE, then later port to GTA San Andreas.

- **Project name**: MVL-clone / workspace `mvlclone`

**Language policy (binding)**: pure C/C++ only — no Rust. Fast, lean C in the
RenderWare-era idiom; match original performance. The original mixes languages
(C++ core + embedded Python 3.3 + XML); our C++ core replaces all of it, with the
Python host as a late-phase module behind a C ABI bridge.

**Config format progression**: game INI → MVL XML (compat layer reads MVL XMLs
1:1) → O3DE-style JSON (settings-registry / layered-JSON conventions) as the
project's open end-format for parity with the O3DE community. See `PLAN.md` §2.
- **Plugin target**: `vehmod`
- **Primary RE target**: `original_files/maxovehmod098e/vehmod_retail.dll`
- **Version range**: `original_files/maxovehmod*` 0.9 → 0.98e (+ `xmlgen_launcher`)

## Repo Layout

| Path | Purpose | Owner |
|------|---------|-------|
| `original_files/` | READ-ONLY RE targets by version | RE (read-only) |
| `src/` | Clone implementation | orchestrator |
| `docs/` | Obsidian vault: RE notes | harness |
| `premake5.lua` / `premake5.cmd` | Build definition | orchestrator |
| `external/` | Third-party deps | orchestrator |
| `tools/` | Build/RE tooling | orchestrator |
| `PLAN.md` | Master plan | orchestrator |
| `AGENTS.md`, `README.md`, `memory-bank/`, `.sadie/`, `.serena/`, `.opencode/`, `.harness-memory/`, `.lsai/`, `.obsidian/` | Harness | harness |

## Build

```bash
premake5.cmd    # premake5.lua → build/MVL-clone.sln (VS2022)
msbuild build/MVL-clone.sln /p:Configuration=Release /p:Platform=Win32
```

- **MSBuild**: VS2022 at `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe`
- **SDK**: Windows 10 SDK 26100
- **Platform**: x86 (Win32), static CRT (/MT)
- **Output**: `bin/Release/vehmod.dll`
- **Deploy**: `%MVL_GAME_DIR%\mvl\vehmod_retail.dll` — game dir unknown (env var `MVL_GAME_DIR` is a placeholder; ask the user)
- **Host**: GTA Vice City 1.0 (`gta-vc.exe`, x86, D3D8)

## Domain Concepts

| Concept | Status |
|---------|--------|
| XML vehicle config schema | RE pending — document in docs/ |
| 7z vehicle archives | RE pending — loader + naming convention |
| extraflags bitmask | RE pending — per-vehicle feature flags |
| Python 3.3 scripting API | RE pending — python33.dll hosting |
| Engine patch sites | RE pending — log per version |

## Rules

### Critical RE Rules
- **Never modify `original_files/`** — these are pristine RE targets. Copy out anything you need to analyze.
- **Cite binary + version for every claim** — e.g., "vehmod_retail.dll 0.98e @ RVA 0x1234". Unattributed addresses are worthless.
- **Log RE findings into `.serena/memories/re-*.md`** — one file per version or per subsystem, plus the docs/ vault for durable notes.
- **Version-diff first** — diff adjacent versions (0.9 → 0.98e) to isolate feature code before deep-diving 0.98e. See `.opencode/skills/mvl-re/SKILL.md`.
- **x86 assumptions** — VC 1.0 is 32-bit; all addresses/RVAs are 32-bit. Watch for MBCS strings and stdcall/cdecl conventions.

### Safety Rules
- **Null Pointer Guards**: check pointers returned by the plugin's own allocators and game functions before use.
- **Buffer Safety**: use strncpy/strncat/snprintf, never strcpy/sprintf — MVL parses untrusted XML/archive data.
- **Memory Safety**: prefer stack allocation; match every heap alloc/free; the plugin hosts Python 3.3 — respect refcounting.
- **CRT**: static link (/MT) — the plugin must not depend on the user's CRT runtime (except python33.dll, which MVL itself ships).

### Token Efficiency Rules
- **AGGRESSIVE COMPRESSION**: Compress early, compress often. Every 5-8 tool calls, evaluate whether older ranges are stale and compress them. Do NOT wait for the max context warning.
- **LSAI First**: Use LSAI for all symbol lookup (saves 90%+ tokens). Never grep/glob for symbol search. Use lsai_source instead of Read for method bodies.
- **Specialist Routing**: Single-file mechanical fix → @fixer (1/2 cost). Symbol lookup → @explorer (2x faster, 1/2 cost). External research → @librarian (2x faster, 1/2 cost). Architecture decisions → @oracle (5x better decisions).
- **Parallel Execution**: Independent tasks → parallel background specialists. Dependent tasks → sequential with dependency tracking. Write conflicts → never parallelize overlapping writes.
- **Minimal Context**: Reference paths/lines instead of pasting files. Concise status updates instead of narrating work.

## Specialist Routing

| Agent | Use for |
|-------|---------|
| @explorer | Symbol lookup, code discovery, call chains |
| @librarian | External research (GTA modding forums, 7z/Python docs) |
| @fixer | Mechanical single-file fixes, harness edits |
| @oracle | Architecture decisions, RE strategy |
| RE agents | Binary analysis lanes (Ghidra/IDA imports, version diffs) — log to `.serena/memories/re-*.md` |

## Knowledge System

### Memory Layers (7 total)

| # | Layer | Location | Purpose |
|---|-------|----------|---------|
| 1 | Conversation | Current session | Working context |
| 2 | Memory Bank | `memory-bank/*.md` (+ `.opencode/memory-bank/` mirror) | Structured project state |
| 3 | Serena | `.serena/memories/*.md` | Curated knowledge notes + RE findings |
| 4 | Sadie | `.sadie/sadie.db` | Queryable SQLite index of ALL layers |
| 5 | Harness-memory | `.harness-memory/memory.sqlite` | Dream-consolidated memories |
| 6 | Obsidian Vault | `docs/` | MVL RE notes (vault) |
| 7 | Harness commands | `.opencode/commands/harness-memory-*.md` | Memory CLI workflows |

### Sadie Knowledge Librarian

```bash
python .sadie/sadie.py init                     # Build/rebuild database from all sources
python .sadie/sadie_vault_sync.py               # Sync docs/ vault into sadie.db
python .sadie/sadie.py search "<term>"          # Search items by title/content/tags
python .sadie/sadie.py query "<sql>"            # Run SQL query
python .sadie/sadie.py stats                    # Show database statistics
python .sadie/sadie.py locations                # List file locations
python .sadie/sadie.py gotchas                  # Show critical items
```

### Harness-Memory Commands

`.opencode/commands/harness-memory-{init,add,dream,extract,list,review,stats,why}.md` — CLI workflows over `.harness-memory/memory.sqlite`.

### Obsidian Vault

- Vault root: `docs/` (stub: `index.md` + `MVL/`)
- MCP: `obsidian-mcp` via `tools/obsidian-mcp-wrapper.ps1` (configured in `.opencode/opencode.json`)
- Sync into sadie: `python .sadie/sadie_vault_sync.py`

### Skills

| Skill | Purpose |
|-------|---------|
| `mvl-re` | RE workflow: version-diff methodology, Ghidra/IDA imports, patch-site logging |
| `token-efficiency` | Context management rules |
| `memory-octree` | Layered memory retrieval protocol |
| `obsidian-skills` | Vault operations |

## Gotchas

- MVL ships its own Python 3.3 runtime — do not assume a system Python; the plugin loads `python33.dll` from its own directory.
- 7z support is bundled in the MVL distribution — identify the bundled 7z runtime (dll vs exe invocation) during RE.
- `.flt` and `.dll` variants of vehmod exist across versions — confirm which format each version uses before diffing.
- VC 1.0 exe addresses differ between retail variants — always record which exe base a patch site was found against.
