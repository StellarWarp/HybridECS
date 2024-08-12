#pragma once

#include "../multicast_delegate.h"
#include <iostream>
namespace test_auto_multicast_delegate
{
    struct type1
    {
        int value;
    };

#define ARG_LIST type1 t1, type1& t2, const type1 t1c, const type1& t2c
#define ARG_LIST_FORWARD t1, std::ref(t2), std::forward<const type1>(t1c), std::cref(t2c)

    struct A
    {
        multicast_auto_delegate<void(ARG_LIST)> event;
        multicast_delegate<int(ARG_LIST)> event_ret;
        multicast_auto_delegate_extern_ref<void(ARG_LIST)> event_ref;
    };

    struct B : public generic_ref_reflector
    {
        std::string name;
        B(std::string name) : name(name) {}
        void action(ARG_LIST) noexcept
        {
            std::cout << name << " call " << t1.value << " " << t2.value << " " << t1c.value << " " << t2c.value << std::endl;
        }

        int function(ARG_LIST) noexcept
        {
            std::cout << name << " call " << t1.value << " " << t2.value << " " << t1c.value << " " << t2c.value << std::endl;
            return t1.value + t2.value + t1c.value + t2c.value;
        }
    };

    struct C
    {
        std::string name;
        C(std::string name) : name(name) {}
        void action(ARG_LIST) noexcept
        {
            std::cout << name << " call " << t1.value << " " << t2.value << " " << t1c.value << " " << t2c.value << std::endl;
        }

        int function(ARG_LIST) noexcept
        {
            std::cout << name << " call " << t1.value << " " << t2.value << " " << t1c.value << " " << t2c.value << std::endl;
            return t1.value + t2.value + t1c.value + t2c.value;
        }
    };


    void test()
    {
		std::cout << "test_auto_multicast_delegate" << std::endl;

        A* a = new A();
        auto b1 = new B("b1");
        auto b2 = new B("b2");



        a->event.bind(b1, &B::action);
        a->event.bind(b2, [](auto&& b, ARG_LIST) { b.action(ARG_LIST_FORWARD); });

        struct
        {
            type1 v1{1};
            type1 v2{2};
            type1 v3{3};
        } params;

        a->event.invoke(params.v1, params.v2, params.v1, params.v2);

        auto temp = new B(std::move(*b1));
        delete b1;
        b1 = temp;

        a->event.invoke(params.v1, params.v2, params.v1, params.v2);

        delete b2;

        a->event.invoke(params.v1, params.v2, params.v1, params.v2);

        b2 = new B("new b2");

        auto temp2 = new A(std::move(*a));
        delete a;
        a = temp2;

		a->event.invoke(params.v1, params.v2, params.v1, params.v2);

		std::cout << a;
		
        a->event_ret.bind(b1, &B::function);

        a->event_ret.bind(b2, &B::function);

        a->event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                           [](auto&& results)
                            {
                                for (auto&& ret: results)
                                {
                                    std::cout << "result " << ret << std::endl;
                                }
                            });

		return;

        std::cout << "delete b2" << std::endl;
        delete b2;
        a->event.invoke(params.v1, params.v2, params.v1, params.v2);
        a->event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                            [](auto&& results)
                            {
                                for (auto&& ret: results)
                                {
                                    printf("%d\n", ret);
                                }
                            });

        std::cout << "delete b1" << std::endl;
        delete b1;
        a->event.invoke(params.v1, params.v2, params.v1, params.v2);
        a->event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                            [](auto&& results)
                            {
                                for (auto&& ret: results)
                                {
                                    printf("%d\n", ret);
                                }
                            });
        b1 = new B("new b1");
        b2 = new B("new b2");
        a->event_ret.bind(b1, &B::function);
        a->event_ret.bind(b1, &B::function);
        a->event_ret.bind(b1, &B::function);
        a->event_ret.bind(b1, &B::function);
        a->event_ret.bind(b1, &B::function);
        a->event_ret.bind(b1, &B::function);
        a->event_ret.bind(b1, &B::function);
        a->event_ret.bind(b1, &B::function);
        a->event_ret.bind(b2, &B::function);
        a->event_ret.invoke(params.v1, params.v2, params.v1, params.v2,
                            [](auto&& results)
                            {
                                for (auto&& ret: results)
                                {
                                    printf("%d\n", ret);
                                }
                            });
        delete b1;
        delete a;
        delete b2;

        a = new A();
        unique_reference c = new reference_reflector<C>("c");
		reference_reflector<C>;
		std::cout << intptr_t((C*)(reference_reflector<C>*)0xFFFFFFFF) - intptr_t((reference_reflector<C>*)0xFFFFFFFF) << std::endl;
		std::cout << intptr_t((C*)(reference_reflector<C>*)c.get()) - intptr_t((reference_reflector<C>*)c.get()) << std::endl;
		std::cout << intptr_t((extern_reflector_generic_object_t*)(reference_reflector<extern_reflector_generic_object_t>*)0xFFFFFFFF) - intptr_t((reference_reflector<extern_reflector_generic_object_t>*)0xFFFFFFFF) << std::endl;


        a->event_ref.bind(c, &C::action);
        a->event_ref.bind(c, [](auto&& o, ARG_LIST) { o.action(ARG_LIST_FORWARD); });

        a->event_ref.invoke(params.v1, params.v2, params.v1, params.v2);
        delete a;

    }

#undef ARG_LIST
#undef ARG_LIST_FORWARD
}
