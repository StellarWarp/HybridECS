#pragma once
#include "archetype_registry.h"
namespace hyecs
{

	class intensive_taged_archetype_storage
	{
		archetype_index m_archetype_index;
		std::vector<taged_archetype_storage> m_storages;
		dense_set<entity> m_entities;
		dense_set<sort_key> m_sort_keys;
		table* m_table;
	};


	class query
	{
		//info
		struct query_condition
		{
			vector<component_type_index> all;
			vector<component_type_index> any;
			vector<component_type_index> none;
		};

		struct archetype_access_info
		{
			archetype_storage* storage;
			vector<uint32_t> component_index;
		};

		struct taged_archetype_access_info
		{
			intensive_taged_archetype_storage* storage;
			vector<uint32_t> component_index;
		};

		query_condition m_condition;
		vector<archetype_access_info> m_archetype_access_info;
		vector<taged_archetype_access_info> m_taged_archetype_access_info;
	};


}