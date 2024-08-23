#pragma once

#include "../reference_safe_delegate/reference_safe_delegate.h"
#include <random>
#include <iostream>

using namespace auto_delegate;

inline uint64_t invoke_hash = 0;
inline uint64_t static_hash = rand();


namespace test_multicast
{
    struct type1
    {
        int value;

        auto operator<=>(const type1&) const = default;
    };

#define ARG_LIST type1 t1, type1& t2, const type1 t1c, const type1& t2c
#define ARG_LIST_FORWARD t1, std::ref(t2), std::forward<const type1>(t1c), std::cref(t2c)

    struct
    {
        type1 v1{1};
        type1 v2{2};
        type1 v3{3};
        type1 v4{4};
    } params;

#define PARAM_LIST params.v1, params.v2, params.v3, params.v4


    struct B : public generic_ref_reflector
    {
        std::string name;

        B(std::string name) : name(name) {}

        uint64_t hash()
        {
            return std::hash<std::string>()(name);
        }

        void action(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
        }

        int function(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
            return 0;
        }

        virtual void virtual_action(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
        }

        virtual int virtual_function(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
            return 0;
        }

        int operator()(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
            return 0;
        }

        static int static_function(ARG_LIST) noexcept
        {
            invoke_hash += static_hash;
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
            return 0;
        }

    };


    struct C
    {
        std::string name;

        C() = default;

        C(std::string name) : name(name) {}

        uint64_t hash()
        {
            return std::hash<std::string>()(name);
        }

        void action(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
        }

        int function(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
            return 0;
        }

        virtual void virtual_action(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
        }

        virtual int virtual_function(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
            return 0;
        }

        int operator()(ARG_LIST) noexcept
        {
            invoke_hash += hash();
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
            return 0;
        }

        static int static_function(ARG_LIST) noexcept
        {
            invoke_hash += static_hash;
            assert(t1 == params.v1);
            assert(t2 == params.v2);
            assert(t1c == params.v3);
            assert(t2c == params.v4);
            return 0;
        }
    };

}

TEST(auto_multicast, test_action)
{
    using namespace test_multicast;
    using A = multicast_auto_delegate<void(ARG_LIST)>;

    auto a = new A();
    auto b1 = new B("b1");
    auto b2 = new B("b2");

    a->bind<B, &B::action>(b1);

    a->bind(b2, [](auto&& b, ARG_LIST) { b.action(ARG_LIST_FORWARD); });


    invoke_hash = 0;
    a->invoke(PARAM_LIST);
    ASSERT_EQ(invoke_hash, b1->hash() + b2->hash());


    auto temp = new B(std::move(*b1));
    delete b1;
    b1 = temp;

    invoke_hash = 0;
    a->invoke(PARAM_LIST);
    ASSERT_EQ(invoke_hash, b1->hash() + b2->hash());

    delete b2;

    invoke_hash = 0;
    a->invoke(PARAM_LIST);
    ASSERT_EQ(invoke_hash, b1->hash());

    b2 = new B("new b2");

    a->bind(b2, [](auto&& b, ARG_LIST) { b.action(ARG_LIST_FORWARD); });


    auto tempa = new A(std::move(*a));
    delete a;
    a = tempa;

    invoke_hash = 0;
    a->invoke(PARAM_LIST);
    ASSERT_EQ(invoke_hash, b1->hash() + b2->hash());
}

TEST(auto_multicast, test_function)
{
    using namespace test_multicast;
    using A = multicast_auto_delegate<int(ARG_LIST)>;

    A* a = new A();
    auto b1 = new B("b1");
    auto b2 = new B("b2");

    size_t hash = 0;

    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();

    a->bind<&B::virtual_function>(b2);
    hash += b2->hash();

    auto static_function_h
            = a->bind<&B::static_function>();
    hash += static_hash;

    a->bind(b1);
    hash += b1->hash();

    auto static_lambda_h
            = a->bind([](ARG_LIST)
                      {
                          invoke_hash += static_hash;
                          assert(t1 == params.v1);
                          assert(t2 == params.v2);
                          assert(t1c == params.v3);
                          assert(t2c == params.v4);
                          return 0;
                      });
    hash += static_hash;

    invoke_hash = 0;
    a->invoke(PARAM_LIST,
              [](auto&& results)
              {
                  for (auto&& ret: results)
                  {
                      ASSERT_EQ(ret, 0);
                  }
              });
    ASSERT_EQ(invoke_hash, hash);


    hash -= b2->hash();
    hash -= b1->hash();
    hash -= b1->hash();
    delete b2;
    delete b1;

    invoke_hash = 0;
    a->invoke(PARAM_LIST,
              [](auto&& results)
              {
                  for (auto&& ret: results)
                  {
                      ASSERT_EQ(ret, 0);
                  }
              });
    ASSERT_EQ(invoke_hash, hash);


    a->unbind(static_lambda_h);
    hash -= static_hash;
    a->unbind(static_function_h);
    hash -= static_hash;

    invoke_hash = 0;
    a->invoke(PARAM_LIST,
              [](auto&& results)
              {
                  for (auto&& ret: results)
                  {
                      ASSERT_EQ(ret, 0);
                  }
              });
    ASSERT_EQ(invoke_hash, hash);


    b1 = new B("new b1");
    b2 = new B("new b2");
    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();
    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();
    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();
    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();
    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();
    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();
    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();
    a->bind<&B::virtual_function>(b1);
    hash += b1->hash();

    auto tempa = new A(std::move(*a));
    delete a;
    a = tempa;

    invoke_hash = 0;
    a->invoke(PARAM_LIST,
              [](auto&& results)
              {
                  for (auto&& ret: results)
                  {
                      ASSERT_EQ(ret, 0);
                  }
              });
    ASSERT_EQ(invoke_hash, hash);

    delete b1;
    delete a;
    delete b2;
}

TEST(auto_multicast, test_extern_ref)
{
    using namespace test_multicast;
    using A = multicast_auto_delegate_extern_ref<void(ARG_LIST)>;

    auto a = new A();
    unique_reference c = new reference_reflector<C>("c");

    a->bind<&C::action>(c);
    a->bind(c, [](auto&& o, ARG_LIST) { o.action(ARG_LIST_FORWARD); });

    auto tempa = new A(std::move(*a));
    delete a;
    a = tempa;

    invoke_hash = 0;
    a->invoke(PARAM_LIST);
    ASSERT_EQ(invoke_hash, c->hash() + c->hash());

    delete a;
}

TEST(auto_multicast, unique_handle)
{
    using namespace test_multicast;
    using A = multicast_delegate<void(ARG_LIST)>;

    auto a = new A();
    unique_reference c = new reference_reflector<C>("c");

    {
        auto h1
                = a->bind<&C::action>(c);
        auto h2
                = a->bind(c, [](auto&& o, ARG_LIST) { o.action(ARG_LIST_FORWARD); });

        invoke_hash = 0;
        a->invoke(PARAM_LIST);
        ASSERT_EQ(invoke_hash, c->hash() + c->hash());
    }
    invoke_hash = 0;
    a->invoke(PARAM_LIST);
    ASSERT_EQ(invoke_hash, 0);


    {
        auto h1
                = a->bind<&C::action>(c);
        auto h2
                = a->bind(c, [](auto&& o, ARG_LIST) { o.action(ARG_LIST_FORWARD); });

        auto tempa = new A(std::move(*a));
        delete a;
        a = tempa;

        invoke_hash = 0;
        a->invoke(PARAM_LIST);
        ASSERT_EQ(invoke_hash, c->hash() + c->hash());
    }
    invoke_hash = 0;
    a->invoke(PARAM_LIST);
    ASSERT_EQ(invoke_hash, 0);


    delete a;
}

#undef ARG_LIST
#undef ARG_LIST_FORWARD
#undef PARAM_LIST