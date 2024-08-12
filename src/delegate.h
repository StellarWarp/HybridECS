#pragma once

#include "auto_reference.h"
#include "function_traits.h"
#include <concepts>


template<typename Func, typename GenericPtr = void*>
class delegate;

//reduce guide
template<typename GenericPtr, typename T_MemFunc>
delegate(GenericPtr, T_MemFunc) ->
delegate<typename function_traits<T_MemFunc>::function_type, GenericPtr>;

template<typename Ret, typename... Args, typename GenericPtr>
class delegate<Ret(Args...), GenericPtr>
{
    struct generic_class
    {
        template<typename T, auto Callable>
        Ret Invoker(Args... args)
        {
            return Callable(*reinterpret_cast<T*>(this), std::forward<Args>(args)...);
        }
    };

    using mem_func_t = Ret(generic_class::*)(Args...);
    GenericPtr ptr;
    mem_func_t mem_fn;

    template<class T, class T_ptr>
    static constexpr bool is_pointer_of = std::same_as<typename std::pointer_traits<T_ptr>::element_type, T>;

public:
    using function_type = Ret(Args...);

    delegate() : ptr(), mem_fn(nullptr) {}

    template<typename T_ptr, typename T>
    requires is_pointer_of<T, T_ptr>
    delegate(const T_ptr& obj, Ret(T::*mem_fn)(Args...)) : ptr(obj), mem_fn((mem_func_t) mem_fn) {}

    template<typename T_ptr, typename T>
    requires is_pointer_of<T, T_ptr>
    void bind(const T_ptr& obj, Ret(T::*mem_func)(Args...))
    {
        ptr = obj;
        mem_fn = (mem_func_t) mem_func;
    }

    template<typename T_ptr, typename Callable>
    requires std::is_empty_v<Callable>
    void bind(const T_ptr& obj, Callable&& func)
    {
        using T = typename std::pointer_traits<T_ptr>::element_type;
        ptr = obj;
        mem_fn = &generic_class::template Invoker<T, Callable{}>;
    }

    template<typename T_ptr>
    void bind(const T_ptr& callable)
    requires std::same_as<
            std::invoke_result_t<std::decay_t<decltype(*callable)>, Args...>,
            Ret>
    {
        ptr = callable;
        mem_fn = (mem_func_t) &std::decay_t<decltype(*callable)>::operator();
    }

    template<typename Callable>
    void bind(Callable&& callable)
    requires std::same_as<std::invoke_result_t<std::decay_t<Callable>, Args...>, Ret>
             && std::is_empty_v<std::decay_t<Callable>>
    {
        if constexpr (requires { ptr = nullptr; })
            ptr = nullptr;
        else
            ptr.reset();
        mem_fn = (mem_func_t) &std::decay_t<Callable>::operator();
    }

    operator bool() const { return ptr != nullptr; }

    Ret invoke(Args... args)
    {
        if constexpr (requires { static_cast<generic_class*>(ptr); })
            return (static_cast<generic_class*>(ptr)->*mem_fn)(std::forward<Args>(args)...);
        else if constexpr (requires { ptr.get(); })
            return (reinterpret_cast<generic_class*>(ptr.get())->*mem_fn)(std::forward<Args>(args)...);
        else if constexpr (requires { ptr.lock().get(); })
            return (reinterpret_cast<generic_class*>(ptr.lock().get())->*mem_fn)(std::forward<Args>(args)...);
        else
            static_assert(!std::same_as<int, int>);
    }

    Ret operator()(Args... args)
    {
        return invoke(std::forward<Args>(args)...);
    }
};


template<typename Func>
using auto_delegate = delegate<Func, weak_reference<generic_ref_reflector, generic_ref_reflector>>;
template<typename Func>
using weak_delegate = delegate<Func, std::weak_ptr<void>>;
template<typename Func>
using shared_delegate = delegate<Func, std::shared_ptr<void>>;