#pragma once

#include <hk/Result.h>

HK_RESULT_MODULE(413)
HK_DEFINE_RESULT_RANGE(Calypso, 0, 10)
HK_DEFINE_RESULT(EOFReached, 0)
HK_DEFINE_RESULT(SignatureMismatch, 1)
