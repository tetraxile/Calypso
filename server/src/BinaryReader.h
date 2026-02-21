#pragma once

#include <QDataStream>
#include <QFile>
#include <QString>

#include <hk/types.h>
#include <hk/util/Math.h>

class BinaryReader {
public:
	BinaryReader(QFile& file, size size) : mBuffer(size, 0) {
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
	hk::ValueOrResult<hk::util::Vector2i> readVec2i();
	hk::ValueOrResult<hk::util::Vector2f> readVec2f();
	hk::ValueOrResult<hk::util::Vector3i> readVec3i();
	hk::ValueOrResult<hk::util::Vector3f> readVec3f();

	hk::Result checkSignature(const QString& expected);
	void alignUp(size alignment);

	void seek(size offset) { mCursor = offset; }

	void seekRel(size offset) { mCursor += offset; }

	size position() const { return mCursor; }

private:
	u8 at(size offset) const { return mBuffer.at(offset); }

	QByteArray mBuffer;
	size mCursor = 0;
};
