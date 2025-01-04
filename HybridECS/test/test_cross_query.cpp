#include "pch.h"

#include "ecs/static_data_registry.h"
#include "ecs/type/component_group.h"
#include "../test_util/ut.hpp"
#include "../test_util/managed_object_tester.h"
#include "../test_util/memleak_detect.h"

using namespace hyecs;

namespace test_cross_query
{

#define CONCATENATE_DIRECT(a, b) a##b
#define CONCATENATE(a, b) CONCATENATE_DIRECT(a, b)
#define ANON_TYPE using CONCATENATE(_inline_reflect_, __LINE__)
#define ANON CONCATENATE(_ecs_register_, __COUNTER__)

    constexpr auto group_a = named_component_group<"Group A">();
    constexpr auto group_b = named_component_group<"Group B">();
    constexpr auto group_c = named_component_group<"Group C">();
    ecs_rtti_group_register ANON(group_a);
    ecs_rtti_group_register ANON(group_b);
    ecs_rtti_group_register ANON(group_c);

    using tester = managed_object_tester<[]{}>;
    struct A : tester
    {
        A(int x) : tester(x,x+1,x+2,x+3){}
    };

    struct B
    {
        int x;
    };

    struct C
    {
        int x;
    };

    struct D
    {
        int x;
    };

    struct E
    {
        int x;
    };

    struct F
    {
        int x;
    };

//todo untagged zero size types
    struct T1
    {
    };

    struct T2
    {
    };

    struct T3
    {
    };


    ecs_rtti_register<A, group_a> ANON;
    ecs_rtti_register<B, group_a> ANON;
    ecs_rtti_register<C, group_a> ANON;
    ecs_rtti_register<D, group_a> ANON;
    ecs_rtti_register<E, group_a> ANON;
    ecs_rtti_register<F, group_a> ANON;
    ecs_rtti_register<T1, group_a> ANON;
    ecs_rtti_register<T2, group_a> ANON;
    ecs_rtti_register<T3, group_a> ANON;

	using tester_Gb = managed_object_tester<[]{}>;
    struct Gb_A : tester_Gb
    {
        Gb_A(int x) : tester_Gb(x, x + 1, x + 2, x + 3) {}
    };

    struct Gb_B
    {
        int x;
    };

    struct Gb_C
    {
        int x;
    };

    ecs_rtti_register<Gb_A, group_b> ANON;
    ecs_rtti_register<Gb_B, group_b> ANON;
    ecs_rtti_register<Gb_C, group_b> ANON;

	using tester_Gc = managed_object_tester<[] {}>;
    struct Gc_A : tester_Gc
    {
        Gc_A(int x) : tester_Gc(x, x + 1, x + 2, x + 3) {}
    };

    struct Gc_B
    {
        int x;
    };

    struct Gc_C
    {
        int x;
    };

    ecs_rtti_register<Gc_A, group_c> ANON;
    ecs_rtti_register<Gc_B, group_c> ANON;
    ecs_rtti_register<Gc_C, group_c> ANON;




    struct register_idents
    {
        enum
        {
            main,
        };
    };


    class main_registry : public immediate_data_registry<register_idents::main>
    {
        using immediate_data_registry::immediate_data_registry;
    };
}
namespace ut = boost::ut;

static ut::suite test_suite = []
{
    using namespace ut;
    using namespace test_cross_query;

//    "object small scale"_test = []{
//        data_registry registry(ecs_global_rtti_context::register_context);
//
//        vector<entity> entities2(10);
//        registry.emplace_(entities2, A{1}, B{2}, T1{});
//    };
//
//    "manage"_test = []
//    {
//        expect(A::objects == 0) << "A objects should be 0 value: " << A::objects;
//        A::check();
//    };

    "cross query"_test = []
    {
        MemoryLeakDetector detector;


        main_registry registry(ecs_global_rtti_context::register_context());

		A::check_log();
        Gb_A::check_log();
        Gc_A::check_log();

        vector <entity> entities2(10);
        registry.emplace_static(entities2, A{1}, B{2}, T1{});
        A::check_log();
        registry.emplace_static(entities2, A{1}, C{2});
        A::check_log();
        registry.emplace_static(entities2, T1{}, T3{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, E{5}, T1{}, T2{}, T3{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, E{5}, T1{}, T2{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, E{5}, T1{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, E{5}, T2{});
        registry.emplace_static(entities2, A{1}, B{2}, D{4}, E{5}, T2{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, T1{});
        //registry.emplace_static(entities2, A{1} ,Gb_A{1}, Gb_B{2}, Gb_C{3});

        //registry.emplace_static(entities2, A{1} ,Gb_A{1}, Gb_B{2}, Gb_C{3}, C{3}, D{4}, T1{});

        //registry.emplace_static(entities2, A{1} , Gb_B{2}, Gb_C{3}, C{4}, Gc_A{1}, Gc_B{2}, Gc_C{3});

		A::check_log();
        Gb_A::check_log();
		Gc_A::check_log();


        {



        registry.get_query({
                                   {registry.component_types<T1>()},
                                   {}
                           });

        registry.get_query({
                                   {registry.component_types<T1, T2>()},
                                   {}
                           });

        registry.get_query({
                                   {registry.component_types<A, B, T1, T2>()},
                                   {}
                           });

//        auto& q = registry.get_query({
//                                             {registry.component_types<A, B>()},
//                                             {{registry.component_types<C, D>()}, {registry.component_types<T3, T2>()}}
//                                     });
//
//        q.dynamic_for_each(q.get_access_info(registry.component_types<A,B>()),
//        [](entity e ,sequence_ref<void*> data)
//        {
//        	auto [a,b] = data.cast_tuple<A*,B*>();
//        	std::cout << e.id() << " : " << a->x << " " << b->x << std::endl;
//        });

        registry.emplace_static(entities2, A{1}, B{2}, C{3});

        std::cout << "-------------------" << std::endl;

        auto& q = registry.get_query({
                                             {registry.component_types<A, B>()},
                                             {},
                                             {registry.component_types<C>()}
                                     });

        registry.emplace_static(entities2, A{1}, D{4}, E{5}, T1{}, T2{});
        entities2.resize(300);

        registry.emplace_static(entities2, A{1}, B{2});


        q.dynamic_for_each(q.get_access_info(registry.component_types<A, B>()),
                           [](entity e, sequence_ref<void*> data)
                           {
                               auto [a, b] = data.cast_tuple<A*, B*>();
                               expect(a->a == 1 && b->x == 2);
                           });

        auto& qa = registry.get_query({
                                              {registry.component_types<A>()},
                                              {},
                                              {}
                                      });
        int Acounter = 0;
        qa.dynamic_for_each(qa.get_access_info(registry.component_types<A>()),
                            [&Acounter](entity e, sequence_ref<void*> data)
                            {
                                Acounter++;
                            });
        if(!expect(A::object_counter == Acounter))
        {
            A::check_log();
        }

        }

        auto q1 = registry.get_cross_query({
            {registry.component_types<A,B, Gb_A>()},
            {},
            {}
        });

        auto& access_info = q1.get_access_info(registry.component_types<A,B, Gb_A>());
        q1.dynamic_for_each(access_info,
                            [](entity e, sequence_ref<void*> data)
                            {
                                auto [a, b, gb_a] = data.cast_tuple<A*, B*, Gb_A*>();
                                expect(a->a == 1 && b->x == 2 && gb_a->a == 1);
                            });



    };

    "leak"_test = []
    {
        if(!expect(A::object_counter == 0))
        {
            A::check_log();
        }
    };


};
