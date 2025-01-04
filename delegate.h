#pragma once

#include <concepts>
#include <type_traits>
#include "function_traits.h"

namespace auto_delegate
{
    template<auto>
    struct func_tag {};

    template<typename Func, typename GenericPtr = void*>
    class delegate;

    template<typename T_ptr, typename T_MemFunc>
    delegate(T_ptr, T_MemFunc) ->
    delegate<typename details::function_traits<T_MemFunc>::function_type, void*>;

    template<typename Ret, typename... Args, typename GenericPtr>
    class delegate<Ret(Args...), GenericPtr>
    {
        template<typename, typename>
        friend class multicast_delegate;

        template<typename T, auto MemFunc>
        static Ret Invoker(void* obj, Args... args)
        {
            return (reinterpret_cast<T*>(obj)->*MemFunc)(std::forward<Args>(args)...);
        }

        template<typename T, auto Lambda>
        static Ret LambdaInvoker(void* obj, Args... args)
        {
            return Lambda(*(T*) obj, std::forward<Args>(args)...);
        }

        template<auto Callable>
        static Ret StaticInvoker(void*, Args... args)
        {
            return Callable(std::forward<Args>(args)...);
        }

        template<auto Lambda>
        static Ret StaticLambdaInvoker(void*, Args... args)
        {
            return Lambda(std::forward<Args>(args)...);
        }

        using invoker_t = Ret (*)(void*, Args...);

        GenericPtr ptr;
        invoker_t invoker;

        template<class T, class T_ptr>
        static constexpr bool is_pointer_of = std::same_as<typename std::pointer_traits<T_ptr>::element_type, T>;

        template<typename T_ptr>
        using value_of = typename std::pointer_traits<T_ptr>::element_type;
    public:
        using function_type = Ret(Args...);
        using function_pointer = Ret(*)(Args...);

        delegate() : ptr(), invoker() {}

        //bind methods
        template<auto MemFunc, typename T_ptr>
        delegate(const T_ptr& obj, func_tag<MemFunc>) : ptr(obj), invoker(Invoker<value_of<T_ptr>, MemFunc>) {}

        //bind methods
        template<auto MemFunc, typename T_ptr>
        void bind(const T_ptr& obj, func_tag<MemFunc> = {})
        {
            ptr = obj;
            invoker = Invoker<value_of<T_ptr>, MemFunc>;
        }

        //bind object with lambda
        template<typename T_ptr, typename Callable>
        requires std::is_empty_v<Callable> && requires { LambdaInvoker<value_of<T_ptr>, std::decay_t<Callable>{}>; }
        void bind(const T_ptr& obj, Callable&& func)
        {
            ptr = obj;
            invoker = LambdaInvoker<value_of<T_ptr>, std::decay_t<Callable>{}>;
        }

        //bind callable object
        template<typename T_ptr>
        void bind(const T_ptr& callable)
        requires std::same_as<
                std::invoke_result_t<std::decay_t<decltype(*callable)>, Args...>,
                Ret>
        {
            ptr = callable;
            invoker = Invoker<value_of<T_ptr>, &std::decay_t<decltype(*callable)>::operator()>;
        }

        //bind static function
        template<function_pointer StaticFunc>
        void bind()
        {
            if constexpr (requires { ptr = nullptr; })
                ptr = nullptr;
            else
                ptr.reset();
            invoker = StaticInvoker<StaticFunc>;
        }

        //bind a stateless callable object
        template<typename Callable>
        void bind(Callable&& callable)
        requires std::same_as<std::invoke_result_t<std::decay_t<Callable>, Args...>, Ret>
                 && std::is_empty_v<std::decay_t<Callable>>
        {
            if constexpr (requires { ptr = nullptr; })
                ptr = nullptr;
            else
                ptr.reset();
            invoker = StaticLambdaInvoker<Callable{}>;
        }

        bool is_bound() const { return ptr != nullptr; }
        operator bool() const { return ptr != nullptr; }

        Ret invoke(Args... args)
        {
            void* c;
            if constexpr (requires { static_cast<void*>(ptr); })
                c = static_cast<void*>(ptr);
            else if constexpr (requires { ptr.get(); })
                c = reinterpret_cast<void*>(ptr.get());
            else if constexpr (requires { ptr.lock().get(); })
            {
                return reinterpret_cast<invoker_t>(invoker)(
                        reinterpret_cast<void*>(ptr.lock().get()),
                        std::forward<Args>(args)...);
            } else
                static_assert(!std::is_same_v<Ret, Ret>, "ptr get method is not implemented");
            return reinterpret_cast<invoker_t>(invoker)(c, std::forward<Args>(args)...);
        }

        Ret operator()(Args... args)
        {
            return invoke(std::forward<Args>(args)...);
        }

        void reset()
        {
            if constexpr (requires { ptr = nullptr; })
                ptr = nullptr;
            else
                ptr.reset();
        }
    };

}
