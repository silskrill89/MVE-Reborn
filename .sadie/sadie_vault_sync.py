#!/usr/bin/env python3
"""sadie_vault_sync.py — bridge the project Obsidian vault into sadie.db.

Vault 1: docs/ (MVL reverse engineering vault, inside this project root)

Walks the vault, upserts every .md note as a sadie item (category 'vault',
item_type 'vault_note') so memory lookups hit SQLite instead of scanning.
Idempotent: re-running updates content and skips unchanged notes.
"""
import hashlib
import sqlite3
import sys
from pathlib import Path

VAULTS = {
    'mvl': Path(__file__).resolve().parent.parent / 'docs',
}
DB = Path(__file__).resolve().parent / 'sadie.db'
EXCLUDE = {'.obsidian', '.trash', '.smart-env', '.sadie', '.serena'}


def note_hash(text: str) -> str:
    return hashlib.sha1(text.encode('utf-8', 'replace')).hexdigest()


def sync_vault(con, vault_name: str, vault_path: Path):
    """Sync a single vault into sadie.db. Returns (added, updated, skipped)."""
    if not vault_path.exists():
        print(f'  [{vault_name}] vault not found: {vault_path}')
        return 0, 0, 0

    added = updated = skipped = 0
    for p in vault_path.rglob('*.md'):
        rel = str(p.relative_to(vault_path)).replace('\\', '/')
        if any(part in EXCLUDE for part in p.parts):
            continue
        try:
            text = p.read_text(encoding='utf-8', errors='replace')
        except OSError:
            continue
        h = note_hash(text)
        source_key = f'vault/{vault_name}:{rel}'
        row = con.execute('SELECT content_hash FROM sync_state WHERE source_file=?',
                          (source_key,)).fetchone()
        if row and row[0] == h:
            skipped += 1
            continue
        title = p.stem
        # frontmatter tags line -> tags column
        tags = f'obsidian,vault,{vault_name}'
        if text.startswith('---'):
            end = text.find('---', 3)
            if end > 0:
                fm = text[3:end]
                for ln in fm.splitlines():
                    if ln.strip().lower().startswith('tags:'):
                        tags = f'obsidian,vault,{vault_name},' + ln.split(':', 1)[1].strip()
        # first heading or first non-empty line as summary prefix
        body = text[end + 3:] if text.startswith('---') and end > 0 else text
        summary = ''
        for ln in body.splitlines():
            s = ln.strip().lstrip('#').strip()
            if s:
                summary = s[:300]
                break
        content = f'[vault/{vault_name}:{rel}] {summary}\n\n{body[:4000]}'
        cat = f'vault/{vault_name}'
        existing = con.execute(
            "SELECT id FROM items WHERE source_file=? AND category=?",
            (source_key, cat)).fetchone()
        if existing:
            con.execute("""UPDATE items SET title=?, content=?, tags=?, category=?,
                last_accessed=CURRENT_TIMESTAMP WHERE id=?""",
                        (title, content, tags, cat, existing[0]))
            con.execute('UPDATE sync_state SET content_hash=? WHERE source_file=?', (h, source_key))
            updated += 1
        else:
            con.execute("""INSERT INTO items (source_file, section, category, item_type,
                title, content, tags, importance) VALUES (?,?,?,?,?,?,?,2)""",
                        (source_key, None, cat, 'vault_note', title, content, tags))
            con.execute('INSERT OR REPLACE INTO sync_state (source_file, content_hash) VALUES (?,?)',
                        (source_key, h))
            added += 1
    return added, updated, skipped


def main():
    con = sqlite3.connect(str(DB))
    con.execute("""CREATE TABLE IF NOT EXISTS sync_state (
        source_file TEXT PRIMARY KEY, content_hash TEXT,
        synced_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP)""")

    total_added = total_updated = total_skipped = 0
    for vault_name, vault_path in VAULTS.items():
        print(f'Syncing vault: {vault_name} ({vault_path})')
        a, u, s = sync_vault(con, vault_name, vault_path)
        print(f'  +{a} added, ~{u} updated, ={s} unchanged')
        total_added += a
        total_updated += u
        total_skipped += s

    con.commit()
    total = con.execute("SELECT COUNT(*) FROM items WHERE category LIKE 'vault%'").fetchone()[0]
    grand = con.execute('SELECT COUNT(*) FROM items').fetchone()[0]
    con.close()
    print(f"vault sync complete: +{total_added} added, ~{total_updated} updated, ={total_skipped} unchanged; "
          f"vault items={total}, total items={grand}")


if __name__ == '__main__':
    main()
