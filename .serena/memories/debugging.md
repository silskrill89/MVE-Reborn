# Debugging & Crash Patterns — MVL-clone

## Diagnostic Log

No plugin log path confirmed yet — RE pending. Once identified, record it here:
- Log file: `%MVL_GAME_DIR%\...` (game dir = `H:\games\VC`; filename TBD — update after RE)

## Common Crash Patterns (generic, to be filled with MVL evidence)

### Plugin not loaded
1. DLL not x86 or dynamically linked CRT → rebuild with /MT, Win32
2. Wrong export/entry signature for the VC 1.0 filter loader
3. Missing Python 3.3 DLL dependencies (MVL hosts Python 3.3)

### Crash on startup
1. Engine patch applied to wrong address/version → verify patch site against `.serena/memories/re-*.md` and the exact VC 1.0 exe
2. XML config parse failure → check XML files next to the plugin
3. 7z archive open failure → check archive paths and 7z runtime

### Vehicles missing / not loading
1. XML schema mismatch → compare against RE'd schema in docs/
2. extraflags bitmask misinterpreted → check per-flag RE notes
3. Archive naming convention mismatch

## Workflow

1. Reproduce with the exact VC 1.0 build MVL targets
2. Attach debugger (x86), note faulting address + module
3. Map address to RE findings (cite binary + version)
4. Log new findings into `.serena/memories/re-*.md` and docs/ vault
