#pragma once

#include "../delegate.h"

namespace test_delegate
{
    struct type1
    {
        int value;
    };

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

#undef ARG_LIST
#undef ARG_LIST_FORWARD
#undef PARAM_LIST
}
