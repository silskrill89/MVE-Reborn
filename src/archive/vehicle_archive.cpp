// vehicle_archive.cpp — CMvlVehicle archive-parsing layer (see vehicle_archive.h).
//
// Semantics per docs/MVL/version-matrix.md: a vehicle archive is a 7z with
// ANY filenames containing one XML + .dff + .txd + .col (+ .ifp, tuning1-9.dff,
// weapons.txd/dff, icon.png etc.). Classification is by extension, with
// special-name overrides handled by RegisterArchiveFile (0.98e @ 0x455d0).
#include "vehicle_archive.h"

#include "../loader/log.h"

#include <ctype.h>

void
SMvlArchiveContent::Clear(void)
{
	xml = SMvlArchiveEntry();            hasXml = false;
	model = SMvlArchiveEntry();          hasModel = false;
	texture = SMvlArchiveEntry();        hasTexture = false;
	collision = SMvlArchiveEntry();      hasCollision = false;
	animation = SMvlArchiveEntry();      hasAnimation = false;
	for(int i = 0; i < 9; i++){
		tuning[i] = SMvlArchiveEntry();
		hasTuning[i] = false;
	}
	weaponsTxd = SMvlArchiveEntry();     hasWeaponsTxd = false;
	weaponsDff = SMvlArchiveEntry();     hasWeaponsDff = false;
	images.clear();
}

CMvlVehicle::CMvlVehicle()
{
}

// Lowercased extension including the dot, or "" when none. Bounded scan; the
// name comes from the archive so it is untrusted input.
static void
GetLowerExtension(const char *name, char *ext, size_t extSize)
{
	ext[0] = '\0';
	if(!name || extSize < 2)
		return;

	const char *dot = strrchr(name, '.');
	if(!dot || dot == name) // no dot, or dotfile with no extension
		return;

	// Copy at most extSize-1 chars; snprintf-style bound.
	size_t i = 0;
	for(const char *p = dot; *p && i < extSize - 1; p++, i++)
		ext[i] = (char)tolower((unsigned char)*p);
	ext[i] = '\0';
}

// Case-insensitive base-name compare for the special names handled by
// RegisterArchiveFile (original matches names case-insensitively on Windows).
static bool
NameEqualsIgnoreCase(const char *a, const char *b)
{
	return _stricmp(a, b) == 0;
}

// RegisterArchiveFile — vehmod_retail.dll 0.98e @ RVA 0x455d0.
// Handles tuning1.dff..tuning9.dff, weapons.txd, weapons.dff by exact name.
/*virtual-ish private*/ bool
CMvlVehicle::RegisterArchiveFile(const char *name, const std::vector<unsigned char> &data,
                                 SMvlArchiveContent &content)
{
	// tuning1-9.dff — the original registers these as extra model parts
	// (0.98e @ 0x455d0; docs/MVL/version-matrix.md).
	if(_strnicmp(name, "tuning", 6) == 0){
		const char *p = name + 6;
		if(p[0] >= '1' && p[0] <= '9' && _stricmp(p + 1, ".dff") == 0){
			const int slot = p[0] - '1';
			content.tuning[slot].name = name;
			content.tuning[slot].data = data;
			content.hasTuning[slot] = true;
			return true;
		}
	}

	if(NameEqualsIgnoreCase(name, "weapons.txd")){
		content.weaponsTxd.name = name;
		content.weaponsTxd.data = data;
		content.hasWeaponsTxd = true;
		return true;
	}
	if(NameEqualsIgnoreCase(name, "weapons.dff")){
		content.weaponsDff.name = name;
		content.weaponsDff.data = data;
		content.hasWeaponsDff = true;
		return true;
	}
	return false;
}

bool
CMvlVehicle::ParseArchive(CSevenWalker &walker, SMvlArchiveContent &content)
{
	content.Clear();
	if(!walker.IsOpen()){
		MvlErrorLog("archive: ParseArchive called with closed walker");
		return false;
	}

	const UInt32 num = walker.GetNumEntries();
	char ext[16];

	for(UInt32 i = 0; i < num; i++){
		const char *name = walker.GetEntryName(i);
		if(walker.IsDirectory(i))
			continue;

		GetLowerExtension(name, ext, sizeof(ext));

		// Extract every classified entry to memory. The walker keeps the
		// solid-block cache, so a solid archive decompresses once.
		std::vector<unsigned char> data;
		if(!walker.ExtractToMemory(i, data))
			return false; // extraction failure is fatal for this archive

		// Special names first (RegisterArchiveFile @ 0x455d0).
		if(RegisterArchiveFile(name, data, content)){
			continue;
		}

		if(strcmp(ext, ".xml") == 0){
			// The original expects exactly one XML per archive; if more than
			// one is present the first wins and we log the anomaly.
			if(content.hasXml)
				MvlLog("archive: multiple .xml entries, keeping '%s', ignoring '%s'",
				       content.xml.name.c_str(), name);
			else{
				content.xml.name = name;
				content.xml.data = data;
				content.hasXml = true;
			}
		}
		else if(strcmp(ext, ".dff") == 0){
			if(content.hasModel)
				MvlLog("archive: multiple .dff entries, keeping '%s', ignoring '%s'",
				       content.model.name.c_str(), name);
			else{
				content.model.name = name;
				content.model.data = data;
				content.hasModel = true;
			}
		}
		else if(strcmp(ext, ".txd") == 0){
			if(content.hasTexture)
				MvlLog("archive: multiple .txd entries, keeping '%s', ignoring '%s'",
				       content.texture.name.c_str(), name);
			else{
				content.texture.name = name;
				content.texture.data = data;
				content.hasTexture = true;
			}
		}
		else if(strcmp(ext, ".col") == 0){
			content.collision.name = name;
			content.collision.data = data;
			content.hasCollision = true;
		}
		else if(strcmp(ext, ".ifp") == 0){
			content.animation.name = name;
			content.animation.data = data;
			content.hasAnimation = true;
		}
		else if(strcmp(ext, ".png") == 0){
			SMvlArchiveEntry img;
			img.name = name;
			img.data = data;
			content.images.push_back(img);
		}
		else{
			// Unknown extension — original archives can carry extras
			// (readme.txt etc.); keep them out of the typed slots.
			MvlLog("archive: unclassified entry '%s' (%u bytes)",
			       name, (unsigned)data.size());
		}
	}

	// Summary log in the original's style (message-log lineage).
	MvlLog("archive: %s, %u entries, xml=%s, dff=%d, txd=%d, col=%d, ifp=%d, png=%d, tuning=%d, weapons=%d",
	       walker.GetPath(), (unsigned)num,
	       content.hasXml ? content.xml.name.c_str() : "(none)",
	       content.hasModel ? 1 : 0,
	       content.hasTexture ? 1 : 0,
	       content.hasCollision ? 1 : 0,
	       content.hasAnimation ? 1 : 0,
	       (int)content.images.size(),
	       (int)((content.hasTuning[0] ? 1 : 0) + (content.hasTuning[1] ? 1 : 0) +
	             (content.hasTuning[2] ? 1 : 0) + (content.hasTuning[3] ? 1 : 0) +
	             (content.hasTuning[4] ? 1 : 0) + (content.hasTuning[5] ? 1 : 0) +
	             (content.hasTuning[6] ? 1 : 0) + (content.hasTuning[7] ? 1 : 0) +
	             (content.hasTuning[8] ? 1 : 0)),
	       (content.hasWeaponsTxd ? 1 : 0) + (content.hasWeaponsDff ? 1 : 0));

	// TODO(hook) LoadTextureFile   @ 0x45a5a — content.texture -> RW texture stage
	// TODO(hook) LoadModelFile     @ 0x45ccb — content.model -> RW DFF stage
	// TODO(hook) LoadCollisionFile @ 0x45ed7 — content.collision -> COL stage
	// TODO(hook) LoadAnimationFile @ 0x45f50 — content.animation -> CCustomAnimFile
	// TODO(hook) LoadWeaponFiles   @ 0x462fe — weapons.txd/dff -> weapon stage

	return true;
}

// VerifyRequiredFiles — vehmod_retail.dll 0.98e @ RVA 0x45870.
// XML is mandatory; everything else is optional and only logged.
bool
CMvlVehicle::VerifyRequiredFiles(const SMvlArchiveContent &content, const char *archiveName)
{
	if(!content.hasXml){
		MvlErrorLog("archive: '%s' has no vehicle XML — archive rejected "
		            "(VerifyRequiredFiles)", archiveName ? archiveName : "(null)");
		return false;
	}

	// Optional parts: log which are present, matching the original's
	// "%s sector is missing…" diagnostic style (re-098e-map.md §1).
	if(!content.hasModel)
		MvlLog("archive: '%s' has no .dff model", archiveName ? archiveName : "(null)");
	if(!content.hasTexture)
		MvlLog("archive: '%s' has no .txd texture", archiveName ? archiveName : "(null)");
	if(!content.hasCollision)
		MvlLog("archive: '%s' has no .col collision", archiveName ? archiveName : "(null)");
	if(!content.hasAnimation)
		MvlLog("archive: '%s' has no .ifp animation", archiveName ? archiveName : "(null)");

	return true;
}
