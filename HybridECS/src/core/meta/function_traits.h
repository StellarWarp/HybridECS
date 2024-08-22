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
        using pointer = ReturnType(*)(Args...);

        using args = type_list<Args...>;

        using tuple_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
        using bare_tuple_type = std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...>;
    };

    // function pointer
    template<typename ReturnType, typename... Args>
    struct function_traits<ReturnType(*)(Args...)> : function_traits<ReturnType(Args...)> {};

    // member function pointer
    template<typename ReturnType, typename ClassType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...)> : function_traits<ReturnType(Args...)>
    {
        using class_type = ClassType;
    };

#define DECLARE_MEMBER_FUNCTION_TRAITS(type) \
template <typename ReturnType, typename ClassType, typename... Args>\
struct function_traits<ReturnType(ClassType::*)(Args...) type> : function_traits<ReturnType(ClassType::*)(Args...)> {};

    DECLARE_MEMBER_FUNCTION_TRAITS(const);
    DECLARE_MEMBER_FUNCTION_TRAITS(noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(volatile);
    DECLARE_MEMBER_FUNCTION_TRAITS(const volatile);
    DECLARE_MEMBER_FUNCTION_TRAITS(const noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(volatile noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(const volatile noexcept);

#undef DECLARE_MEMBER_FUNCTION_TRAITS

    // callable object
    template<typename Callable>
    struct function_traits : function_traits<decltype(&Callable::operator())> {};
}

