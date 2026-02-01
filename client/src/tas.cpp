#include "tas.h"
#include "menu.h"

#include <cstdio>

#include <hk/diag/diag.h>

#include <nn/fs.h>
#include <sead/controller/seadController.h>
#include <sead/heap/seadHeap.h>

namespace cly::tas {

SEAD_SINGLETON_DISPOSER_IMPL(System);
SEAD_SINGLETON_DISPOSER_IMPL(Pauser);

void System::init(sead::Heap* heap) {
	mHeap = heap;
}

void System::loadScript(const sead::SafeString& filename) {
	System* self = instance();

	if (self->mScriptInfo.isLoaded) {
		unloadScript();
	}

	char filePath[0x200];
	snprintf(filePath, sizeof(filePath), "sd:/Calypso/scripts/%s", filename.cstr());

	nn::fs::FileHandle handle;
	nn::Result r = nn::fs::OpenFile(&handle, filePath, nn::fs::OpenMode_Read);
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
	FileHeader header = *(FileHeader*)&mScriptData[mCursor];
	mCursor += sizeof(header);

	if (!(header.magic[0] == 'S' && header.magic[1] == 'T' && header.magic[2] == 'A' && header.magic[3] == 'S')) {
		Menu::log("ERROR: script does not use STAS format");
		return 1;
	}

	Menu::log("STAS version: %d (game: %d, editor: %d)", header.formatVersion, header.addonsVersion, header.editorVersion);
	if (header.formatVersion != FORMAT_VERSION || header.addonsVersion != ADDONS_VERSION || header.editorVersion != EDITOR_VERSION) {
		Menu::log("ERROR: script version not supported");
		// return 1;
	}

	Menu::log("Title ID: %016lx", header.gameTitleId);
	if (header.gameTitleId != TITLE_ID) {
		Menu::log("ERROR: script was not made for SMO");
		// return 1;
	}

	ScriptHeader scriptHeader = *(ScriptHeader*)&mScriptData[mCursor];
	mCursor += sizeof(scriptHeader);

	mScriptInfo.isLoaded = true;
	mScriptInfo.playerCount = scriptHeader.playerCount;
	mScriptInfo.commandCount = scriptHeader.commandCount;

	for (u8 i = 0; i < scriptHeader.playerCount; i++) {
		mScriptInfo.controllerTypes[i] = read<u8>();
	}
	mCursor = util::roundUp(mCursor, 4);

	u16 authorNameLen = read<u16>();
	char* authorName = new (mHeap) char[authorNameLen];
	for (u16 i = 0; i < authorNameLen; i++) {
		authorName[i] = read<u8>();
	}
	mCursor = util::roundUp(mCursor, 4);

	return 0;
}

bool System::tryReadCommand(CommandType* cmdType) {
	if (mCommandIdx >= mScriptInfo.commandCount) return false;

	CommandType type = read<CommandType>();
	u16 size = read<u16>();

	if (type == CommandType::FRAME) {
		Menu::log("command: FRAME");
		mNextFrameIdx = read<u32>();
	} else if (type == CommandType::CONTROLLER) {
		Menu::log("command: CONTROLLER");

		u8 playerID = read<u8>();
		mCursor += 3;
		u64 buttons = read<u64>();
		s32 leftStickX = read<s32>();
		s32 leftStickY = read<s32>();
		s32 rightStickX = read<s32>();
		s32 rightStickY = read<s32>();

		mCurFrame.padHold = buttons;
		mCurFrame.leftStick = { (f32)leftStickX, (f32)leftStickY };
		mCurFrame.rightStick = { (f32)rightStickX, (f32)rightStickY };
	} else {
		Menu::log("ERROR: unsupported command type %d", type);
		mCursor -= 4;
		return false;
	}

	*cmdType = type;
	mCommandIdx++;

	return true;
}

void System::getNextFrame() {
	System* self = instance();
	if (!self->mScriptData || !self->isReplaying()) {
		Menu::log("ERROR: no script loaded");
		return;
	}

	CommandType cmdType = CommandType::INVALID;
	if (self->mFrameIdx == self->mNextFrameIdx) {
		// read commands until next FRAME type command
		do {
			// if script ends or errors then stop replaying
			if (!self->tryReadCommand(&cmdType)) {
				self->stopReplay();
				return;
			}
		} while (cmdType != CommandType::FRAME);
	}

	Menu::log("%03d: %016lx %016lx", self->mFrameIdx, self->mCurFrame.padHold);

	self->mFrameIdx++;
}

bool System::tryReadCurFrame(InputFrame* out) {
	System* self = instance();

	bool isReplay = self->isReplaying();
	if (isReplay) *out = self->mCurFrame;

	return isReplay;
}

void System::startReplay() {
	System* self = instance();
	if (self->mIsReplaying) return;

	if (!self->mScriptData) {
		Menu::log("ERROR: no script loaded");
		return;
	}
	self->mIsReplaying = true;
	Menu::log("started replaying");

	// read commands up to first FRAME command (metadata)
	CommandType cmdType = CommandType::INVALID;
	do {
		if (!self->tryReadCommand(&cmdType)) {
			self->stopReplay();
			return;
		}
	} while (cmdType != CommandType::FRAME);
}

void System::stopReplay() {
	System* self = instance();
	if (self->mIsReplaying) {
		self->mIsReplaying = false;
		self->unloadScript();
		Menu::log("stopped replaying");
	}
}

bool System::isReplaying() {
	return instance()->mIsReplaying;
}

u32 System::getFrameCount() {
	// TODO: iterate through commands to get frame count
	return 123456789;
}

void System::unloadScript() {
	System* self = instance();
	self->mScriptInfo.clear();
	self->mCursor = 0;
	self->mCommandIdx = 0;
	self->mFrameIdx = 0;
	self->mNextFrameIdx = 0;
	delete self->mScriptData;
	self->mScriptData = nullptr;
}

void System::ScriptInfo::clear() {
	isLoaded = false;
	playerCount = 0;
	commandCount = 0;
}

sead::BitFlag32 convertButtonsSTASToSead(sead::BitFlag64 stasPad) {
	sead::BitFlag32 mask = 0;
	for (s32 i = 0; i < 64; i++) {
		if (stasPad.isOnBit(i)) {
			if (i == cSTAS_A)
				mask.setBit(sead::Controller::cPadIdx_A);
			else if (i == cSTAS_B)
				mask.setBit(sead::Controller::cPadIdx_B);
			else if (i == cSTAS_X)
				mask.setBit(sead::Controller::cPadIdx_X);
			else if (i == cSTAS_Y)
				mask.setBit(sead::Controller::cPadIdx_Y);
			else if (i == cSTAS_LeftStick)
				mask.setBit(sead::Controller::cPadIdx_1);
			else if (i == cSTAS_RightStick)
				mask.setBit(sead::Controller::cPadIdx_2);
			else if (i == cSTAS_L)
				mask.setBit(sead::Controller::cPadIdx_L);
			else if (i == cSTAS_R)
				mask.setBit(sead::Controller::cPadIdx_R);
			else if (i == cSTAS_ZL)
				mask.setBit(sead::Controller::cPadIdx_ZL);
			else if (i == cSTAS_ZR)
				mask.setBit(sead::Controller::cPadIdx_ZR);
			else if (i == cSTAS_Plus) {
				mask.setBit(sead::Controller::cPadIdx_Plus);
				mask.setBit(sead::Controller::cPadIdx_Start);
			} else if (i == cSTAS_Minus)
				mask.setBit(sead::Controller::cPadIdx_Minus);
			else if (i == cSTAS_DLeft)
				mask.setBit(sead::Controller::cPadIdx_Left);
			else if (i == cSTAS_DUp)
				mask.setBit(sead::Controller::cPadIdx_Up);
			else if (i == cSTAS_DRight)
				mask.setBit(sead::Controller::cPadIdx_Right);
			else if (i == cSTAS_DDown)
				mask.setBit(sead::Controller::cPadIdx_Down);
			else if (i == cSTAS_LeftStickLeft)
				mask.setBit(sead::Controller::cPadIdx_LeftStickLeft);
			else if (i == cSTAS_LeftStickUp)
				mask.setBit(sead::Controller::cPadIdx_LeftStickUp);
			else if (i == cSTAS_LeftStickRight)
				mask.setBit(sead::Controller::cPadIdx_LeftStickRight);
			else if (i == cSTAS_LeftStickDown)
				mask.setBit(sead::Controller::cPadIdx_LeftStickDown);
			else if (i == cSTAS_RightStickLeft)
				mask.setBit(sead::Controller::cPadIdx_RightStickLeft);
			else if (i == cSTAS_RightStickUp)
				mask.setBit(sead::Controller::cPadIdx_RightStickUp);
			else if (i == cSTAS_RightStickRight)
				mask.setBit(sead::Controller::cPadIdx_RightStickRight);
			else if (i == cSTAS_RightStickDown)
				mask.setBit(sead::Controller::cPadIdx_RightStickDown);
		}
	}

	return mask;
}

} // namespace cly::tas
