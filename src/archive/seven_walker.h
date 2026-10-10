// seven_walker.h — 7z archive walker for MVL vehicle archives.
//
// Clone of CSevenWalker from the original plugin. The class name is recovered
// from RTTI/error strings: "CSevenWalker" (vehmod_retail.dll 0.98e, named-strings.txt;
// loader entry CSevenWalker::LoadSeven @ RVA 0x2bf00 — see .serena/memories/re-098e-map.md §7).
//
// The original 0.9 build used the static LZMA SDK SzArEx_Open lineage
// (docs/MVL/version-matrix.md); 0.98e keeps the same SzArEx API surface.
// We use the vendored LZMA SDK 25.01 in external/lzma/C (read-only).
//
// Buffer-safety: no strcpy/sprintf anywhere; all SDK calls go through the
// C boundary without exceptions (SRes return codes only).
#pragma once

#include "../core/stdafx.h"

// LZMA SDK is C — headers already carry EXTERN_C guards.
#include "7z.h"
#include "7zAlloc.h"
#include "7zFile.h"

#include <string>
#include <vector>

class CSevenWalker
{
public:
	CSevenWalker();
	~CSevenWalker();

	// CSevenWalker::LoadSeven — vehmod_retail.dll 0.98e @ RVA 0x2bf00.
	// Opens path, runs the SzArEx_Open setup (CFileInStream + CLookToRead2 +
	// ISzAlloc pattern from external/lzma/C/Util/7z/7zMain.c) and caches the
	// entry names as UTF-8. Returns true on success; on failure the walker is
	// left closed.
	bool LoadSeven(const char *path);

	// Release everything: solid-block cache, name table, SzArEx_Free + file.
	void Close(void);

	bool IsOpen(void) const { return m_open; }
	const char *GetPath(void) const { return m_path.c_str(); }

	// Entry enumeration (files only are indexed; directories are skipped in
	// the name table exactly like the original's flat vehicle archives).
	UInt32 GetNumEntries(void) const { return m_numEntries; }
	const char *GetEntryName(UInt32 index) const;   // UTF-8, never NULL
	UInt64 GetEntrySize(UInt32 index) const;        // uncompressed size
	bool IsDirectory(UInt32 index) const;

	// Extract entry 'index' into out (resized to exact file size).
	// Handles both stored (0-compression) and LZMA-compressed entries via
	// SzArEx_Extract; the solid-block cache is kept between calls so a solid
	// archive is decompressed once (7zMain.c cache contract).
	// Returns false on any SZ_ERROR_*; out is left empty then.
	bool ExtractToMemory(UInt32 index, std::vector<unsigned char> &out);

private:
	CSevenWalker(const CSevenWalker &);             // non-copyable (owns C resources)
	CSevenWalker &operator=(const CSevenWalker &);

	bool BuildNameTable(void);
	static bool Utf16ToUtf8(const UInt16 *src, size_t srcLen, std::string &dst);

	// --- LZMA SDK state (7zMain.c pattern) ---
	CFileInStream m_archiveStream;
	CLookToRead2 m_lookStream;
	CSzArEx m_db;
	ISzAlloc m_allocImp;     // { SzAlloc, SzFree } — 7zAlloc.h
	ISzAlloc m_allocTempImp; // { SzAllocTemp, SzFreeTemp }

	// Solid-block extraction cache (SzArEx_Extract contract: *outBuffer must
	// be 0 before first call for each new archive; freed with SzFree).
	UInt32 m_blockIndex;
	Byte *m_outBuffer;
	size_t m_outBufferSize;

	// Cached entry table
	UInt32 m_numEntries;
	std::vector<std::string> m_names;
	std::string m_path;
	bool m_open;
};
