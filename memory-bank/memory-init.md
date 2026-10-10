# Memory System Initialization — Session Startup

## Quick Start (5 Steps)

### Step 1: Read Active Context (Layer 2)
```python
# Read current state from memory bank
read memory-bank/activeContext.md
```
**Cost**: ~50 tokens | **Gives**: Current state, active issues, next steps

### Step 2: Search Agent Memory (Layer 3)
```python
# Search for relevant patterns from past sessions
agentmemory_memory_smart_search(query="<topic keywords>", limit=5)
```
**Cost**: ~100 tokens | **Gives**: Past solutions, patterns, lessons learned

### Step 3: Query Codebase Graph (Layer 4)
```python
# Find code structure and relationships
codebase-memory-mcp_search_graph(project="<indexed project name>", query="<function/pattern>")
```
**Cost**: ~100 tokens | **Gives**: Function locations, call chains, dependencies

### Step 4: Check Vault (Layer 5)
```python
# Read relevant documentation
obsidian_get_note(target={"type": "path", "path": "<doc path>"}, format="content")
```
**Cost**: ~200 tokens | **Gives**: RE notes, XML schema docs, references

### Step 5: Load Skills
```python
# Load relevant skills for the task
skill(name="mvl-re")            # RE workflow / version-diff methodology
skill(name="token-efficiency")  # Context management
skill(name="<task-specific>")   # e.g., systematic-debugging, memory-octree
```
**Cost**: ~500 tokens | **Gives**: Rules, patterns, verification paths

## Total Startup Cost: ~950 tokens

This is much cheaper than re-exploring the codebase from scratch (5000+ tokens).

## During Work: Memory Layer Usage

| Task | Use Layer | Tool | Why |
|---|---|---|---|
| Find a function | Layer 4 | `search_graph` | 90%+ cheaper than grep |
| Understand call chain | Layer 4 | `trace_path` | Structured, not raw text |
| Past solution | Layer 3 | `smart_search` | Avoid repeating work |
| Current state | Layer 2 | `read activeContext.md` | Always up to date |
| RE notes | Layer 5 | `obsidian_get_note` | Human-readable reference |
| Symbol lookup | LSAI | `lsai_search/source` | Compiler-grade intelligence |

## Session End: Update Memory

### Update Active Context
```python
# Update memory-bank/activeContext.md with current state
edit memory-bank/activeContext.md
```

### Save Important Observations
```python
# Save patterns, solutions, lessons learned
agentmemory_memory_save(
    content="<observation>",
    type="pattern|bug|architecture|workflow",
    concepts="<comma-separated key concepts>",
    files="<relevant file paths>",
    project="<indexed project name>"
)
```

### Update Codebase Graph (if new files)
```python
# Re-index if new files added
codebase-memory-mcp_index_repository(
    repo_path="E:/dev(dave)/projects/Maxos_vehicle_revnres",
    mode="fast"
)
```

## Token Budget Summary

| Phase | Tokens | Purpose |
|---|---|---|
| Startup | ~950 | Read all layers |
| Per query | ~100-300 | Targeted lookups |
| Compression | ~200 | Stale range summary |
| Session end | ~300 | Update layers |
| **Total per session** | **~2000-3000** | vs 10000+ without memory system |
