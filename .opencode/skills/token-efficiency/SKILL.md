---
name: token-efficiency
description: Context management and token efficiency rules for MVL-clone. Aggressive compression, LSAI-first lookups, specialist routing, and parallel execution patterns.
triggers:
  - Every session start (apply these rules proactively)
  - Context approaching limits
  - Multi-step work requiring planning
---

# Token Efficiency Rules — MVL-clone

## 1. Aggressive Compression

This project hits context limits extremely fast (RE targets in original_files/, clone in src/).

### When to Compress
- Every **5-8 tool calls**, evaluate whether older ranges are stale
- **Immediately** when user asks to compress
- When a research phase concludes and findings are clear
- When implementation finishes and verification passes
- When exploration is exhausted and patterns are understood

### What to Compress
- Raw exploration output → distilled findings only
- Verbose tool results → key data points
- Failed attempts → lessons learned (not the failures themselves)
- File reads of unchanged files → the relevant excerpt

### What NOT to Compress
- Active code being edited (need exact line numbers)
- Current error messages (may need for next fix)
- User instructions (preserve intent exactly)
- Exact file:line references for pending work

### Compression Format
```
## Topic
- Key finding 1
- Key finding 2
- Decision made
- File:line references
- Next steps
```

## 2. LSAI First (90%+ Token Savings)

Use LSAI for ALL symbol lookups. Never use grep/glob for symbol search.

| Task | Wrong Way | Right Way | Savings |
|---|---|---|---|
| Find a function | `grep -rn "functionName"` | `lsai_search("functionName")` | 90%+ |
| Read a method body | `read entire file` | `lsai_source("methodName")` | 80%+ |
| See class members | `read entire file` | `lsai_outline(typeName="ClassName")` | 90%+ |
| Find references | `grep -rn "symbol"` | `lsai_usages(symbolName="symbol")` | 90%+ |
| Check who calls | `grep -rn "methodName"` | `lsai_callers(methodName="methodName")` | 85%+ |
| Assess change risk | Manual analysis | `lsai_impact(symbolName="symbol")` | 95%+ |

### When to Use Grep Instead
- Macro uses (`#define`, `#ifdef`) — LSAI doesn't index macros
- Non-code files (CMake, JSON, YAML)
- Pattern matching across multiple file types
- Finding TODO/FIXME/HACK comments

## 3. Specialist Routing (Cost Optimization)

| Task Type | Specialist | Cost vs Orchestrator | When |
|---|---|---|---|
| Single-file mechanical fix | @fixer | 1/2 cost | Bounded, clear changes |
| Symbol lookup / codebase search | @explorer | 2x faster, 1/2 cost | Discovery, pattern finding |
| External library docs / web research | @librarian | 2x faster, 1/2 cost | API docs, library internals |
| Architecture decision / code review | @oracle | 5x better decisions | High-stakes, complex tradeoffs |
| UI/UX design and polish | @designer | 10x better design | Visual, responsive, interaction |

### Routing Rules
- **Never** do multi-file implementation in orchestrator — dispatch to @fixer
- **Never** search the codebase manually when LSAI or @explorer exists
- **Never** research library APIs when @librarian is available
- **Always** route architecture decisions to @oracle for quality
- **Always** route UI/visual work to @designer

## 4. Parallel Execution

### Independent Tasks → Parallel Background
```python
# WRONG: Sequential
result1 = task(explorer, "find X")
result2 = task(explorer, "find Y")  # Could run in parallel

# RIGHT: Parallel
task(explorer, "find X", background=True)
task(explorer, "find Y", background=True)
# Reconcile later
```

### Dependent Tasks → Sequential
```python
# Must be sequential — result2 depends on result1
result1 = task(fixer, "fix X")
result2 = task(fixer, "fix Y based on X")  # Needs result1
```

### Write Conflicts → Never Parallelize
- Two @fixer tasks editing the same file = conflict
- Split by file, not by feature, when parallelizing writes

## 5. Session Reuse

### When to Reuse
- Same specialist type needed again
- Context from previous session is still relevant
- Work is additive (continuing same task)

### When Fresh Session
- Too much unrelated context in previous session
- Different subdomain (e.g., RE notes → build system)
- Previous session was complex and results are reconciled

### How to Reuse
- Always pass `task_id` when resuming a session
- Check Background Job Board for available sessions
- Prefer most recently used matching session

## 6. Minimal Context Principle

### File References
- **Wrong**: Paste entire 1000-line file
- **Right**: `src/render/vehiclePipe.cpp:193` (exact line)

### Delegation Notices
- **Wrong**: "I'm going to delegate to @fixer because the task involves..."
- **Right**: "Dispatching to @fixer for the normalization fixes."

### Status Updates
- **Wrong**: Narrate every tool call and result
- **Right**: "Fixed 3 division guards in postfx.cpp. Building now."

## 7. Python-Triggered Workflows (Time + Token Savings)

Python scripts can handle many tasks faster than agent analysis. Use them to save tokens and time.

### Build & Deploy
| Task | Python Script | Token Savings |
|---|---|---|
| Full build | `python tools/fast_build.py` | 100% (no agent needed) |
| Kill/restart game | `python tools/proto.py restart` | 100% |
| Tail debug log | `python tools/proto.py log` | 100% |

### Asset Analysis
| Task | Python Script | Token Savings |
|---|---|---|
| Parse DFF binary | `python tools/dff_analyzer.py file.dff` | 95% (vs manual hex analysis) |
| Deep RW plugin scan | `python tools/deep_rw_scan.py file.dff` | 95% |
| Wheel audit | `python tools/wheel_audit.py` | 95% |
| Wheel pipeline | `python tools/wheel_pipeline.py` | 98% |
| Generate normal maps | `python tools/gta_normalmap_gen.py` | 100% |
| Generate envmaps | `python tools/generate_envmaps.py` | 100% |

### Packaging & Distribution
| Task | Python Script | Token Savings |
|---|---|---|
| Create release | `python tools/release_packager.py` | 100% |
| List releases | `python tools/release_packager.py --list` | 100% |
| Run installer | `python installer.py` | 100% |

### When to Use Python vs Agent
- **Use Python**: Build, deploy, asset analysis, packaging, file conversion, data extraction
- **Use Agent**: Code modification, architecture decisions, debugging logic, UI/UX design
- **Use LSAI**: Symbol lookup, call graph, impact analysis, code navigation

### Key Rule
If a Python script exists for the task, **run the script** instead of analyzing manually. This saves both tokens and wall-clock time.

## 8. Verification Efficiency

### Proportional Verification
| Change Scope | Verification |
|---|---|
| Single-line fix | Build only |
| Multi-line, single file | Build + targeted test |
| Multi-file, same feature | Build + runtime test |
| Architecture change | Build + full regression |
| DLL change | Build + in-game vehicle load test |

### Never
- Run project-wide tests for a 1-line change
- Rebuild everything when only RE notes changed
- Launch game for a comment-only change

## 9. Memory Layer Routing (5-Layer System)

### Query Routing Rules

| Question | Best Layer | Tool | Token Cost |
|---|---|---|---|
| "What is the current state?" | Layer 2 (Memory Bank) | `read memory-bank/activeContext.md` | ~50 tokens |
| "How did we fix this before?" | Layer 3 (Agent Memory) | `agentmemory_memory_smart_search` | ~100 tokens |
| "Where is this function?" | Layer 4 (Codebase Graph) | `codebase-memory-mcp_search_graph` | ~100 tokens |
| "Who calls this function?" | Layer 4 (Codebase Graph) | `codebase-memory-mcp_trace_path` | ~150 tokens |
| "What does the docs say?" | Layer 5 (Obsidian Vault) | `obsidian_get_note` with section | ~200 tokens |
| "What's the architecture?" | Layer 4 (Codebase Graph) | `codebase-memory-mcp_get_architecture` | ~300 tokens |

### Anti-Patterns (Waste Tokens)

| Wrong | Right | Savings |
|---|---|---|
| grep for function location | `search_graph(query="functionName")` | 90%+ |
| Read entire file to find one function | `get_code_snippet(qualified_name="...")` | 80%+ |
| Search agent memory with export | `smart_search(query="...", limit=5)` | 95%+ |
| Read full vault doc | `obsidian_get_note(section={"type":"heading", "target":"..."})` | 70%+ |

### Session Lifecycle

```
START:
  1. Read memory-bank/activeContext.md (Layer 2) — 50 tokens
  2. Search agent memory for relevant patterns (Layer 3) — 100 tokens
  3. Query codebase graph for code structure (Layer 4) — 100 tokens

DURING WORK:
  4. Use LSAI for symbol lookups (90%+ savings)
  5. Use codebase graph for call chains (90%+ savings)
  6. Use agent memory for past solutions (95%+ savings)
  7. Compress every 5-8 tool calls

END:
  8. Update memory-bank/activeContext.md (Layer 2)
  9. Save important observations to agent memory (Layer 3)
  10. Update codebase graph if new files (Layer 4)
```

## 10. Context Monitoring

### Signals to Compress Now
- Tool output truncation warnings
- Reading files you've already read
- Re-explaining context to specialists
- More than 3 compressed blocks visible

### Compression Budget
- Target: Stay under 80% context window
- Emergency: Compress everything except current task at 90%
- Prevention: Compress proactively at 60%
