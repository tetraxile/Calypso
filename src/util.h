#pragma once

#include <hk/gfx/Util.h>

namespace tas::util {

struct Color4f {
	float r, g, b, a;

	u32 toU32() const { return hk::gfx::rgbaf(r, g, b, a); }

	operator u32() const { return toU32(); }
};

}; // namespace tas::util
