#pragma once
#include "entity.h"
#include "../container/container.h"


namespace hyecs
{

	struct storage_key
	{
		friend class storage_key_registry;
	private:
		union 
		{
			uint64_t key;
			struct 
			{
				uint32_t table_index;
				uint32_t table_offset;
			};
		};
		storage_key(uint64_t key) : key(key) {}
	public:
		storage_key(uint32_t table_index, uint32_t table_offset) : table_index(table_index), table_offset(table_offset) {}
		storage_key() : key(std::numeric_limits<uint64_t>::max()) {}

		static const storage_key null;

		bool is_sparse_storage() const { return  }
		uint32_t get_table_index() const { return table_index; }
		uint32_t get_table_offset() const { return table_offset; }
	};


	class storage_key_registry
	{
		dense_map<entity, storage_key> m_entity_storage_keys;


	public:

		static storage_key from_table_index_offset(uint32_t table_index, uint32_t table_offset)
		{
			return storage_key(table_index, table_offset);
		}

		class group_key_accessor
		{
			storage_key_registry& m_registry;
			dense_map<entity, storage_key>& storage_map()
			{
				return m_registry.m_entity_storage_keys;
			}
		public:
			void insert(entity e, storage_key key)
			{
				storage_map().insert({ e, key });
			}

			storage_key at(entity e)
			{
				return storage_map().at(e);
			}

			storage_key operator[](entity e)
			{
				return storage_map()[e];
			}

			bool contains(entity e)
			{
				return storage_map().contains(e);
			}
		};
	};


	using storage_key_accessor = storage_key_registry::group_key_accessor;

}