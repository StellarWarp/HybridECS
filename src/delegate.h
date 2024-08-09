#pragma once

#include <vector>
#include <concepts>
#include "function_traits.h"
#include <set>
#include <map>
#include <assert.h>
#include <ranges>


template<auto...var>
class var_list
{
};


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
        mem_fn = (mem_func_t) mem_fn;
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

class reference_handler_interface
{
    template<class T>
    friend
    class reference_handler_base;

protected:
    virtual void move_handel(reference_handler_interface* old, reference_handler_interface* h) noexcept = 0;

    virtual void remove_handel(reference_handler_interface* h) noexcept = 0;
};

template<typename Derived>
class reference_handler_base : public reference_handler_interface
{

#ifndef NDEBUG
    bool has_dereferenced = false;
#endif
public:
    reference_handler_base() = default;

    reference_handler_base(const reference_handler_base&) = delete;

    reference_handler_base(reference_handler_base&& other) noexcept
    {
        static_cast<Derived*>(&other)->for_each_referencing([&](reference_handler_interface* h)
                                                            {
                                                                h->move_handel(&other, this);
                                                            });
    }


    ~reference_handler_base() { assert(has_dereferenced); }

protected:
    void dereference() noexcept
    {
        static_cast<Derived*>(this)->for_each_referencing([&](reference_handler_interface* h)
                                                          {
                                                              h->remove_handel(this);
                                                          });
    }

    void dereference_at_destory() noexcept
    {
#ifndef NDEBUG
        has_dereferenced = true;
#endif
        static_cast<Derived*>(this)->for_each_referencing([&](reference_handler_interface* h)
                                                          {
                                                              h->remove_handel(this);
                                                          });
    }
};

class reference_handle : public reference_handler_base<reference_handle>
{
    friend class reference_handler_base<reference_handle>;

public:
    reference_handle() = default;

    reference_handle(const reference_handle&) = delete;

    reference_handle(reference_handle&& other) noexcept = default;

    ~reference_handle() { dereference_at_destory(); }

private:
    std::set<reference_handler_interface*> referencing{};

    template<typename Callable>
    void for_each_referencing(Callable&& func)
    {
        for (auto h: referencing)
            func(h);
    }

    void move_handel(reference_handler_interface* old, reference_handler_interface* h) noexcept override
    {
        referencing.erase(old);
        referencing.insert(h);
    }

    void remove_handel(reference_handler_interface* h) noexcept override
    {
        referencing.erase(h);
    }

public:
    template<typename T>
    void add_handel(T* obj)
    {
        referencing.insert(obj);
    }
};

template<typename T> requires std::is_convertible_v<T*, reference_handle*>
class referencer : public reference_handler_base<referencer<T>>
{
    using super = reference_handler_base<referencer<T>>;
    friend super;
    T* ptr = nullptr;

    template<class Callable>
    void for_each_referencing(Callable&& func)
    {
        if (ptr) func(static_cast<reference_handle*>(ptr));
    }

    void move_handel(reference_handler_interface* old, reference_handler_interface* h) noexcept override
    {
        constexpr auto offset = reinterpret_cast<size_t>(static_cast<reference_handler_interface*>((T*) 0));
        ptr = reinterpret_cast<T*>(reinterpret_cast<size_t>(h) - offset);
    }

    void remove_handel(reference_handler_interface* h) noexcept override
    {
        ptr = nullptr;
    }

    void bind(T* obj)
    {
        if (ptr) super::dereference();
        ptr = obj;
        if (obj) static_cast<reference_handle*>(obj)->add_handel(this);
    }

public:
    referencer() = default;

    referencer(T* obj) { bind(obj); }

    referencer(const referencer& other)
    {
        bind(other.ptr);
    }

    referencer(referencer&& other) :
            super(std::move(other)),
            ptr(other.ptr) { other.ptr = nullptr; }

    referencer& operator=(const referencer& other) noexcept
    {
        if (this == &other) return *this;
        bind(other.ptr);
        return *this;
    }

    referencer& operator=(referencer&& other) noexcept
    {
        if (this == &other) return *this;
        bind(other.ptr);
        other.dereference();
        return *this;
    }

    referencer& operator=(T* obj) noexcept
    {
        bind(obj);
        return *this;
    }

    ~referencer() { super::dereference_at_destory(); }

    T& operator*() { return *ptr; }

    T* operator->() { return ptr; }

    explicit operator T*() { return ptr; }

    operator bool() const { return ptr != nullptr; }
};



