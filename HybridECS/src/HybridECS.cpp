#include "pch.h"
#include "core/archetype_registry.h"
#include "container/container.h"
//#include "meta/meta_utils.h"
//#include "core/sparse_table.h"
#include "core/table.h"

//#include "core/archetype_storage.h"
//#include "core/archetype_storage.h"
//#include "test_archetype_registry.h"

using namespace hyecs;



int main()
{

	sequence_ref<const int> seq = { 1, 2, 3, 4, 5 };

	vector<int> vec = { 1, 2, 3, 4, 5 };
	sequence_ref<int> seq2 = make_sequence_ref(vec);

	std::cout << sizeof(seq) << std::endl;
	std::cout << sizeof(seq2) << std::endl;

	auto seq3 = make_sequence_ref(vec.begin(), vec.end());
	std::cout << sizeof(seq3) << std::endl;

	set<int> set;
	for (int i = 0; i < 10; i++)
	{
		set.insert(i);
	}

	auto seq4 = make_sequence_ref(set);
	std::cout << sizeof(seq4) << std::endl;

	sequence_ref<int,vector<int>> seq5 = vec;
	sequence_ref<int, int*> seq6 = vec;

	std::cout << sizeof(seq5) << std::endl;
	std::cout << sizeof(seq6) << std::endl;

	auto seq7 = make_sequence_ref(set.begin(), set.end(), set.size());

	initializer_list<int> il1 = seq2;


	sequence_ref<const int> seq8 = seq2;



}