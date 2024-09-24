#include "pch.h"

#include "ecs/data_registry.h"
#include "ecs/type/component_group.h"
#include "../test_util/ut.hpp"
#include "../test_util/managed_object_tester.h"
#include "ecs/data_registry.h"

using namespace hyecs;


namespace test_query_api
{
#define CONCATENATE_DIRECT(a, b) a##b
#define CONCATENATE(a, b) CONCATENATE_DIRECT(a, b)
#define ANON_TYPE using CONCATENATE(_inline_reflect_, __LINE__)
#define ANON CONCATENATE(_inline_reflect_, __LINE__)

    constexpr auto group_a = component_group_id("Group A");
    constexpr auto group_b = component_group_id("Group B");
    constexpr auto group_c = component_group_id("Group C");
    ecs_rtti_group_register ANON("Group A");
    ecs_rtti_group_register ANON("Group B");
    ecs_rtti_group_register ANON("Group C");

    using tester = managed_object_tester<[] {}>;

    struct A : tester
    {
        A(int x) : tester(x, x + 1, x + 2, x + 3) {}
    };

    struct B
    {
        int x;
    };

    static_assert(type_hash::of<A>() != type_hash::of<B>());

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

}
namespace ut = boost::ut;


static ut::suite test_suite = []
{
    using namespace ut;
    using namespace test_query_api;


    "query api"_test = []
    {
        data_registry registry(ecs_global_rtti_context::register_context());

        executer_builder builder(registry);

        struct parent;
        struct child;

        using namespace hyecs::query_parameter;

        const auto x = std::is_base_of_v<
                entity_relation_param,
                relation<parent(const A&, B&)>
        >;
        static_assert(x);

        auto lambda = [](
                const A& a,
                B& b,
                optional<const C&> c,
                D& d,
                none_of<E, F>,

                relation<parent(
                        const A&,
                        B&,
                        none_of<C, D>,

                        relation<parent(
                                const A&,
                                B&
                        )>
                )> rel1,

                multi_relation<child(
                        A&,
                        B&
                )> rel2
        )
        {
            auto& [a1, b1, rel11] = rel1.components;
            auto& [a11, b11] = rel11.components;
        };


        builder.register_executer(lambda);

        struct relation1;
        struct relation2;
        struct relation3;

        // clang-format off
        // @formatter:off
        builder.register_executer([](
                const A& a,
                B& b,
                D& d,
                optional<const C&> c,
                none_of<E, F>,

                relation_ref<"o1">,
                relation_ref<"o2">,

                begin_rel_scope<relation1, "o1">,
                    const A& a1,
                    B& b1,
                    none_of<C, D>,
                    relation_ref<"o3">,
                end_rel_scope,

                begin_rel_scope<relation2, "o2">,
                    const A& a2,
                    const B& b2,
                    const C& c2,
                    relation_ref<"o3">,
                end_rel_scope,

                begin_rel_scope<relation3, "o3">,
                    A& a3,
                    B& b3,
                end_rel_scope,

                multi_relation<child(
                        A&,
                        B&
                )> rel2
        )// clang-format on // @formatter:on
        {

        });


        // registry.query_with()
        //.all<B2,A,A3,B,A2>()
        //.for_each(
        //	[](entity e, const B& b, const A& a, storage_key key)
        //	{
        //		std::cout << e.id() << " "
        //		<< "Sort key : " << (uint32_t)key.get_table_offset() << " "
        //		<< "A : " << a.x << " "
        //		<< "B : " << b.x << std::endl;
        //	});
    };
};
