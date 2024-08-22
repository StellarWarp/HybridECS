#pragma once

#include "type_list.h"
namespace hyecs
{
	template<typename T>
	struct function_traits;

	// base
	template<typename ReturnType, typename... Args>
	struct function_traits<ReturnType(Args...)>
	{
		enum { arity = sizeof...(Args) };
		using return_type = ReturnType;
		using function_type = ReturnType(Args...);
		using stl_function_type = std::function<function_type>;
		using pointer = ReturnType(*)(Args...);

		using args = type_list< Args...>;

		using tuple_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
		using bare_tuple_type = std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...>;
	};

	// function pointer
	template<typename ReturnType, typename... Args>
	struct function_traits<ReturnType(*)(Args...)> : function_traits<ReturnType(Args...)> {};

	// std::function
	template<typename ReturnType, typename... Args>
	struct function_traits<std::function<ReturnType(Args...)>> : function_traits<ReturnType(Args...)> {};

	template <typename ReturnType, typename ClassType, typename... Args>
	struct function_traits<ReturnType(ClassType::*)(Args...) > : function_traits<ReturnType(Args...)> {};
	template <typename ReturnType, typename ClassType, typename... Args>
	struct function_traits<ReturnType(ClassType::*)(Args...) const> : function_traits<ReturnType(Args...)> {};
	template <typename ReturnType, typename ClassType, typename... Args>
	struct function_traits<ReturnType(ClassType::*)(Args...) volatile> : function_traits<ReturnType(Args...)> {};
	template <typename ReturnType, typename ClassType, typename... Args>
	struct function_traits<ReturnType(ClassType::*)(Args...) const volatile> : function_traits<ReturnType(Args...)> {};

	// callable object
	template<typename Callable>
	struct function_traits : function_traits<decltype(&Callable::operator())> {};
}

