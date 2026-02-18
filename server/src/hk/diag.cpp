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

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#include "hk/Result.h"
#include "hk/diag/diag.h"

namespace hk::diag {

static void printTimestamp() {
	time_t timestamp = time(nullptr);
	struct tm* t = localtime(&timestamp);

	printf("[%02d.%02d.%0004d %02d:%02d:%02d] ", t->tm_mday, t->tm_mon + 1, t->tm_year + 1900, t->tm_hour, t->tm_min, t->tm_sec);
}

constexpr char sAbortFormat[] = R"(
~~~ HakkunAbort ~~~
File: %s:%d
)";

hk_noreturn void abortImpl(Result result, const char* file, int line, const char* msgFmt, ...) {
	va_list arg;
	va_start(arg, msgFmt);
	abortImpl(result, file, line, msgFmt, arg);
	va_end(arg);
}

hk_noreturn void abortImpl(Result result, const char* file, int line, const char* msgFmt, std::va_list arg) {
	char userMsgBuf[0x80];
	vsnprintf(userMsgBuf, sizeof(userMsgBuf), msgFmt, arg);

	char headerMsgBuf[0x80];
	snprintf(headerMsgBuf, sizeof(headerMsgBuf), sAbortFormat, file, line);

	logLine(headerMsgBuf);
	logLine(userMsgBuf);

	abort();
}

extern "C" void __attribute__((weak)) hkLogSink(const char* msg, size len) {}

void logBuffer(const char* buf, size length) {
	printf("%s", buf);
	hkLogSink(buf, length);
}

void logImpl(const char* fmt, std::va_list list) {
	std::va_list listCopy;
	va_copy(listCopy, list);

	size len = vsnprintf(nullptr, 0, fmt, list);
	char buf[len + 1];
	vsnprintf(buf, len + 1, fmt, listCopy);

	printTimestamp();
	printf("%.*s", int(len + 1), buf);
	hkLogSink(buf, len);
}

void log(const char* fmt, ...) {
	std::va_list args;
	va_start(args, fmt);
	logImpl(fmt, args);
	va_end(args);
}

void logLineImpl(const char* fmt, std::va_list list) {
	std::va_list listCopy;
	va_copy(listCopy, list);

	size len = vsnprintf(nullptr, 0, fmt, list);
	char buf[len + 2];
	vsnprintf(buf, len + 2, fmt, listCopy);
	printTimestamp();
	printf("%.*s\n", int(len + 1), buf);

	buf[len] = '\n';
	hkLogSink(buf, len + 1);
}

void logLine(const char* fmt, ...) {
	std::va_list args;
	va_start(args, fmt);
	logLineImpl(fmt, args);
	va_end(args);
}

void debugLog(const char* fmt, ...) {
	std::va_list args;
	va_start(args, fmt);
	logLineImpl(fmt, args);
	va_end(args);
}

} // namespace hk::diag
