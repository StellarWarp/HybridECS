#pragma once

#include "core/table.h"
#include "core/sparse_table.h"

using namespace hyecs;

int main()
{
	//raw_entity_dense_map table(4);

	//unordered_map<entity, int> map;
	//for (uint32_t i = 0; i < 1024; i++)
	//{
	//	new(table.allocate_value(entity{ i,0 })) int(i);
	//	map[entity{ i,0 }] = i;
	//}

	////random remove

	//for (uint32_t i = 0; i < 1024; i++)
	//{
	//	if (rand() % 2 == 0)
	//	{
	//		table.deallocate_value(entity{ i,0 });
	//		map.erase(entity{ i,0 });
	//	}
	//}

	//for (uint32_t i = 1024; i < 2048; i++)
	//{
	//	new(table.allocate_value(entity{ i,0 })) int(i);
	//	map[entity{ i,0 }] = i;
	//}

	//for (uint32_t i = 0; i < 1024; i++)
	//{
	//	if (rand() % 2 == 0)
	//	{
	//		if(table.contains(entity{ i,0 }))
	//			table.deallocate_value(entity{ i,0 });
	//		map.erase(entity{ i,0 });
	//	}
	//}

	//for (uint32_t i = 0; i < 2048; i++)
	//{
	//	if (rand() % 2 == 0)
	//	{
	//		if (table.contains(entity{ i,0 }))
	//			table.deallocate_value(entity{ i,0 });
	//		map.erase(entity{ i,0 });
	//	}
	//}


	//for(raw_entity_dense_map::entity_value ev:table)
	//{
	//	std::cout << ev.e << " " << *(int*)ev.value << std::endl;
	//}

	//for (auto& [e, v] : map)
	//{
	//	assert(*(int*)table.at(e) == v);
	//}

	//entity_sparse_set set;
	//
	//set.insert(entity{ 0,0 });

	struct A
	{
		int a, b, c, d;
	};

	struct B
	{
		int a, b, c, d;
	};

	component_group_index g{};

	component_type_index c1 = component_type_info(generic::type_info::of<A>(), g, false);
	component_type_index c2 = component_type_info(generic::type_info::of<B>(), g, false);

	component_storage s1(c1);
	component_storage s2(c2);

	vector<component_storage*> storages{ &s1,&s2 };
	std::sort(storages.begin(), storages.end(), [](const component_storage* a, const component_storage* b) {return a->component_type() < b->component_type(); });
	sparse_table table(storages);


	//table table({ c1,c2 });

	const uint32_t scale = 2048;

	vector<entity> entities;
	for (uint32_t i = 0; i < scale; i++)
	{
		entities.push_back(entity{ i,0 });
	}

	unordered_map<entity, storage_key> key_map;

	auto accessor = table.get_allocate_accessor(sequence_ref(entities).as_const(), [&](entity e, storage_key key)
		{

		}
	);

	for (auto e : entities)
	{
		key_map.insert({ e,{} });
	}


	int j = 0;
	for (auto& component_accessor : accessor)
	{
		int i = 0;
		for (void* addr : component_accessor)
		{
			new(addr) A{ j * 10000 + i,j * 10000 + i + 1,j * 10000 + i + 2,j * 10000 + i + 3 };
			i++;
		}
		j++;
	}

	accessor.notify_construct_finish();

	auto raw_accessor = table.get_raw_accessor();
	for (auto& component_accessor : raw_accessor)
	{
		auto entity_accessor = table.get_entities();
		auto entity_iter = entity_accessor.begin();
		for (void* addr : component_accessor)
		{
			A* a = (A*)addr;
			std::cout << entity_iter->id() << " : "
				<< a->a << " " << a->b << " " << a->c << " " << a->d << std::endl;
			entity_iter++;
		}
	}

	vector<entity> deallocate_entities;
	//radom deallocate
	for (uint32_t _ = 0; _ < scale; _++)
	{
		if (uint32_t i = rand() % scale; key_map.contains(entities[i]))
		{
			deallocate_entities.push_back(entities[i]);
			key_map.erase(entities[i]);
		}
	}


	auto deallocate_accessor = table.get_deallocate_accessor(deallocate_entities);

	deallocate_accessor.destruct();

	{
		auto raw_accessor = table.get_raw_accessor();
		for (auto& component_accessor : raw_accessor)
		{
			auto entity_accessor = table.get_entities();
			auto entity_iter = entity_accessor.begin();
			for (void* addr : component_accessor)
			{
				A* a = (A*)addr;
				std::cout << entity_iter->id() << " : "
					<< a->a << " " << a->b << " " << a->c << " " << a->d << std::endl;
				entity_iter++;
			}
		}
	}

	auto entity_accessor = table.get_entities();
	for (auto e : entity_accessor)
	{
		assert(key_map.contains(e));
	}
}