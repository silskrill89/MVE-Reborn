---
name: sadie-search
description: Search the MVL-clone knowledge library (sadie.db) for MVL RE findings, vehicle XML schema, extraflags bitmask, version diffs, engine patches, and project knowledge. Triggers on: search sadie, search vehmod, search mvl, search xml, what do we know about X, find knowledge about X, sadie query, look up in sadie.
---

# sadie-search — Query the MVL Knowledge Library

Search the `.sadie/sadie.db` SQLite database for indexed project knowledge.

## When to use

- User asks "what do we know about X?"
- Looking for RE findings (patch sites, hook addresses, version diffs)
- Finding file locations for RE targets/build output/vault files
- Checking what reference implementations exist for a feature
- Looking up critical bugs or design decisions

## How to search

Run sadie.py commands via bash:

```bash
# Keyword search across all items
python .sadie/sadie.py search "<term>"

# SQL queries for precise filtering
python .sadie/sadie.py query "SELECT title, category, importance FROM items WHERE category='mvl'"

# Find gotchas and critical items
python .sadie/sadie.py gotchas

# List all known file locations
python .sadie/sadie.py locations

# Show database stats
python .sadie/sadie.py stats

# Export all items as JSON
python .sadie/sadie.py export
```

## Key categories

| Category | Content |
|----------|---------|
| `rw` | RenderWare formats, DFF chunks, vertex formats, R* plugins |
| `mvl` | MVL RE findings: vehmod.dll, XML schema, extraflags, engine patches |
| `sa-data` | GTA data formats, coordinates, data statistics |
| `build` | Build system, MSBuild, premake, deploy |
| `debug` | Known bugs, crash analysis, diagnostic procedures |
| `vault/mvl` | docs/ MVL reverse engineering vault |

## SQL examples

```sql
-- Find all MVL RE-related items
SELECT title, content FROM items WHERE category='mvl' AND item_type='knowledge'

-- Find high-importance RW references
SELECT title, source_file FROM items WHERE category='rw' AND importance >= 3

-- Search vault items from the MVL vault
SELECT title, source_file FROM items WHERE source_file LIKE 'vault/mvl:%' LIMIT 20

-- Find items related to version diffs
SELECT title, section FROM items WHERE content LIKE '%0.96%' OR content LIKE '%extraflags%'

-- Recent access history
SELECT i.title, a.access_type, a.timestamp FROM access_log a JOIN items i ON a.item_id = i.id ORDER BY a.timestamp DESC
```

## Logging access

When you use sadie results in your work, log the access:

```bash
python .sadie/sadie.py log <item_id> <your_name> <access_type> "<context>"
```

This tracks which knowledge items are actually used, helping prioritize what to document.
