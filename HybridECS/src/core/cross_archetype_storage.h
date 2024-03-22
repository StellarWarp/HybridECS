#pragma once
#include "archetype_storage.h"

namespace hyecs 
{
	class cross_archetype_storage
	{
		archetype_index m_archetype_index;
		dense_set<entity> m_entities;
		struct group_info
		{
			archetype_storage* storage;
			taged_archetype_storage* taged_storage;
			dense_set<storage_key> m_sort_keys;
		};

		vector<group_info> m_groups;
	};

}