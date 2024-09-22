//
// Created by Estelle on 2024-08-27.
//
#pragma once

#include "function.h"
#include "multicast_delegate.h"
#include <vector>
#include <optional>

#ifdef no_unique_address
#undef no_unique_address
#endif
#ifdef _MSC_VER
#define no_unique_address msvc::no_unique_address
#endif

namespace auto_delegate
{

    template<typename Func>
    class multicast_function;

    template<typename Ret, typename... Args> requires (not std::is_rvalue_reference_v<Args> && ...)

    class multicast_function<Ret(Args...)>
    {

        using invoker_t = Ret (*)(void*, Args...);

        template<class T>
        using mem_func_t = Ret(T::*)(Args...);

        using wrapper_ret_t = std::conditional_t<std::is_void_v<Ret>, void, std::optional<Ret>>;

    public:
        using function_t = function<Ret(Args...)>;

        struct object_container : public std::vector<function_t>
        {
            using super = std::vector<function_t>;
            super::iterator iteraion_end;
#ifndef NDEBUG
            bool in_iteration = false;
#endif

            void remove(const void* func)
            {
                assert(!in_iteration);
                assert((intptr_t(func) - intptr_t(super::data())) % sizeof(function_t) == 0);
                size_t index = (function_t*) func - super::data();
                auto& back = super::back();
                super::data()[index] = std::move(back);
                super::pop_back();
            }

            Ret remove_on_call(const void* func, Args... args)
            {
                assert(in_iteration);
                assert((intptr_t(func) - intptr_t(super::data())) % sizeof(function_t) == 0);
                size_t index = (function_t*) func - super::data();
                function_t& current = super::at(index);
                --iteraion_end;
                auto& end = *iteraion_end;
                //if last element is remove in call would cause a fake return
                if (&current == &end)
                {
                    if constexpr (not std::is_void_v<Ret>)
                    {
                        union no_return
                        {
                            Ret ret;
                            std::array<uint8_t, sizeof(Ret)> dammy;
                            no_return() { dammy.fill(0xFF); }
                        };
                        return no_return().ret;
                    } else return;
                }
                current = std::move(end);
                return current(std::forward<Args>(args)...);
            }

            void release_removed()
            {
#ifndef NDEBUG
                assert(in_iteration);
                in_iteration = false;
#endif
                super::erase(iteraion_end, super::end());
            }

            using super::emplace_back;
            using super::begin;

            const super::iterator& end()
            {
#ifndef NDEBUG
                assert(!in_iteration);
                in_iteration = true;
#endif
                iteraion_end = super::end();
                return iteraion_end;
            }
        };

    private:
        using object_container_t = object_container;

        object_container objects;

        template<typename T_ptr>
        using value_of = typename std::pointer_traits<T_ptr>::element_type;
    public:
        multicast_function() = default;

        multicast_function(const multicast_function&) = delete;

        multicast_function(multicast_function&& other) noexcept = default;

        auto size() { return objects.size(); }

        bool empty() { return objects.empty(); }

        void clear() { objects.clear(); }

    public:

        using function_type = Ret(Args...);
        using function_pointer = Ret(*)(Args...);

        template<typename T_Ptr, auto MemFunc>
        struct member_func_wrapper
        {
            T_Ptr obj;

            explicit member_func_wrapper(const T_Ptr& obj) : obj(obj) {}

            Ret operator()(Args... args)
            {
                return (obj->*MemFunc)(std::forward<Args>(args)...);
            }
        };

        template<typename T_Ptr, typename Lambda>
        struct object_lambda_extend_wrapper
        {
            T_Ptr obj;
            Lambda lambda;

            template<typename OtherLambda>
            explicit object_lambda_extend_wrapper(T_Ptr obj, OtherLambda&& other)
                    : obj(obj), lambda(std::forward<OtherLambda>(other)) {}

            Ret operator()(Args... args)
            {
                return lambda(*obj, std::forward<Args>(args)...);
            }
        };

        template<typename Callable>
        static constexpr bool bindable = std::same_as<std::invoke_result_t<Callable, Args...>, Ret>
                                         and std::move_constructible<Callable>
                                         and std::copy_constructible<Callable>;


        decltype(auto) notify_bind(auto& functor)
        {
            if constexpr (requires { functor.on_bind(); })
                return functor.on_bind();
            else if constexpr (requires { functor.on_bind(&objects); })
                return functor.on_bind(&objects);
            else if constexpr (requires { std::decay_t<decltype(functor)>::on_bind; })
                static_assert(!std::same_as<int, int>, "on_bind is not implemented");
            else
                return;
        }

        template<typename Callable>
        requires bindable<Callable>
        decltype(auto) bind(Callable&& callable)
        {
            using callable_t = std::decay_t<Callable>;
            auto& callee = (callable_t&) objects.emplace_back(function(std::forward<Callable>(callable)));
            return notify_bind(callee);
        }

        //bind methods
        template<auto MemFunc, typename T_ptr>
        requires std::same_as<typename details::function_traits<decltype(MemFunc)>::decay_function_type, Ret(Args...)>
        decltype(auto) bind(const T_ptr& obj)
        {
            return bind(member_func_wrapper<T_ptr, MemFunc>(obj));
        }

        //bind methods with signature inference
        template<typename T, mem_func_t<T> MemFunc, typename T_ptr>
        decltype(auto) bind(const T_ptr& obj)
        {
            return bind<MemFunc>(obj);
        }

        //bind methods
        template<auto Func>
        decltype(auto) bind()
        {
            return bind([](Args... args) -> Ret { return Func(std::forward<Args>(args)...); });
        }

        //bind object with lambda
        template<typename T_ptr, typename Callable>
        decltype(auto) bind(const T_ptr& obj, Callable&& func)
        {
            return bind(object_lambda_extend_wrapper<T_ptr, Callable>(obj, std::forward<Callable>(func)));
        }

        template<typename Callable> requires std::same_as<std::decay_t<Callable>, Callable>
        struct handled_wrapper
        {
            Callable functor;
            delegate_handle_ref inv_handle;

            handled_wrapper(Callable&& functor) : functor(std::forward<Callable>(functor)) {}

            Ret operator()(Args... args) { return functor(std::forward<Args>(args)...); }

            delegate_handle on_bind() { return delegate_handle(&inv_handle); }
        };

        template<typename Callable>
        requires bindable<Callable>
        decltype(auto) bind_handled(Callable&& callable)
        {
            return bind(handled_wrapper(std::forward<Callable>(callable)));
        }

        template<typename Callable> requires std::same_as<std::decay_t<Callable>, Callable>
        struct unique_handled_wrapper
        {
            Callable functor;
            unique_delegate_handle_ref inv_handle;

            unique_handled_wrapper(Callable&& functor) : functor(std::forward<Callable>(functor)), inv_handle() {}

            unique_handled_wrapper(const unique_handled_wrapper&) { assert(false); }

            unique_handled_wrapper(unique_handled_wrapper&&) = default;

            Ret operator()(Args... args) { return functor(std::forward<Args>(args)...); }

            auto on_bind(object_container_t* container)
            {
                return unique_delegate_handle_lambda(container, &inv_handle, [](void* container, delegate_handle_ref* inv)
                {
                    auto this_address = (char*) inv - offsetof(unique_handled_wrapper, inv_handle);
                    static_cast<object_container_t*>(container)->remove(this_address);
                });
            }
        };

        template<auto Func>
        struct unique_handled_wrapper_static
        {
            unique_delegate_handle_ref inv_handle{};

            Ret operator()(Args... args) { return Func(std::forward<Args>(args)...); }

            unique_handled_wrapper_static() = default;

            unique_handled_wrapper_static(const unique_handled_wrapper_static&) { assert(false); }

            unique_handled_wrapper_static(unique_handled_wrapper_static&&) = default;

            auto on_bind(object_container_t* container)
            {
                return unique_delegate_handle_lambda(container, &inv_handle, [](void* container, delegate_handle_ref* inv)
                {
                    static_cast<object_container_t*>(container)->remove(inv);
                });
            }
        };

        template<typename Callable>
        requires bindable<Callable>
        decltype(auto) bind_unique_handled(Callable&& callable)
        {
            using callable_t = std::decay_t<Callable>;
            return bind(unique_handled_wrapper<callable_t>(std::forward<Callable>(callable)));
        }

        template<auto Func>
        //requires bindable<decltype(Func)>
        decltype(auto) bind_unique_handled()
        {
            return bind(unique_handled_wrapper_static<Func>());
        }

        template<typename T>
        struct weak_ptr_wrapper_base
        {
            std::weak_ptr<T> obj;
            object_container_t* conatiner;

            template<typename T_ptr>
            requires std::same_as<value_of<std::decay_t<T_ptr>>, T>
            explicit weak_ptr_wrapper_base(const T_ptr& obj) : obj(obj) {}

            void on_bind(object_container_t* container)
            {
                this->conatiner = container;
            }

            Ret _invoke(Args... args, auto&& invoker)
            {
                if (obj.expired()) return conatiner->remove_on_call(this, std::forward<Args>(args)...);
                return invoker(std::forward<Args>(args)...);
            }
        };


        template<typename T, auto MemFunc>
        struct weak_ptr_wrapper : weak_ptr_wrapper_base<T>
        {
            using super = weak_ptr_wrapper_base<T>;
            using super::super;

            Ret operator()(Args... args)
            {
                return super::_invoke(std::forward<Args>(args)...,
                                      [&](Args... args_)
                                      {
                                          return (this->obj.lock().get()->*MemFunc)(std::forward<Args>(args_)...);
                                      });

            }
        };

        template<typename T, typename Lambda>
        struct weak_ptr_wrapper_lambda : weak_ptr_wrapper_base<T>
        {
            using super = weak_ptr_wrapper_base<T>;
            using super::super;
            Lambda lambda;

            template<typename T_Ptr, typename OtherLambda>
            explicit weak_ptr_wrapper_lambda(const T_Ptr& obj, OtherLambda&& other)
                    : super(obj), lambda(std::forward<OtherLambda>(other)) {}

            Ret operator()(Args... args)
            {
                return super::_invoke(std::forward<Args>(args)...,
                                      [&](Args... args_)
                                      {
                                          return lambda(*this->obj.lock().get(), std::forward<Args>(args_)...);
                                      });

            }
        };

        template<auto MemFunc, typename T_Ptr>
        void bind_weak(const T_Ptr& obj)
        {
            using T = value_of<T_Ptr>;
            bind(weak_ptr_wrapper<T, MemFunc>(obj));
        }

        template<typename T_Ptr, typename Lambda>
        void bind_weak(const T_Ptr& obj, Lambda&& lambda)
        {
            using T = value_of<T_Ptr>;
            bind(weak_ptr_wrapper_lambda<T, std::decay_t<Lambda>>(obj, std::forward<Lambda>(lambda)));
        }

        template<typename Callable>
        requires bindable<Callable>
        decltype(auto) operator+=(Callable&& callable)
        {
            return bind(std::forward<Callable>(callable));
        }





//        template<typename T_ptr>
//        void
//        unbind(const T_ptr& obj)
//        requires requires{ typename std::pointer_traits<T_ptr>; }
//                 and (not enable_delegate_handle)
//        {
//            objects.unbind(obj);
//        }


    public:
        //no need to forward as the parameters types are already defined
        void invoke(Args... args) requires std::same_as<Ret, void>
        {
            auto iter = objects.begin();
            auto& end = objects.end();
            //require iter < end there in call remove and ++iter get iterator out of range
            for (; iter < end; ++iter)
            {
                auto& functor = *iter;
                functor(std::forward<Args>(args)...);
            }
            objects.release_removed();
        }

        void operator()(Args... args) requires std::same_as<Ret, void>
        {
            invoke(std::forward<Args>(args)...);
        }

        template<typename Callable>
        requires (!std::same_as<Ret, void>)
        void for_each_invoke(Args... args, Callable&& result_proc)
        {
            auto iter = objects.begin();
            auto& end = objects.end();
            //require iter < end there in call remove and ++iter get iterator out of range
            for (; iter < end; iter++)
            {
                auto& functor = *iter;
                Ret&& ret = functor(std::forward<Args>(args)...);
                if (iter == end) break;//fake return
                result_proc(std::forward<Ret>(ret));
            }
            objects.release_removed();
        }

        template<typename Callable>
        void for_each(Callable&& func) requires std::same_as<Ret, void>
        {
            auto iter = objects.begin();
            auto& end = objects.end();
            //require iter < end there in call remove and ++iter get iterator out of range
            for (; iter < end; iter++)
            {
                auto& functor = *iter;
                bool fake_return = false;
                func([&](Args... args) mutable
                     {
                         if (fake_return) return;
                         functor(std::forward<Args>(args)...);
                         if (iter == end) fake_return = true;//fake invoke
                     });
            }

            objects.release_removed();
        }

        //the call need to be interrupt when the second is true
        template<typename Callable>
        void for_each(Callable&& func) requires (!std::same_as<Ret, void>)
        {
            auto iter = objects.begin();
            auto& end = objects.end();
            //require iter < end there in call remove and ++iter get iterator out of range
            for (; iter < end; iter++)
            {
                auto& functor = *iter;
                bool fake_return = false;
                func([&](Args... args) mutable
                     {
                         return std::pair<Ret, bool>(functor(std::forward<Args>(args)...), iter == end);
                     });
            }
            objects.release_removed();
        }
    };


    struct object_binder_tag {};

    template<typename... Binders>
    using bind_info = std::tuple<Binders...>;

    template<typename T>
    struct binder : object_binder_tag
    {
        using type = T;
        using pointer = T*;
        T* obj;

        binder(T* obj) : obj(obj) {}

        auto invoke(auto&& invoker, auto&& ... args)
        {
            return invoker(obj, std::forward<decltype(args)>(args)...);
        }

        template<typename FuncT>
        auto convert()
        {
            return *this;
        }
    };

    template<typename ObjectBinder, auto Memfunc, typename T, typename Ret, typename... Args>
    struct member_func_wrapper : ObjectBinder
    {
        member_func_wrapper(ObjectBinder&& obj) : ObjectBinder(std::move(obj)) {}

        Ret operator()(Args... args)
        {
            return this->invoke([](auto&& ptr, Args... args_)
                                {
                                    auto o = static_cast<T*>(ptr);
                                    return (o->*Memfunc)(std::forward<Args>(args_)...);
                                }, std::forward<Args>(args)...);
        }
    };

    template<auto Memfunc>
    struct bind_memfn_impl {};

    template<auto Memfunc>
    inline constexpr bind_memfn_impl<Memfunc> bind_memfn;

    template<std::derived_from<object_binder_tag> ObjectBinder, auto Memfunc>
    auto operator|(ObjectBinder&& b_obj, bind_memfn_impl<Memfunc>)
    {
        using namespace details;
        using function_type = function_traits<decltype(Memfunc)>::decay_function_type;
        using object_type = function_traits<decltype(Memfunc)>::class_type;
        return [&]<typename Ret, typename... Args>(function_traits<Ret(Args...)>)
        {
            return []<typename Binder>(Binder&& obj)
            {
                using binder_t = std::decay_t<Binder>;
                return member_func_wrapper<
                        binder_t,
                        Memfunc,
                        object_type,
                        Ret,
                        Args...
                >{std::forward<Binder>(obj)};
            }(b_obj.template convert<Ret(Args...)>());
        }(function_traits<function_type>());
    };

    template<typename ObjectBinder, typename Lambda, typename T, typename Ret, typename... Args>
    struct bind_into_lambda_warpper : ObjectBinder
    {
        Lambda lambda;

        bind_into_lambda_warpper(ObjectBinder&& obj, Lambda&& lambda)
                : ObjectBinder(std::move(obj)), lambda(std::move(lambda)) {}

        Ret operator()(Args... args)
        {
            // return ObjectBinder::invoke(lambda, std::forward<decltype(args)>(args)...);
            return ObjectBinder::invoke([&](auto&& ptr, Args...args)
                                        {
                                            auto o = static_cast<T*>(ptr);
                                            return lambda(*o, std::forward<Args>(args)...);
                                        }, std::forward<decltype(args)>(args)...);
        }
    };

    template<typename Lambda>
    struct bind_into_lambda
    {
        Lambda lambda;

        bind_into_lambda(Lambda&& lambda) : lambda(std::forward<Lambda>(lambda)) {}
    };

    template<typename Lambda>
    bind_into_lambda(Lambda&&) -> bind_into_lambda<std::decay_t<Lambda>>;

    template<std::derived_from<object_binder_tag> ObjectBinder, typename Lambda>
    auto operator|(ObjectBinder&& b_obj, bind_into_lambda<Lambda> b_lambda)
    {
        using namespace details;
        using object_type = std::decay_t<ObjectBinder>::type;
        using function_type = function_traits<decltype(&Lambda::template operator()<object_type&>)>::decay_function_type;
        return [&]<typename Ret, typename _T_, typename... Args>(function_traits<Ret(_T_, Args...)>)
        {
            return [&]<typename Binder>(Binder&& obj)
            {
                using binder_t = std::decay_t<Binder>;
                return bind_into_lambda_warpper<
                        binder_t,
                        Lambda,
                        object_type,
                        Ret,
                        Args...
                >{std::forward<Binder>(obj), std::move(b_lambda.lambda)};
            }(b_obj.template convert<Ret(Args...)>());
        }(function_traits<function_type>());
    }

//    template<typename Base, typename Callable, typename Ret, typename... Args>
//    struct lambda_append_wrapper : Base
//    {
//        [[no_unique_address]] Callable functor;
//
//        lambda_append_wrapper(Callable&& functor) : functor(std::forward<Callable>(functor)), inv_handle() {}
//
//        lambda_append_wrapper(const lambda_append_wrapper&) { assert(false); }
//
//        lambda_append_wrapper(lambda_append_wrapper&&) = default;
//
//        Ret operator()(Args... args)
//        {
//            functor(std::forward<Args>(args)...);
//            return Base::operator()(std::forward<Args>(args)...);
//        }
//
//        template<typename Container>
//        auto on_bind(Container* container)
//        {
//            if constexpr(requires {Base::on_bind(container);})
//                return Base::on_bind(container);
//        }
//    };
//
//    template<typename Lambda>
//    struct bind_with_lambda{
//        Lambda lambda;
//        bind_with_lambda(Lambda&& lambda) : lambda(std::forward<Lambda>(lambda)){}
//    };
//
//    template<typename Callable>
//    auto operator | (Callable&& callable, bind_with_lambda b_lambda)
//    {
//        using func_traits = function_traits<decltype(&Callable::operator())>;
//        return [&]<typename Ret, typename... Args>(function_traits<Ret(Args...)>){
//            using callable_t = std::decay_t<Callable>;
//            return unique_handled_wrapper<callable_t, Ret, Args...>{std::forward<Callable>(callable)};
//        }(func_traits());
//    }



    template<typename Callable, typename Ret, typename... Args>
    struct unique_handled_wrapper
    {
        struct empty_t {};

        [[no_unique_address]]
        union copy_wrapper_t
        {
            Callable functor;
            empty_t _;

            copy_wrapper_t(Callable&& functor) : functor(std::move(functor)) {}

            copy_wrapper_t() : _() {}

            copy_wrapper_t(copy_wrapper_t&& other) : functor(std::move(other.functor)) {}

            ~copy_wrapper_t() { functor.~Callable(); }
        } copy_wrapper;

        unique_delegate_handle_ref inv_handle;

        unique_handled_wrapper(Callable&& functor) : copy_wrapper(std::forward<Callable>(functor)), inv_handle() {}

        unique_handled_wrapper(const unique_handled_wrapper&) { assert(false); }

        unique_handled_wrapper(unique_handled_wrapper&&) = default;

        Ret operator()(Args... args)
        {
            auto& functor = copy_wrapper.functor;
            return functor(std::forward<Args>(args)...);
        }

        template<typename Container>
        auto on_bind(Container* container)
        {
            auto& functor = copy_wrapper.functor;
            if constexpr (requires { functor.on_bind(container); })
                functor.on_bind(container);
            return unique_delegate_handle_lambda(container, &inv_handle, [](void* container, delegate_handle_ref* inv)
            {
                auto this_address = (char*) inv - offsetof(unique_handled_wrapper, inv_handle);
                static_cast<Container*>(container)->remove(this_address);
            });
        }
    };

    struct bind_handle_t {};
    inline constexpr bind_handle_t bind_handle;

    template<typename Callable>
    auto operator|(Callable&& callable, bind_handle_t)
    {
        using namespace details;
        using func_traits = function_traits<decltype(&Callable::operator())>;
        return [&]<typename Ret, typename... Args>(function_traits<Ret(Args...)>)
        {
            using callable_t = std::decay_t<Callable>;
            return unique_handled_wrapper<callable_t, Ret, Args...>{std::forward<Callable>(callable)};
        }(func_traits());
    }


    template<typename T, typename FuncT>
    struct weak_binder_impl;

    template<typename T, typename Ret, typename... Args>
    struct weak_binder_impl<T, Ret(Args...)>
    {
        std::weak_ptr<T> obj;
        void* container;

        Ret (* on_expired)(weak_binder_impl* self, Args... args);

        template<typename T_ptr>
        explicit weak_binder_impl(const T_ptr& obj) : obj(obj), container(nullptr), on_expired(nullptr) {}

        template<typename Container>
        void on_bind(Container* c)
        {
            this->container = c;
            on_expired = [](weak_binder_impl* self, Args... args)
            {
                return static_cast<Container*>(self->container)->remove_on_call(self, std::forward<Args>(args)...);
            };
        }

        Ret invoke(auto&& invoker, Args... args)
        {
            if (obj.expired()) return on_expired(this, std::forward<Args>(args)...);
            auto shared = obj.lock();
            return invoker(shared.get(), std::forward<Args>(args)...);
        }
    };

    template<typename T_Ptr>
    struct weak_binder : object_binder_tag
    {
        using type = std::pointer_traits<std::decay_t<T_Ptr>>::element_type;
        const T_Ptr& ptr;

        weak_binder(const T_Ptr& ptr) : ptr(ptr) {}

        template<typename FuncT>
        decltype(auto) convert()
        {
            return weak_binder_impl<type, FuncT>(ptr);
        }
    };


    template<typename T, typename FuncT>
    struct shared_binder_impl;

    template<typename T, typename Ret, typename... Args>
    struct shared_binder_impl<T, Ret(Args...)>
    {
        std::shared_ptr<T> obj;
        void* container;

        Ret (* on_expired)(shared_binder_impl* self, Args... args);

        template<typename T_ptr>
        explicit shared_binder_impl(const T_ptr& obj) : obj(obj) {}

        template<typename Container>
        void on_bind(Container* c)
        {
            container = c;
            on_expired = [](shared_binder_impl* self, Args... args)
            {
                return static_cast<Container*>(self->container)->remove_on_call(self, std::forward<Args>(args)...);
            };
        }

        Ret invoke(auto&& invoker, Args... args)
        {
            assert(obj);
            return invoker(obj.get(), std::forward<Args>(args)...);
        }
    };

    template<typename T>
    struct shared_binder : object_binder_tag
    {
        using type = T;
        const std::shared_ptr<T>& ptr;

        shared_binder(const std::shared_ptr<T>& ptr) : ptr(ptr) {}

        template<typename FuncT>
        decltype(auto) convert()
        {
            return shared_binder_impl<type, FuncT>(ptr);
        }
    };


}


#undef no_unique_address
