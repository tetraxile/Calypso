#include "ScriptSTAS.h"

#include "reader.h"

ScriptSTAS::ScriptSTAS(QFile& file) : file(file) {}

void ScriptSTAS::close() {
	file.close();
}

hk::Result ScriptSTAS::readHeader() {
	QDataStream in(&file);

	printf("reading header...\n");

	hk::Result r = reader::checkSignature(in, 0, "STAS");
	if (r.failed()) return r;

	return hk::ResultSuccess();
}
