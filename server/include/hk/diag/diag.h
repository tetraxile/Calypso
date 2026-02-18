/*
LibHakkun license:

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS” AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF
USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include <cstdarg>
#include "hk/diag/results.h" // IWYU pragma: keep

namespace hk::diag {

const char* getResultName(hk::Result result);

hk_noreturn void abortImpl(Result result, const char* file, int line, const char* msgFmt, ...);
hk_noreturn void abortImpl(Result result, const char* file, int line, const char* msgFmt, std::va_list arg);

constexpr char cAssertionFailFormat[] = "AssertionFailed: %s";
constexpr char cAbortUnlessResultFormat[] = "ResultAbort (%04d-%04d/0x%x) [from %s]";
constexpr char cAbortUnlessResultFormatWithName[] = "ResultAbort (%04d-%04d/%s) [from %s]";
constexpr char cUnwrapResultFormat[] = "Unwrap (%04d-%04d/0x%x) [from %s]";
constexpr char cUnwrapResultFormatWithName[] = "Unwrap (%04d-%04d/%s) [from %s]";
constexpr char cNullptrUnwrapFormat[] = "Unwrap (nullptr) [from %s]";

#define HK_ASSERT(CONDITION)                                                                                                                                   \
	do {                                                                                                                                                       \
		const bool _condition_temp = (CONDITION);                                                                                                              \
		if (_condition_temp == false) {                                                                                                                        \
			::hk::diag::abortImpl(::hk::diag::ResultAssertionFailure(), __FILE__, __LINE__, ::hk::diag::cAssertionFailFormat, #CONDITION);                     \
		}                                                                                                                                                      \
	} while (0)

#define HK_ABORT(FMT, ...)                                                                                                                                     \
	do {                                                                                                                                                       \
		::hk::diag::abortImpl(::hk::diag::ResultAbort(), __FILE__, __LINE__, "\n" FMT "\n" __VA_OPT__(, ) __VA_ARGS__);                                        \
	} while (0)

#define HK_ABORT_UNLESS(CONDITION, FMT, ...)                                                                                                                   \
	do {                                                                                                                                                       \
		const bool _condition_temp = (CONDITION);                                                                                                              \
		const char* _fmt = FMT;                                                                                                                                \
		if (_condition_temp == false) {                                                                                                                        \
			::hk::diag::abortImpl(::hk::diag::ResultAbort(), __FILE__, __LINE__, "\n" FMT "\n" __VA_OPT__(, ) __VA_ARGS__);                                    \
		}                                                                                                                                                      \
	} while (0)

#define HK_ABORT_UNLESS_R(RESULT)                                                                                                                              \
	do {                                                                                                                                                       \
		const ::hk::Result _result_temp = RESULT;                                                                                                              \
		if (_result_temp.failed()) {                                                                                                                           \
			const char* _result_temp_name = ::hk::diag::getResultName(_result_temp);                                                                           \
			if (_result_temp_name != nullptr) {                                                                                                                \
				::hk::diag::abortImpl(                                                                                                                         \
					_result_temp, __FILE__, __LINE__, ::hk::diag::cAbortUnlessResultFormatWithName, _result_temp.getModule() + 2000,                           \
					_result_temp.getDescription(), _result_temp_name, #RESULT                                                                                  \
				);                                                                                                                                             \
			} else {                                                                                                                                           \
				::hk::diag::abortImpl(                                                                                                                         \
					_result_temp, __FILE__, __LINE__, ::hk::diag::cAbortUnlessResultFormat, _result_temp.getModule() + 2000, _result_temp.getDescription(),    \
					_result_temp.getValue(), #RESULT                                                                                                           \
				);                                                                                                                                             \
			}                                                                                                                                                  \
		}                                                                                                                                                      \
	} while (0)

#define HK_TODO(...) HK_ABORT("todo" __VA_OPT__(": ", ) __VA_ARGS__)

/**
 * @brief Weak sink function that will be logged to from hk::diag::log
 *
 */
extern "C" void hkLogSink(const char* msg, size len);

void logBuffer(const char* buf, size len);
void logImpl(const char* fmt, std::va_list list);
void log(const char* fmt, ...);
void logLineImpl(const char* fmt, std::va_list list);
void logLine(const char* fmt, ...);
[[deprecated("use hk::debug::logLine instead")]] void debugLog(const char* fmt, ...);

} // namespace hk::diag
