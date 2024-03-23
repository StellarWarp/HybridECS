#pragma once
#include "archetype_registry.h"
#include "archetype_storage.h"
#include "taged_archetype_storage.h"

namespace hyecs
{
	struct sort_key
	{

	};

	class data_registry
	{
		archetype_registry m_archetype_registry;
		vaildref_map<uint64_t, component_storage> m_component_storages;
		vaildref_map<uint64_t, archetype_storage> m_archetypes_storage;
		vaildref_map<uint64_t, taged_archetype_storage> m_taged_archetypes_storage;
		vaildref_map<uint64_t, table> m_tables;

		dense_set<entity> m_entities;

		struct group_storage
		{
			archetype_index archetype;
			dense_map<entity, storage_key> m_entity_storage_keys;

		};





	};
}

