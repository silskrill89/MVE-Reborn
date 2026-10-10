# sadie Quick Reference — MVL Knowledge Queries

## Common Development Queries

### "Where is X file?"
```bash
python .sadie/sadie.py query "SELECT name, path, purpose FROM locations WHERE name LIKE '%X%'"
```

### "What do we know about the vehicle XML schema?"
```bash
python .sadie/sadie.py search "xml schema"
python .sadie/sadie.py query "SELECT title, content FROM items WHERE category='mvl' AND content LIKE '%xml%' LIMIT 5"
```

### "How does the extraflags bitmask work?"
```bash
python .sadie/sadie.py search "extraflags"
python .sadie/sadie.py search "bitmask"
```

### "What changed between MVL 0.96 and 0.97?"
```bash
python .sadie/sadie.py search "version diff"
python .sadie/sadie.py query "SELECT title, content FROM items WHERE content LIKE '%0.96%' OR content LIKE '%0.97%' LIMIT 10"
```

### "What do we know about the 7z vehicle archives?"
```bash
python .sadie/sadie.py search "7z"
python .sadie/sadie.py search "vehicle archive"
```

### "What's the build command?"
```bash
python .sadie/sadie.py search "premake5"
python .sadie/sadie.py search "MSBuild"
```

### "What critical bugs exist?"
```bash
python .sadie/sadie.py query "SELECT title, content FROM items WHERE importance >= 4"
python .sadie/sadie.py gotchas
```

### "What's in the Obsidian vaults?"
```bash
python .sadie/sadie.py query "SELECT title, source_file FROM items WHERE category LIKE 'vault%' LIMIT 20"
python .sadie/sadie.py search "renderware"
```

### "Show me RW format references"
```bash
python .sadie/sadie.py query "SELECT title, content FROM items WHERE category='rw' LIMIT 10"
```

### "What SDK locations are known?"
```bash
python .sadie/sadie.py locations
```

## Access Tracking

After using sadie results, log access:
```bash
python .sadie/sadie.py log <item_id> <your_name> <type> "context"
```
