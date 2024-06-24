#pragma once
#include "function_traits.h"
#include "type_indexed_array.h"

namespace hyecs
{
	template <typename... Ts, typename T>
	constexpr auto tuple_append(std::tuple<Ts...> t1, T arg)
	{
		return std::tuple_cat(t1, std::tuple<T>(arg));
	}

	template <typename T, typename... Ts, size_t... I>
	constexpr auto tuple_remove(T t, std::index_sequence<I...>)
	{
		return std::tuple<Ts...>(std::get<I + 1>(t)...);
	}

	template <typename... Ts, typename T>
	constexpr auto tuple_remove(std::tuple<T, Ts...> t)
	{
		return tuple_remove<std::tuple<T, Ts...>, Ts...>(t, std::make_index_sequence<sizeof...(Ts)>{});
	}

	template <typename... Ts, typename... Us>
	constexpr auto tuple_trans(std::tuple<Ts...> t1, std::tuple<Us...> arg)
	{
		return std::tuple_cat(
			t1,
			std::tuple<std::tuple_element_t<0, decltype(arg)>>(std::get<0>(arg)));
	}

	template <template <typename> typename Condition, typename T, typename... Ts, typename... Res>
	constexpr auto filter_args(std::tuple<Res...> t_res, T val, Ts... vals)
	{
		if constexpr (Condition<T>{})
		{
			if constexpr (sizeof...(Ts) == 0)
				return tuple_append(t_res, val);
			else
				return filter_args<Condition>(tuple_append(t_res, val), vals...);
		}
		else
		{
			if constexpr (sizeof...(Ts) == 0)
				return t_res;
			else
				return filter_args<Condition>(t_res, vals...);
		}
	}

	template <template <typename> typename Condition, typename... T>
	constexpr auto filter_args(T... args)
	{
		return filter_args<Condition>(std::tuple<>{}, args...);
	}

	template <class T>
	struct uncheck_pointer_traits;

	template <class T>
	[[nodiscard]] constexpr T* uncheck_to_address(T* const ptr) noexcept {
		static_assert(!std::is_function_v<T>, "N4950 [pointer.conversion]/1: Mandates: T is not a function type.");
		return ptr;
	}
	template <class Iter>
	[[nodiscard]] constexpr auto uncheck_to_address(const Iter& it) noexcept {
		if constexpr (requires { uncheck_pointer_traits<Iter>::to_address(it); })
			return uncheck_pointer_traits<Iter>::to_address(it);
		else if constexpr (requires { std::pointer_traits<Iter>::to_address(it); })
			return std::pointer_traits<Iter>::to_address(it);
		else
			return uncheck_to_address(it.operator->());
	}

#ifdef _DEBUG
#ifdef _MSC_VER
	template <typename T>
	[[nodiscard]] constexpr auto uncheck_to_address(const std::_Vector_iterator<T>& it)
	{
		return it._Ptr;
	}
	template <typename T>
	[[nodiscard]] constexpr auto uncheck_to_address(const std::_Vector_const_iterator<T>& it)
	{
		return it._Ptr;
	}
#endif
#endif


	//extension for std::initializer_list
	template <typename T>
	struct initializer_list : public std::initializer_list<T>
	{
		using std::initializer_list<T>::initializer_list;

		initializer_list(std::initializer_list<T> list)
			: std::initializer_list<T>(list)
		{
		}

		//template<class Iter>
		//initializer_list(Iter begin, Iter end)
		//	: std::initializer_list<T>(begin.operator->(), end.operator->()) {}

		initializer_list(vector<T>& vec)
			: std::initializer_list<T>(vec.data(), vec.data() + vec.size())
		{
		}

		template <size_t N>
		initializer_list(std::array<T, N>& arr)
			: std::initializer_list<T>(arr.data(), arr.data() + N)
		{
		}

#ifdef _MSC_VER //hack for MSVC in case for error, not advice to use this
		initializer_list(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end)
			: std::initializer_list<T>(
				pointer_from_vector_iterator<T>(begin),
				pointer_from_vector_iterator<T>(end))
		{
		}
#endif
	};

	template <typename... T, typename Callable, size_t... I>
	void for_each_indexed_arg_impl(Callable callable, std::index_sequence<I...>, T&&... args)
	{
		(callable(std::forward<T>(args), std::integral_constant<size_t, I>{}), ...);
	}


	template <typename... T, typename Callable>
	void for_each_arg_indexed(Callable callable, T&&... args)
	{
		for_each_indexed_arg_impl(callable, std::make_index_sequence<sizeof ...(T)>(), std::forward<T>(args)...);
	}

	template <typename Callable>
	auto for_each_arg_impl(Callable)
	{
	};

	template <typename T, typename... Ts, typename Callable>
	auto for_each_arg_impl(Callable callable, T&& arg, Ts&&... args)
	{
		static_assert(std::is_invocable_v<Callable, T>);
		using invoke_result = std::invoke_result_t<Callable, T>;
		if constexpr (std::is_void_v<invoke_result>)
		{
			callable(std::forward<T>(arg));
			for_each_arg_impl(callable, std::forward<Ts>(args)...);
		}
		else
			return callable(std::forward<T>(arg));
	}

	template <typename... T, typename Callable>
	auto for_each_arg(Callable callable, T&&... args)
	{
		return for_each_arg_impl(callable, std::forward<T>(args)...);
	}

	template <typename T, typename Ctx, typename CtxTransform>
	auto for_each_arg_acc_impl(Ctx ctx, CtxTransform ctx_transform, T&& arg)
	{
		return ctx_transform(std::forward<T>(arg), ctx);
	}

	template <typename T, typename... Ts, typename Ctx, typename CtxTransform>
	auto for_each_arg_acc_impl(Ctx ctx, CtxTransform ctx_transform,
		T&& arg, Ts&&... args)
	{
		return for_each_arg_acc_impl(
			ctx_transform(std::forward<T>(arg), ctx),
			std::forward<Ts>(args)...
		);
	}

	template <typename... T, typename Ctx, typename CtxTransform>
	auto for_each_arg_acc(Ctx ctx, CtxTransform ctx_transform,
		T&&... args)
	{
		return for_each_arg_acc_impl(ctx, ctx_transform, std::forward<T>(args)...);
	}
}


//std::get<I> for std::integer_sequence

namespace std
{
	template <size_t I, typename T, T Val, T... Vals>
	constexpr auto get_helper(std::integer_sequence<T, Val, Vals...>)
	{
		if constexpr (I == 0)
			return Val;
		else
			return get_helper<I - 1>(std::integer_sequence<T, Vals...>{});
	}

	template <size_t I, typename T, T... Vals>
	constexpr auto get(std::integer_sequence<T, Vals...> seq)
	{
		return get_helper<I>(seq);
	}
}
