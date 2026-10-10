# Build & Deploy — MVL-clone

## Build Commands

```bash
premake5.cmd    # premake5.lua → build/MVL-clone.sln (VS2022)
msbuild build/MVL-clone.sln /p:Configuration=Release /p:Platform=Win32
```

## Toolchain

- MSBuild: VS2022 at `C:\Program Files\Microsoft Visual Studio\2022\Enterprise\MSBuild\Current\Bin\MSBuild.exe`
- Windows 10 SDK 26100
- Platform: x86 (Win32), static CRT (/MT)
- Output: `bin/Release/vehmod.dll`

## Deploy

- Game dir: env var `MVL_GAME_DIR` = `H:\games\VC` (user-confirmed 2026-10-10; setx-persisted — only NEW processes see it; `b.bat` falls back to `H:\games\VC` if unset)
- Target: `%MVL_GAME_DIR%\mvl\vehmod_retail.dll`
- Host game: GTA Vice City 1.0 (gta-vc.exe)

## Paths

| Path | Purpose |
|------|---------|
| `premake5.lua` / `premake5.cmd` | Build definition (orchestrator-owned) |
| `src/` | Clone implementation (orchestrator-owned) |
| `bin/` | Build output |
| `original_files/` | READ-ONLY RE targets |

## Ghidra MCP (2026-10-10)
- Installed: bethington/ghidra-mcp v7.0.0 (built from source; only fork compatible with Ghidra 12.1.4; upstream LaurieWired v1.4 = 11.3.2 only). Extension jar at C:\Users\aaaaaaaaa\AppData\Roaming\ghidra\ghidra_12.1.4_PUBLIC\Extensions\GhidraMCP\lib\GhidraMCP-7.0.0.jar; tool configs patched via repo's own patch_ghidra_user_configs.
- Build gotchas: Gradle daemon JVM crashed (EXCEPTION_ACCESS_VIOLATION in jvm.dll) → fixed with `--no-daemon` + GRADLE_OPTS="-Dorg.gradle.jvmargs=-Xmx2g -XX:TieredStopAtLevel=1". 32-bit system Python SEGFAULTS on this box → always use E:/dev(dave)/tools/ghidra-mcp/.venv/Scripts/python.exe (64-bit). Paren paths break cmd batch → subst M: or PowerShell quoting.
- Wiring: .opencode/opencode.json `ghidra` stdio entry (uv.exe absolute path → bridge-mcp-ghidra --transport stdio, GHIDRA_MCP_URL=http://127.0.0.1:8089). Bridge smoke test PASSED (initialize handshake OK, v1.30.0; backend-down until Ghidra GUI runs = expected).
- Start procedure: launch Ghidra GUI with a project open (plugin serves HTTP :8089) → MCP tools go live. Repo also ships a Headless Server mode (docs/) — evaluate for analyzeHeadless batch workflow later.
- IDA Free 9.3: NO MCP possible (no IDAPython/plugins in Free — Hex-Rays FAQ; ida-pro-mcp hard-blocks Free). IDA Free = manual GUI cross-check only. IDA Home (paid) would unlock ida-pro-mcp.

## Vendored deps 2026-10-10

- **TinyXML 2.6.2** (classic, last release — matches MVL 0.9 RTTI: TiXmlDocument/TiXmlElement/TiXmlAttribute) → `external/tinyxml/` (tinyxml.cpp/h, tinyxmlparser.cpp, tinyxmlerror.cpp, tinystr.cpp/h). Source: sourceforge `tinyxml_2_6_2.zip`. Compiled into the project, **no TIXML_USE_STL** (plain char* API, MBCS-safe).
- **LZMA SDK 25.01** (7-Zip, www.7-zip.org/a/lzma2501.7z) → `external/lzma/C/` (full original C/ layout kept; only the archive-reader subset is added to the build: 7zAlloc/7zArcIn/7zBuf/7zBuf2/7zCrc/7zCrcOpt/7zDec/7zFile/7zStream/Bcj2/Bra/Bra86/BraIA64/CpuArch/Delta/LzmaDec/Lzma2Dec/Lzma2DecMt/Ppmd7/Ppmd7Dec/Alloc/Precomp). Extracted with `E:\SDKs\III.VC.SA.WindowedMode\data\7za.exe`. Note: SDK 25.01 has no `7zIn.c`/`7zVersion.c` (renamed to `7zArcIn.c`; version is header-only `7zVersion.h`).
- **premake wiring**: `files` globs for `external/tinyxml/**.cpp` + `external/lzma/C/**.c`; includedirs `external/tinyxml`, `external/lzma/C`, `external/lzma/C/Util/7z`. Vendored SDK files get `/wd4100 /wd4189 /wd4245 /wd4244 /wd4267` (warnings not errors — never enable WAE on vendored code).
