#pragma once

#include <concepts>
#include <cstring>
#include <cassert>
#include "function_traits.h"
#include <typeinfo>
#include <cstdint>
#include <cstddef>
#include <optional>

namespace auto_delegate::function_v1
{
    namespace internal
    {
        enum class func_storage_op
        {
            st_copy,
            st_move_delete,
            st_delete,
            st_get_type_info
        };

        template<typename T, typename RTTI_T, typename Ret, typename... Args> requires std::copy_constructible<T> and std::move_constructible<T>
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
                        ::new(self) T((const T&) other_);
                        break;
                    case func_storage_op::st_move_delete:
                        ::new(self) T(std::move(other_));
                        other_.~T();
                        break;
                    case func_storage_op::st_delete:
                        self_.~T();
                        break;
#if __cpp_rtti
                        case func_storage_op::st_get_type_info:
                            return &typeid(RTTI_T);
#endif
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
                    : callee(new Callable(std::forward<Other>(callee))) {}

            functor_box_wrapper(const functor_box_wrapper& other)
                    : callee(new Callable(*other.callee)) {}

            functor_box_wrapper(functor_box_wrapper&& other) noexcept
                    : callee(other.callee) { other.callee = nullptr; }

            ~functor_box_wrapper() { delete callee; }

            Ret operator()(Args... args)
            {
                return (*callee)(std::forward<Args>(args)...);
            }

            const Callable* get() const noexcept { return callee; }
        };
    }

    template<typename FuncT, size_t SOB = 48>
    struct function;

    template<typename Callable>
    function(Callable&&) -> function<typename details::function_traits<std::decay_t<Callable>>::decay_function_type>;

    template<typename Ret, typename... Args, size_t SOB>
    class function<Ret(Args...), SOB>
    {
        static_assert(SOB % 8 == 0);
        using invoker_t = Ret (*)(void*, Args...);
        using manager_t = const void* (*)(void*, void*, internal::func_storage_op);

        alignas(std::max_align_t) void* data[SOB / sizeof(void*)];
        invoker_t invoker;
        manager_t manager;
        static constexpr size_t inline_storage_size = sizeof(data);
        static constexpr uintptr_t non_trivial_bit_mask = uintptr_t(1) << (sizeof(uintptr_t)*8-1);
        static constexpr uintptr_t pointer_mask = ~non_trivial_bit_mask;

        [[nodiscard]] bool non_trivial() const
        {
            return uintptr_t(manager) & non_trivial_bit_mask;
        }

        const void* manage(void* self, void* other, internal::func_storage_op op) const
        {
            auto m = manager_t(uintptr_t(manager) & pointer_mask);
            return m(self, other, op);
        }

        void set_manager_trivial(manager_t m, bool trivial)
        {
            uintptr_t& ptr = *(uintptr_t*) &m;
            assert((ptr & non_trivial_bit_mask) == 0);
            if (not trivial) ptr |= non_trivial_bit_mask;
            manager = manager_t(ptr);
        }

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
                using traits = internal::functor_object_traits<inline_functor_t, callable_t, Ret, Args...>;
                ::new(data) inline_functor_t(std::forward<Callable>(callable));
                invoker = traits::invoker;
                set_manager_trivial(traits::manager, traits::is_trivial);
            } else
            {
                using inline_functor_t = internal::functor_box_wrapper<callable_t, Ret, Args...>;
                using traits = internal::functor_object_traits<inline_functor_t, callable_t, Ret, Args...>;
                ::new(data) inline_functor_t(std::forward<Callable>(callable));
                invoker = traits::invoker;
                set_manager_trivial(traits::manager, false);
            }
        }


        function() : invoker(nullptr), manager(nullptr) {}

        function(const function& other) : invoker(other.invoker), manager(other.manager)
        {
            if (non_trivial()) manage(data, (void*) other.data, internal::func_storage_op::st_copy);
            else std::memcpy(data, other.data, sizeof(data));
        }

        function(function&& other) noexcept: invoker(other.invoker), manager(other.manager)
        {
            if (non_trivial()) manage(data, other.data, internal::func_storage_op::st_move_delete);
            else std::memcpy(data, other.data, sizeof(data));
            other.invoker = nullptr;
            other.manager = nullptr;
        }

        ~function()
        {
            if (non_trivial()) manage(data, nullptr, internal::func_storage_op::st_delete);
        }

        void swap(function& other) noexcept
        {
            function temp = std::move(other);
            ::new(&other) function(std::move(*this));
            ::new(this) function(std::move(temp));
        }

        function& operator=(const function& other)
        {
            function(other).swap(*this);
            return *this;
        }

        function& operator=(function&& other)
        {
            function(std::move(other)).swap(*this);
            return *this;
        }

        template<typename Callable>
        requires (not std::same_as<std::decay_t<Callable>, function>)
        function& operator=(Callable&& callable)
        {
            function(std::forward<Callable>(callable)).swap(*this);
            return *this;
        }

        Ret operator()(Args... args) const
        {
            return invoker((void*) data, std::forward<Args>(args)...);
        }

        operator bool() const noexcept { return invoker != nullptr; }

#if __cpp_rtti
        [[nodiscard]] const std::type_info& target_type() const noexcept
        {
            return *static_cast<const std::type_info*>(manage(nullptr, nullptr, internal::func_storage_op::st_get_type_info));
        }
        template<typename Callable>
        [[nodiscard]] const Callable* target() const noexcept
        {
            using callable_t = Callable;
            if(typeid(callable_t) != target_type()) return nullptr;
            if constexpr (sizeof(callable_t) <= inline_storage_size)
            {
                using inline_functor_t = callable_t;
                return static_cast<const inline_functor_t*>(data);
            }
            else
            {
                using inline_functor_t = internal::functor_box_wrapper<callable_t, Ret, Args...>;
                return static_cast<const inline_functor_t*>(data)->get();
            }
        }
#endif
    };
}

namespace auto_delegate::function_v2
{
    struct function_validate_tag {};

    namespace internal
    {
        enum class func_storage_op
        {
            st_copy,
            st_move_delete,
            st_delete,
            st_get_type_info
        };

        enum class func_invoke_op
        {
            validate,
            invoke
        };

        struct validation_tag {};
        struct invoke_tag {};

        template<typename Ret, typename... Args>
        struct functor_invoker_traits_validate
        {
            union invoker_ret_no_ret
            {
                bool valid;

                invoker_ret_no_ret(validation_tag, bool valid) : valid(valid) {}

                invoker_ret_no_ret() {}
            };

            union invoker_ret_with_ret
            {
                Ret ret;
                bool valid;

                invoker_ret_with_ret(validation_tag, bool valid) : valid(valid) {}

                invoker_ret_with_ret(invoke_tag, Ret&& ret) : ret(std::forward<Ret>(ret)) {}

                invoker_ret_with_ret() {}
            };

            using invoker_ret = std::conditional_t<std::is_void_v<Ret>, invoker_ret_no_ret, invoker_ret_with_ret>;

            using invoker_t = invoker_ret (*)(func_invoke_op, void*, std::tuple<Args...>*);

            template<typename T, auto ValidateMemFunc>
            static invoker_ret invoker_with_validater(func_invoke_op op, void* self, std::tuple<Args...>* args)
            {
                T& self_ = *static_cast<T*>(self);

                auto invoke = [&]<size_t... I>(std::index_sequence<I...>)
                {
                    if constexpr (std::is_void_v<Ret>)
                    {
                        self_(std::forward<Args>(std::get<I>(*args))...);
                        return invoker_ret{};
                    } else
                    {
                        return invoker_ret{invoke_tag{}, self_(std::forward<Args>(std::get<I>(*args))...)};
                    }
                };

                if constexpr (ValidateMemFunc == nullptr)
                    static_assert(!std::same_as<T, T>);//no validation
                else
                    switch (op)
                    {
                        case func_invoke_op::validate:
                            return invoker_ret{validation_tag{}, (self_.*ValidateMemFunc)(function_validate_tag{})};
                        case func_invoke_op::invoke:
                            return invoke(std::make_index_sequence<sizeof...(Args)>{});
                        default:
                            assert(false);
                            return invoker_ret{};
                    }
            }
        };

        template<typename Ret, typename... Args>
        struct functor_invoker_traits_trivial
        {
            using invoker_t = Ret(*)(void* self, Args... args);

            template<typename T>
            static Ret invoker(void* self, Args... args)
            {
                T& self_ = *static_cast<T*>(self);
                return self_(std::forward<Args>(args)...);
            }
        };

        template<typename T, auto ValidateMemFunc, typename Ret, typename... Args>
        struct functor_invoker_traits
        {
            using invoker_t = functor_invoker_traits_validate<Ret, Args...>::invoker_t;
            static constexpr auto invoker = functor_invoker_traits_validate<Ret, Args...>::
            template invoker_with_validater<T, ValidateMemFunc>;
        };
        template<typename T, typename Ret, typename... Args>
        struct functor_invoker_traits<T, nullptr, Ret, Args...>
        {
            using invoker_t = functor_invoker_traits_validate<Ret, Args...>::invoker_t;
            static constexpr auto invoker = functor_invoker_traits_trivial<Ret, Args...>::
            template invoker<T>;
        };


        template<typename T>
        struct validator_traits;
        template<typename T> requires requires(T t) { t.validate(function_validate_tag{}); }
        struct validator_traits<T>
        {
            constexpr static auto validator = &T::validate;
            constexpr static bool has_validator = true;
        };
        template<typename T> requires (not requires(T t) { t.validate(function_validate_tag{}); })
        struct validator_traits<T>
        {
            constexpr static std::nullptr_t validator = nullptr;
            constexpr static bool has_validator = false;
        };

        template<typename T,
                typename RTTI_T,
                auto ValidateMemFunc,
                typename Ret,
                typename... Args> requires std::copy_constructible<T> and std::move_constructible<T>
        struct functor_object_traits
        {
            constexpr static auto invoker = functor_invoker_traits<T, ValidateMemFunc, Ret, Args...>::invoker;

            static const void* manager(void* self, void* other, func_storage_op op)
            {
                T& self_ = *static_cast<T*>(self);
                T& other_ = *static_cast<T*>(other);
                switch (op)
                {
                    case func_storage_op::st_copy:
                        new(self) T((const T&) other_);
                        break;
                    case func_storage_op::st_move_delete:
                        new(self) T(std::move(other_));
                        other_.~T();
                        break;
                    case func_storage_op::st_delete:
                        self_.~T();
                        break;
#if __cpp_rtti
                        case func_storage_op::st_get_type_info:
                            return &typeid(RTTI_T);
#endif
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
                    : callee(new Callable(std::forward<Other>(callee))) {}

            functor_box_wrapper(const functor_box_wrapper& other)
                    : callee(new Callable(*other.callee)) {}

            functor_box_wrapper(functor_box_wrapper&& other)
                    : callee(other.callee) { other.callee = nullptr; }

            ~functor_box_wrapper() { delete callee; }

            Ret operator()(Args... args)
            {
                return (*callee)(std::forward<Args>(args)...);
            }

            [[nodiscard]] bool validate(function_validate_tag) const requires validator_traits<Callable>::has_validator
            {
                return callee->validate(function_validate_tag{});
            }

            const Callable* get() const noexcept { return callee; }
        };
    }

    template<typename FuncT, size_t SOB = 48>
    struct function;

    template<typename Callable>
    function(Callable&&) -> function<typename details::function_traits<std::decay_t<Callable>>::decay_function_type>;

    template<typename Ret, typename... Args, size_t SOB>
    class function<Ret(Args...), SOB>
    {
    protected:
        using invoker_t = internal::functor_invoker_traits_validate<Ret, Args...>::invoker_t;
        using trivial_invoker_t = Ret(*)(void*, Args...);
        using manager_t = const void* (*)(void*, void*, internal::func_storage_op);

        alignas(std::max_align_t) void* data[SOB / sizeof(void*)];
        void* invoker;
        manager_t manager;
        static constexpr size_t inline_storage_size = sizeof(data);
        static constexpr uintptr_t non_trivial_bit_mask = uintptr_t(1) << (sizeof (uintptr_t) * 8 - 1);
        static constexpr uintptr_t validator_bit_mask = non_trivial_bit_mask >> 1;
        static constexpr uintptr_t tag_mask = non_trivial_bit_mask | validator_bit_mask;
        static constexpr uintptr_t pointer_mask = ~(tag_mask);

        [[nodiscard]] bool non_trivial() const
        {
            return uintptr_t(manager) & non_trivial_bit_mask;
        }

        [[nodiscard]] bool has_validator() const
        {
            return uintptr_t(manager) & validator_bit_mask;
        }

        const void* manage(void* self, void* other, internal::func_storage_op op) const
        {
            auto m = manager_t(uintptr_t(manager) & pointer_mask);
            return m(self, other, op);
        }

        void set_manager_and_tags(manager_t m, bool trivial, bool has_validator)
        {
            uintptr_t& ptr = *(uintptr_t*) &m;
            assert((ptr & tag_mask) == 0);
            if (not trivial) ptr |= non_trivial_bit_mask;
            if (has_validator) ptr |= validator_bit_mask;
            manager = manager_t(ptr);

        }

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
                constexpr auto validator = internal::validator_traits<inline_functor_t>::validator;
                constexpr bool has_validator = internal::validator_traits<inline_functor_t>::has_validator;
                using traits = internal::functor_object_traits<
                        inline_functor_t,
                        callable_t,
                        validator,
                        Ret, Args...>;
                ::new(data) inline_functor_t(std::forward<Callable>(callable));
                invoker = (void*) (traits::invoker);
                set_manager_and_tags(traits::manager, traits::is_trivial, has_validator);
            } else
            {
                using inline_functor_t = internal::functor_box_wrapper<callable_t, Ret, Args...>;
                constexpr auto validator = internal::validator_traits<inline_functor_t>::validator;
                constexpr bool has_validator = internal::validator_traits<inline_functor_t>::has_validator;
                using traits = internal::functor_object_traits<
                        inline_functor_t,
                        callable_t,
                        validator,
                        Ret, Args...>;
                ::new(data) inline_functor_t(std::forward<Callable>(callable));
                invoker = (void*) (traits::invoker);
                set_manager_and_tags(traits::manager, false, has_validator);
            }
        }

        function() : invoker(nullptr), manager(nullptr) {}

        function(const function& other) : invoker(other.invoker), manager(other.manager)
        {
            if (non_trivial()) manage(data, (void*) other.data, internal::func_storage_op::st_copy);
            else std::memcpy(data, other.data, sizeof(data));
        }

        function(function&& other) noexcept: invoker(other.invoker), manager(other.manager)
        {
            if (non_trivial()) manage(data, other.data, internal::func_storage_op::st_move_delete);
            else std::memcpy(data, other.data, sizeof(data));
            other.invoker = nullptr;
            other.manager = nullptr;
        }

        ~function()
        {
            if (non_trivial()) manage(data, nullptr, internal::func_storage_op::st_delete);
        }

        void swap(function& other) noexcept
        {
            function temp = std::move(other);
            ::new(&other) function(std::move(*this));
            ::new(this) function(std::move(temp));
        }

        function& operator=(const function& other)
        {
            function(other).swap(*this);
            return *this;
        }

        function& operator=(function&& other)
        {
            function(std::move(other)).swap(*this);
            return *this;
        }

        template<typename Callable>
        requires (not std::same_as<std::decay_t<Callable>, function>)
        function& operator=(Callable&& callable)
        {
            function(std::forward<Callable>(callable)).swap(*this);
            return *this;
        }

    private:

        [[nodiscard]] bool validater_() const
        {
            return reinterpret_cast<invoker_t>(invoker)(internal::func_invoke_op::validate, (void*) data, nullptr).valid;
        }

        Ret invoke_(std::tuple<Args...> args_tuple) const
        {
            if constexpr (std::is_void_v<Ret>)
                reinterpret_cast<invoker_t>(invoker)(internal::func_invoke_op::invoke, (void*) data, &args_tuple);
            else
                return reinterpret_cast<invoker_t>(invoker)(internal::func_invoke_op::invoke, (void*) data, &args_tuple).ret;
        }

        Ret invoke_trivial_(Args... args) const
        {
            return reinterpret_cast<trivial_invoker_t>(invoker)((void*) data, std::forward<Args>(args)...);

        }

    public:

        [[nodiscard]] bool validate() const
        {
            if (has_validator())
                return validater_();
            else
                return true;
        }

        Ret operator()(Args... args) const
        {
            assert(invoker);
            if (has_validator())
                return invoke_({std::forward<Args>(args)...});
            else
                return invoke_trivial_(args...);
        }

        std::optional<Ret> try_invoke(Args... args)
        {
            assert(invoker);
            if (has_validator())
                if (validater_())
                    return invoke_({std::forward<Args>(args)...});
                else
                    return std::nullopt;
            else
                return invoke_trivial_(args...);
        }

        operator bool() const noexcept { return invoker != nullptr; }

#if __cpp_rtti
        [[nodiscard]] const std::type_info& target_type() const noexcept
        {
            return *static_cast<const std::type_info*>(manage(nullptr, nullptr, internal::func_storage_op::st_get_type_info));
        }
        template<typename Callable>
        [[nodiscard]] const Callable* target() const noexcept
        {
            using callable_t = Callable;
            if(typeid(callable_t) != target_type()) return nullptr;
            if constexpr (sizeof(callable_t) <= inline_storage_size)
            {
                using inline_functor_t = callable_t;
                return static_cast<const inline_functor_t*>(data);
            }
            else
            {
                using inline_functor_t = internal::functor_box_wrapper<callable_t, Ret, Args...>;
                return static_cast<const inline_functor_t*>(data)->get();
            }
        }
#endif
    };
}

namespace auto_delegate
{
    using function_v1::function;
}