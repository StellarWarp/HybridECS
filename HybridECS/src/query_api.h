#pragma once
#include "meta/type_list.h"
#include "container/container.h"

using namespace hyecs;

struct A {};
struct B {};
struct C {};
struct D {};
struct E {};
struct F {};

struct entity {};

struct parent {};

struct child {};

template<typename First, typename Second>
class relation
{

};

template<typename First, typename... Components> //Components is type_list<...>
struct entity_relation
{
	entity e;
	std::tuple<Components...> components;
};

template<typename First, typename... Components>
struct entity_multi_relation
{
	entity e;
	vector<std::tuple<Components...>> components;
};

template<typename... T>
struct all_of {};

template<typename... T>
using any_of = std::variant<T...>;

template<typename... T>
struct none_of {};

struct builder_context
{
};

template <typename...>
struct query_builder;

template<typename... All, typename... Any, typename... None>
struct query_builder<type_list<All...>, type_list<Any...>, type_list<None...>>
{
	builder_context context;

	query_builder(builder_context ctx)
		: context(ctx)
	{
	}

	template<typename... T>
		requires (is_static_component<T>::value && ...)
	auto all() const { return query_builder<type_list<All..., T...>, type_list<Any...>, type_list<None...>>{context}; }
	template<typename... T>
		requires (is_static_component<T>::value && ...)
	auto any() const { return query_builder<type_list<All...>, type_list<Any..., T...>, type_list<None...>>{context}; }
	template<typename... T>
		requires (is_static_component<T>::value && ...)
	auto none() const { return query_builder<type_list<All...>, type_list<Any...>, type_list<None..., T...>>{context}; }

	template<typename... T>
		requires (std::is_same_v<T, generic::type_info> && ...)
	auto dynamic_visit(T... t)
	{
		return query_builder<type_list<All...>, type_list<Any...>, type_list<None..., T...>>{context};
	}
};

struct rt_type
{
	void* address;

	template<typename T>
	T& cast()
	{
		return *static_cast<T*>(address);
	};
};

//template<typename T>
//concept c_test = requires(T obj) {
//	{ obj.print() } -> std::same_as<void>;
//};
//
//void aaa(c_test auto arg)
//{
//	arg.print();
//}


auto func(std::integral auto a, int b)
{
	return a + b;
}








void test(query_builder<type_list<>, type_list<>, type_list<>> builder)
{

	type_hash runtime_type_hash = type_hash::of<A>();

	vector<int> a = { 1,2,3 };
	auto b = std::views::take(2);



	auto exec = [](
		const A& read_a,
		B& read_write_b,
		any_of<const C&, const D&> cd_ro,
		none_of<E, F>,
		all_of<A, B>,
		const rt_type& rt_ro,
		entity_relation<parent,
			const int&,
			float&,
			entity_relation<parent,
				int&, 
				float&>> rel1,
		entity_multi_relation<child,
			int&,
			float&> rel2
		)
	{
		auto [int_ro, float_rw, rel_2] = rel1.components;
		auto [int_rw2, float_rw2] = rel_2.components;

		for (auto& [int_rw, float_rw] : rel2.components)
		{
			int_rw = 0;
			float_rw = 0.0f;
		}

		visit([]<typename T>(T & t)
		{
			if constexpr (std::is_same_v<T, C>)
			{
				t = C{};
			}
			else if constexpr (std::is_same_v<T, D>)
			{
				t = D{};
			}
		}, cd_ro);



	};
}