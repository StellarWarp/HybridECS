#pragma once
#include "pch.h"
#include "core/archetype_registry.h"
//#include "container/stl_container.h"

using namespace hyecs;

std::ostream& operator<<(std::ostream& os, const component_type_index& index) {
	os << index.get_info().name();
	return os;
}

std::ostream& operator<<(std::ostream& os, const archetype_index& index) {
	const auto& arch = index.get_info();
	for (auto& comp : arch) {
		os << comp << " ";
	}
	return os;
}

struct A { int x; };
struct B { int x; };
struct C { int x; };
struct D { int x; };
struct E { int x; };
struct T1 {};
struct T2 {};
struct T3 {};

int main()
{
	int x[] = { 1,2,3,4,5,6,7,8,9,10 };
	sequence_ref<int> list(x, x + 10);
	sequence_ref<const int> list2 = { 1,2,3,4,5,6,7,8,9,10 };


	generic::type_index int_type = generic::type_registry::register_type<int>();



	component_type_index a_type = component_type_info::from_template<A>();
	component_type_index b_type = component_type_info::from_template<B>();
	component_type_index c_type = component_type_info::from_template<C>();
	component_type_index d_type = component_type_info::from_template<D>();
	component_type_index e_type = component_type_info::from_template<E>();
	component_type_index t1_type = component_type_info::from_template<T1>();
	component_type_index t2_type = component_type_info::from_template<T2>();
	component_type_index t3_type = component_type_info::from_template<T3>();

	archetype_registry registry;

	registry.register_component(a_type);
	registry.register_component(b_type);
	registry.register_component(c_type);
	registry.register_component(t1_type);
	registry.register_component(t2_type);
	registry.register_component(t3_type);

	registry.register_query({ a_type, b_type }, [](archetype_index arch) {
		std::cout << "Query AB: " << arch << std::endl;
		});

	registry.register_query({ a_type, c_type }, [](archetype_index arch) {
		std::cout << "Query AC: " << arch << std::endl;
		});

	registry.register_query({ a_type, b_type, c_type }, [](archetype_index arch) {
		std::cout << "Query ABC: " << arch << std::endl;
		});

	registry.register_query({ a_type, b_type, c_type, t1_type }, [](archetype_index arch) {
		std::cout << "Query ABC T1: " << arch << std::endl;
		});

	registry.register_query({ a_type, b_type, c_type, t2_type }, [](archetype_index arch) {
		std::cout << "Query ABC T2: " << arch << std::endl;
		});

	registry.register_query({ a_type, b_type, c_type, t3_type }, [](archetype_index arch) {
		std::cout << "Query ABC T3: " << arch << std::endl;
		});

	auto arch_ab = registry.register_archetype({ a_type, b_type });
	auto arch_ac = registry.register_archetype({ a_type, c_type });
	auto arch_abc = registry.register_archetype({ a_type, b_type, c_type });
	auto arch_bc = registry.register_archetype({ b_type, c_type });
	auto arch_bad = registry.register_archetype({ b_type, a_type, d_type });
	auto arch_abc_t1 = registry.register_archetype({ a_type, b_type, c_type, t1_type });
	auto arch_abc_t2 = registry.register_archetype({ a_type, b_type, c_type, t2_type });
	auto arch_abc_t3 = registry.register_archetype({ a_type, b_type, c_type, t3_type });
	auto arch_abc_t1t3 = registry.register_archetype({ a_type, b_type, c_type, t1_type, t3_type });
	auto arch_abc_t1t2t3 = registry.register_archetype({ a_type, b_type, c_type, t1_type, t2_type, t3_type });

	std::cout << "\n late query registration\n\n";
	//late query registration
	registry.register_query({ a_type, t1_type, t3_type }, [](archetype_index arch) {
		std::cout << "Query A T1 T3: " << arch << std::endl;
		});
	auto arch_ab_ref = registry.register_archetype({ a_type, b_type });

	registry.register_query({ a_type }, [](archetype_index arch) {
		std::cout << "Query A: " << arch << std::endl;
		});

	registry.register_query({ t1_type }, [](archetype_index arch) {
		std::cout << "Query T1: " << arch << std::endl;
		});




	auto arch_t1t3 = registry.register_archetype({ t1_type, t3_type });
	auto arch_t1t2 = registry.register_archetype({ t1_type, t2_type });

	registry.register_query({ b_type }, [](archetype_index arch) {
		std::cout << "Query B: " << arch << std::endl;
		});






}