# Session Rules — MVL-clone

## Memory Layers (7 total)
1. Conversation — current session
2. Memory Bank — .opencode/memory-bank/*.md (6 files)
3. Serena — .serena/memories/*.md (curated knowledge)
4. Sadie — .sadie/sadie.db (queryable SQLite index)
5. Harness-memory — .harness-memory/memory.sqlite (dream-consolidated)
6. Obsidian Vault — docs/ (MVL RE notes)
7. Harness-memory commands — .opencode/commands/harness-memory-*.md

## Session Startup
- Run python tools/memory-startup.py --compact if available
- Load compact reference from all memory layers
- Check dream/distill schedule

## Token Efficiency
- LSAI for all symbol lookup (90%+ savings)
- search_graph for code discovery
- Compress stale ranges aggressively
- Route to specialists: @explorer for search, @librarian for docs, @fixer for implementation
