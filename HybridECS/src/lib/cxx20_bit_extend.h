#pragma once


// bit standard header (core)

// Copyright (c) Microsoft Corporation.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception

#ifndef _BIT_
#define _BIT_
#include <yvals_core.h>
#if _STL_COMPILER_PREPROCESSOR
#if !_HAS_CXX20

#include <__msvc_bit_utils.hpp>
#include <cstdlib>
#include <type_traits>

#include _STL_INTRIN_HEADER

#pragma pack(push, _CRT_PACKING)
#pragma warning(push, _STL_WARNING_LEVEL)
#pragma warning(disable : _STL_DISABLED_WARNINGS)
_STL_DISABLE_CLANG_WARNINGS
#pragma push_macro("new")
#undef new

_STD_BEGIN

_EXPORT_STD template <class _To, class _From,
	enable_if_t<conjunction_v<bool_constant<sizeof(_To) == sizeof(_From)>, is_trivially_copyable<_To>,
	is_trivially_copyable<_From>>,
	int> = 0>
	_NODISCARD constexpr _To bit_cast(const _From& _Val) noexcept {
	return __builtin_bit_cast(_To, _Val);
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr int countl_zero(_Ty _Val) noexcept;

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr bool has_single_bit(const _Ty _Val) noexcept {
	return _Val != 0 && (_Val & (_Val - 1)) == 0;
}

inline void _Precondition_violation_in_bit_ceil() noexcept {}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr _Ty bit_ceil(const _Ty _Val) noexcept /* strengthened */ {
	if (_Val <= 1u) {
		return _Ty{ 1 };
	}

	const int _Num = _Unsigned_integer_digits<_Ty> -_STD countl_zero(static_cast<_Ty>(_Val - 1));

	if constexpr (sizeof(_Ty) < sizeof(unsigned int)) { // for types subject to integral promotion
		if (_STD _Is_constant_evaluated()) {
			// Because N4950 [expr.shift]/1 "integral promotions are performed"
			// the compiler will not generate a compile time error for
			// uint8_t{1} << 8
			// or
			// uint16_t{1} << 16
			// so we must manually enforce N4950 [bit.pow.two]/5, 8:
			// "Preconditions: N is representable as a value of type T."
			// "Remarks: A function call expression that violates the precondition in the Preconditions: element
			// is not a core constant expression (7.7)."
			if (_Num == _Unsigned_integer_digits<_Ty>) {
				_Precondition_violation_in_bit_ceil();
			}
		}
	}

	return static_cast<_Ty>(_Ty{ 1 } << _Num);
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr _Ty bit_floor(const _Ty _Val) noexcept {
	if (_Val == 0) {
		return 0;
	}

	return static_cast<_Ty>(_Ty{ 1 } << (_Unsigned_integer_digits<_Ty> -1 - _STD countl_zero(_Val)));
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr int bit_width(const _Ty _Val) noexcept {
	return _Unsigned_integer_digits<_Ty> -_STD countl_zero(_Val);
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr _Ty rotr(_Ty _Val, int _Rotation) noexcept;

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr _Ty rotl(const _Ty _Val, const int _Rotation) noexcept {
	constexpr auto _Digits = _Unsigned_integer_digits<_Ty>;

	if (!_STD _Is_constant_evaluated()) {
		if constexpr (_Digits == 64) {
			return _rotl64(_Val, _Rotation);
		}
		else if constexpr (_Digits == 32) {
			return _rotl(_Val, _Rotation);
		}
		else if constexpr (_Digits == 16) {
			return _rotl16(_Val, static_cast<unsigned char>(_Rotation));
		}
		else {
			_STL_INTERNAL_STATIC_ASSERT(_Digits == 8);
			return _rotl8(_Val, static_cast<unsigned char>(_Rotation));
		}
	}

	const auto _Remainder = _Rotation % _Digits;
	if (_Remainder > 0) {
		return static_cast<_Ty>(
			static_cast<_Ty>(_Val << _Remainder) | static_cast<_Ty>(_Val >> (_Digits - _Remainder)));
	}
	else if (_Remainder == 0) {
		return _Val;
	}
	else { // _Remainder < 0
		return _STD rotr(_Val, -_Remainder);
	}
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> /* = 0 */>
_NODISCARD constexpr _Ty rotr(const _Ty _Val, const int _Rotation) noexcept {
	constexpr auto _Digits = _Unsigned_integer_digits<_Ty>;

	if (!_STD _Is_constant_evaluated()) {
		if constexpr (_Digits == 64) {
			return _rotr64(_Val, _Rotation);
		}
		else if constexpr (_Digits == 32) {
			return _rotr(_Val, _Rotation);
		}
		else if constexpr (_Digits == 16) {
			return _rotr16(_Val, static_cast<unsigned char>(_Rotation));
		}
		else {
			_STL_INTERNAL_STATIC_ASSERT(_Digits == 8);
			return _rotr8(_Val, static_cast<unsigned char>(_Rotation));
		}
	}

	const auto _Remainder = _Rotation % _Digits;
	if (_Remainder > 0) {
		return static_cast<_Ty>(
			static_cast<_Ty>(_Val >> _Remainder) | static_cast<_Ty>(_Val << (_Digits - _Remainder)));
	}
	else if (_Remainder == 0) {
		return _Val;
	}
	else { // _Remainder < 0
		return _STD rotl(_Val, -_Remainder);
	}
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> /* = 0 */>
_NODISCARD constexpr int countl_zero(const _Ty _Val) noexcept {
#if _HAS_COUNTL_ZERO_INTRINSICS
#if defined(_M_IX86) || (defined(_M_X64) && !defined(_M_ARM64EC))
	if (!_STD _Is_constant_evaluated()) {
		return _Checked_x86_x64_countl_zero(_Val);
	}
#elif defined(_M_ARM) || defined(_M_ARM64)
	if (!_STD _Is_constant_evaluated()) {
		return _Checked_arm_arm64_countl_zero(_Val);
	}
#endif // defined(_M_ARM) || defined(_M_ARM64)
#endif // _HAS_COUNTL_ZERO_INTRINSICS

	return _Countl_zero_fallback(_Val);
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr int countl_one(const _Ty _Val) noexcept {
	return _STD countl_zero(static_cast<_Ty>(~_Val));
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr int countr_zero(const _Ty _Val) noexcept {
	return _Countr_zero(_Val);
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr int countr_one(const _Ty _Val) noexcept {
	return _Countr_zero(static_cast<_Ty>(~_Val));
}

_EXPORT_STD template <class _Ty, enable_if_t<_Is_standard_unsigned_integer<_Ty>, int> = 0>
_NODISCARD constexpr int popcount(const _Ty _Val) noexcept {
	return _Popcount(_Val);
}

_EXPORT_STD enum class endian { little = 0, big = 1, native = little };

_STD_END
#pragma pop_macro("new")
_STL_RESTORE_CLANG_WARNINGS
#pragma warning(pop)
#pragma pack(pop)
#endif // _HAS_CXX20
#endif // _STL_COMPILER_PREPROCESSOR
#endif // _BIT_
