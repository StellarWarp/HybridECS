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


template<typename Func>
class multicast_auto_delegate;

template<typename Ret, typename... Args> requires (!std::is_rvalue_reference_v<Args> && ...)

class multicast_auto_delegate<Ret(Args...)> : public reference_handler_base<multicast_auto_delegate<Ret(Args...)>>
{
    using super = reference_handler_base<multicast_auto_delegate<Ret(Args...)>>;
    friend super;

    struct delegate_object
    {
        using invoker_t = Ret (*)(void*, Args...);
        void* ptr;
        invoker_t invoker;
    };

    using object_container_t = std::map<reference_handler_interface*, delegate_object>;

    object_container_t referencing{};

#pragma region reference_handler_interface

    template<typename Callable>
    void for_each_referencing(Callable&& func)
    {
        for (auto [h, _]: referencing)
            func(h);
    }

    void move_handel(reference_handler_interface* old, reference_handler_interface* h) noexcept override
    {
        auto it = referencing.find(old);
        const delegate_object& old_object = it->second;
        uint64_t offset = reinterpret_cast<uint64_t>(old_object.ptr) - reinterpret_cast<uint64_t>(old);
        void* new_object_ptr = reinterpret_cast<void*>(reinterpret_cast<uint64_t>(h) + offset);
        referencing.insert({h, {new_object_ptr, old_object.invoker}});
        referencing.erase(it);
    }

    void remove_handel(reference_handler_interface* h) noexcept override
    {
        referencing.erase(h);
    }

#pragma endregion

public:
    multicast_auto_delegate() = default;

    multicast_auto_delegate(const multicast_auto_delegate&) = delete;

    multicast_auto_delegate(multicast_auto_delegate&& other) noexcept = default;

    ~multicast_auto_delegate() { super::dereference_at_destory(); }


protected:

    template<typename T, auto MemFunc>
    static Ret Invoker(void* obj, Args... args)
    {
        return ((T*) (obj)->*MemFunc)(std::forward<Args>(args)...);
    }

public:

    template<auto MemFunc, typename T>
    requires std::convertible_to<T*, reference_handler_interface*>
    void bind(T* obj, var_list<MemFunc> = {})
    {
        referencing.insert({
                                   static_cast<reference_handler_interface*>(obj),
                                   {obj, Invoker<T, MemFunc>}
                           });
        obj->add_handel(this);
    }

    template<typename T, typename Callable>
    requires std::convertible_to<T*, reference_handler_interface*> && std::is_empty_v<Callable>
    void bind(T* obj, Callable&& func)
    {
        referencing.insert({
                                   static_cast<reference_handler_interface*>(obj),
                                   {obj, [](void* ptr, Args... args)
                                   {
                                       return Callable()(*(T*) ptr, std::forward<Args>(args)...);
                                   }}
                           });
        obj->add_handel(this);
    }

    template<typename T>
    requires std::convertible_to<T*, reference_handler_interface*>
    void unbind(T* obj)
    {
        remove_handel(obj);
        obj->remove_handel(this);
    }

    //no need to forward as the parameters types are already defined
    void invoke(Args... args) requires std::same_as<Ret, void>
    {
        for (auto& [_, obj]: referencing)
            obj.invoker(obj.ptr, std::forward<Args>(args)...);
    }

    void operator()(Args... args) requires std::same_as<Ret, void>
    {
        invoke(std::forward<Args>(args)...);
    }

    template<typename Callable>
    requires (!std::same_as<Ret, void>)
    void invoke(Args... args, Callable&& result_proc)
    {
        class iteratable
        {
            using object_iterator_t = object_container_t::iterator;
            const object_iterator_t begin_it;
            const object_iterator_t end_it;
            const std::tuple<Args && ...> args_tuple;
        public:
            explicit iteratable(object_container_t& objects, Args... args) :
                    begin_it(objects.begin()), end_it(objects.end()),
                    args_tuple(std::forward_as_tuple(std::forward<Args>(args)...)) {}

            class iterator
            {
            public:
                using iterator_category = std::input_iterator_tag;
                using difference_type = std::ptrdiff_t;
                using value_type = Ret;
                using pointer = Ret*;
                using reference = Ret&;
            private:
                const iteratable& info;
                object_iterator_t it;
            public:
                iterator(const iteratable& info) : info(info), it(info.begin_it) {}

                Ret operator*()
                {
                    auto& obj = it->second;
                    return [&]<size_t... I>(const std::tuple<Args && ...>& args_1,
                                            std::integer_sequence<std::size_t, I...>)
                    {
                        return [&](Args&& ... args_2)
                        {
                            return obj.invoker(obj.ptr, std::forward<Args>(args_2)...);
                        }(std::forward<Args>(std::get<I>(args_1))...);
                    }(info.args_tuple, std::make_index_sequence<sizeof...(Args)>{});
                }

                void operator++() { ++it; }

                void operator++(int) { ++it; }

                bool operator==(nullptr_t)
                {
                    return it == info.end_it;
                }
            };

            iterator begin() { return iterator(*this); }

            nullptr_t end() { return {}; }
        };

        result_proc(iteratable(referencing, std::forward<Args>(args)...));
    }
};


template<typename Func>
class multicast_weak_delegate;

template<typename Ret, typename... Args> requires (!std::is_rvalue_reference_v<Args> && ...)

class multicast_weak_delegate<Ret(Args...)>
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

    struct delegate_object
    {
        std::weak_ptr<generic_class> ptr;
        mem_func_t mem_fn;
    };

    std::vector<delegate_object> referencing;


public:

    template<typename T>
    void bind(const std::shared_ptr<T>& obj, Ret(T::*mem_func)(Args...))
    {
        referencing.emplace_back(
                std::reinterpret_pointer_cast<generic_class>(obj),
                reinterpret_cast<mem_func_t>(mem_func));
    }

    template<typename T>
    void bind(std::weak_ptr<T>&& obj, Ret(T::*mem_func)(Args...))
    {
        bind(obj.lock(), mem_func);
    }


    template<typename T, typename Callable>
    requires std::is_empty_v<Callable>
    void bind(const std::shared_ptr<T>& obj, Callable&& func)
    {
        referencing.emplace_back(
                std::reinterpret_pointer_cast<generic_class>(obj),
                &generic_class::template Invoker<T, Callable>);
    }

    template<typename T, typename Callable>
    requires std::is_empty_v<Callable>
    void bind(std::weak_ptr<T>&& obj, Callable&& func)
    {
        bind(obj.lock(), func);
    }

    void unbind(std::weak_ptr<void> obj)
    {
        for (int i = 0; i < referencing.size(); ++i)
        {
            if (referencing[i].ptr == obj)
            {
                referencing[i] = referencing.back();
                referencing.pop_back();
                break;
            }
        }
    }

public:
    //no need to forward as the parameters types are already defined
    void invoke(Args... args) requires std::same_as<Ret, void>
    {
        auto begin = referencing.begin();
        auto end = referencing.end();
        auto iter = begin;
        while (iter != end)
        {
            auto& [w_ptr, mem_fn] = *iter;
            if (w_ptr.expired())
            {
                end--;
                *iter = *end;
                continue;
            }
            (w_ptr.lock().get()->*mem_fn)(std::forward<Args>(args)...);
            ++iter;
        }
        referencing.resize(std::distance(begin, end));
    }

    void operator()(Args... args) requires std::same_as<Ret, void>
    {
        invoke(std::forward<Args>(args)...);
    }

    template<typename Callable>
    requires (!std::same_as<Ret, void>)
    void invoke(Args... args, Callable&& result_proc)
    {
        class iteratable
        {
            using object_container_t = std::vector<delegate_object>;
            using object_iterator_t = object_container_t::iterator;
            object_container_t& objects;
            const std::tuple<Args && ...> args_tuple;
        public:
            explicit iteratable(object_container_t& objects, Args... args) :
                    objects(objects),
                    args_tuple(std::forward_as_tuple(std::forward<Args>(args)...)) {}

            class iterator
            {
            public:
                using iterator_category = std::input_iterator_tag;
                using difference_type = std::ptrdiff_t;
                using value_type = Ret;
                using pointer = Ret*;
                using reference = Ret&;
            private:
                const iteratable& info;
                object_iterator_t it;
                object_iterator_t end_it;

                void get_valid_iterator()
                {
                    while (it->ptr.expired())
                    {
                        end_it--;
                        if (it == end_it) return;
                        *it = *end_it;
                    }
                }

            public:
                iterator(const iteratable& info) :
                        info(info),
                        it(info.objects.begin()),
                        end_it(info.objects.end()) { get_valid_iterator(); }

                Ret operator*()
                {
                    auto& [w_ptr, mem_fn] = *it;
                    return [&]<size_t... I>(const std::tuple<Args && ...>& args_1,
                                            std::integer_sequence<std::size_t, I...>)
                    {
                        return [&](Args&& ... args_2)
                        {
                            return (w_ptr.lock().get()->*mem_fn)(std::forward<Args>(args_2)...);
                        }(std::forward<Args>(std::get<I>(args_1))...);
                    }(info.args_tuple, std::make_index_sequence<sizeof...(Args)>{});
                }

                void operator++()
                {
                    ++it;
                    get_valid_iterator();
                }

                void operator++(int) { operator++(); }

                bool operator==(nullptr_t)
                {
                    if (it == end_it)
                    {
                        info.objects.resize(end_it - info.objects.begin());
                        return true;
                    }
                    return false;
                }
            };

            iterator begin() { return iterator(*this); }

            nullptr_t end() { return {}; }
        };

        result_proc(iteratable(referencing, std::forward<Args>(args)...));
    }
};
