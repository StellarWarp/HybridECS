#pragma once

#include <memory>
#include "auto_reference.h"


template<typename Func>
class multicast_auto_delegate;

template<typename Ret, typename... Args> requires (!std::is_rvalue_reference_v<Args> && ...)

class multicast_auto_delegate<Ret(Args...)>
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

    using object_container_t = array_ref_charger<true, mem_func_t>;

    object_container_t referencing;

public:
    multicast_auto_delegate() = default;

    multicast_auto_delegate(const multicast_auto_delegate&) = delete;

    multicast_auto_delegate(multicast_auto_delegate&& other) noexcept = default;

public:

    template<typename T>
    void bind(T* obj, Ret(T::*mem_func)(Args...))
    {
        auto& data = referencing.bind(obj);
        data = reinterpret_cast<mem_func_t>(mem_func);
    }

    template<typename T, typename Callable>
    requires std::is_empty_v<Callable>
    void bind(T* obj, Callable&& func)
    {
        auto& data = referencing.bind(obj);
        data = &generic_class::template Invoker<T, Callable>;
    }

    template<typename T>
    void unbind(T* obj)
    {
        referencing.unbind(obj);
    }


    //no need to forward as the parameters types are already defined
    void invoke(Args... args) requires std::same_as<Ret, void>
    {
        for (auto [obj,mem_fn]: referencing)
            (static_cast<generic_class*>(obj)->*mem_fn)(std::forward<Args>(args)...);
    }

    void operator()(Args... args) requires std::same_as<Ret, void>
    {
        invoke(std::forward<Args>(args)...);
    }

private:
    class iterable
    {
        using object_iterator_t = object_container_t::iterator;
        const object_iterator_t begin_it;
        const object_iterator_t end_it;
        const std::tuple<Args && ...> args_tuple;
    public:
        explicit iterable(object_container_t& objects, Args... args) :
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
            const iterable& info;
            object_iterator_t it;
        public:
            iterator(const iterable& info) : info(info), it(info.begin_it) {}

            Ret operator*()
            {
                auto [obj,mem_fn] = *it;
                return [&]<size_t... I>(const std::tuple<Args && ...>& args_1,
                                        std::integer_sequence<std::size_t, I...>)
                {
                    return [&](Args&& ... args_2)
                    {
                        return (static_cast<generic_class*>(obj)->*mem_fn)(std::forward<Args>(args_2)...);
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
public:

    template<typename Callable>
    requires (!std::same_as<Ret, void>)
    void invoke(Args... args, Callable&& result_proc)
    {
        result_proc(iterable(referencing, std::forward<Args>(args)...));
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
                referencing[i] = std::move(referencing.back());
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

private:
    class iterable
    {
        using object_container_t = std::vector<delegate_object>;
        using object_iterator_t = object_container_t::iterator;
        object_container_t& objects;
        const std::tuple<Args && ...> args_tuple;
    public:
        explicit iterable(object_container_t& objects, Args... args) :
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
            const iterable& info;
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
            iterator(const iterable& info) :
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
public:
    template<typename Callable>
    requires (!std::same_as<Ret, void>)
    void invoke(Args... args, Callable&& result_proc)
    {
        result_proc(iterable(referencing, std::forward<Args>(args)...));
    }
};