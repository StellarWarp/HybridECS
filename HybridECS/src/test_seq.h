#pragma once
#include "pch.h"
#include "core/archetype_registry.h"
#include "container/container.h"

//#include "core/entity.h"
////#include "meta/meta_utils.h"
#include "core/sparse_table.h"
#include "core/table.h"

#include "core/archetype_storage.h"
//#include "core/component_storage.h"
#include "core/tag_archetype_storage.h"
#include "core/data_registry.h"

//#include "core/archetype_storage.h"
//#include "core/archetype_storage.h"
//#include "test_archetype_registry.h"

using namespace hyecs;

template<typename SeqParam>
void test_func(sequence_ref<const int, SeqParam> seq)
{
	for (auto i : seq)
	{
		std::cout << i << std::endl;
	}
}
//template<typename Seq>
//void test_func_warp(Seq seq)
//{
//	test_func < >(seq);
//}

int main()
{

	sequence_ref<const int> seq = { 1, 2, 3, 4, 5 };

	vector<int> vec = { 1, 2, 3, 4, 5 };
	sequence_ref<int> seq2 = make_sequence_ref(vec);

	std::cout << sizeof(seq) << std::endl;
	std::cout << sizeof(seq2) << std::endl;

	auto seq3 = make_sequence_ref(vec.begin(), vec.end());
	std::cout << sizeof(seq3) << std::endl;

	//print seq3
	for (auto i : seq3)
	{
		std::cout << i << std::endl;
	}

	vector<int> vec2 = {  };
	auto seq4 = make_sequence_ref(vec2.begin(), vec2.end());

	//print seq4
	for (auto i : seq4)
	{
		std::cout << i << std::endl;
	}


	const vector<int> vec3 = { 1, 2, 3, 4, 5 };
	//sequence_ref<const int> seq6(vec3.data(), vec3.data() + vec3.size());



	test_func(seq2.as_const());

	//sequence_ref<const int>::raw_type;

	//set<int> set;
	//for (int i = 0; i < 10; i++)
	//{
	//	set.insert(i);
	//}

	//auto seq4 = make_sequence_ref(set);
	//std::cout << sizeof(seq4) << std::endl;

	//sequence_ref<int,vector<int>> seq5 = vec;
	//sequence_ref<int, int*> seq6 = vec;

	//std::cout << sizeof(seq5) << std::endl;
	//std::cout << sizeof(seq6) << std::endl;

	//auto seq7 = make_sequence_ref(set.begin(), set.end(), set.size());

	//initializer_list<int> il1 = seq2;


	//sequence_ref<const int> seq8 = seq2;



}