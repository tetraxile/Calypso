#include "ScriptSTAS.h"

#include <hk/ValueOrResult.h>

#include "results.h"

ScriptSTAS::ScriptSTAS(QFile& file, LogWidget& logWidget) : mFile(file), mReader(file, file.size()), mLogWidget(logWidget) {
	mInfo.name.assign(file.filesystemFileName().filename().u16string());
}

void ScriptSTAS::close() {
	if (mFile.isOpen()) mFile.close();
}

hk::Result ScriptSTAS::readHeader() {
	HK_ASSERT(!mHeader.isValid);

	HK_TRY(mReader.checkSignature("STAS"));

	mHeader.formatVersion = HK_TRY(mReader.readU16());
	mHeader.gameVersion = HK_TRY(mReader.readU16());
	mHeader.editorVersion = HK_TRY(mReader.readU16());
	mReader.alignUp(sizeof(u32));
	mHeader.titleID = HK_TRY(mReader.readU64());
	if (mHeader.titleID != SMO_TITLE_ID) {
		mLogWidget.log("script's title ID must match SMO's! (got: %016lx)", mHeader.titleID);
		return hk::ResultFailed();
	}

	if (mHeader.formatVersion != 0) {
		mLogWidget.log("only STAS version 0 supported! (got: %d)", mHeader.formatVersion);
		return hk::ResultFailed();
	}

	if (mHeader.gameVersion != 0) {
		mLogWidget.log("only STAS SMO version 0 supported! (got: %d)", mHeader.gameVersion);
		return hk::ResultFailed();
	}

	mHeader.commandCount = HK_TRY(mReader.readU32());
	mHeader.secondsEdited = HK_TRY(mReader.readU32());
	mHeader.playerCount = HK_TRY(mReader.readU8());
	if (mHeader.playerCount != 1 && mHeader.playerCount != 2) {
		mLogWidget.log("game only supports 1 or 2 players! (got: %d)", mHeader.playerCount);
		return hk::ResultFailed();
	}

	mReader.alignUp(sizeof(u32));
	for (s32 i = 0; i < 4; i++)
		mHeader.controllerTypes[i] = ControllerType(HK_TRY(mReader.readU8()));
	u16 authorNameLength = HK_TRY(mReader.readU16());
	mInfo.author = HK_TRY(mReader.readString(authorNameLength));
	mReader.alignUp(4);

	mHeader.isValid = true;
	mHeader.endOffset = mReader.position();

	return hk::ResultSuccess();
}

hk::Result ScriptSTAS::readMetadata() {
	CommandType cmdType = cCommandType_Invalid;
	do {
		HK_TRY(tryReadCommand(cmdType));
	} while (cmdType != cCommandType_Frame);

	return hk::ResultSuccess();
}

hk::ValueOrResult<ScriptSTAS::Packet&> ScriptSTAS::getNextFrame() {
	if (mFrameIdx == mNextFrameIdx) {
		CommandType cmdType = cCommandType_Invalid;
		do {
			HK_TRY(tryReadCommand(cmdType));
		} while (cmdType != cCommandType_Frame);
	}

	mCurPacket.frame.frameIdx = mFrameIdx++;
	return mCurPacket;
}

hk::Result ScriptSTAS::tryReadCommand(CommandType& cmdType) {
	HK_ASSERT(mHeader.isValid);

	if (mCommandIdx > mHeader.commandCount - 1) {
		return ResultEndOfScriptReached();
	}

	cmdType = CommandType(HK_TRY(mReader.readU16()));
	u16 size = HK_TRY(mReader.readU16());

	switch (cmdType) {
	case cCommandType_Frame: {
		mNextFrameIdx = HK_TRY(mReader.readU32());
		break;
	}
	case cCommandType_Controller: {
		u8 playerID = HK_TRY(mReader.readU8());
		mReader.alignUp(4);
		Packet::Controller& controller = playerID == 0 ? mCurPacket.frame.player1 : mCurPacket.frame.player2;

		controller.buttons = HK_TRY(mReader.readU64());
		controller.leftStick = HK_TRY(mReader.readVec2i());
		controller.rightStick = HK_TRY(mReader.readVec2i());
		break;
	}
	case cCommandType_Motion: {
		u8 playerID = HK_TRY(mReader.readU8());
		u8 playerControllerID = HK_TRY(mReader.readU8());
		Packet::Controller& controller = playerID == 0 ? mCurPacket.frame.player1 : mCurPacket.frame.player2;

		mReader.alignUp(4);
		if (playerControllerID == 0) {
			controller.accelLeft = HK_TRY(mReader.readVec3f());
			controller.gyroLeft = HK_TRY(mReader.readVec3f());
		} else {
			controller.accelRight = HK_TRY(mReader.readVec3f());
			controller.gyroRight = HK_TRY(mReader.readVec3f());
		}
		break;
	}
	case cCommandType_Amiibo: {
		mCurPacket.frame.amiibo = HK_TRY(mReader.readU64());
		break;
	}

	default: cmdType = cCommandType_Invalid; return hk::ResultFailed();
	}

	mCommandIdx++;
	return hk::ResultSuccess();
}

hk::Result ScriptSTAS::verify() {
	HK_ASSERT(mHeader.isValid);

	mReader.seek(mHeader.endOffset);

	u32 cmdIdx = 0;
	u32 lastFrame = 0;

	auto checkSize = [this, cmdIdx, lastFrame](size got, size expected) -> hk::Result {
		if (got != expected) {
			this->mLogWidget.log("incorrect command size! (expected %#zx, got %#zx) (frame: %d, command: %d)", expected, got, lastFrame, cmdIdx);
			return hk::ResultFailed();
		} else
			return hk::ResultSuccess();
	};

	for (cmdIdx = 0; cmdIdx < mHeader.commandCount; cmdIdx++) {
		CommandType cmdType = CommandType(HK_TRY(mReader.readU16()));
		u16 size = HK_TRY(mReader.readU16());

		switch (cmdType) {
		case cCommandType_Frame: {
			HK_TRY(checkSize(size, 0x4));
			u32 frameIdx = HK_TRY(mReader.readU32());
			if (frameIdx >= lastFrame) {
				lastFrame = frameIdx;
			} else {
				mLogWidget.log("frame out of order! (prev frame: %d, cur frame: %d, command: %d)", lastFrame, frameIdx, cmdIdx);
				return hk::ResultFailed();
			}
			break;
		}
		case cCommandType_Controller: {
			HK_TRY(checkSize(size, 0x1c));
			u8 playerID = HK_TRY(mReader.readU8());

			if (mHeader.playerCount == 1 && playerID != 0) {
				mLogWidget.log("script only supports 1 player! (player: %d, frame: %d, command: %d)", playerID, lastFrame, cmdIdx);
				return hk::ResultFailed();
			} else if (mHeader.playerCount == 2 && playerID != 0 && playerID != 1) {
				mLogWidget.log("script only supports 2 players! (player: %d, frame: %d, command: %d)", playerID, lastFrame, cmdIdx);
				return hk::ResultFailed();
			}

			mReader.alignUp(4);
			u64 buttons = HK_TRY(mReader.readU64());
			Vector2i leftStick = HK_TRY(mReader.readVec2i());
			Vector2i rightStick = HK_TRY(mReader.readVec2i());
			break;
		}
		case cCommandType_Motion: {
			HK_TRY(checkSize(size, 0x1c));
			u8 playerID = HK_TRY(mReader.readU8());
			u8 playerControllerID = HK_TRY(mReader.readU8());

			if (mHeader.playerCount == 1 && playerID != 0) {
				mLogWidget.log("script only supports 1 player! (player: %d, frame: %d, command: %d)", playerID, lastFrame, cmdIdx);
				return hk::ResultFailed();
			} else if (mHeader.playerCount == 2 && playerID != 0 && playerID != 1) {
				mLogWidget.log("script only supports 2 players! (player: %d, frame: %d, command: %d)", playerID, lastFrame, cmdIdx);
				return hk::ResultFailed();
			}

			if (mHeader.controllerTypes[playerID] != cControllerType_DualJoycon && playerControllerID != 0) {
				mLogWidget.log("controller type only supports one motion device! (frame: %d, command: %d)", lastFrame, cmdIdx);
				return hk::ResultFailed();
			} else if (playerControllerID != 0 && playerControllerID != 1) {
				mLogWidget.log("controller type only supports two motion devices! (frame: %d, command: %d)", lastFrame, cmdIdx);
				return hk::ResultFailed();
			}

			mReader.alignUp(4);
			Vector3f accel = HK_TRY(mReader.readVec3f());
			Vector3f gyro = HK_TRY(mReader.readVec3f());
			break;
		}
		// case cCommandType_Amiibo: {
		//	HK_TRY(checkSize(size, 0x8));
		// 	mReader.seekRel(0x8);
		// 	break;
		// }
		default: {
			mLogWidget.log("unsupported command type %d! (frame: %d, command: %d)", cmdType, lastFrame, cmdIdx);
			return hk::ResultFailed();
		}
		}
	}

	mInfo.frameCount = lastFrame + 1;
	mReader.seek(mHeader.endOffset);

	return hk::ResultSuccess();
}
