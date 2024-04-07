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


	//hack
	template<typename T>
	T* pointer_from_vector_iterator(typename vector<T>::iterator& it)
	{
#ifdef HYECS_USING_STD_VECTOR
#ifdef _DEBUG
#ifdef _MSC_VER
		return *((T**)&(it)+2);
#endif
#endif
#endif
		return it.operator->();
	}

	template<typename T>
	const T* pointer_from_vector_iterator(typename vector<T>::const_iterator& it)
	{
#ifdef HYECS_USING_STD_VECTOR
#ifdef _DEBUG
#ifdef _MSC_VER
		return *((T**)&(it)+2);
#endif
#endif
#endif
		return it.operator->();
	}

	//extension for std::initializer_list
	template<typename T>
	struct initializer_list :public std::initializer_list<T> {

		using std::initializer_list<T>::initializer_list;

		initializer_list(std::initializer_list<T> list)
			:std::initializer_list<T>(list) {}

		//template<class Iter>
		//initializer_list(Iter begin, Iter end)
		//	: std::initializer_list<T>(begin.operator->(), end.operator->()) {}

		initializer_list(vector<T>& vec)
			: std::initializer_list<T>(vec.data(), vec.data() + vec.size()) {}

		template<size_t N>
		initializer_list(std::array<T, N>& arr)
			: std::initializer_list<T>(arr.data(), arr.data() + N) {}

#ifdef _MSC_VER //hack for MSVC in case for error, not advice to use this
		initializer_list(typename std::vector<T>::iterator begin, typename std::vector<T>::iterator end)
			: std::initializer_list<T>(
				pointer_from_vector_iterator<T>(begin),
				pointer_from_vector_iterator<T>(end)) {}
#endif
	};



	}
