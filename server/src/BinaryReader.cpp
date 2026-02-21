#include "BinaryReader.h"

#include <bit>

#include "hk/ValueOrResult.h"
#include "results.h"

hk::Result BinaryReader::checkSignature(const QString& expected) {
	size len = expected.length();
	char buffer[len + 1];
	buffer[len] = '\0';

	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	for (s32 i = 0; i < len; i++)
		buffer[i] = mBuffer.at(mCursor++);

	if (expected != buffer)
		return ResultSignatureMismatch();
	else
		return hk::ResultSuccess();
}

hk::ValueOrResult<u8> BinaryReader::readU8() {
	const size len = sizeof(u8);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();
	return at(mCursor++);
}

hk::ValueOrResult<u16> BinaryReader::readU16() {
	const size len = sizeof(u16);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	u16 out = 0;
	for (s32 i = 0; i < 2; i++)
		out |= static_cast<u16>(at(start + i)) << (i * 8);
	return out;
}

hk::ValueOrResult<u32> BinaryReader::readU32() {
	const size len = sizeof(u32);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	u32 out = 0;
	for (s32 i = 0; i < 4; i++)
		out |= static_cast<u32>(at(start + i)) << (i * 8);
	return out;
}

hk::ValueOrResult<u64> BinaryReader::readU64() {
	const size len = sizeof(u64);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	u64 out = 0;
	for (s32 i = 0; i < 8; i++)
		out |= static_cast<u64>(at(start + i)) << (i * 8);
	return out;
}

hk::ValueOrResult<s8> BinaryReader::readS8() {
	const size len = sizeof(u8);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();
	return at(mCursor++);
}

hk::ValueOrResult<s16> BinaryReader::readS16() {
	const size len = sizeof(u16);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	s16 out = 0;
	for (s32 i = 0; i < 2; i++)
		out |= static_cast<s16>(at(start + i)) << (i * 8);
	return out;
}

hk::ValueOrResult<s32> BinaryReader::readS32() {
	const size len = sizeof(u32);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	s32 out = 0;
	for (s32 i = 0; i < 4; i++)
		out |= static_cast<s32>(at(start + i)) << (i * 8);
	return out;
}

hk::ValueOrResult<s64> BinaryReader::readS64() {
	const size len = sizeof(u64);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	s64 out = 0;
	for (s32 i = 0; i < 8; i++)
		out |= static_cast<s64>(at(start + i)) << (i * 8);
	return out;
}

hk::ValueOrResult<f32> BinaryReader::readF32() {
	const size len = sizeof(u32);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	u32 tmp = 0;
	for (s32 i = 0; i < 4; i++)
		tmp |= static_cast<u32>(at(start + i)) << (i * 8);
	return std::bit_cast<f32>(tmp);
}

hk::ValueOrResult<f64> BinaryReader::readF64() {
	const size len = sizeof(u64);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	u64 tmp = 0;
	for (s32 i = 0; i < 8; i++)
		tmp |= static_cast<u64>(at(start + i)) << (i * 8);
	return std::bit_cast<f64>(tmp);
}

hk::ValueOrResult<bool> BinaryReader::readBool() {
	const size len = sizeof(u8);
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();
	return at(mCursor++);
}

hk::ValueOrResult<QString> BinaryReader::readString(size len) {
	if (mCursor + len > mBuffer.length()) return ResultEOFReached();
	QString out = mBuffer.sliced(mCursor, len);

	mCursor += len;
	return out;
}

hk::ValueOrResult<hk::util::Vector2i> BinaryReader::readVec2i() {
	s32 x = HK_TRY(readS32());
	s32 y = HK_TRY(readS32());
	return hk::util::Vector2i(x, y);
}

hk::ValueOrResult<hk::util::Vector2f> BinaryReader::readVec2f() {
	f32 x = HK_TRY(readF32());
	f32 y = HK_TRY(readF32());
	return hk::util::Vector2f(x, y);
}

hk::ValueOrResult<hk::util::Vector3i> BinaryReader::readVec3i() {
	s32 x = HK_TRY(readS32());
	s32 y = HK_TRY(readS32());
	s32 z = HK_TRY(readS32());
	return hk::util::Vector3i(x, y, z);
}

hk::ValueOrResult<hk::util::Vector3f> BinaryReader::readVec3f() {
	f32 x = HK_TRY(readF32());
	f32 y = HK_TRY(readF32());
	f32 z = HK_TRY(readF32());
	return hk::util::Vector3f(x, y, z);
}

void BinaryReader::alignUp(size alignment) {
	mCursor = hk::alignUp(mCursor, alignment);
}
