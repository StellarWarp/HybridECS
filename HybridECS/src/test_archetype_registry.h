#pragma once
#include "ecs/static_data_registry.h"
#include "ecs/component_group.h"

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


int main()
{


	main_registry registry(ecs_static_register_context::register_context);


	vector<entity> entities2(10);
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

	auto& q = registry.get_query({
		{registry.component_types<A, B>()},
		{{registry.component_types<C, D>()}, {registry.component_types<T3, T2>()}}
	});

	//q.dynamic_for_each(q.get_access_info(registry.component_types<A,B>()),
	//[](entity e ,sequence_ref<void*> data)
	//{
	//	auto [a,b] = data.cast_tuple<A*,B*>();
	//	std::cout << e.id() << " : " << a->x << " " << b->x << std::endl;
	//});

	//registry.emplace_static(entities2,  A{ 1 }, B{ 2 }, C{ 3 });

	//std::cout << "-------------------" << std::endl;

	//auto& q = registry.get_query({
	//	{registry.component_types<A,B>()},{},{registry.component_types<C>()}
	//});

	//registry.emplace_static(entities2, A{ 1 }, D{ 4 }, E{ 5 }, T1{}, T2{});
	//entities2.resize(300);
	//registry.emplace_static(entities2, A{ 1 }, B{ 2 });


	//q.dynamic_for_each(q.get_access_info(registry.component_types<A,B>()),
	//	[](entity e ,sequence_ref<void*> data)
	//{
	//	auto [a,b] = data.cast_tuple<A*,B*>();
	//	std::cout << e.id() << " : " << a->x << " " << b->x << std::endl;
	//});


	//registry.register_executer([](entity e, A& a, B& b) {

	//});


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
}
