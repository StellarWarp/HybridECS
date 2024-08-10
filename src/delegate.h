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

template<typename Owner,typename Target>
class bi_ref
{
    template<class U,class V>
    friend class bi_ref;

    Target* ptr = nullptr;
    bi_ref<Target, Owner>* referencer;

    void on_referencer_set(bi_ref<Target, Owner>* new_ref)
    {
        referencer = new_ref;
        ptr = new_ref->owner;
    }

    void on_referencer_remove()
    {
        referencer = nullptr;
        ptr = nullptr;
    }

public:
    bi_ref() {}
    bi_ref(bi_ref<Target, Owner>* ref) noexcept {bind(ref);}

    bi_ref(const bi_ref&) = delete;

    bi_ref(bi_ref&& other) noexcept
        : ptr(other.ptr), owner(other.owner), referencer(other.referencer)
    {
        referencer->on_referencer_set(this);
        other.on_referencer_remove();
    }
    bi_ref(Owner* new_owner, bi_ref&& other) noexcept
    : ptr(other.ptr), owner(new_owner), referencer(other.referencer)
    {
        referencer->on_referencer_set(this);
        other.on_referencer_remove();
    }
    bi_ref& operator=(bi_ref&& other) noexcept
    {
        if (this != &other)
        {
            if(referencer) referencer->on_referencer_remove();
            ptr = other.ptr;
            owner = other.owner;
            referencer = other.referencer;
            referencer->on_referencer_set(this);
            other.on_referencer_remove();
        }
        return *this;
    }

    ~bi_ref()
    {
        if(referencer) referencer->on_referencer_remove();
    }

    void bind(bi_ref<Target, Owner>* ref)
    {
        on_referencer_set(ref);
        ref->on_referencer_set(this);
    }

    void unbind()
    {
        referencer->on_referencer_remove();
        on_referencer_remove();
    }

    Target* get() const { return ptr; }

    void notify_owner_change(Owner* new_owner)
    {
        owner = new_owner;
        referencer->ptr = new_owner;
    }

    operator bool() const
    {
        return ptr != nullptr;
    }

    bool operator==(auto* p) const{ return ptr == p; }

};

class referencer_base
{
protected:
    friend class reference_handle;

    void* ptr = nullptr;

    void move_handel(reference_handle* old_handle, reference_handle* new_handle)
    {
        auto mem_shift = reinterpret_cast<size_t>(new_handle) - reinterpret_cast<size_t>(old_handle);
        ptr = reinterpret_cast<void*>(reinterpret_cast<size_t>(ptr) + mem_shift);
    }

    void remove_handel(reference_handle* h)
    {
        ptr = nullptr;
    }

};

class reference_handle
{
    template<class T>
    friend
    class referencer;

    std::set<class referencer_base*> referencing{};

public:
    reference_handle() = default;

    reference_handle(const reference_handle&) = delete;

    reference_handle(reference_handle&& other) noexcept
    {
        for (auto ref: other.referencing)
            ref->move_handel(&other, this);
    }

    ~reference_handle()
    {
        for (auto ref: referencing)
            ref->remove_handel(this);
    }

private:

    template<typename Callable>
    void for_each_referencing(Callable&& func)
    {
        for (auto h: referencing)
            func(h);
    }

    void move_handel(referencer_base* old, referencer_base* h) noexcept
    {
        referencing.erase(old);
        referencing.insert(h);
    }

    void remove_handel(referencer_base* h) noexcept
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

template<typename T>
class referencer : referencer_base
{
    T* ptr = nullptr;

    void remove_reference() noexcept
    {
        if (ptr) static_cast<reference_handle*>(ptr)->remove_handel(this);
    }

    void set_reference(T* obj) noexcept
    {
        if (obj) static_cast<reference_handle*>(obj)->add_handel(this);
        ptr = obj;
    }

    void bind(T* obj) noexcept
    {
        remove_reference();
        set_reference();
    }

public:
    referencer() = default;

    referencer(T* obj) noexcept { set_reference(obj); }

    referencer(const referencer& other) noexcept
    {
        set_reference(other.ptr);
    }

    referencer(referencer&& other) noexcept: ptr(other.ptr)
    {
        if (ptr) static_cast<reference_handle*>(ptr)->move_handel(&other, this);
        other.ptr = nullptr;
    }

    referencer& operator=(const referencer& other) noexcept
    {
        if (this == &other) return *this;
        bind(other.ptr);
        return *this;
    }

    referencer& operator=(referencer&& other) noexcept
    {
        if (this == &other) return *this;
        remove_reference();
        ptr = other.ptr;
        if (ptr) static_cast<reference_handle*>(ptr)->move_handel(&other, this);
        other.ptr = nullptr;
        return *this;
    }

    referencer& operator=(T* obj) noexcept
    {
        bind(obj);
        return *this;
    }

    ~referencer() { remove_reference(); }

    T& operator*() { return *ptr; }

    T* operator->() { return ptr; }

    explicit operator T*() { return ptr; }

    operator bool() const { return ptr != nullptr; }
};


