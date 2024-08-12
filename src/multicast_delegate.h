#pragma once

#include <memory>
#include <ranges>
#include "auto_reference.h"


template<typename MemFunc>
class default_delegate_container
{

    struct delegate_object
    {
        void* ptr;
        MemFunc mem_fn;
    };

    std::vector<delegate_object> objects;

public:

    //    template<class T>
    MemFunc& bind(void* obj)
    {
        auto& res = objects.emplace_back(obj, nullptr);
        return res.mem_fn;
    }
    //    MemFunc& bind(nullptr_t)
    //    {
    //        auto&res = objects.emplace_back({}, nullptr);
    //        return res.mem_fn;
    //    }

    void unbind(void* obj)
    {
        for (int i = 0; i < objects.size(); ++i)
        {
            if (objects[i].ptr == obj)
            {
                objects[i] = std::move(objects.back());
                objects.pop_back();
                return;
            }
        }
        assert(false);
    }

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::tuple<void*, MemFunc>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
    private:
        std::vector<delegate_object>::iterator it;
        std::vector<delegate_object>::iterator end_it;

    public:
        iterator(std::vector<delegate_object>& vec)
                : it(vec.begin()), end_it(vec.end()) {}

        void operator++() { ++it; }

        void operator++(int) { operator++(); }

        value_type operator*() { return value_type(it->ptr, it->mem_fn); }

        bool operator==(const iterator& other) { return it == other.it; }

        bool operator==(std::default_sentinel_t)
        {
            return it == end_it;
        }
    };

    iterator begin() { return iterator(objects); }

    std::default_sentinel_t end() { return {}; }
};


template<typename Func, template<typename> typename DelegateContainer = default_delegate_container>
class multicast_delegate;

template<template<typename> typename DelegateContainer, typename Ret, typename... Args>
requires (!std::is_rvalue_reference_v<Args> && ...)

class multicast_delegate<Ret(Args...), DelegateContainer>
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

    using object_container_t = DelegateContainer<mem_func_t>;

    object_container_t referencing;

public:
    multicast_delegate() = default;

    multicast_delegate(const multicast_delegate&) = delete;

    multicast_delegate(multicast_delegate&& other) noexcept = default;

public:

    template<typename T, typename T_ptr>
    requires std::same_as<typename std::pointer_traits<T_ptr>::element_type, T>
    void bind(const T_ptr& obj, Ret(T::* mem_func)(Args...))
    {
        auto& fn = referencing.bind(obj);
        fn = (mem_func_t) mem_func;
    }

    template<typename T_ptr, typename Callable>
    requires std::is_empty_v<Callable>
    void bind(const T_ptr& obj, Callable&& func)
    {
        using T = typename std::pointer_traits<T_ptr>::element_type;
        auto& fn = referencing.bind(obj);
        fn = &generic_class::template Invoker<T, Callable>;
    }

    template<typename T_ptr>
    void
    bind(const T_ptr& callable)requires std::same_as<std::invoke_result_t<std::decay_t<decltype(*callable)>, Args...>, Ret>
    {
        auto& fn = referencing.bind(callable);
        fn = (mem_func_t) &std::decay_t<decltype(*callable)>::operator();
    }

    //unable to unbind from nullptr so not support for static function bind
//    template<typename Callable>
//    void bind(Callable&& callable)
//    requires std::same_as<std::invoke_result_t<std::decay_t<Callable>, Args...>, Ret>
//             && std::is_empty_v<std::decay_t<Callable>>
//    {
//        auto& fn = referencing.bind(nullptr);
//        fn = (mem_func_t) &std::decay_t<Callable>::operator();
//    }

    template<typename T_ptr>
    void unbind(const T_ptr& obj) requires requires{ typename std::pointer_traits<T_ptr>; }
    {
        referencing.unbind(obj);
    }

private:
    static Ret invoke_single(const auto& ptr, mem_func_t mem_fn, Args... args)
    {
        generic_class* c;
        if constexpr (requires { static_cast<generic_class*>(ptr); })
            c = static_cast<generic_class*>(ptr);
        else if constexpr (requires { ptr.get(); })
            c = reinterpret_cast<generic_class*>(ptr.get());
        else if constexpr (requires { ptr.lock().get(); })
        {
            return (reinterpret_cast<generic_class*>(ptr.lock().get())
                    ->*mem_fn)(std::forward<Args>(args)...);
        } else
            static_assert(!std::same_as<int, int>);
        //union
        //{
        //	mem_func_t fn;
        //	void* mem_fn_address;
        //}u;
        //u.fn = mem_fn;
        //std::cout << "ptr " << c << " mem_fn " << u.mem_fn_address << std::endl;
        return (c->*mem_fn)(std::forward<Args>(args)...);
    }

public:
    //no need to forward as the parameters types are already defined
    void invoke(Args... args) requires std::same_as<Ret, void>
    {
        for (auto [obj, mem_fn]: referencing)
        {
            invoke_single(obj, mem_fn, std::forward<Args>(args)...);
        }
    }

    void operator()(Args... args) requires std::same_as<Ret, void>
    {
        invoke(std::forward<Args>(args)...);
    }

private:
    template<typename Invoker>
    class iterable
    {
        using object_iterator_t = object_container_t::iterator;
        using sentinel_t = decltype(std::declval<object_container_t&>().end());
        const object_iterator_t begin_it;
        const sentinel_t end_it;
        const Invoker& invoker;
    public:
        iterable(object_container_t& objects, const Invoker& invoker) :
                begin_it(objects.begin()), end_it(objects.end()),
                invoker(invoker) {}

        class iterator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Ret;
            using pointer = Ret*;
            using reference = Ret&;
        private:
            iterable* info;
            object_iterator_t it;
        public:
            iterator(iterable& info) : info(&info), it(info.begin_it) {}

            Ret operator*()
            {
                return info->invoker(*it);
            }

            void operator++() { ++it; }

            void operator++(int) { operator++(); }

            bool operator==(std::default_sentinel_t)
            {
                return it == info->end_it;
            }
        };

        iterator begin() { return iterator(*this); }

        std::default_sentinel_t end() { return {}; }
    };

    template<typename Invoker>
    iterable(object_container_t& objects,const Invoker& invoker) -> iterable<std::decay_t<Invoker>>;

public:

    template<typename Callable>
    requires (!std::same_as<Ret, void>)
    void invoke(Args... args, Callable&& result_proc)
    {
        auto lambda = [&](typename object_container_t::iterator::value_type invoke_info) -> Ret
        {
            auto [obj, mem_fn] = invoke_info;
            return invoke_single(obj, mem_fn, std::forward<Args>(args)...);
        };
        result_proc(iterable(referencing, lambda));
    }
};

template<typename Func>
using auto_delegate_container = array_ref_charger<generic_ref_protocol, Func>;
template<typename Func>
using multicast_auto_delegate = multicast_delegate<Func, auto_delegate_container>;
template<typename Func>
using auto_delegate_container_extern_ref = array_ref_charger<reference_reflector_ref_protocol, Func>;
template<typename Func>
using multicast_auto_delegate_extern_ref = multicast_delegate<Func, auto_delegate_container_extern_ref>;

template<typename MemFunc>
class weak_delegate_container
{

    struct delegate_object
    {
        std::weak_ptr<void> ptr;
        MemFunc mem_fn;
    };

    std::vector<delegate_object> objects;

    template<typename T, typename U>
    inline bool equals(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
    {
        return !t.owner_before(u) && !u.owner_before(t);
    }

public:

    //    template<class T>
    MemFunc& bind(const std::weak_ptr<void>& obj)
    {
        auto& res = objects.emplace_back(obj, nullptr);
        return res.mem_fn;
    }
    //    MemFunc& bind(nullptr_t)
    //    {
    //        auto&res = objects.emplace_back({}, nullptr);
    //        return res.mem_fn;
    //    }

    void unbind(const std::weak_ptr<void>& obj)
    {
        for (int i = 0; i < objects.size(); ++i)
        {
            if (equals(objects[i].ptr, obj))
            {
                objects[i] = std::move(objects.back());
                objects.pop_back();
                return;
            }
        }
        assert(false);
    }

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::tuple<const std::weak_ptr<void>&, MemFunc>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
    private:
        std::vector<delegate_object>::iterator it;
        std::vector<delegate_object>::iterator end_it;
        std::vector<delegate_object>& vec;

    public:
        iterator(std::vector<delegate_object>& vec)
                : it(vec.begin()), end_it(vec.end()), vec(vec) {}

        void operator++() { ++it; }

        void operator++(int) { operator++(); }

        value_type operator*() { return value_type(it->ptr, it->mem_fn); }

        bool operator==(const iterator& other) { return it == other.it; }

        bool operator==(nullptr_t)
        {
            if (it == end_it)
            {
                vec.resize(end_it - vec.begin());
                return true;
            }
            while (it->ptr.expired())
            {
                end_it--;
                if (it == end_it)
                {
                    vec.resize(end_it - vec.begin());
                    return true;
                }
                *it = *end_it;
            }
            return false;
        }
    };

    iterator begin() { return iterator(objects); }

    nullptr_t end() { return {}; }
};

template<typename Func>
using multicast_weak_delegate = multicast_delegate<Func, weak_delegate_container>;