#include "ScriptSTAS.h"

#include <hk/ValueOrResult.h>

ScriptSTAS::ScriptSTAS(QFile& file) : file(file), reader(file) {}

void ScriptSTAS::close() {
	if (file.isOpen()) file.close();
}

hk::Result ScriptSTAS::readHeader() {
	printf("reading header...\n");

	HK_TRY(reader.checkSignature("STAS"));

	u16 formatVersion = HK_TRY(reader.readU16());
	u16 gameVersion = HK_TRY(reader.readU16());
	u16 editorVersion = HK_TRY(reader.readU16());
	u64 titleID = HK_TRY(reader.readU64());

	u32 commandCount = HK_TRY(reader.readU32());
	u32 secondsEditing = HK_TRY(reader.readU32());
	u8 playerCount = HK_TRY(reader.readU8());
	reader.alignUp(sizeof(u32));
	u32 controllerTypes = HK_TRY(reader.readU32());
	u16 authorNameLength = HK_TRY(reader.readU16());

	return hk::ResultSuccess();
}
