#include "pch.h"

#include "core/data_registry.h"
#include "container/type_hash.h"

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

template <size_t Np1, typename CharT = char>
class static_string
{
public:
	const CharT(&data)[Np1];
	constexpr static_string(const CharT(&str)[Np1]) : data{ str } {}
	constexpr const CharT* c_str() const { return data; }
	constexpr const CharT* begin() const { return data; }
	constexpr const CharT* end() const { return data + Np1 - 1; }
};

template <size_t Np1, typename CharT = char>
struct tracked_string
{
	CharT data[Np1];

	constexpr tracked_string(const CharT(&str)[Np1]) : tracked_string(str, std::make_index_sequence<Np1-1>{}) {}

	template<size_t... I>
	constexpr tracked_string(const CharT* str, std::index_sequence < I...>) : data{ str[I]..., '\0' } {}

	/*template<size_t start, size_t end>
	constexpr auto sub_string() const
	{
		return tracked_string<end - start + 1, CharT>(data + start, std::make_index_sequence<end - start>{});
	}*/

};



template <typename T>
void test(const T& t)
{
	std::cout << typeid(t).name() << std::endl;
	std::cout << name_of(t) << std::endl;
}

//template <typename T>
//constexpr auto hash_of()
//{
//	return type_hash::hash_code<T>;
//}

//template <typename T>
//constexpr const char* maintain_str()
//{
//	constexpr std::string_view sv = type_name<T>;
//	constexpr size_t len = sv.size();
//	static char x[len+1] = {0};
//	std::memcpy(x, sv.data(), len);
//	return &x[0];
//}

template<size_t N>
class holder {
	char x[N];
};

constexpr size_t test_h = internal::static_type_hash<A>();

int main()
{

	//const auto x = tracked_string(sv);

	constexpr auto hash = type_hash::hash_code<A>;
	auto test_sv = type_name_sv<A>;
	constexpr auto test = internal::type_name<A>();
	//constexpr auto test2 = test.data;

	//Test t0;
	//Test& t = t0;
	//test(t);
	//std::cout << name_of_<A>() << std::endl;
	//data_registry registry(ecs_static_register_context::register_context);

	//auto types = registry.get_component_types<C, B, A>();
	//for (auto& type : types)
	//{
	//	std::cout << type.name() << std::endl;
	//}
	//vector<generic::constructor> constructors;
	//for (auto type : types)
	//{
	//	constructors.push_back(generic::constructor(
	//		(generic::type_index)type,
	//		std::function<void* (void*)>([](void* x) { *(int*)x = 0; return x; })
	//	));
	//}
	//vector<entity> entities(10);
	//registry.emplace(
	//	types,
	//	constructors,
	//	entities
	//);

	//auto type2 = registry.get_component_types<B, T2, A>();
	//for (auto& type : types)
	//{
	//	std::cout << type.name() << std::endl;
	//}

}