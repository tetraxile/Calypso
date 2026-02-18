#pragma once

#include <QDataStream>
#include <QFile>
#include <QString>

#include <hk/types.h>

class BinaryReader {
public:
	BinaryReader(QFile& file) : mBuffer(0x1000, 0) {
		QDataStream in(&file);
		in.readRawData(mBuffer.data(), mBuffer.size());
	}

	hk::ValueOrResult<u8> readU8();
	hk::ValueOrResult<u16> readU16();
	hk::ValueOrResult<u32> readU32();
	hk::ValueOrResult<u64> readU64();
	hk::ValueOrResult<s8> readS8();
	hk::ValueOrResult<s16> readS16();
	hk::ValueOrResult<s32> readS32();
	hk::ValueOrResult<s64> readS64();
	hk::ValueOrResult<f32> readF32();
	hk::ValueOrResult<f64> readF64();
	hk::ValueOrResult<bool> readBool();
	hk::ValueOrResult<QString> readString(size len);

	hk::Result checkSignature(const QString& expected);
	void alignUp(size alignment);

	size position() const { return mCursor; }

private:
	u8 at(size offset) const { return mBuffer.at(offset); }

	QByteArray mBuffer;
	size mCursor = 0;
};
