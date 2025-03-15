#include "tas.h"
#include "menu.h"

#include <cstdio>

#include <hk/diag/diag.h>

#include <nn/fs.h>
#include <sead/heap/seadHeap.h>

namespace cly::tas {

SEAD_SINGLETON_DISPOSER_IMPL(System);

void System::init(sead::Heap* heap) {
	mHeap = heap;
}

void System::loadScript(const sead::SafeString& filename) {
	System* self = instance();

	char filePath[0x200];
	snprintf(filePath, sizeof(filePath), "sd:/Calypso/scripts/%s", filename.cstr());

	nn::Result r;
	nn::fs::FileHandle handle;
	r = nn::fs::OpenFile(&handle, filePath, nn::fs::OpenMode_Read);
	if (r.IsFailure()) {
		Menu::log("ERROR: couldn't load script '%s' (%04d-%04d)", filename.cstr(), r.GetModule() + 2000, r.GetDescription());
		return;
	}

	nn::fs::GetFileSize(&self->mScriptLength, handle);

	self->mScriptData = new (self->mHeap) u8[self->mScriptLength];
	if (!self->mScriptData) {
		Menu::log("ERROR: failed to allocate script memory");
		Menu::log("tried to allocate %d bytes, %d available", self->mScriptLength, self->mHeap->getFreeSize());
		nn::fs::CloseFile(handle);
		return;
	}

	nn::fs::ReadFile(handle, 0, self->mScriptData, self->mScriptLength);
	nn::fs::CloseFile(handle);
	Menu::log("Loaded script: '%s'", filename.cstr());

	if (self->readScriptHeader() != 0) {
		unloadScript();
	}
}

s32 System::readScriptHeader() {
	u32 cursor = 0;

	FileHeader header = *(FileHeader*)&mScriptData[cursor];
	cursor += sizeof(header);

	if (!(header.magic[0] == 'S' && header.magic[1] == 'T' && header.magic[2] == 'A' && header.magic[3] == 'S')) {
		Menu::log("ERROR: script does not use STAS format");
		return 1;
	}

	Menu::log("STAS version: %d (game: %d, editor: %d)", header.formatVersion, header.addonsVersion, header.editorVersion);
	if (header.formatVersion != FORMAT_VERSION || header.addonsVersion != ADDONS_VERSION || header.editorVersion != EDITOR_VERSION) {
		Menu::log("ERROR: script version not supported");
		return 1;
	}

	Menu::log("Title ID: %016lx", header.gameTitleId);
	if (header.gameTitleId != TITLE_ID) {
		Menu::log("ERROR: script was not made for SMO");
		return 1;
	}

	ScriptHeader scriptHeader = *(ScriptHeader*)&mScriptData[cursor];
	cursor += sizeof(scriptHeader);

	mScriptInfo.isLoaded = true;
	mScriptInfo.playerCount = scriptHeader.playerCount;
	mScriptInfo.commandCount = scriptHeader.commandCount;

	for (u8 i = 0; i < scriptHeader.playerCount; i++) {
		mScriptInfo.controllerTypes[i] = mScriptData[cursor++];
	}
	cursor = util::roundUp(cursor, 4);

	u16 authorNameLen = *(u16*)&mScriptData[cursor];
	cursor += 2;
	char* authorName = new (mHeap) char[authorNameLen];
	for (u16 i = 0; i < authorNameLen; i++) {
		authorName[i] = mScriptData[cursor++];
	}
	cursor = util::roundUp(cursor, 4);

	return 0;
}

void System::unloadScript() {
	System* self = instance();
	self->mScriptInfo.clear();
	delete self->mScriptData;
}

void System::ScriptInfo::clear() {
	isLoaded = false;
	playerCount = 0;
	commandCount = 0;
}

} // namespace cly::tas
