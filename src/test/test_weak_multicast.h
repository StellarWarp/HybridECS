#pragma once

#include "../multicast_delegate.h"

namespace test_weak_multicast_delegate
{
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
}
