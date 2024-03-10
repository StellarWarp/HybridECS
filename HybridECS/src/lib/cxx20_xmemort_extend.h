#pragma once
#include <xmemory>
#include "cxx20_xutility_extend.h"

namespace std
{
#if !_HAS_CXX20
	_EXPORT_STD template <class _Ty, class _Alloc, class... _Types, enable_if_t<!_Is_cv_pair<_Ty>, int> = 0>
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al, _Types&&... _Args) noexcept {
		if constexpr (!uses_allocator_v<remove_cv_t<_Ty>, _Alloc>) {
			static_assert(is_constructible_v<_Ty, _Types...>,
				"If uses_allocator_v<remove_cv_t<T>, Alloc> does not hold, T must be constructible from Types...");
			(void)_Al;
			return _STD forward_as_tuple(_STD forward<_Types>(_Args)...);
		}
		else if constexpr (is_constructible_v<_Ty, allocator_arg_t, const _Alloc&, _Types...>) {
			using _ReturnType = tuple<allocator_arg_t, const _Alloc&, _Types&&...>;
			return _ReturnType{ allocator_arg, _Al, _STD forward<_Types>(_Args)... };
		}
		else if constexpr (is_constructible_v<_Ty, _Types..., const _Alloc&>) {
			return _STD forward_as_tuple(_STD forward<_Types>(_Args)..., _Al);
		}
		else {
			static_assert(_Always_false<_Ty>,
				"T must be constructible from either (allocator_arg_t, const Alloc&, Types...) "
				"or (Types..., const Alloc&) if uses_allocator_v<remove_cv_t<T>, Alloc> is true");
		}
	}

	_EXPORT_STD template <class _Ty, class _Alloc, enable_if_t<_Is_cv_pair<_Ty>, int> = 0>
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al) noexcept;

	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty1, class _Uty2, enable_if_t<_Is_cv_pair<_Ty>, int> = 0>
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al, _Uty1&& _Val1, _Uty2&& _Val2) noexcept;


	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty1, class _Uty2, enable_if_t<_Is_cv_pair<_Ty>, int> = 0>
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al, const pair<_Uty1, _Uty2>& _Pair) noexcept;

	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty1, class _Uty2, enable_if_t<_Is_cv_pair<_Ty>, int> = 0>
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al, pair<_Uty1, _Uty2>&& _Pair) noexcept;



#if _HAS_CXX23 && defined(__cpp_lib_concepts) // TRANSITION, GH-395
	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty>
		requires _Is_cv_pair<_Ty> && (_Pair_like<_Uty> || !_Is_deducible_as_pair<_Uty&>)
#else // ^^^ C++23 with concepts / C++20 or no concepts vvv
	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty,
		enable_if_t<_Is_cv_pair<_Ty> && !_Is_deducible_as_pair<_Uty&>, int> = 0>
#endif // ^^^ C++20 or no concepts ^^^
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al, _Uty&& _Ux) noexcept;

	_EXPORT_STD template <class _Ty, class _Alloc, class _Tuple1, class _Tuple2, enable_if_t<_Is_cv_pair<_Ty>, int> = 0>
		_NODISCARD constexpr auto uses_allocator_construction_args(
			const _Alloc& _Al, piecewise_construct_t, _Tuple1&& _Tup1, _Tuple2&& _Tup2) noexcept {
		return _STD make_tuple(piecewise_construct,
			_STD apply(
				[&_Al](auto&&... _Tuple_args) {
					return _STD uses_allocator_construction_args<typename _Ty::first_type>(
						_Al, _STD forward<decltype(_Tuple_args)>(_Tuple_args)...);
				},
				_STD forward<_Tuple1>(_Tup1)),
			_STD apply(
				[&_Al](auto&&... _Tuple_args) {
					return _STD uses_allocator_construction_args<typename _Ty::second_type>(
						_Al, _STD forward<decltype(_Tuple_args)>(_Tuple_args)...);
				},
				_STD forward<_Tuple2>(_Tup2)));
	}

	_EXPORT_STD template <class _Ty, class _Alloc, enable_if_t<_Is_cv_pair<_Ty>, int> /* = 0 */>
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al) noexcept {
		// equivalent to
		// return _STD uses_allocator_construction_args<_Ty>(_Al, piecewise_construct, tuple<>{}, tuple<>{});
		return _STD make_tuple(piecewise_construct, _STD uses_allocator_construction_args<typename _Ty::first_type>(_Al),
			_STD uses_allocator_construction_args<typename _Ty::second_type>(_Al));
	}

	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty1, class _Uty2, enable_if_t<_Is_cv_pair<_Ty>, int> /* = 0 */>
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al, _Uty1&& _Val1, _Uty2&& _Val2) noexcept {
		// equivalent to
		// return _STD uses_allocator_construction_args<_Ty>(_Al, piecewise_construct,
		//     _STD forward_as_tuple(_STD forward<_Uty1>(_Val1)), _STD forward_as_tuple(_STD forward<_Uty2>(_Val2)));
		return _STD make_tuple(piecewise_construct,
			_STD uses_allocator_construction_args<typename _Ty::first_type>(_Al, _STD forward<_Uty1>(_Val1)),
			_STD uses_allocator_construction_args<typename _Ty::second_type>(_Al, _STD forward<_Uty2>(_Val2)));
	}


	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty1, class _Uty2, enable_if_t<_Is_cv_pair<_Ty>, int> /* = 0 */>
		_NODISCARD constexpr auto uses_allocator_construction_args(
			const _Alloc& _Al, const pair<_Uty1, _Uty2>& _Pair) noexcept {
		// equivalent to
		// return _STD uses_allocator_construction_args<_Ty>(_Al, piecewise_construct,
		//     _STD forward_as_tuple(_Pair.first), _STD forward_as_tuple(_Pair.second));
		return _STD make_tuple(piecewise_construct,
			_STD uses_allocator_construction_args<typename _Ty::first_type>(_Al, _Pair.first),
			_STD uses_allocator_construction_args<typename _Ty::second_type>(_Al, _Pair.second));
	}

	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty1, class _Uty2, enable_if_t<_Is_cv_pair<_Ty>, int> /* = 0 */>
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al, pair<_Uty1, _Uty2>&& _Pair) noexcept {
		// equivalent to
		// return _STD uses_allocator_construction_args<_Ty>(_Al, piecewise_construct,
		//     _STD forward_as_tuple(_STD get<0>(_STD move(_Pair)), _STD forward_as_tuple(_STD get<1>(_STD move(_Pair)));
		return _STD make_tuple(piecewise_construct,
			_STD uses_allocator_construction_args<typename _Ty::first_type>(_Al, _STD get<0>(_STD move(_Pair))),
			_STD uses_allocator_construction_args<typename _Ty::second_type>(_Al, _STD get<1>(_STD move(_Pair))));
	}



#if _HAS_CXX23 && defined(__cpp_lib_concepts) // TRANSITION, GH-395
	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty>
		requires _Is_cv_pair<_Ty> && (_Pair_like<_Uty> || !_Is_deducible_as_pair<_Uty&>)
#else // ^^^ C++23 with concepts / C++20 or no concepts vvv
	_EXPORT_STD template <class _Ty, class _Alloc, class _Uty,
		enable_if_t<_Is_cv_pair<_Ty> && !_Is_deducible_as_pair<_Uty&>, int> /* = 0 */>
#endif // ^^^ C++20 or no concepts ^^^
		_NODISCARD constexpr auto uses_allocator_construction_args(const _Alloc& _Al, _Uty&& _Ux) noexcept {
		{
			struct _Pair_remaker {
				const _Alloc& _Al;
				_Uty& _Ux;

				constexpr operator remove_cv_t<_Ty>() const {
					using _Pair_t = remove_cv_t<_Ty>;
					static_assert(_Is_normally_bindable<_Pair_t, _Uty>,
						"The argument must be bindable to a reference to the std::pair type.");

					using _Pair_first_t = typename _Pair_t::first_type;
					using _Pair_second_t = typename _Pair_t::second_type;
					using _Pair_ref_t = _Normally_bound_ref<_Pair_t, _Uty>;
					_Pair_ref_t _Pair_ref = _STD forward<_Uty>(_Ux);
					if constexpr (is_same_v<_Pair_ref_t, const _Pair_t&>) {
						// equivalent to
						// return _STD make_obj_using_allocator<_Pair_t>(_Al, _Pair_ref);
						return _Pair_t{ piecewise_construct,
							_STD uses_allocator_construction_args<_Pair_first_t>(_Al, _Pair_ref.first),
							_STD uses_allocator_construction_args<_Pair_second_t>(_Al, _Pair_ref.second) };
					}
					else {
						// equivalent to
						// return _STD make_obj_using_allocator<_Pair_t>(_Al, _STD move(_Pair_ref));
						return _Pair_t{ piecewise_construct,
							_STD uses_allocator_construction_args<_Pair_first_t>(_Al, _STD get<0>(_STD move(_Pair_ref))),
							_STD uses_allocator_construction_args<_Pair_second_t>(_Al, _STD get<1>(_STD move(_Pair_ref))) };
					}
				}
			};

			// equivalent to
			// return _STD make_tuple(_Pair_remaker{_Al, _Ux});
			return tuple<_Pair_remaker>({ _Al, _Ux });
		}
}

	_EXPORT_STD template <class _Ty, class _Alloc, class... _Types>
		_NODISCARD constexpr _Ty make_obj_using_allocator(const _Alloc& _Al, _Types&&... _Args) {
		return _STD make_from_tuple<_Ty>(_STD uses_allocator_construction_args<_Ty>(_Al, _STD forward<_Types>(_Args)...));
	}

	_EXPORT_STD template <class _Ty, class _Alloc, class... _Types>
		constexpr _Ty* uninitialized_construct_using_allocator(_Ty* _Ptr, const _Alloc& _Al, _Types&&... _Args) {
		return _STD apply(
			[&](auto&&... _Construct_args) {
				return _STD construct_at(_Ptr, _STD forward<decltype(_Construct_args)>(_Construct_args)...);
			},
			_STD uses_allocator_construction_args<_Ty>(_Al, _STD forward<_Types>(_Args)...));
	}
}
#endif // _HAS_CXX20