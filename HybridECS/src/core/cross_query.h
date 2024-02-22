#pragma once
#include "cross_archetype_storage.h"

namespace hyecs
{

	class cross_query
	{
		struct intensive_cross_archetype
		{
			archetype_index m_index;
			dense_set<entity> entities;
			//table set
			vector<table*> tables;
			//archetype set
			vector<cross_archetype_storage*> archetypes;


		};

		struct archetype_access_info
		{
			intensive_cross_archetype* storage;
			struct group_info
			{
				vector<uint32_t> component_index;
			};
			vector<group_info> m_groups;
		};


		struct query_condition
		{
			vector<component_type_index> all;
			vector<component_type_index> any;
			vector<component_type_index> none;
		};

		dense_set<entity> m_entities;
		vector<archetype_access_info> m_access_info;
		query_condition m_condition;

	};
}