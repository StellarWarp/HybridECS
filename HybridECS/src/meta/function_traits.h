#pragma once

#include "type_list.h"
namespace hyecs
{
	// 原型
	template<typename T>
	struct function_traits;

	// 普通函数
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

	// 函数指针
	template<typename ReturnType, typename... Args>
	struct function_traits<ReturnType(*)(Args...)> : function_traits<ReturnType(Args...)> {};

	// std::function
	template<typename ReturnType, typename... Args>
	struct function_traits<std::function<ReturnType(Args...)>> : function_traits<ReturnType(Args...)> {};

	// 成员函数
#define FUNCTION_TRAITS(...)\
template <typename ReturnType, typename ClassType, typename... Args>\
struct function_traits<ReturnType(ClassType::*)(Args...) __VA_ARGS__> : function_traits<ReturnType(Args...)>{};\

	FUNCTION_TRAITS()
	FUNCTION_TRAITS(const)
	FUNCTION_TRAITS(volatile)
	FUNCTION_TRAITS(const volatile)

	// 函数对象
	template<typename Callable>
	struct function_traits : function_traits<decltype(&Callable::operator())> {};
}

