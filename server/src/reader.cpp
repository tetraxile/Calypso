#include "reader.h"

namespace reader {

hk::Result checkSignature(QDataStream& stream, size offset, const QString& expected) {
	size len = expected.length();
	char buffer[len + 1];
	buffer[len] = '\0';

	s64 outLen = stream.readRawData(buffer, len);

	if (len != outLen or expected != buffer)
		return hk::ResultSuccess();
	else
		return hk::ResultSuccess();
}

} // namespace reader
