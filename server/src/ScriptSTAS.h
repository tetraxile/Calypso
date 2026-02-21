#pragma once

#include <QFile>

#include <hk/Result.h>
#include <hk/types.h>
#include <hk/util/Math.h>

#include "BinaryReader.h"
#include "LogWidget.h"

class ScriptSTAS {
private:
	using Vector2i = hk::util::Vector2i;
	using Vector3f = hk::util::Vector3f;

	constexpr static u64 SMO_TITLE_ID = 0x0100000000010000;

	enum ControllerType : u8 {
		cControllerType_ProController = 0,
		cControllerType_DualJoycon,
		cControllerType_LeftJoycon,
		cControllerType_RightJoycon,
		cControllerType_Handheld,
	};

	enum CommandType : u16 {
		cCommandType_Frame = 0,
		cCommandType_Controller = 1,
		cCommandType_Motion = 2,
		cCommandType_Amiibo = 3,
		cCommandType_Touch = 4,
		cCommandType_Invalid = 0xffff
	};

	struct Packet {
		struct Controller {
			u64 flags;
			u64 buttons;
			Vector2i leftStick;
			Vector2i rightStick;
			Vector3f accelLeft;
			Vector3f gyroLeft;
			Vector3f accelRight;
			Vector3f gyroRight;
		};

		struct Frame {
			u32 flags;
			u32 frameIdx;
			Controller player1;
			Controller player2;
			u64 amiibo;
		};

		char signature[4];
		u8 version = 0;
		u8 type = 0;
		u16 size = 0x8 + sizeof(Frame);
		Frame frame;
	};

public:
	ScriptSTAS(QFile& file, LogWidget& logWidget);

	hk::Result readHeader();
	hk::Result verify();
	hk::ValueOrResult<Packet> getNextFrame();
	void close();

	struct {
		QString name;
		QString author;
		u32 frameCount = 0;
	} mInfo;

private:
	hk::Result tryReadCommand(CommandType& cmdType);

	QFile& mFile;
	BinaryReader mReader;
	LogWidget& mLogWidget;

	u32 mCommandIdx = 0;
	Packet mCurPacket;

	struct {
		u64 titleID = 0;
		u16 formatVersion = 0;
		u16 gameVersion = 0;
		u16 editorVersion = 0;
		u8 playerCount = 0;
		bool isValid = false;
		u32 commandCount = 0;
		u32 secondsEdited = 0;
		ControllerType controllerTypes[8] = {};
		size endOffset = 0;
	} mHeader;
};
