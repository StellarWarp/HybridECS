#include "pch.h"
//#include "test_seq.h"
#include "core/data_registry.h"
#include "core/static_data_registry.h"
#include "core/query.h"
//#include "test_type.h"

using namespace hyecs;


volatile const auto group_a = ecs_static_register_context::register_context.add_group("Group A");


struct A { int x; };
struct B { int x; };
struct C { int x; };
struct D { int x; };
struct E { int x; };
struct T1 {};
struct T2 {};
struct T3 {};

REGISTER_HYECS_COMPONENT(A, group_a);
REGISTER_HYECS_COMPONENT(B, group_a);
REGISTER_HYECS_COMPONENT(C, group_a);
REGISTER_HYECS_COMPONENT(D, group_a);
REGISTER_HYECS_COMPONENT(E, group_a);
REGISTER_HYECS_COMPONENT(T1, group_a);
REGISTER_HYECS_COMPONENT(T2, group_a);
REGISTER_HYECS_COMPONENT(T3, group_a);

enum register_idents
{
	ECS_Identifier_Main,
};
class main_registry : public static_data_registry<ECS_Identifier_Main> { using static_data_registry::static_data_registry; };


int main()
{
	main_registry registry(ecs_static_register_context::register_context);

	//const auto types = registry.get_component_types<C, B, A>();
	//const auto constructors = types.cast([](auto& val, auto wrap_t) {
	//	using type = typename decltype(wrap_t)::type;
	//	return generic::constructor::of<type>();
	//	});

	//vector<entity> entities(10);
	//registry.emplace(
	//	types.as_array(),
	//	constructors.as_array(),
	//	entities
	//);

	vector<entity> entities2(10);
	registry.emplace_static(entities2, A{ 1 }, B{ 2 }, T1{});
	registry.emplace_static(entities2, A{ 1 }, C{ 2 });



}