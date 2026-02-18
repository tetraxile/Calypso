#pragma once

#include <QFile>

#include <hk/Result.h>
#include <hk/types.h>

#include "BinaryReader.h"

struct ScriptSTAS {
	constexpr static u64 SMO_TITLE_ID = 0x0100000000010000;

	enum class ControllerType : u8 {
		ProController = 0,
		DualJoycon,
		LeftJoycon,
		RightJoycon,
		Handheld,
	};

	ScriptSTAS(QFile& file);
	hk::Result readHeader();
	void close();

	QFile& file;
	QString name;

	struct {
		u16 formatVersion;
		u16 gameVersion;
		u16 editorVersion;
		u64 titleID;
		u32 commandCount;
		u32 secondsEdited;
		u8 playerCount;
		ControllerType controllerTypes[8];
		QString author;
	} header;

private:
	BinaryReader reader;
};
