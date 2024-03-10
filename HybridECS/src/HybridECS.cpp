#include "pch.h"
#include "container/generic_container.h"
//#include "container/stl_container.h"

using namespace hyecs;

int main()
{
	generic::type_index int_type = generic::type_registry::register_type<int>();

	//generic_vector vec(int_type);

	//vec.push_back(1);
	//vec.emplace_back<int>(2);
	//for (int i = 0; i < 10; ++i)
	//{
	//	vec.push_back(i);
	//}

	//for (int i = 0; i < vec.size(); ++i)
	//{
	//	int* ptr = static_cast<int*>(vec[i]);
	//	std::cout << *ptr << " ";
	//}
	//std::cout << std::endl;


	//for (auto i_ptr : vec)
	//{
	//	int* ptr = static_cast<int*>(i_ptr);
	//	std::cout << *ptr << " ";
	//}
	//std::cout << std::endl;

}