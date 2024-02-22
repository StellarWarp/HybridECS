#pragma once
#include "function_traits.h"
#include "type_indexed_array.h"

namespace hyecs
{

	template<typename... Ts, typename T>
	constexpr auto tuple_append(std::tuple<Ts...> t1, T arg)
	{
		return std::tuple_cat(t1, std::tuple<T>(arg));
	}

	template<typename T, typename... Ts, size_t... I>
	constexpr auto tuple_remove(T t, std::index_sequence<I...>)
	{
		return std::tuple<Ts...>(std::get<I + 1>(t)...);
	}

	template<typename... Ts, typename T>
	constexpr auto tuple_remove(std::tuple<T, Ts...> t)
	{
		return tuple_remove<std::tuple<T, Ts...>, Ts...>(t, std::make_index_sequence<sizeof...(Ts)>{});
	}

	template<typename... Ts, typename...Us>
	constexpr auto tuple_trans(std::tuple<Ts...> t1, std::tuple<Us...> arg)
	{
		return std::tuple_cat(t1,
			std::tuple<std::tuple_element_t<0, decltype(arg)>>(std::get<0>(arg)));
	}

	template<template <typename> typename Condition, typename T, typename... Ts, typename... Res>
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

	template<template <typename> typename Condition, typename... T>
	constexpr auto filter_args(T... args)
	{
		return filter_args<Condition>(std::tuple<>{}, args...);
	}

	
#if defined(_DEBUG) || (!defined(NDEBUG) && !defined(_NDEBUG))
#define ASSERTION_CODE(...) __VA_ARGS__
#define HYECS_DEBUG
#else
#define ASSERTION_CODE(...)
#endif 
#define ASSERTION_CODE_BEGIN() ASSERTION_CODE(
#define ASSERTION_CODE_END() )



}
