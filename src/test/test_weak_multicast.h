#pragma once

#include "../delegate/multicast_delegate.h"

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
            std::cout << "call action" << t1.value << " " << t2.value << " " << t1c.value << " " << t2c.value << std::endl;
        }

        int function(ARG_LIST) noexcept
        {
            std::cout << "call virtual_function" << t1.value << " " << t2.value << " " << t1c.value << " " << t2c.value << std::endl;
            return t1.value + t2.value + t1c.value + t2c.value;
            return 0;
        }
    };

    void test()
    {
		std::cout << "test_weak_multicast_delegate" << std::endl;

        A2 a2;
        std::vector<std::shared_ptr<B2>> bs;
        for (int i = 0; i < 10; ++i)
        {
            auto b = std::make_shared<B2>();
            bs.push_back(b);
        }

        std::cout <<"normal bind" << std::endl;

        for (auto b: bs)
            a2.event.bind<&B2::action>(b);
        for (auto b: bs)
            a2.event.bind(b, [](auto&& b, ARG_LIST) { b.action(ARG_LIST_FORWARD); });
        for (auto b: bs)
            a2.event_ret.bind<&B2::function>(b);

        struct
        {
            type1 v1{1};
            type1 v2{2};
            type1 v3{3};
        } params;
        a2.event.invoke(params.v1, params.v2, params.v1, params.v2);
        a2.event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                            [](auto&& results)
                            {
                                for (auto&& ret: results)
                                    std::cout << ret << std::endl;
                            });
        //random remove
        for(int i = 0; i < 5; ++i)
        {
            int idx = rand() % bs.size();
            bs.erase(bs.begin() + idx);
        }
        std::cout <<"after remove" << std::endl;


        a2.event.invoke(params.v1, params.v2, params.v1, params.v2);
        a2.event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                            [](auto&& results)
                            {
                                for (auto&& ret: results)
                                    std::cout << ret << std::endl;
                            });

        auto b = std::make_shared<B2>();
        auto handle = a2.event_ret.bind<&B2::function>(b);
        a2.event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                            [](auto&& results)
                            {
                                for (auto&& ret: results)
                                    std::cout << ret << std::endl;
                            });
        a2.event_ret.unbind(handle);
        a2.event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                            [](auto&& results)
                            {
                                for (auto&& ret: results)
                                    std::cout << ret << std::endl;
                            });



    }

#undef ARG_LIST
#undef ARG_LIST_FORWARD
}
