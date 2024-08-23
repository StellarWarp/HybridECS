
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

    struct
    {
        type1 v1{1};
        type1 v2{2};
        type1 v3{3};
    } params;
#define PARAM_LIST params.v1, params.v2, std::move(params.v3),params.v1, params.v2, std::move(params.v3)

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


}

TEST(auto_delegate, unsafe)
{
    using namespace test_auto_delegate;
    using A = delegate<int(ARG_LIST)>;
    A* a = new A();
    auto b1 = new B("b1");

    a->bind<&B::function>(b1);
    a->invoke(PARAM_LIST);

    a->invoke(PARAM_LIST);

    auto b2 = new B("new b2");
    a->bind(b2, [](auto&& b, ARG_LIST) { return b.virtual_function(ARG_LIST_FORWARD); });
    a->invoke(PARAM_LIST);

    auto temp2 = new A(std::move(*a));
    delete a;
    a = temp2;

    a->invoke(PARAM_LIST);

    a->bind(b2);
    a->invoke(PARAM_LIST);

    delete a;
    delete b2;
}

TEST(auto_delegate, weak_ptr)
{
    using namespace test_auto_delegate;
    using A = weak_delegate<int(ARG_LIST)>;

    A* a = new A();
    auto b1 = new B("b1");

    a = new A();
    auto bw1 = std::make_shared<B>("bw1");
    a->bind<&B::function>(bw1);
    a->invoke(PARAM_LIST);

    a->bind(bw1, [](auto&& b, ARG_LIST) { return b.virtual_function(ARG_LIST_FORWARD); });
    a->invoke(PARAM_LIST);

    a->bind(bw1);
    a->invoke(PARAM_LIST);

    a->bind([](ARG_LIST)
            {
                std::cout << "call pure virtual_function lambda" << std::endl;
                return 0;
            });
    a->invoke(PARAM_LIST);

    delete a;
}

TEST(auto_delegate, shared_ptr)
{
    using namespace test_auto_delegate;
    using A = shared_delegate<int(ARG_LIST)>;

    A* a = new A();
    auto b1 = new B("b1");


    a = new A();
    auto bw1 = std::make_shared<B>("bw1");

    a->bind<&B::function>(bw1);
    a->invoke(PARAM_LIST);

    a->bind(bw1, [](auto&& b, ARG_LIST) { return b.virtual_function(ARG_LIST_FORWARD); });
    a->invoke(PARAM_LIST);

    a->bind(bw1);
    a->invoke(PARAM_LIST);

    a->bind([](ARG_LIST)
                         {
                             std::cout << "call pure virtual_function lambda" << std::endl;
                             return 0;
                         });
    a->invoke(PARAM_LIST);

    delete a;
}

#undef ARG_LIST
#undef ARG_LIST_FORWARD
