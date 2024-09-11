#pragma once
#include <concepts>
#include <cstring>
#include "function_traits.h"

namespace auto_delegate
{
    enum class func_storage_op
    {
        st_copy,
        st_move,
        st_delete,
        st_get_type_info
    };

    template<typename T, typename Ret, typename... Args>
    requires std::copy_constructible<T> and std::move_constructible<T>
    struct functor_object_traits
    {
        static Ret invoker(void* self, Args... args)
        {
            T& self_ = *static_cast<T*>(self);
            return self_(std::forward<Args>(args)...);
        }

        static const void* manager(void* self, void* other, func_storage_op op)
        {
            T& self_ = *static_cast<T*>(self);
            T& other_ = *static_cast<T*>(other);
            switch (op)
            {
                case func_storage_op::st_copy:
                    new(self) T((const T&)other_);
                    break;
                case func_storage_op::st_move:
                    new(self) T(std::move(other_));
                    break;
                case func_storage_op::st_delete:
                    self_.~T();
                    break;
                case func_storage_op::st_get_type_info:
                    return &typeid(T);
            }
            return nullptr;
        }

        static constexpr bool is_trivial =
                std::is_trivially_copyable_v<T>
                and std::is_trivially_move_constructible_v<T>
                and std::is_trivially_destructible_v<T>;
    };


    template<typename Callable, typename Ret, typename... Args>
    struct functor_box_wrapper
    {
        Callable* callee;

        template<typename Other>
        explicit functor_box_wrapper(Other&& callee)
                : callee(new Callable(std::forward<Other>(callee)))
        {}
        functor_box_wrapper(const functor_box_wrapper& other)
                : callee(new Callable(*other.callee))
        {}
        functor_box_wrapper(functor_box_wrapper&& other)
                : callee(other.callee)
        { other.callee = nullptr; }
        ~functor_box_wrapper(){ delete callee; }

        Ret operator()(Args... args)
        {
            return (*callee)(std::forward<Args>(args)...);
        }
    };

    template<typename FuncT>
    struct function;

    template<typename Callable>
    function(Callable&&) -> function<typename function_traits<std::decay_t<Callable>>::decay_function_type>;

    template<typename Ret, typename... Args>
    class function<Ret(Args...)>
    {
        using invoker_t = Ret (*)(void*, Args...);
        using manager_t = const void* (*)(void*, void*, func_storage_op);

        alignas(std::max_align_t) void* data[6];
        invoker_t invoker;
        manager_t manager;
    protected:
        static constexpr size_t inline_storage_size = sizeof(data);
    public:
        template<typename Callable>
        requires std::same_as<std::invoke_result_t<Callable, Args...>, Ret>
                 and (not std::same_as<std::decay_t<Callable>, function>)
                 and std::move_constructible<Callable>
                 and std::copy_constructible<Callable>
        function(Callable&& callable)
        {
            using callable_t = std::decay_t<Callable>;
            if constexpr (sizeof(callable_t) <= inline_storage_size)
            {
                using inline_functor_t = callable_t;
                using traits = functor_object_traits<inline_functor_t, Ret, Args...>;
                ::new(data) inline_functor_t(std::forward<Callable>(callable));
                invoker = traits::invoker;
                if constexpr (traits::is_trivial)
                    manager = nullptr;
                else
                    manager = traits::manager;
            }
            else
            {
                using inline_functor_t = functor_box_wrapper<callable_t, Ret, Args...>;
                using traits = functor_object_traits<inline_functor_t, Ret, Args...>;
                ::new(data) inline_functor_t(std::forward<Callable>(callable));
                invoker = traits::invoker;
                manager = traits::manager;
            }
        }

        template<typename Callable>
        function& operator=(Callable&& callable)
        {
            this->~function();
            new(this) function(std::forward<Callable>(callable));
            return *this;
        }

        function() : invoker(nullptr), manager(nullptr){}
        function(const function& other) : invoker(other.invoker), manager(other.manager)
        {
            if(manager) manager(data, (void*)other.data, func_storage_op::st_copy);
            else std::memcpy(data, other.data, sizeof(data) );
        }

        function(function&& other)noexcept : invoker(other.invoker), manager(other.manager)
        {
            if(manager) manager(data, other.data, func_storage_op::st_move);
            else std::memcpy(data, other.data, sizeof(data) );
            other.invoker = nullptr;
            other.manager = nullptr;
        }
        ~function()
        {
            if(manager) manager(data, nullptr, func_storage_op::st_delete);
        }

        function& operator=(const function& other) noexcept
        {
            if (this == &other) return *this;
            this->~function();
            new(this) function(other);
            return *this;
        }
        function& operator=(function&& other) noexcept
        {
            if (this == &other) return *this;
            this->~function();
            new(this) function(std::move(other));
            return *this;
        }

        Ret operator()(Args... args) const
        {
            return invoker((void*)data, std::forward<Args>(args)...);
        }
        operator bool() const{ return invoker != nullptr; }

//        const type_info& target_type() const
//        {
//            if(manager) return *static_cast<const type_info*>(manager(nullptr, nullptr, func_storage_op::st_get_type_info));
//            else  typeid(void);
//        }
    };


}