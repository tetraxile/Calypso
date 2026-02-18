#include "ScriptSTAS.h"

#include <hk/ValueOrResult.h>

#include "results.h"

ScriptSTAS::ScriptSTAS(QFile& file) : file(file), reader(file), name(file.filesystemFileName().filename().u16string()) {}

void ScriptSTAS::close() {
	if (file.isOpen()) file.close();
}

hk::Result ScriptSTAS::readHeader() {
	HK_TRY(reader.checkSignature("STAS"));

	header.formatVersion = HK_TRY(reader.readU16());
	if (header.formatVersion != 0) return ResultUnsupportedVersion();

	header.gameVersion = HK_TRY(reader.readU16());
	if (header.gameVersion != 0) return ResultUnsupportedVersion();

	header.editorVersion = HK_TRY(reader.readU16());
	reader.alignUp(sizeof(u32));
	header.titleID = HK_TRY(reader.readU64());
	if (header.titleID != SMO_TITLE_ID) return ResultInvalidTitleID();

	header.commandCount = HK_TRY(reader.readU32());
	header.secondsEdited = HK_TRY(reader.readU32());
	header.playerCount = HK_TRY(reader.readU8());
	reader.alignUp(sizeof(u32));
	for (s32 i = 0; i < 4; i++)
		header.controllerTypes[i] = ControllerType(HK_TRY(reader.readU8()));
	u16 authorNameLength = HK_TRY(reader.readU16());
	header.author = HK_TRY(reader.readString(authorNameLength));

	return hk::ResultSuccess();
}
