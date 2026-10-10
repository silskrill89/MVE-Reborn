---
name: memory-octree
description: Hierarchical memory orchestration across all 8 project memory layers. Uses octree-style traversal (broad→specific, fast→slow) for efficient knowledge retrieval. Invoke at session start, during debugging, or when context is uncertain.
triggers:
  - Session start (always)
  - Debugging any issue
  - "What do we know about X?"
  - Context uncertainty (lost, confused, stale)
  - Before major code changes
  - After git pull / merge
---

# Memory Octree — Hierarchical Knowledge Retrieval

## Concept

An octree subdivides 3D space into 8 octants for efficient spatial queries.
This skill subdivides **knowledge space** into 8 layers ranked by access speed,
then traverses broad→specific like an octree traversal: check the fast, cheap
layers first; only descend into slow, expensive layers when the fast ones lack
the answer.

```
                    ┌─────────────────────┐
                    │   ROOT: Session      │  ← conversation context (free)
                    │   (active context)   │
                    └─────────┬───────────┘
                              │
          ┌───────────────────┼───────────────────┐
          │                   │                   │
    ┌─────┴─────┐      ┌─────┴─────┐      ┌─────┴─────┐
    │  TIER 1   │      │  TIER 2   │      │  TIER 3   │
    │  <100 tok │      │  ~200 tok │      │  ~500 tok │
    │  instant  │      │  fast     │      │  moderate │
    └─────┬─────┘      └─────┬─────┘      └─────┬─────┘
          │                   │                   │
    ┌─────┴─────┐      ┌─────┴─────┐      ┌─────┴─────┐
    │ L2: MemBank│     │ L4: Sadie │      │ L6: Git   │
    │ L3: Serena │     │ L5: Harness│     │ L7: Obsidian│
    └───────────┘      └───────────┘      │ L8: Obsidian│
                                          └───────────┘
```

## The 8 Layers

| # | Layer | Location | Access Speed | Token Cost | Query Method |
|---|-------|----------|-------------|------------|--------------|
| 1 | Conversation | Current session | Instant | 0 | Already in context |
| 2 | Memory Bank | `.opencode/memory-bank/*.md` | ~50ms | ~100 | `Read()` |
| 3 | Serena | `.serena/memories/*.md` | ~50ms | ~100 | `Read()` |
| 4 | Sadie | `.sadie/sadie.db` | ~200ms | ~50 | `python .sadie/sadie.py search` |
| 5 | Harness-memory | `.harness-memory/memory.sqlite` | ~200ms | ~50 | `python tools/inspect_db.py` |
| 6 | Git | Repository history | ~500ms | ~200 | `git log/diff/show` |
| 7 | Obsidian RE | RE Vault (MCP) | ~1s | ~300 | `obsidian-re-vault_*` |
| 8 | Obsidian MVL | docs/ vault (MCP) | ~1s | ~300 | `obsidian-mcp_*` |

## Octree Traversal Protocol

### Phase 1: TIER 1 — Instant Context (always check first)

**Cost: ~200 tokens total | Speed: instant**

```
1. Read conversation context (Layer 1) — already in memory
2. Read memory-bank/activeContext.md (Layer 2) — current focus, blockers
3. Read .serena/memories/project.md (Layer 3) — project overview
```

**Decision gate:** If the answer is found → STOP. If not → descend to Tier 2.

### Phase 2: TIER 2 — Fast Lookup (check if Tier 1 insufficient)

**Cost: ~200 tokens total | Speed: fast**

```
4. Read memory-bank/progress.md (Layer 2) — what's done, what's pending
5. Read memory-bank/systemPatterns.md (Layer 2) — architecture patterns
6. Read .serena/memories/debugging.md (Layer 3) — crash patterns
7. Read .serena/memories/build-process.md (Layer 3) — build/deploy details
8. Read .serena/memories/project.md (Layer 3) — RE targets
9. python .sadie/sadie.py search "<topic>" (Layer 4) — 371 indexed items
```

**Decision gate:** If the answer is found → STOP. If not → descend to Tier 3.

### Phase 3: TIER 3 — Deep Search (only when Tier 2 insufficient)

**Cost: ~500-1000 tokens | Speed: moderate**

```
10. python tools/inspect_db.py .harness-memory/memory.sqlite (Layer 5)
    — dream-consolidated architectural constraints
11. git log --oneline -20 (Layer 6) — recent commit history
12. git diff HEAD~5 --stat (Layer 6) — recent file changes
13. git show <commit> -- <file> (Layer 6) — specific change
```

**Decision gate:** If the answer is found → STOP. If not → descend to Tier 4.

### Phase 4: TIER 4 — Vault Search (expensive, only when needed)

**Cost: ~500-1000 tokens | Speed: slow**

```
14. obsidian-mcp_obsidian_search_notes (Layer 7)
    — MVL RE notes (binary findings, XML schema, version diffs)
15. sadie.py search "vault/mvl" (Layer 8)
    — MVL-specific vault notes
```

**Decision gate:** If the answer is found → STOP. If not → the knowledge may not exist in any layer.

## Octree Subdivisions Within Each Layer

Like an octree subdivides each octant into 8 children, each memory layer has
internal subdivisions that can be queried at increasing depth.

### Layer 2: Memory Bank Subdivisions

```
memory-bank/
├── activeContext.md    ← DEPTH 1: always read first (current state)
├── progress.md         ← DEPTH 1: what's done/pending
├── systemPatterns.md   ← DEPTH 2: architecture patterns
├── techContext.md      ← DEPTH 2: tech stack details
├── productContext.md   ← DEPTH 3: product goals/scope
└── history.md          ← DEPTH 3: session-by-session history
```

### Layer 3: Serena Subdivisions

```
.serena/memories/
├── project.md          ← DEPTH 1: project overview
├── build-process.md    ← DEPTH 1: build commands
├── agent-skills.md    ← DEPTH 2: routing evidence
├── session-rules.md    ← DEPTH 2: session rules
├── debugging.md        ← DEPTH 2: crash patterns
└── vault-refs.md       ← DEPTH 3: vault locations
```

### Layer 4: Sadie Subdivisions (by category)

```
sadie.db categories:
├── vault/mvl (?n)     ← DEPTH 1: MVL RE vault notes
├── mvl (n)            ← DEPTH 2: MVL RE findings
├── rw (n)             ← DEPTH 2: RW data structures
├── build (8)           ← DEPTH 1: build-related
├── rw (8)              ← DEPTH 2: RenderWare docs
├── debug (2)           ← DEPTH 1: debugging
└── general (13)        ← DEPTH 3: misc
```

### Layer 6: Git Subdivisions (by time)

```
git history:
├── HEAD~3 (last 3 commits)  ← DEPTH 1: most recent changes
├── HEAD~10 (last 10)        ← DEPTH 2: recent session work
├── HEAD~30 (last 30)        ← DEPTH 3: full project history
└── --since="<date>"         ← DEPTH 2: date-range queries
```

## Quantification Metrics

### Layer Relevance Score (0-1)

Each layer gets a relevance score based on query type:

| Query Type | L1 | L2 | L3 | L4 | L5 | L6 | L7 | L8 |
|-----------|-----|-----|-----|-----|-----|-----|------|------|
| "What am I doing?" | 1.0 | 0.8 | 0.3 | 0.1 | 0.0 | 0.0 | 0.0 | 0.0 |
| "How to build?" | 0.5 | 1.0 | 0.9 | 0.3 | 0.0 | 0.0 | 0.0 | 0.0 |
| "Why did X crash?" | 0.3 | 0.5 | 0.8 | 0.6 | 0.4 | 0.7 | 0.3 | 0.2 |
| "What's the register layout?" | 0.0 | 0.3 | 0.9 | 0.5 | 0.0 | 0.0 | 0.2 | 0.8 |
| "What changed recently?" | 0.2 | 0.4 | 0.2 | 0.1 | 0.0 | 1.0 | 0.0 | 0.0 |
| "What does RW format look like?" | 0.0 | 0.0 | 0.1 | 0.5 | 0.0 | 0.0 | 0.9 | 0.3 |
| "Architecture constraint?" | 0.1 | 0.3 | 0.3 | 0.2 | 1.0 | 0.0 | 0.0 | 0.0 |
| "MVL RE finding?" | 0.0 | 0.5 | 0.9 | 0.4 | 0.0 | 0.0 | 0.1 | 0.9 |

### Cost-Benefit Formula

```
priority = relevance_score / (token_cost * access_time_ms)
```

Always query the highest priority layer first. Skip layers with relevance < 0.2.

### Octree Depth Budget

Total token budget per query cycle: **~2000 tokens**

| Depth | Layers Queried | Cumulative Cost | When to Stop |
|-------|---------------|-----------------|--------------|
| 1 | L1 + L2(active) + L3(project) | ~200 | Answer found |
| 2 | + L2(all) + L3(all) + L4(category) | ~500 | Answer found |
| 3 | + L5 + L6(recent) | ~800 | Answer found |
| 4 | + L6(deep) + L7 + L8 | ~2000 | Exhausted |

## Callable Protocols

### Protocol 1: Session Start (always run)

```markdown
/memory-octree init

1. Read activeContext.md → know current focus
2. Read progress.md → know what's done/pending
3. Read project.md → know project overview
4. Check git log --oneline -5 → know recent changes
5. Verify memory bank is in sync with git (compare dates)
```

**Cost: ~300 tokens | Time: ~200ms**

### Protocol 2: Debug Query (when investigating)

```markdown
/memory-octree debug "<issue>"

1. Check conversation context for clues
2. Read debugging.md → crash patterns
3. Search Sadie for "<issue>" → indexed knowledge
4. Check harness-memory for architectural constraints
5. git log --all --grep="<keyword>" → related commits
6. If still stuck → search Obsidian vaults
```

**Cost: ~500-1500 tokens | Time: ~1-3s**

### Protocol 3: Architecture Query (when designing)

```markdown
/memory-octree arch "<topic>"

1. Read systemPatterns.md → known patterns
2. Read build-process.md + project.md → build/RE details
3. Search Sadie category="vault/mvl" → MVL vault notes
4. Check harness-memory for constraints
5. If needed → search Obsidian MVL vault
```

**Cost: ~500-1000 tokens | Time: ~1-2s**

### Protocol 4: History Query (when tracing changes)

```markdown
/memory-octree history "<topic>"

1. Read history.md → session summaries
2. git log --oneline --all --grep="<topic>"
3. git log --oneline --since="<date>" -- <file>
4. git show <commit> -- <file>
5. If needed → search Obsidian RE vault for background
```

**Cost: ~300-800 tokens | Time: ~1-2s**

### Protocol 5: Sync Check (verify all layers are current)

```markdown
/memory-octree sync

1. Compare memory-bank dates with git log dates
2. Run python .sadie/sadie.py stats → check item counts
3. Check harness-memory row counts
4. Verify Obsidian MCP connections (list_notes)
5. Report any stale layers with recommended actions
```

**Cost: ~500 tokens | Time: ~2s**

## Integration with Existing Skills

| Situation | Load memory-octree THEN... |
|-----------|---------------------------|
| Session start | Run `init` protocol, then load mvl-re |
| Debugging crash | Run `debug` protocol, then load systematic-debugging |
| Architecture decision | Run `arch` protocol, then load mvl-re |
| RE work | Run `arch` protocol, then load mvl-re |
| Build issues | Run `init` protocol, then load systematic-debugging |
| After git pull | Run `sync` protocol to detect stale layers |

## Stale Layer Detection

A layer is **stale** if:
- Memory bank files older than latest git commit
- Sadie sync_state dates older than latest vault change
- Harness-memory has no dream runs in last 7 days
- Serena files don't reflect recent commits

**Auto-repair actions:**
- Memory bank stale → update files from git diff
- Sadie stale → run `python .sadie/sadie_vault_sync.py`
- Harness-memory stale → run dream consolidation
- Serena stale → update from conversation context

## Quick Reference Card

```
┌─────────────────────────────────────────────────┐
│           MEMORY OCTREE QUICK REFERENCE          │
├─────────────────────────────────────────────────┤
│ TIER 1 (instant, ~200 tok):                     │
│   L1: conversation  L2: activeContext/progress   │
│   L3: project.md                                │
├─────────────────────────────────────────────────┤
│ TIER 2 (fast, ~300 tok):                        │
│   L2: all memory-bank files                     │
│   L3: all serena memories                       │
│   L4: sadie.py search "<topic>"                 │
├─────────────────────────────────────────────────┤
│ TIER 3 (moderate, ~300 tok):                    │
│   L5: harness-memory constraints                │
│   L6: git log/diff/show                         │
├─────────────────────────────────────────────────┤
│ TIER 4 (expensive, ~1000 tok):                  │
│   L7: obsidian-re-vault search                  │
│   L8: obsidian-mcp search               │
├─────────────────────────────────────────────────┤
│ RULE: Always start at Tier 1. Only descend      │
│ when current tier lacks the answer.              │
│ BUDGET: ~2000 tokens per query cycle.            │
└─────────────────────────────────────────────────┘
```

## Companion Script

`tools/memory_octree.py` — Python implementation of the octree traversal.

```bash
python tools/memory_octree.py init              # Session startup (Tier 1)
python tools/memory_octree.py search "<query>"  # Full octree search
python tools/memory_octree.py debug "<issue>"   # Debug-specific search
python tools/memory_octree.py arch "<topic>"    # Architecture search
python tools/memory_octree.py history "<topic>" # History search
python tools/memory_octree.py sync              # Check all layers for staleness
python tools/memory_octree.py status            # Layer health overview
```

The script auto-classifies queries (crash/build/RE/architecture/etc.) and
applies relevance-weighted layer selection. Use it for automated memory retrieval
or to verify layer health before starting work.
