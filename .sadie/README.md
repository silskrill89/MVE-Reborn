# .sadie — MVL-clone Knowledge Librarian

> Pure Python + SQLite harness. Zero dependencies. Cross-platform.

## Architecture

```
.serena/memories/  ──[index]──>  .sadie/sadie.db  ──[query]──>  results
   (markdown)                      (SQLite)                     (CLI)

Obsidian Vault     ──[sync]──>   .sadie/sadie.db
(docs/ MVL RE)                    (vault/* items)
```

`.sadie` is a **processing layer** on top of `.serena` and the project Obsidian vault:
- Reads `.serena` markdown files
- Syncs the project vault (`docs/`)
- Indexes everything into a queryable SQLite database
- Tracks access patterns (who/what accessed which item)
- Provides CLI commands for search, query, logging

## Quick Start

```bash
# Initialize the database from all sources
python .sadie/sadie.py init

# Sync the Obsidian vault (docs/)
python .sadie/sadie_vault_sync.py

# Search for items
python .sadie/sadie.py search "vehmod"

# Run SQL queries
python .sadie/sadie.py query "SELECT title, importance FROM items WHERE category='re'"

# Log an access (tracking layer)
python .sadie/sadie.py log <item_id> <accessor> <access_type> [context]

# Show stats
python .sadie/sadie.py stats

# List locations
python .sadie/sadie.py locations

# Show gotchas
python .sadie/sadie.py gotchas

# Export all items as JSON
python .sadie/sadie.py export
```

## Database Schema

| Table | Purpose |
|-------|---------|
| `items` | Indexed knowledge from .serena + vault (source_file, section, category, content, tags, importance) |
| `access_log` | Who accessed what, when, why |
| `processing_log` | How content was processed/indexed |
| `locations` | File path registry (RE targets, build output, vault) |
| `sync_state` | Vault sync hash tracking (idempotent) |

## Obsidian Vault

One vault is synced:

| Vault | Path | Content |
|-------|------|---------|
| MVL | `<project root>/docs/` | MVL reverse engineering notes, XML schema docs, version diffs |

Sync with: `python .sadie/sadie_vault_sync.py`

## Dependencies

**None.** Uses only Python stdlib (sqlite3, pathlib, json, re).
Works on: CPython 3.6+, PyPy, any platform with Python.
