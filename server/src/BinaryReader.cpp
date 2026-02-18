#include "BinaryReader.h"

#include <bit>

#include "hk/ValueOrResult.h"
#include "results.h"

hk::Result BinaryReader::checkSignature(const QString& expected) {
	size len = expected.length();
	char buffer[len + 1];
	buffer[len] = '\0';

	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	for (s32 i = 0; i < len; i++)
		buffer[i] = mBuffer.at(mCursor++);

	if (expected != buffer)
		return ResultSignatureMismatch();
	else
		return hk::ResultSuccess();
}

hk::ValueOrResult<u8> BinaryReader::readU8() {
	const size len = sizeof(u8);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();
	return mBuffer.at(mCursor++);
}

hk::ValueOrResult<u16> BinaryReader::readU16() {
	const size len = sizeof(u16);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;
	return mBuffer.at(start) | (mBuffer.at(start + 1) << 8);
}

hk::ValueOrResult<u32> BinaryReader::readU32() {
	const size len = sizeof(u32);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;
	return mBuffer.at(start) | (mBuffer.at(start + 1) << 8) | (mBuffer.at(start + 2) << 16) | (mBuffer.at(start + 3) << 24);
}

hk::ValueOrResult<u64> BinaryReader::readU64() {
	const size len = sizeof(u64);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	u64 out = 0;
	for (s32 i = 7; i >= 0; i--) {
		out |= mBuffer.at(start + i);
		out <<= 8;
	}

	return out;
}

hk::ValueOrResult<s8> BinaryReader::readS8() {
	const size len = sizeof(u8);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();
	return mBuffer.at(mCursor++);
}

hk::ValueOrResult<s16> BinaryReader::readS16() {
	const size len = sizeof(u16);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;
	return mBuffer.at(start) | (mBuffer.at(start + 1) << 8);
}

hk::ValueOrResult<s32> BinaryReader::readS32() {
	const size len = sizeof(u32);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;
	return mBuffer.at(start) | (mBuffer.at(start + 1) << 8) | (mBuffer.at(start + 2) << 16) | (mBuffer.at(start + 3) << 24);
}

hk::ValueOrResult<s64> BinaryReader::readS64() {
	const size len = sizeof(u64);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	s64 out = 0;
	for (s32 i = 7; i >= 0; i--) {
		out |= mBuffer.at(start + i);
		out <<= 8;
	}

	return out;
}

hk::ValueOrResult<f32> BinaryReader::readF32() {
	const size len = sizeof(u32);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;
	u32 tmp = mBuffer.at(start) | (mBuffer.at(start + 1) << 8) | (mBuffer.at(start + 2) << 16) | (mBuffer.at(start + 3) << 24);

	return std::bit_cast<f32>(tmp);
}

hk::ValueOrResult<f64> BinaryReader::readF64() {
	const size len = sizeof(u64);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();

	size start = mCursor;
	mCursor += len;

	u64 tmp = 0;
	for (s32 i = 7; i >= 0; i--) {
		tmp |= mBuffer.at(start + i);
		tmp <<= 8;
	}

	return std::bit_cast<f64>(tmp);
}

hk::ValueOrResult<bool> BinaryReader::readBool() {
	const size len = sizeof(u8);
	if (mCursor + len >= mBuffer.length()) return ResultEOFReached();
	return mBuffer.at(mCursor++);
}

void BinaryReader::alignUp(size alignment) {
	mCursor = hk::alignUp(mCursor, alignment);
}
