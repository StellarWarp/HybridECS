#pragma once
#include "core/static_data_registry.h"


using namespace hyecs;


constexpr auto group_a = component_group_id("Group A");
inline const auto _ = ecs_static_register_context::register_context.add_group("Group A");


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

struct T1
{
};

struct T2
{
};

struct T3
{
};

REGISTER_HYECS_COMPONENT(A, group_a);

REGISTER_HYECS_COMPONENT(B, group_a);

REGISTER_HYECS_COMPONENT(C, group_a);

REGISTER_HYECS_COMPONENT(D, group_a);

REGISTER_HYECS_COMPONENT(E, group_a);

REGISTER_HYECS_COMPONENT(T1, group_a);

REGISTER_HYECS_COMPONENT(T2, group_a);

REGISTER_HYECS_COMPONENT(T3, group_a);

struct register_idents
{
	enum
	{
		main,
	};
};


class main_registry : public static_data_registry<register_idents::main>
{
	using static_data_registry::static_data_registry;
};


template <typename Callable, typename... Args, size_t... I>
void in_group_for_each_impl(Callable&& func, type_list<Args...>, std::index_sequence<I...>)
{
	using component_param = typename type_list<Args...>::template filter_with<is_static_component>;
	using non_component_param = typename type_list<Args...>::template filter_without<is_static_component>;

	std::cout << type_name<component_param> << std::endl;
	std::cout << type_name<non_component_param> << std::endl;

	std::array<void*,non_component_param::size> comp_addr{ nullptr };
	
	auto get_param = [&](auto type) -> decltype(auto)
	{
		using param_type = typename decltype(type)::type;
		using base_type = typename std::decay_t<param_type>;
		if constexpr (is_static_component<param_type>::value)
		{
			size_t index = component_param::template index_of<param_type>;
			return static_cast<param_type>(*static_cast<base_type*>(comp_addr[index]));
		}
		else
		{
			return param_type{};
		}
	};
	
	func(get_param(type_wrapper<Args>{})...);
	
}


template <typename Callable>
auto in_group_for_each(Callable&& func)
{
	in_group_for_each_impl<Callable>(
		std::forward<Callable>(func),
		typename function_traits<Callable>::args{},
		std::make_index_sequence<function_traits<Callable>::arity>{}
	);
}

int main()
{
	// main_registry registry(ecs_static_register_context::register_context);


	// vector<entity> entities2(10);
	// registry.emplace_static(entities2, A{ 1 }, B{ 2 }, T1{});
	// registry.emplace_static(entities2, A{ 1 }, C{ 2 });
	// registry.emplace_static(entities2, T1{}, T3{});
	// registry.emplace_static(entities2, A{ 1 }, B{ 2 }, C{ 3 }, D{ 4 }, E{ 5 }, T1{}, T2{}, T3{});
	// registry.emplace_static(entities2, A{ 1 }, D{ 4 }, E{ 5 }, T1{}, T2{}, T3{});
	// registry.emplace_static(entities2, A{ 1 }, B{ 2 });
	// registry.emplace_static(entities2, A{ 1 });
	//
	//
	//
	// registry.get_query({
	// 	{registry.component_types<A,B>()},
	// 	{}
	// });
	//
	// registry.emplace_static(entities2,  A{ 1 }, B{ 2 }, C{ 3 });
	//
	// std::cout << "-------------------" << std::endl;
	//
	// auto& q = registry.get_query({
	// 	{registry.component_types<A,B>()},{},{registry.component_types<C>()}
	// });
	//
	// entities2.resize(300);
	// registry.emplace_static(entities2, A{ 1 }, B{ 2 });
	//
	//
	//
	// q.dynamic_for_each(q.get_access_info(registry.component_types<A,B>()),
	// 	[](entity e ,sequence_ref<void*> data)
	// {
	// 	auto [a,b] = data.cast_tuple<A*,B*>();
	// 	std::cout << e.id() << " : " << a->x << " " << b->x << std::endl;
	// });

	in_group_for_each(
		[](entity, const B&, D&, C, A&)
		{
		});
}
