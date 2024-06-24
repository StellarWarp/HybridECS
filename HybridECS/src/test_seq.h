#pragma once
#include "pch.h"
//#include "core/archetype_registry.h"
#include "container/container.h"

//#include "core/entity.h"
////#include "meta/meta_utils.h"
// #include "core/sparse_table.h"
// #include "core/table.h"

// #include "core/archetype_storage.h"
//#include "core/component_storage.h"
// #include "core/tag_archetype_storage.h"
 //#include "core/data_registry.h"

//#include "core/archetype_storage.h"
//#include "core/archetype_storage.h"
//#include "test_archetype_registry.h"

using namespace hyecs;

template <typename SeqParam>
void test_func(sequence_cref<int, SeqParam> seq)
{
	for (auto i : seq)
	{
		std::cout << i << std::endl;
	}
}


struct test_container
{
	struct iterator {
		using difference_type = ptrdiff_t;
		using value_type = int;
		int& operator *() { static int a; return a; }
		iterator& operator++() { return *this; }
		iterator operator++(int) { return *this; }
	};

	
	iterator begin() { return {}; }
	nullptr_t end() { return {}; }
};

int main()
{	
	std::array a1{ 1, 2, 3, 4, 5 };
	sequence_cref seq = a1;

	vector<int> vec = {1, 2, 3, 4, 5};

	std::iterator_traits<int*>::value_type;

	static_assert(type_list<int, int, int>::is_same);

	sequence_ref seq2 = vec;

	std::cout << sizeof(seq) << std::endl;
	std::cout << sizeof(seq2) << std::endl;

	auto seq3 = sequence_ref{ vec.begin(), vec.end() };
	std::cout << sizeof(seq3) << std::endl;

	//print seq3
	for (auto i : seq3)
	{
		std::cout << i << std::endl;
	}

	small_vector<int> svec{ seq2 };
	sequence_ref seq2_sv = svec;

	vector<int> vec2 = {};
	sequence_ref seq4 = { vec2.begin(), vec2.end() };

	//print seq4
	for (auto i : seq4)
	{
		std::cout << i << std::endl;
	}


	const vector<int> vec3 = {1, 2, 3, 4, 5};
	//sequence_ref<const int> seq6(vec3.data(), vec3.data() + vec3.size());

	sequence_cref csr1 = vec;
	sequence_cref<int> csr2 = seq2;

	test_func(seq2.as_const());

	//sequence_ref<const int>::raw_type;

	set<int> set;
	for (int i = 0; i < 10; i++)
	{
		set.insert(i);
	}

	sequence_ref seq5 = set;

	sequence_cref seq6 = set;

	auto seq7 = sequence_cref(set);

	test_container test;

	std::ranges::begin(test);

	using t = std::ranges::iterator_t<test_container>;
	using t1 = std::iter_value_t<t>;


	sequence_ref seq8 = test;
}
