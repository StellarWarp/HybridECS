#pragma once
#include "ecs/type/entity.h"
#include "container/container.h"
#include "ecs/storage/entity_map.h"
#include "table.h"

namespace hyecs
{


	class storage_key_registry
	{
		dense_map<entity, storage_key> m_entity_storage_keys;
		vector<table*> m_tables;
		stack<size_t> free_table_indices;


	public:

        storage_key at(entity e)
        {
            return m_entity_storage_keys.at(e);
        }

        auto find(entity e)
        {
            return m_entity_storage_keys.find(e);
        }

        auto begin() { return m_entity_storage_keys.begin(); }

        auto end() { return m_entity_storage_keys.end(); }

		void register_table(table* table)
        {
	        if (free_table_indices.empty())
	        {
	        	uint32_t index = m_tables.size();
	        	assert(index <= storage_key::max_table_index);
	        	m_tables.push_back(table);
	        	//todo is the group id necessary?
	        	table->set_table_index({uint32_t(-1),index,false});
	        }
        	else
        	{
				uint32_t index = free_table_indices.top();
				free_table_indices.pop();
				m_tables[index] = table;
				table->set_table_index({uint32_t(-1),index,false});
        	}
        }

		void unregister_table(table* table)
		{
			auto index = table->get_table_index().table_index();
			m_tables[index] = nullptr;
			free_table_indices.push(index);
		}

		table* find_table(storage_key::table_index_t table_index) const
        {
        	return m_tables[table_index.table_index()];
        }

		class group_key_accessor
		{
			storage_key_registry& m_registry;
			dense_map<entity, storage_key>& storage_map()
			{
				return m_registry.m_entity_storage_keys;
			}
            const dense_map<entity, storage_key>& storage_map() const
            {
                return m_registry.m_entity_storage_keys;
            }
		public:
			group_key_accessor(storage_key_registry& registry) : m_registry(registry) {}
			void insert(entity e, storage_key key)
			{
				storage_map().insert({ e, key });
			}

			void erase(entity e)
			{
				storage_map().erase(e);
			}

			storage_key at(entity e) const
			{
				return storage_map().at(e);
			}

//			storage_key operator[](entity e) const
//			{
//				return storage_map()[e];
//			}

			bool contains(entity e) const
			{
				return storage_map().contains(e);
			}

        	void register_table(table* table)
			{
				m_registry.register_table(table);
			}

        	void unregister_table(table* table)
			{
				m_registry.unregister_table(table);
			}

        	table* find_table(storage_key::table_index_t table_index) const
	        {
				return m_registry.find_table(table_index);
			}
		};

		group_key_accessor get_group_key_accessor()
		{
			return group_key_accessor{ *this };
		}
	};


	using storage_key_accessor = storage_key_registry::group_key_accessor;

}


template<>
struct std::hash<hyecs::storage_key>
{
	std::size_t operator()(const hyecs::storage_key& key) const
	{
		return key.hash();
	}
};