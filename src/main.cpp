#include <iostream>
#include <memory>
#include "multicast_delegate.h"
class Test
{
};


class A : std::enable_shared_from_this<A>
{
public:
    multicast_auto_delegate<void(int)> event;
    multicast_auto_delegate<int(int, Test&)> event2;
};

class B : public reference_handle
{
public:
    void call(int i) noexcept
    {
        printf("call %d\n", i);
    }
};


class C : public reference_handle
{
public:
    int call(int i, Test& t) noexcept
    {
        return i;
    }
};

size_t alloc_mem = 0;

//void* operator new(std::size_t size)
//{
//    printf("new for %lld bytes\n", size);
//    alloc_mem += size;
//    return std::malloc(size);
//}
//
//void operator delete(void* ptr, std::size_t size)
//{
//    printf("delete for %lld bytes\n", size);
//    alloc_mem -= size;
//    std::free(ptr);
//}


void test_func()
{
    A* a = new A();
    B* b = new B();

    a->event.bind<&B::call>(b);
    a->event.invoke(1);
    B* b1 = new B(std::move(*b));
    delete b;
    a->event.invoke(1);
    auto b2 = std::make_unique<B>();
    a->event.bind(b2.get(), [](auto&& b, int i) { b.call(i); });

    C c;
    a->event2.bind<&C::call>(&c);

    A* a1 = new A(std::move(*a));
    delete a;
    a = a1;

    Test t{};

    a->event2.invoke(3, t, [](auto&& results)
    {
        for (auto&& ret: results)
        {
            printf("%d\n", ret);
        }
    });

    delete b1;
    a->event.invoke(2);
    b = new B();
    a->event.bind<&B::call>(b);
    delete a;

    delete b;
}


struct type1
{
    int value;
};

#define ARG_LIST type1 t1, type1& t2, const type1 t1c, const type1& t2c
#define ARG_LIST_FORWARD t1, std::ref(t2), std::forward<const type1>(t1c), std::cref(t2c)

struct A2
{
    multicast_weak_delegate<void(ARG_LIST)> event;
    multicast_weak_delegate<int(ARG_LIST)> event_ret;
};

struct B2
{
    void action(ARG_LIST) noexcept
    {
        printf("call %d %d %d %d\n", t1.value, t2.value, t1c.value, t2c.value);
    }

    int function(ARG_LIST) noexcept
    {
        printf("call %d %d %d %d\n", t1.value, t2.value, t1c.value, t2c.value);
        return t1.value + t2.value + t1c.value + t2c.value;
        return 0;
    }
};

void test_func2()
{
    A2 a2;
    auto b = std::make_shared<B2>();


    a2.event.bind(b,&B2::action);
    a2.event.bind(b, [](auto&& b, ARG_LIST) { b.action(ARG_LIST_FORWARD); });
    struct
    {
        type1 v1{1};
        type1 v2{2};
        type1 v3{3};
    } params;
    a2.event.invoke(params.v1, params.v2, params.v1, params.v2);
    a2.event_ret.bind(b, &B2::function);
    {
        auto b2 = std::make_shared<B2>();
        a2.event_ret.bind(b2, &B2::function);
    }
    a2.event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                        [](auto&& results)
                        {
                            for (auto&& ret: results)
                            {
                                printf("%d\n", ret);
                            }
                        });
}
#undef ARG_LIST
#undef ARG_LIST_FORWARD
#define ARG_LIST type1 t1, type1& t2, type1&& t3, const type1 t1c, const type1& t2c, const type1&& t3c
#define ARG_LIST_FORWARD t1, std::ref(t2), std::move(t3), std::forward<const type1>(t1c), std::cref(t2c), std::move(t3c)

struct B3
{
    void action(ARG_LIST) noexcept
    {
        printf("call %d %d %d %d %d %d\n", t1.value, t2.value, t1c.value, t2c.value, t3.value, t3c.value);
    }

    int function(ARG_LIST) noexcept
    {
        printf("call %d %d %d %d %d %d\n", t1.value, t2.value, t1c.value, t2c.value, t3.value, t3c.value);
        return 0;
    }
};

void test3()
{
    struct
    {
        type1 v1{1};
        type1 v2{2};
        type1 v3{3};
    } params;
#define PARAM_LIST params.v1, params.v2, std::move(params.v3),params.v1, params.v2, std::move(params.v3)


    B3 b;


    delegate d(&b, &B3::function);
    d.invoke(PARAM_LIST);

    d.bind(&b, [](auto&& b, ARG_LIST) { return b.function(ARG_LIST_FORWARD); });
    d.invoke(PARAM_LIST);

    d.bind([](ARG_LIST) { return 1111; });
    auto res = d.invoke(PARAM_LIST);
    printf("res %d\n", res);

    auto lambda = [&](ARG_LIST)
    {
        return b.function(PARAM_LIST);
    };
    d.bind(lambda);
    d.invoke(PARAM_LIST);
}

int main()
{

    if (alloc_mem != 0)
        printf("memory leak: %lld bytes\n", alloc_mem);
}
