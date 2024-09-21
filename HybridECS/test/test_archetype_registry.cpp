#include "pch.h"

#include "ecs/static_data_registry.h"
#include "ecs/type/component_group.h"
#include "ut.hpp"

using namespace hyecs;


constexpr auto group_a = component_group_id("Group A");
constexpr auto group_b = component_group_id("Group B");
constexpr auto group_c = component_group_id("Group C");
inline const auto _ = ecs_global_rtti_context::register_context.add_group("Group A");
inline const auto __ = ecs_global_rtti_context::register_context.add_group("Group B");
inline const auto ___ = ecs_global_rtti_context::register_context.add_group("Group C");

struct A
{
    int x;
    size_t s;
    static inline size_t objects;
    static inline size_t seq = 0;
    static inline auto flags = vector<bool>(1000, false);
    static inline auto flags_ = vector<bool>(1000, false);

    static inline size_t constructor_call;
    static inline size_t move_call;
    static inline size_t copy_call;
    static inline size_t delete_call;
    static void before_check()
    {
        flags_ = flags;
    }
    static void check()
    {
        for (size_t i = 0; i < flags.size(); i++)
        {
            boost::ut::expect(flags[i] == false) << "leaked object at " << i;
        }
    }

    A(int x) : x(x)
    {
        s = seq;
        flags[seq++] = true;
        objects++;
        constructor_call++;
    }

    A(A&& other) noexcept: x(other.x)
    {
        s = seq;
        flags[seq++] = true;
        objects++;
        move_call++;
    }

    A(const A& other) : x(other.x)
    {
        s = seq;
        flags[seq++] = true;
        objects++;
        copy_call++;
    }

    ~A()
    {
        flags[s] = false;
        delete_call++;
        objects--;
    }
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


#define CONCATENATE_DIRECT(a, b) a##b
#define CONCATENATE(a, b) CONCATENATE_DIRECT(a, b)
#define ANON_TYPE using CONCATENATE(_inline_reflect_, __LINE__)
#define ANON CONCATENATE(_inline_reflect_, __LINE__)

ecs_rtti_register<A, group_a> ANON;
ecs_rtti_register<B, group_a> ANON;
ecs_rtti_register<C, group_a> ANON;
ecs_rtti_register<D, group_a> ANON;
ecs_rtti_register<E, group_a> ANON;
ecs_rtti_register<F, group_a> ANON;
ecs_rtti_register<T1, group_a> ANON;
ecs_rtti_register<T2, group_a> ANON;
ecs_rtti_register<T3, group_a> ANON;

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

static ut::suite test_suite = []
{
    using namespace ut;

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

    "registry"_test = []
    {


        main_registry registry(ecs_global_rtti_context::register_context);


        vector <entity> entities2(10);
        registry.emplace_static(entities2, A{1}, B{2}, T1{});
        registry.emplace_static(entities2, A{1}, C{2});
        registry.emplace_static(entities2, T1{}, T3{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, E{5}, T1{}, T2{}, T3{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, E{5}, T1{}, T2{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, E{5}, T1{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, E{5}, T2{});
        registry.emplace_static(entities2, A{1}, B{2}, D{4}, E{5}, T2{});
        registry.emplace_static(entities2, A{1}, B{2}, C{3}, D{4}, T1{});


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

        ut::log << std::format("object {}, ctor {}, move {}, copy {}, delete {}\n",
                               A::objects, A::constructor_call, A::move_call, A::copy_call, A::delete_call);
        registry.emplace_static(entities2, A{1}, B{2});
        ut::log << std::format("object {}, ctor {}, move {}, copy {}, delete {}\n",
                               A::objects, A::constructor_call, A::move_call, A::copy_call, A::delete_call);
//        A::check();


        q.dynamic_for_each(q.get_access_info(registry.component_types<A, B>()),
                           [](entity e, sequence_ref<void*> data)
                           {
                               auto [a, b] = data.cast_tuple<A*, B*>();
                               expect(a->x == 1 && b->x == 2);
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
        expect(A::objects == Acounter)
                << "A::objects" << A::objects << "\n"
                << "Acounter" << Acounter << "\n"
                << "A::constructor_call" << A::constructor_call << "\n"
                << "A::move_call" << A::move_call << "\n"
                << "A::copy_call" << A::copy_call << "\n"
                << "A::delete_call" << A::delete_call << "\n";

//         registry.query_with()
//        .all<B2,A,A3,B,A2>()
//        .for_each(
//        	[](entity e, const B& b, const A& a, storage_key key)
//        	{
//        		std::cout << e.id() << " "
//        		<< "Sort key : " << (uint32_t)key.get_table_offset() << " "
//        		<< "A : " << a.x << " "
//        		<< "B : " << b.x << std::endl;
//        	});
    };

    "leak"_test = []
    {
        expect(A::objects == 0) << "A objects should be 0 value: " << A::objects;
        A::check();
    };


};
