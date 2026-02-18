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

#include "hk/types.h"

namespace hk {

template <typename T>
class ValueOrResult;

/**
 * @brief Value representing the result of an operation.
 *
 * When the value is ResultSuccess(), the operation was successful.
 * Otherwise, a module and description code can be retrieved to determine the error.
 */
struct Result {
	u32 value = 0;

	// BOOOOOOOOOOOORIIIING
	static constexpr u32 makeResult(int module, int description) { return (module & 0b0111111111) | ((description) & 0b01111111111111) << 9; }

	constexpr Result() : value(0) {}

	constexpr Result(u32 value) : value(value) {}

	constexpr Result(int module, int description) : value(makeResult(module, description)) {}

	constexpr u32 getValue() const { return value; }

	constexpr operator u32() const { return value; }

	constexpr operator bool() const { return failed(); }

	template <typename T>
	constexpr operator ValueOrResult<T>() const;

	constexpr int getModule() const { return value & 0b0111111111; }

	constexpr int getDescription() const { return ((value) >> 9) & 0b01111111111111; }

	constexpr bool operator==(const Result& rhs) const { return rhs.value == value; }

	constexpr bool succeeded() const { return value == 0; }

	constexpr bool failed() const { return !succeeded(); }
};

template <int Module, int Description>
class ResultV : public Result {
public:
	constexpr ResultV() : Result(Module, Description) {}

	ResultV(u32 value) = delete;
	ResultV(int module, int description) = delete;
};

template <u32 Min, u32 Max>
struct ResultRange {
	static constexpr bool includes(Result value) {
		u32 intval = value;
		return intval >= Min && intval <= Max;
	}
};

template <int Module>
struct ResultModule {
	constexpr static int cModule = Module;
};

#define HK_RESULT_MODULE(ID)                                                                                                                                   \
	namespace _hk_result_id_namespace {                                                                                                                        \
	using Module = ::hk::ResultModule<ID>;                                                                                                                     \
	}

/**
 * @brief Define a range for result types to be used in current module.
 *
 */
#define HK_DEFINE_RESULT_RANGE(NAME, MIN, MAX)                                                                                                                 \
	using ResultRange##NAME = ::hk::ResultRange<                                                                                                               \
		::hk::ResultV<_hk_result_id_namespace::Module::cModule, MIN>().getValue(), ::hk::ResultV<_hk_result_id_namespace::Module::cModule, MAX>().getValue()>;

#ifndef HK_DEFINE_RESULT
/**
 * @brief Define a result type for the current module.
 *
 */
# define HK_DEFINE_RESULT(NAME, DESCRIPTION) using Result##NAME = ::hk::ResultV<_hk_result_id_namespace::Module::cModule, DESCRIPTION>;
#endif

template <typename ResultType>
hk_alwaysinline bool isResult(Result value) {
	return value == ResultType();
}

#ifndef INCLUDE_HK_DETAIL_DEFAULTRESULTS

# define INCLUDE_HK_DETAIL_DEFAULTRESULTS
# include "hk/detail/DefaultResults.ih"
# undef INCLUDE_HK_DETAIL_DEFAULTRESULTS

namespace detail {

template <typename T>
struct ResultChecker {
	hk_alwaysinline static constexpr Result check(const T&& value) { return Result(value); }
};

template <typename T>
struct ResultChecker<T*> {
	hk_alwaysinline static constexpr Result check(const T*&& ptr) {
		if (ptr != nullptr)
			return hk::ResultSuccess();
		else
			return hk::ResultNoValue();
	}
};
} // namespace detail

#endif // INCLUDE_HK_DETAIL_DEFAULTRESULTS

/**
 * @brief Return if Result within expression is unsuccessful.
 * If expression is pointer, return ResultNoValue() if it is nullptr.
 * Function must return Result.
 */
#define HK_TRY(VALUE)                                                                                                                                          \
	{                                                                                                                                                          \
		auto&& _value_temp = VALUE;                                                                                                                            \
		using _ValueT = std::remove_reference_t<decltype(_value_temp)>;                                                                                        \
                                                                                                                                                               \
		const ::hk::Result _result_temp = ::hk::detail::ResultChecker<_ValueT>::check(::forward<_ValueT>(_value_temp));                                        \
		if (_result_temp.failed()) return _result_temp;                                                                                                        \
		::move(_value_temp);                                                                                                                                   \
	}

/**
 * @brief Return a Result expression if CONDITION is false.
 * Function must return Result.
 */
#define HK_UNLESS(CONDITION, RESULT)                                                                                                                           \
	{                                                                                                                                                          \
		const bool _condition_temp = (CONDITION);                                                                                                              \
		const ::hk::Result _result_temp = RESULT;                                                                                                              \
		if (_condition_temp == false) return _result_temp;                                                                                                     \
	}

} // namespace hk
