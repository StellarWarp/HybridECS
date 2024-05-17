#pragma once
#include "query.h"
namespace hyecs
{

	class cross_query
	{
		entity_sparse_map<uint32_t> m_potential_entities;
		entity_dense_set m_entities;
		vector<query*> m_in_group_queries;

		uint32_t group_count() const
		{
			return m_in_group_queries.size();
		}

	public:

		cross_query(sequence_ref<query*> queries)
			:m_in_group_queries(queries)
		{
			for (auto q : queries)
			{
				//todo register to add event
			}
		}

		void notify_super_query_add(entity e)
		{
			auto& counter = m_potential_entities[e];
			counter += 1;
			if (counter == group_count())
				m_entities.insert(e);
		}

		void notify_super_query_remove(entity e)
		{
			auto& counter = m_potential_entities[e];
			if (counter == group_count())
				m_entities.erase(e);
			counter -= 1;
		}

		
		
		

	};
}