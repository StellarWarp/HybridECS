#pragma once

#include "delegate.h"


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