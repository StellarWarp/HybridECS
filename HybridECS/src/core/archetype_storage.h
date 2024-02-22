#pragma once
#include "archetype.h"
#include "table.h"
#include "entity.h"

namespace hyecs
{
	struct sort_key
	{

	};

	class shared_table;
	class sparse_table;

	class taged_archetype_storage
	{
		archetype_index m_index;
		dense_set<entity> m_entities;
		dense_set<sort_key> m_sort_keys;
		table* m_table;

		//event
		std::function<void(entity, sort_key)> m_on_entity_add;
		std::function<void(entity, sort_key)> m_on_entity_remove;
	};


	class archetype_storage
	{
		enum class storage_type
		{
			Sparse,
			Chunk,
		};

		archetype_index m_index;
		storage_type m_storage_type;
		shared_table* m_shared_table;
		sparse_table* m_sparse_table;

		//event
		std::function<void(entity, sort_key)> m_on_entity_add;
		std::function<void(entity, sort_key)> m_on_entity_remove;

	public:
		archetype_storage(archetype_index index)
		{
			m_index = index;
			m_storage_type = storage_type::Sparse;
			m_shared_table = nullptr;
			m_sparse_table = nullptr;
		}

		archetype_index archetype_index() const
		{
			return m_index;
		}

		sort_key move_construct_entity(
			const archetype_storage& other,
			...)
		{
			switch (other.m_storage_type)
			{

			}

		}



	};


}
