#pragma once

#include "auto_reference.h"
#include "function_traits.h"
#include <concepts>


template<typename Func>
class delegate;

//reduce guide
template<typename T_MemFunc>
delegate(typename function_traits<T_MemFunc>::class_type*,
         T_MemFunc) -> delegate<typename function_traits<T_MemFunc>::function_type>;

template<typename Ret, typename... Args>
class delegate<Ret(Args...)>
{
    struct generic_class
    {
        template<typename T, typename Lambda>
        Ret Invoker(Args... args)
        {
            return std::decay_t<Lambda>{}(*reinterpret_cast<T*>(this), std::forward<Args>(args)...);
        }
    };

    using mem_func_t = Ret(generic_class::*)(Args...);
    generic_class* ptr;
    mem_func_t mem_fn;

public:
    using function_type = Ret(Args...);

    delegate() : ptr(nullptr), mem_fn(nullptr) {}

    template<typename T>
    delegate(T* obj, Ret(T::*mem_fn)(Args...)) : ptr((generic_class*) obj), mem_fn((mem_func_t) mem_fn) {}

    template<typename T>
    void bind(T* obj, Ret(T::*mem_func)(Args...))
    {
        ptr = reinterpret_cast<generic_class*>(obj);
        mem_fn = (mem_func_t) mem_func;
    }

    template<typename T, typename Callable>
    requires std::is_empty_v<Callable>
    void bind(T* obj, Callable&& func)
    {
        ptr = reinterpret_cast<generic_class*>(obj);
        mem_fn = &generic_class::template Invoker<T, Callable>;
    }

    template<typename Callable>
    requires std::same_as<std::invoke_result_t<Callable, Args...>, Ret>
    void bind(Callable&& callable) requires std::is_lvalue_reference_v<Callable> || std::is_empty_v<Callable>
    {
        if constexpr (std::is_empty_v<Callable>)
            ptr = nullptr;
        else
            ptr = reinterpret_cast<generic_class*>(&callable);
        mem_fn = (mem_func_t) &std::decay_t<Callable>::operator();
    }

    operator bool() const { return ptr != nullptr; }

    Ret invoke(Args... args)
    {
        return (ptr->*mem_fn)(std::forward<Args>(args)...);
    }

    Ret operator()(Args... args)
    {
        return invoke(std::forward<Args>(args)...);
    }
};


template<typename Func>
class auto_delegate;

//reduce guide
template<typename T_MemFunc>
auto_delegate(typename function_traits<T_MemFunc>::class_type*,
              T_MemFunc) -> auto_delegate<typename function_traits<T_MemFunc>::function_type>;

template<typename Ret, typename... Args>
class auto_delegate<Ret(Args...)>
{
    struct generic_class
    {
        template<typename T, typename Lambda>
        Ret Invoker(Args... args)
        {
            return std::decay_t<Lambda>{}(*reinterpret_cast<T*>(this), std::forward<Args>(args)...);
        }
    };


    using mem_func_t = Ret(generic_class::*)(Args...);

    struct object_referencer : referencer_interface
    {
        void notify_reference_removed(void* reference_handel_address) override {};
        ref_handle<array_ref_protocol , true> handle;

        template<class T>
        void bind(T* obj)
        {
            if(obj == handle.cast<T>()) return;
            auto charger = static_cast<array_ref_charger<false>*>(obj);
            assert(charger == obj);
            auto h = charger->new_bind_handle();
            handle.bind(this, charger, h);
            assert(h->get() == this);
        }

        void unbind() { handle.unbind(); }

        operator bool() const { return handle; }

        generic_class* get() { return handle.cast<generic_class>(); }
    };

    object_referencer ptr;
    mem_func_t mem_fn;

public:
    using function_type = Ret(Args...);

    auto_delegate() : ptr(), mem_fn(nullptr) {}

    template<typename T>
    auto_delegate(T* obj, Ret(T::*mem_fn)(Args...)) : ptr((generic_class*) obj), mem_fn((mem_func_t) mem_fn) {}

    template<typename T>
    void bind(T* obj, Ret(T::*mem_func)(Args...))
    {
        ptr.bind(obj);
        mem_fn = (mem_func_t) mem_func;
    }

    template<typename T, typename Callable>
    requires std::is_empty_v<Callable>
    void bind(T* obj, Callable&& func)
    {
        ptr.bind(obj);
        mem_fn = &generic_class::template Invoker<T, Callable>;
    }

    template<typename Callable>
    requires std::same_as<std::invoke_result_t<Callable, Args...>, Ret> &&
             std::derived_from<std::decay_t<Callable>, array_ref_charger<false>>
    void bind(Callable&& callable) requires std::is_lvalue_reference_v<Callable> || std::is_empty_v<Callable>
    {
        if constexpr (std::is_empty_v<Callable>)
            ptr.unbind();
        else
            ptr.bind(&callable);
        mem_fn = (mem_func_t) &std::decay_t<Callable>::operator();
    }

    operator bool() const { return ptr; }

    Ret invoke(Args... args)
    {
        return (ptr.get()->*mem_fn)(std::forward<Args>(args)...);
    }

    Ret operator()(Args... args)
    {
        return invoke(std::forward<Args>(args)...);
    }
};
