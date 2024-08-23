#pragma once
#include <random>

#include "../reference_safe_delegate/reference_safe_delegate.h"
#include "type_list.h"


using namespace auto_delegate;

struct type1
{
    int value;
};

#define ARG_LIST type1 t1, type1& t2, const type1 t1c, const type1& t2c
#define ARG_LIST_FORWARD t1, std::ref(t2), std::forward<const type1>(t1c), std::cref(t2c)

struct A
{
    multicast_delegate<void(ARG_LIST)> event_default;
    multicast_auto_delegate<void(ARG_LIST)> event;
    multicast_auto_delegate<int(ARG_LIST)> event_ret;
    multicast_auto_delegate_extern_ref<void(ARG_LIST)> event_ref;
};

struct B : public generic_ref_reflector
{
    std::string name;
    int value;

    static const uint64_t test_scale = 1 << 4;

    B() = default;

    B(std::string name) : name(name) {}

    void work_load()
    {
//        value = ((value + 21) * (value - 53)) % 1000;
    }

    void action(ARG_LIST) noexcept
    {
        for (int i = 0; i < test_scale; ++i)
            work_load();
    }

    int function(ARG_LIST) noexcept
    {
        for (int i = 0; i < test_scale; ++i)
            work_load();
        return value;
    }

    virtual void virtual_action(ARG_LIST) noexcept
    {
        for (int i = 0; i < test_scale; ++i)
            work_load();
    }

    virtual int virtual_function(ARG_LIST) noexcept
    {
        for (int i = 0; i < test_scale; ++i)
            work_load();
        return value;
    }

    static int static_function(ARG_LIST) noexcept
    {
        int value = 0;
        for (int i = 0; i < test_scale; ++i)
        {

        }
//            work_load();
        return value;
    }

    int operator()(ARG_LIST) noexcept
    {
        for (int i = 0; i < (1 << 11); ++i)
            work_load();
        return value;

    }
};

struct C
{
    std::string name;

    C() = default;

    C(std::string name) : name(name) {}

    void action(ARG_LIST) noexcept
    {

    }

    int function(ARG_LIST) noexcept
    {
        return 0;
    }

    virtual void virtual_action(ARG_LIST) noexcept
    {
    }


    virtual int virtual_function(ARG_LIST) noexcept
    {
        return 0;
    }

    static int static_function(ARG_LIST) noexcept
    {
        return 0;
    }

    int operator()(ARG_LIST) noexcept
    {
        return 0;
    }
};



template<size_t I, typename T>
struct IndexDerived : T
{
};

static constexpr size_t class_count = 1 << 6;
static constexpr size_t object_count = 1 << 8;
static constexpr size_t per_class_count = object_count / class_count;

template<typename BindingType, typename PointerType>
struct TestBindings
{
    std::vector<void*> raw_mem;
    std::vector<PointerType> objects;

    template<size_t I>
    using Derived = IndexDerived<I, BindingType>;


    inline static auto typed_objects = []
    {
        return []<size_t...I>(std::index_sequence<I...>)
        {
            return std::tuple<Derived<I>* ...>{};
        }(std::make_index_sequence<class_count>{});
    }();


    TestBindings()
    {
        raw_mem.reserve(object_count);
        objects.reserve(object_count);
        std::random_device rd;
        std::mt19937 eng(rd());
        for (size_t i = 0; i < per_class_count; ++i)
        {
            [&]<size_t...I>(std::index_sequence<I...>)
            {
                auto lambda = [&]<size_t J>(type_list<Derived<J>>)
                {
                    size_t mem_size = std::uniform_int_distribution<size_t>(1024, 2048)(eng);
                    void* mem = _aligned_malloc(mem_size, alignof(Derived<J>));
                    if (!mem) throw std::bad_alloc();
                    raw_mem.push_back(mem);
                    Derived<J>* b = new(mem) Derived<J>();
                    std::get<J>(typed_objects) = b;
                    objects.push_back(PointerType(b));
                };
                (lambda(type_list<Derived<I>>{}), ...);
            }(std::make_index_sequence<class_count>{});
        }
    }

    ~TestBindings()
    {
        for (auto ptr: raw_mem)
        {
            _aligned_free(ptr);
        }
    }

    static auto& Instance()
    {
        static TestBindings b;
        return b.objects;
    }

};

#define INVOKE_PARAMS PARAMS.v1, PARAMS.v2, PARAMS.v1, PARAMS.v2

struct
{
    type1 v1{1};
    type1 v2{2};
    type1 v3{3};
    type1 v4{4};
} PARAMS;

template<typename DelegateType, typename BindingType, typename PointerType>
void TestTemplate(benchmark::State& state, auto&& bind, auto&& invoke)
{
    DelegateType d{};

    auto& objects = TestBindings<BindingType, PointerType>::Instance();

    std::vector<typename DelegateType::delegate_handle_t> handles;
    handles.reserve(objects.size());

    auto& typed_objects = TestBindings<BindingType, PointerType>::typed_objects;

    for (size_t i = 0; i < per_class_count; ++i)
    {
        [&]<size_t...I>(std::index_sequence<I...>)
        {
            auto lambda = [&]<size_t J>(type_list<IndexDerived<J, BindingType>>)
            {
                auto* ptr = std::get<J>(typed_objects);
                handles.emplace_back(bind(d, ptr));
            };
            (lambda(type_list<IndexDerived<I, BindingType>>{}), ...);
        }(std::make_index_sequence<class_count>{});
    }

    for (auto _: state)
    {
        invoke(d);
    }
}

static void BM_Direct_Function(benchmark::State& state)
{
    auto& objects = TestBindings<B, B*>::Instance();
    for (auto _: state)
    {
        for (auto& o: objects)
        {
            o->function(INVOKE_PARAMS);
        }
    }
}

static void BM_Direct_VirtualFunction(benchmark::State& state)
{
    auto& objects = TestBindings<B, B*>::Instance();
    for (auto _: state)
    {
        for (auto& o: objects)
        {
            o->virtual_function(INVOKE_PARAMS);
        }
    }
}

static void BM_DefaultMulticast_InvokeAction(benchmark::State& state)
{
    TestTemplate<multicast_delegate<void(ARG_LIST)>, B, B*>(
            state,
            [](auto&& d, auto&& ptr)
            {
                using T = std::pointer_traits<std::decay_t<decltype(ptr)>>::element_type;
                return d.template bind<&T::action>(ptr);
            },
            [](auto&& d)
            {
                d.invoke(INVOKE_PARAMS);
            }
    );
}


static void BM_DefaultMulticast_InvokeFunction(benchmark::State& state)
{
    TestTemplate<multicast_delegate<int(ARG_LIST)>, B, B*>(
            state,
            [](auto&& d, auto&& ptr)
            {
                using T = std::pointer_traits<std::decay_t<decltype(ptr)>>::element_type;
                return d.template bind<&T::function>(ptr);
            },
            [](auto&& d)
            {
                d.invoke(INVOKE_PARAMS, [](auto&& iterable)
                {
                    for (auto res: iterable)
                    {
                    }
                });
            }
    );
}


static void BM_DefaultMulticast_InvokeVirtualAction(benchmark::State& state)
{
    TestTemplate<multicast_delegate<void(ARG_LIST)>, B, B*>(
            state,
            [](auto&& d, auto&& ptr)
            {
                using T = std::pointer_traits<std::decay_t<decltype(ptr)>>::element_type;
                return d.template bind<&T::virtual_action>(ptr);
            },
            [](auto&& d)
            {
                d.invoke(INVOKE_PARAMS);
            }
    );
}

static void BM_DefaultMulticast_InvokeVirtualFunction(benchmark::State& state)
{
    TestTemplate<multicast_delegate<int(ARG_LIST)>, B, B*>(
            state,
            [](auto&& d, auto&& ptr)
            {
                using T = std::pointer_traits<std::decay_t<decltype(ptr)>>::element_type;
                return d.template bind<&T::virtual_function>(ptr);
            },
            [](auto&& d)
            {
                d.invoke(INVOKE_PARAMS, [](auto&& iterable)
                {
                    for (auto res: iterable)
                    {
                    }
                });
            }
    );
}

const auto min_warm_up = 0;
const uint64_t invoke_count = 1 << 20;

BENCHMARK(BM_Direct_Function)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);
BENCHMARK(BM_Direct_VirtualFunction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);
BENCHMARK(BM_DefaultMulticast_InvokeAction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);
BENCHMARK(BM_DefaultMulticast_InvokeFunction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);
BENCHMARK(BM_DefaultMulticast_InvokeVirtualAction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);
BENCHMARK(BM_DefaultMulticast_InvokeVirtualFunction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);

template<typename DelegateType>
void BM_WeakMulticast(benchmark::State& state, auto&& bind, auto&& invoke)
{
    DelegateType d{};

    std::vector<std::shared_ptr<C>> objects;
    auto typed_objects = []
    {
        return []<size_t...I>(std::index_sequence<I...>)
        {
            return std::tuple<std::shared_ptr<IndexDerived<I, C>>...>{};
        }(std::make_index_sequence<class_count>{});
    }();

    objects.reserve(object_count);
    std::random_device rd;
    std::mt19937 eng(rd());
    for (size_t i = 0; i < per_class_count; ++i)
    {
        [&]<size_t...I>(std::index_sequence<I...>)
        {
            auto lambda = [&]<size_t J>(type_list<IndexDerived<J, C>>)
            {
                size_t mem_size = std::uniform_int_distribution<size_t>(1024, 2048)(eng);
                void* mem = _aligned_malloc(mem_size, alignof(IndexDerived<J, C>));
                if (!mem) throw std::bad_alloc();
                auto* b = new(mem) IndexDerived<J, C>();
                auto& ptr = std::get<J>(typed_objects) = std::shared_ptr<IndexDerived<J, C>>(b, [](auto* mem)
                {
                    _aligned_free(mem);
                });
                objects.emplace_back(std::static_pointer_cast<C>(ptr));
            };
            (lambda(type_list<IndexDerived<I, C>>{}), ...);
        }(std::make_index_sequence<class_count>{});
    }

    for (size_t i = 0; i < per_class_count; ++i)
    {
        [&]<size_t...I>(std::index_sequence<I...>)
        {
            auto lambda = [&]<size_t J>(type_list<IndexDerived<J, C>>)
            {
                bind(d, std::get<J>(typed_objects));
            };
            (lambda(type_list<IndexDerived<I, C>>{}), ...);
        }(std::make_index_sequence<class_count>{});
    }

    for (auto _: state)
    {
        invoke(d);
    }
}

static void BM_WeakMulticast_InvokeAction(benchmark::State& state)
{
    BM_WeakMulticast<multicast_weak_delegate<void(ARG_LIST)>>(
            state,
            [](auto&& d, auto&& ptr)
            {
                using T = std::pointer_traits<std::decay_t<decltype(ptr)>>::element_type;
                return d.template bind<&T::action>(ptr);
            },
            [](auto&& d)
            {
                d.invoke(INVOKE_PARAMS);
            }
    );
}

static void BM_WeakMulticast_InvokeFunction(benchmark::State& state)
{
    BM_WeakMulticast<multicast_weak_delegate<int(ARG_LIST)>>(
            state,
            [](auto&& d, auto&& ptr)
            {
                using T = std::pointer_traits<std::decay_t<decltype(ptr)>>::element_type;
                return d.template bind<&T::function>(ptr);
            },
            [](auto&& d)
            {
                d.invoke(INVOKE_PARAMS, [](auto&& iterable)
                {
                    for (auto res: iterable)
                    {
                    }
                });
            }
    );
}

static void BM_WeakMulticast_InvokeVirtualAction(benchmark::State& state)
{
    BM_WeakMulticast<multicast_weak_delegate<void(ARG_LIST)>>(
            state,
            [](auto&& d, auto&& ptr)
            {
                using T = std::pointer_traits<std::decay_t<decltype(ptr)>>::element_type;
                return d.template bind<&T::virtual_action>(ptr);
            },
            [](auto&& d)
            {
                d.invoke(INVOKE_PARAMS);
            }
    );
}

static void BM_WeakMulticast_VirtualFunction(benchmark::State& state)
{
    BM_WeakMulticast<multicast_weak_delegate<int(ARG_LIST)>>(
            state,
            [](auto&& d, auto&& ptr)
            {
                using T = std::pointer_traits<std::decay_t<decltype(ptr)>>::element_type;
                return d.template bind<&T::virtual_function>(ptr);
            },
            [](auto&& d)
            {
                d.invoke(INVOKE_PARAMS, [](auto&& iterable)
                {
                    for (auto res: iterable)
                    {
                    }
                });
            }
    );
}

BENCHMARK(BM_WeakMulticast_InvokeAction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);
BENCHMARK(BM_WeakMulticast_InvokeFunction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);
BENCHMARK(BM_WeakMulticast_InvokeVirtualAction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);
BENCHMARK(BM_WeakMulticast_VirtualFunction)
->MinWarmUpTime(min_warm_up)
->Iterations(invoke_count);


#undef ARG_LIST
#undef ARG_LIST_FORWARD