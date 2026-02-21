#pragma once

#include <hk/Result.h>

// clang-format off
HK_RESULT_MODULE(1)
HK_DEFINE_RESULT_RANGE(Calypso, 0, 100)
HK_DEFINE_RESULT(EOFReached,         0)
HK_DEFINE_RESULT(SignatureMismatch,  1)
HK_DEFINE_RESULT(EndOfScriptReached, 2)
// clang-format on
