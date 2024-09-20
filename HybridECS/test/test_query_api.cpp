#include "pch.h"

#include "ecs/static_data_registry.h"
#include "ecs/component_group.h"
#include "ut.hpp"

using namespace hyecs;


constexpr auto group_a = component_group_id("Group A");
constexpr auto group_b = component_group_id("Group B");
constexpr auto group_c = component_group_id("Group C");
inline const auto _ = ecs_static_register_context::register_context.add_group("Group A");
inline const auto __ = ecs_static_register_context::register_context.add_group("Group B");
inline const auto ___ = ecs_static_register_context::register_context.add_group("Group C");

struct A
{
    int x;
    // A(int x) : x(x)
    // {
    // 	std::cout << "A constructor" << std::endl;
    // }
    // A(A&& other) noexcept : x(other.x)
    // {
    // 	std::cout << "A move constructor" << std::endl;
    // }
    // A(const A& other) : x(other.x)
    // {
    // 	std::cout << "A copy constructor" << std::endl;
    // }
    // ~A()
    // {
    // 	std::cout << "A destructor" << std::endl;
    // }
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

struct T1
{
};

struct T2
{
};

struct T3
{
};

//#ifdef __JETBRAINS_IDE__
//template<typename T>
//struct component_static_group_override{};
//template<typename T>
//struct component_register_static : std::false_type{};
//#endif

REGISTER_HYECS_COMPONENT(A, group_a);

REGISTER_HYECS_COMPONENT(B, group_a);

REGISTER_HYECS_COMPONENT(C, group_a);

REGISTER_HYECS_COMPONENT(D, group_a);

REGISTER_HYECS_COMPONENT(E, group_a);

REGISTER_HYECS_COMPONENT(F, group_a);

REGISTER_HYECS_COMPONENT(T1, group_a);

REGISTER_HYECS_COMPONENT(T2, group_a);

REGISTER_HYECS_COMPONENT(T3, group_a);


struct parent
{
};

struct child
{
};

REGISTER_HYECS_COMPONENT(parent, group_a);

REGISTER_HYECS_COMPONENT(child, group_a);

struct A2
{
    int x;
};

REGISTER_HYECS_COMPONENT(A2, group_b);

struct B2
{
    int x;
};

REGISTER_HYECS_COMPONENT(B2, group_b);

struct A3
{
    int x;
};

REGISTER_HYECS_COMPONENT(A3, group_c);

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

namespace ut = boost::ut;

void tset_func() noexcept
{

}

static ut::suite test_suite = []
{
    using namespace ut;

    "query api"_test = []
    {
        data_registry registry(ecs_static_register_context::register_context);

        executer_builder builder(registry);

        using namespace query_parameter;

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
                relation<parent(const A&,
                                B&,
                                none_of<C, D>,
                                relation<parent(
                                        const A&,
                                        B&
                                )>)> rel1,
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
