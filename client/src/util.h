#pragma once

#include <hk/gfx/Util.h>

#include <nn/types.h>
#include <sead/prim/seadSafeString.h>

#define TITLE_ID HK_TITLE_ID
#define FORMAT_VERSION 0
#define ADDONS_VERSION 0
#define EDITOR_VERSION 0

// only use this for non-user errors
#define LOG_R(RESULT)                                                                                                                                          \
	do {                                                                                                                                                       \
		nn::Result _result = RESULT;                                                                                                                           \
		if (_result.IsFailure()) {                                                                                                                             \
			cly::Menu::log("ERROR: %s:%s (%04d-%04d)", __FILE__, __LINE__, _result.GetModule() + 2000, _result.GetDescription());                              \
			return _result.GetInnerValueForDebug();                                                                                                                                            \
		}                                                                                                                                                      \
	} while (0)

namespace cly::util {

struct Color4f {
	float r, g, b, a;

	u32 toU32() const { return hk::gfx::rgbaf(r, g, b, a); }

	operator u32() const { return toU32(); }
};

hk::Result createFile(const sead::SafeString& filePath, s64 size, bool overwrite = false);
bool isFileExist(const sead::SafeString& filePath);

inline u32 roundUp(u32 x, u32 power_of_2) {
	const u32 a = power_of_2 - 1;
	return (x + a) & ~a;
}

} // namespace cly::util
