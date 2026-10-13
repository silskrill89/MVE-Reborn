// seven_walker.cpp — CSevenWalker implementation (see seven_walker.h).
//
// SDK usage follows external/lzma/C/Util/7z/7zMain.c exactly:
//   InFile_Open -> FileInStream_CreateVTable -> LookToRead2_CreateVTable
//   -> lookStream.buf = ISzAlloc_Alloc(kInputBufSize) -> CrcGenerateTable
//   -> SzArEx_Init -> SzArEx_Open -> SzArEx_Extract (block cache) -> SzArEx_Free.
//
// NOTE on names: the vendored LZMA SDK 25.01 exposes SzArEx_GetFileNameUtf16
// only (7z.h:146); there is no SzArEx_GetFileNameUtf8 in this SDK generation.
// We convert UTF-16-LE -> UTF-8 locally with the same algorithm as 7zMain.c's
// Utf16_To_Utf8 (7zMain.c:106). The original MVL 0.9 binary used an older SDK
// that had the same Utf16 storage (docs/MVL/version-matrix.md).
#include "seven_walker.h"

#include "7zCrc.h" // CrcGenerateTable (7zCrc.h:14) — must be called before SzArEx_Open

#include "../loader/log.h"

#define kWalkerInputBufSize ((size_t)1 << 18) // 7zMain.c:35 kInputBufSize
// BUGFIX(stable-1.0): cap for SzArEx_GetFileNameUtf16's returned length
// (includes the NUL). The header is untrusted; on x86 a len near 2^31 would
// overflow len * sizeof(UInt16) in the scratch alloc below. 64K wchars is
// orders of magnitude beyond any real archive name.
#define kWalkerMaxNameChars ((size_t)1 << 16)

CSevenWalker::CSevenWalker()
	: m_blockIndex(0xFFFFFFFF), m_outBuffer(NULL), m_outBufferSize(0),
	  m_numEntries(0), m_open(false)
{
	File_Construct(&m_archiveStream.file);
	m_archiveStream.wres = 0;

	m_allocImp.Alloc = SzAlloc;       // 7zAlloc.h:11
	m_allocImp.Free = SzFree;         // 7zAlloc.h:12
	m_allocTempImp.Alloc = SzAllocTemp;
	m_allocTempImp.Free = SzFreeTemp;

	m_lookStream.buf = NULL;
	LookToRead2_CreateVTable(&m_lookStream, False); // 7zStream.c:167
}

CSevenWalker::~CSevenWalker()
{
	Close();
}

void
CSevenWalker::Close(void)
{
	if(m_outBuffer){
		SzFree(NULL, m_outBuffer); // matches ISzAlloc_Alloc inside SzArEx_Extract
		m_outBuffer = NULL;
	}
	m_outBufferSize = 0;
	m_blockIndex = 0xFFFFFFFF;

	SzArEx_Free(&m_db, &m_allocImp); // 7zArcIn.c:153
	SzArEx_Init(&m_db);              // re-init so Close/Open can cycle safely

	if(m_lookStream.buf){
		ISzAlloc_Free(&m_allocImp, m_lookStream.buf);
		m_lookStream.buf = NULL;
	}
	File_Close(&m_archiveStream.file); // 7zFile.c
	m_archiveStream.wres = 0;

	m_names.clear();
	m_indices.clear();   // BUGFIX(stable-1.0): index map dies with the names
	m_numEntries = 0;
	m_path.clear();
	m_open = false;
}

// UTF-16-LE -> UTF-8 (7zMain.c:106 Utf16_To_Utf8, MY_UTF8_* macros inlined).
// Returns false only on internal inconsistency; dst is always left valid.
/*static*/ bool
CSevenWalker::Utf16ToUtf8(const UInt16 *src, size_t srcLen, std::string &dst)
{
	dst.clear();
	dst.reserve(srcLen * 3 + 1);

	size_t i = 0;
	while(i < srcLen){
		UInt32 val = src[i++];
		if(val < 0x80){
			dst.push_back((char)val);
			continue;
		}
		if(val < 0x800){
			dst.push_back((char)(0xC0 + (val >> 6)));
			dst.push_back((char)(0x80 + (val & 0x3F)));
			continue;
		}
		// Surrogate pair
		if(val >= 0xD800 && val < 0xDC00 && i < srcLen){
			const UInt32 c2 = src[i];
			if(c2 >= 0xDC00 && c2 < 0xE000){
				i++;
				val = (((val - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
				dst.push_back((char)(0xF0 + (val >> 18)));
				dst.push_back((char)(0x80 + ((val >> 12) & 0x3F)));
				dst.push_back((char)(0x80 + ((val >> 6) & 0x3F)));
				dst.push_back((char)(0x80 + (val & 0x3F)));
				continue;
			}
		}
		// Lone surrogate or 3-byte BMP char — encode as 3-byte UTF-8
		// (lone surrogates become CESU-8-ish bytes; MVL archive names are
		// plain ASCII in practice, this is a defensive path only).
		dst.push_back((char)(0xE0 + (val >> 12)));
		dst.push_back((char)(0x80 + ((val >> 6) & 0x3F)));
		dst.push_back((char)(0x80 + (val & 0x3F)));
	}
	return true;
}

bool
CSevenWalker::BuildNameTable(void)
{
	m_names.clear();
	m_indices.clear();   // BUGFIX(stable-1.0): keep archive-entry indices in sync
	m_numEntries = 0;

	// Scratch buffer for UTF-16 names, grown to the max needed length.
	UInt16 *temp = NULL;
	size_t tempSize = 0;
	bool ok = true;

	for(UInt32 i = 0; i < m_db.NumFiles; i++){
		if(SzArEx_IsDir(&m_db, i)) // 7z.h:131 — skip directory entries
			continue;

		const size_t len = SzArEx_GetFileNameUtf16(&m_db, i, NULL); // 7z.h:146
		// BUGFIX(stable-1.0): untrusted-header guards. len includes the NUL
		// so a valid name is >= 1; len == 0 would underflow (len - 1) below
		// into a SIZE_MAX read, and a huge len would overflow
		// tempSize * sizeof(UInt16) on x86. Reject both.
		if(len == 0 || len > kWalkerMaxNameChars){
			MvlErrorLog("archive: entry %u has invalid UTF-16 name length %u",
			            (unsigned)i, (unsigned)len);
			ok = false;
			break;
		}
		if(len > tempSize){
			SzFree(NULL, temp);
			tempSize = len;
			temp = (UInt16 *)SzAlloc(NULL, tempSize * sizeof(temp[0]));
			if(!temp){
				ok = false;
				break;
			}
		}
		SzArEx_GetFileNameUtf16(&m_db, i, temp);

		std::string name;
		if(!Utf16ToUtf8(temp, len - 1, name)){ // len includes NUL terminator
			ok = false;
			break;
		}
		m_names.push_back(name);
		// BUGFIX(stable-1.0): remember the real archive-entry index. The
		// cached list skips directories, so from the first dir entry on,
		// list index != m_db index; SzArEx_GetFileSize/Extract take the
		// m_db index. Vehicle archives are flat in practice, but untrusted
		// input can carry dirs — without this map we'd extract the wrong
		// entry (or a directory's block).
		m_indices.push_back(i);
	}

	if(temp){
		SzFree(NULL, temp);
		temp = NULL;
	}

	if(ok)
		m_numEntries = (UInt32)m_names.size();
	return ok;
}

bool
CSevenWalker::LoadSeven(const char *path)
{
	Close();

	WRes wres = InFile_Open(&m_archiveStream.file, path); // 7zFile.c
	if(wres != 0){
		MvlErrorLog("archive: cannot open '%s' (WRes %lu)", path, (unsigned long)wres);
		return false;
	}

	FileInStream_CreateVTable(&m_archiveStream);   // 7zFile.c
	m_archiveStream.wres = 0;
	LookToRead2_CreateVTable(&m_lookStream, False);
	m_lookStream.buf = NULL;

	SRes res = SZ_OK;
	m_lookStream.buf = (Byte *)ISzAlloc_Alloc(&m_allocImp, kWalkerInputBufSize);
	if(!m_lookStream.buf){
		res = SZ_ERROR_MEM;
	}else{
		m_lookStream.bufSize = kWalkerInputBufSize;
		m_lookStream.realStream = &m_archiveStream.vt;
		LookToRead2_INIT(&m_lookStream) // 7zTypes.h:423
	}

	CrcGenerateTable(); // 7zCrc.c:308 — required before SzArEx_Open
	SzArEx_Init(&m_db);

	if(res == SZ_OK)
		res = SzArEx_Open(&m_db, &m_lookStream.vt, &m_allocImp, &m_allocTempImp); // 7zArcIn.c:1640

	if(res != SZ_OK){
		MvlErrorLog("archive: SzArEx_Open failed on '%s' (SRes %d)", path, (int)res);
		// Free the look-stream buffer before the generic cleanup
		if(m_lookStream.buf){
			ISzAlloc_Free(&m_allocImp, m_lookStream.buf);
			m_lookStream.buf = NULL;
		}
		SzArEx_Free(&m_db, &m_allocImp);
		SzArEx_Init(&m_db);
		File_Close(&m_archiveStream.file);
		return false;
	}

	if(!BuildNameTable()){
		MvlErrorLog("archive: out of memory building name table for '%s'", path);
		Close();
		return false;
	}

	m_open = true;
	m_path = path;
	MvlLog("archive: opened '%s', %u entries", path, (unsigned)m_numEntries);
	return true;
}

const char *
CSevenWalker::GetEntryName(UInt32 index) const
{
	if(index >= m_numEntries)
		return "";
	return m_names[index].c_str();
}

UInt64
CSevenWalker::GetEntrySize(UInt32 index) const
{
	if(index >= m_numEntries)
		return 0;
	// BUGFIX(stable-1.0): map through m_indices — the cached list skips
	// directories, so a raw list index can name a different archive entry.
	return SzArEx_GetFileSize(&m_db, m_indices[index]); // 7z.h:133
}

bool
CSevenWalker::IsDirectory(UInt32 index) const
{
	if(index >= m_numEntries)
		return true;
	return SzArEx_IsDir(&m_db, m_indices[index]) != 0;   // BUGFIX: index map
}

bool
CSevenWalker::ExtractToMemory(UInt32 index, std::vector<unsigned char> &out)
{
	out.clear();
	if(!m_open || index >= m_numEntries)
		return false;

	size_t offset = 0;
	size_t outSizeProcessed = 0;
	// BUGFIX(stable-1.0): extract by the real archive-entry index (the
	// cached list drops directories, so list index != m_db index).
	const SRes res = SzArEx_Extract(&m_db, &m_lookStream.vt, m_indices[index],
	    &m_blockIndex, &m_outBuffer, &m_outBufferSize,
	    &offset, &outSizeProcessed,
	    &m_allocImp, &m_allocTempImp); // 7zArcIn.c:1650
	if(res != SZ_OK){
		MvlErrorLog("archive: extract of '%s' failed (SRes %d)",
		            GetEntryName(index), (int)res);
		return false;
	}

	// BUGFIX(stable-1.0): the (offset, outSizeProcessed) pair is produced by
	// the SDK from untrusted archive data — clamp the slice to the cache the
	// SDK reports, so a corrupt header cannot turn the assign below into an
	// out-of-bounds read of m_outBuffer.
	if(m_outBuffer == NULL || offset > m_outBufferSize ||
	   outSizeProcessed > m_outBufferSize - offset){
		MvlErrorLog("archive: extract of '%s' returned out-of-range slice "
		            "(offset %u size %u of %u)",
		            GetEntryName(index), (unsigned)offset,
		            (unsigned)outSizeProcessed, (unsigned)m_outBufferSize);
		return false;
	}

	// Copy our slice out of the solid-block cache; the cache itself stays
	// owned by the walker and is freed in Close().
	out.assign(m_outBuffer + offset, m_outBuffer + offset + outSizeProcessed);
	return true;
}
