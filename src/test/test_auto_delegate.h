
#pragma once

#include "../delegate.h"
#include <iostream>
#include <functional>


namespace test_auto_delegate
{

    struct type1
    {
        int value;
    };

#define ARG_LIST type1 t1, type1& t2, type1&& t3, const type1 t1c, const type1& t2c, const type1&& t3c
#define ARG_LIST_FORWARD t1, std::ref(t2), std::move(t3), std::forward<const type1>(t1c), std::cref(t2c), std::move(t3c)

    struct A
    {
        auto_delegate<int(ARG_LIST)> event_ret;
    };

    struct B : public array_ref_charger<false>
    {
        std::string name;

        B(std::string name) : name(name) {}

        int function(ARG_LIST) noexcept
        {
            std::cout << name << " call function "
                      << t1.value << " " << t2.value << " " << t3.value << " "
                      << t1c.value << " " << t2c.value << " " << t3c.value << std::endl;
            return t1.value + t2.value + t3.value + t1c.value + t2c.value + t3c.value;
        }

        int operator()(ARG_LIST) noexcept
        {
            std::cout << name << " call operator() "
                      << t1.value << " " << t2.value << " " << t3.value << " "
                      << t1c.value << " " << t2c.value << " " << t3c.value << std::endl;
            return t1.value + t2.value + t3.value + t1c.value + t2c.value + t3c.value;
        }
    };

    void test()
    {
        A* a = new A();
        auto b1 = new B("b1");

        struct
        {
            type1 v1{1};
            type1 v2{2};
            type1 v3{3};
        } params;
#define PARAM_LIST params.v1, params.v2, std::move(params.v3),params.v1, params.v2, std::move(params.v3)

        a->event_ret.bind(b1, &B::function);
        a->event_ret.invoke(PARAM_LIST);

        auto temp = new B(std::move(*b1));
        delete b1;
        b1 = temp;

        a->event_ret.invoke(PARAM_LIST);

        std::cout << "delete b1" << std::endl;
        delete b1;
        std::cout << a->event_ret.operator bool() << std::endl;

        auto b2 = new B("new b2");
        a->event_ret.bind(b2, [](auto&& b, ARG_LIST) { return b.function(ARG_LIST_FORWARD); });
        a->event_ret.invoke(PARAM_LIST);

        auto temp2 = new A(std::move(*a));
        delete a;
        a = temp2;

        a->event_ret.invoke(PARAM_LIST);

        a->event_ret.bind(*b2);
        a->event_ret.invoke(PARAM_LIST);

        delete a;
        delete b2;


    }

#undef ARG_LIST
#undef ARG_LIST_FORWARD
}
