#pragma once
#include <tuple>

namespace auto_delegate
{
    template<typename...>
    struct function_traits;

    // base
    template<typename ReturnType, typename... Args>
    struct function_traits<ReturnType(Args...)>
    {
        enum { arity = sizeof...(Args) };
        using return_type = ReturnType;
        using function_type = ReturnType(Args...);
        using decay_function_type = ReturnType(Args...);
        using pointer = ReturnType(*)(Args...);

        using args = std::tuple<Args...>;

        using tuple_type = std::tuple<std::remove_cv_t<std::remove_reference_t < Args>>...>;
        using bare_tuple_type = std::tuple <std::remove_const_t<std::remove_reference_t < Args>>...>;
    };

    // function pointer
    template<typename ReturnType, typename... Args>
    struct function_traits<ReturnType(*)(Args...)> : function_traits<ReturnType(Args...)> {};

    // member function pointer
    template<typename ReturnType, typename ClassType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...)> : function_traits<ReturnType(Args...)>
    {
        using class_type = ClassType;
        using member_function_type = ReturnType(ClassType::*)(Args...);
        using decay_member_function_type = ReturnType(ClassType::*)(Args...);
        using decay_function_type = ReturnType(Args...);
    };

    //clang-format off
    #define DECLARE_MEMBER_FUNCTION_TRAITS(specifier) \
    template <typename ReturnType, typename ClassType, typename... Args>\
    struct function_traits<ReturnType(ClassType::*)(Args...) specifier> : function_traits<ReturnType(ClassType::*)(Args...)> { \
            using function_type = ReturnType(Args...) specifier;\
            using member_function_type = ReturnType(ClassType::*)(Args...) specifier;\
    }

    DECLARE_MEMBER_FUNCTION_TRAITS(const                     );
    DECLARE_MEMBER_FUNCTION_TRAITS(      volatile            );
    DECLARE_MEMBER_FUNCTION_TRAITS(                  noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(const             noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(      volatile    noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(const volatile            );
    DECLARE_MEMBER_FUNCTION_TRAITS(const volatile    noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(               &          );
    DECLARE_MEMBER_FUNCTION_TRAITS(const          &          );
    DECLARE_MEMBER_FUNCTION_TRAITS(      volatile &          );
    DECLARE_MEMBER_FUNCTION_TRAITS(               &  noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(const          &  noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(      volatile &  noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(const volatile &          );
    DECLARE_MEMBER_FUNCTION_TRAITS(const volatile &  noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(               &&         );
    DECLARE_MEMBER_FUNCTION_TRAITS(const          &&         );
    DECLARE_MEMBER_FUNCTION_TRAITS(      volatile &&         );
    DECLARE_MEMBER_FUNCTION_TRAITS(               && noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(const          && noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(      volatile && noexcept);
    DECLARE_MEMBER_FUNCTION_TRAITS(const volatile &&         );
    DECLARE_MEMBER_FUNCTION_TRAITS(const volatile && noexcept);

    #undef DECLARE_MEMBER_FUNCTION_TRAITS
    //clang-format on

    // callable object
    template<typename Callable> requires requires { &Callable::operator(); }
    struct function_traits<Callable> : function_traits<decltype(&Callable::operator())> {};
}

