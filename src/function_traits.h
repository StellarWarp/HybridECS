#pragma once
#include "type_list.h"

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

    using args = type_list< Args...>;

    using tuple_type = std::tuple<std::remove_cv_t<std::remove_reference_t<Args>>...>;
    using bare_tuple_type = std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...>;
};

// function pointer
template<typename ReturnType, typename... Args>
struct function_traits<ReturnType(*)(Args...)> : function_traits<ReturnType(Args...)> {};

// member function pointer
template <typename ReturnType, typename ClassType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)> : function_traits<ReturnType(Args...)> {
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



// A const-qualified function type means the user is trying to disambiguate
// a member function pointer.
template <class Fun> // Fun = R(As...) const
struct Sig {
    template <class T>
    constexpr Fun T::* operator()(Fun T::* t) const /* nolint */ volatile noexcept {
        return t;
    }
    template <class F, class T>
    constexpr F T::* operator()(F T::* t) const /* nolint */ volatile noexcept {
        return t;
    }
};

// A function type with no arguments means the user is trying to disambiguate
// a member function pointer.
template <class R>
struct Sig<R()> : Sig<R() const> {
    using Fun = R();
    using Sig<R() const>::operator();

    template <class T>
    constexpr Fun T::* operator()(Fun T::* t) const noexcept {
        return t;
    }
};

template <class R, class... As>
struct SigImpl : Sig<R(As...) const> {
    using Fun = R(As...);
    using Sig<R(As...) const>::operator();

    template <class T>
    constexpr Fun T::* operator()(Fun T::* t) const noexcept {
        return t;
    }
    constexpr Fun* operator()(Fun* t) const noexcept { return t; }
    template <class F>
    constexpr F* operator()(F* t) const noexcept {
        return t;
    }
};

// The user could be trying to disambiguate either a member or a free function.
template <class R, class... As>
struct Sig<R(As...)> : SigImpl<R, As...> {};

// This case is like the one above, except we want to add an overload that
// handles the case of a free function where the first argument is more
// const-qualified than the user explicitly specified.
template <class R, class A, class... As>
struct Sig<R(A&, As...)> : SigImpl<R, A&, As...> {
    using CCFun = R(A const&, As...);
    using SigImpl<R, A&, As...>::operator();

    constexpr CCFun* operator()(CCFun* t) const /* nolint */ volatile noexcept {
        return t;
    }
};