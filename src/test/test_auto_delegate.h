
#pragma once

#include "../delegate/delegate.h"
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
        weak_delegate<int(ARG_LIST)> event_weak;
        shared_delegate<int(ARG_LIST)> event_shared;
    };

    struct B : public generic_ref_reflector
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

        virtual int virtual_function(ARG_LIST) noexcept
        {
            std::cout << name << " call virtual_function "
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
		std::cout << "test auto delegate" << std::endl;

        A* a = new A();
        auto b1 = new B("b1");

//        weak_reference<generic_ref_reflector,generic_ref_reflector> r = b1;

        struct
        {
            type1 v1{1};
            type1 v2{2};
            type1 v3{3};
        } params;
#define PARAM_LIST params.v1, params.v2, std::move(params.v3),params.v1, params.v2, std::move(params.v3)

        a->event_ret.bind<&B::function>(b1);
        a->event_ret.invoke(PARAM_LIST);

        auto temp = new B(std::move(*b1));
        delete b1;
        b1 = temp;

        a->event_ret.invoke(PARAM_LIST);

        std::cout << "delete b1" << std::endl;
        delete b1;
        std::cout << a->event_ret.operator bool() << std::endl;

        auto b2 = new B("new b2");
        a->event_ret.bind(b2, [](auto&& b, ARG_LIST) { return b.virtual_function(ARG_LIST_FORWARD); });
        a->event_ret.invoke(PARAM_LIST);

        auto temp2 = new A(std::move(*a));
        delete a;
        a = temp2;

        a->event_ret.invoke(PARAM_LIST);

        a->event_ret.bind(b2);
        a->event_ret.invoke(PARAM_LIST);

        delete a;
        delete b2;

        a = new A();
        auto bw1 = std::make_shared<B>("bw1");
        a->event_weak.bind<&B::function>(bw1);
        a->event_weak.invoke(PARAM_LIST);
        a->event_weak.bind(bw1, [](auto&& b, ARG_LIST) { return b.virtual_function(ARG_LIST_FORWARD); });
        a->event_weak.invoke(PARAM_LIST);
        a->event_weak.bind(bw1);
        a->event_weak.invoke(PARAM_LIST);
        a->event_weak.bind([](ARG_LIST) {
            std::cout << "call pure virtual_function lambda" << std::endl;
            return 0; });
        a->event_weak.invoke(PARAM_LIST);

        a->event_shared.bind<&B::function>(bw1);
        a->event_shared.invoke(PARAM_LIST);
        a->event_shared.bind(bw1, [](auto&& b, ARG_LIST) { return b.virtual_function(ARG_LIST_FORWARD); });
        a->event_shared.invoke(PARAM_LIST);
        a->event_shared.bind(bw1);
        a->event_shared.invoke(PARAM_LIST);
        a->event_shared.bind([](ARG_LIST) {
            std::cout << "call pure virtual_function lambda" << std::endl;
            return 0; });
        a->event_shared.invoke(PARAM_LIST);

        delete a;
    }

#undef ARG_LIST
#undef ARG_LIST_FORWARD
}
