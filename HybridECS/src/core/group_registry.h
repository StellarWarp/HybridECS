#pragma once
#include "table.h"

namespace hyecs
{
	struct sort_key
	{
		uint32_t archetype_index;
		uint32_t archetype_offset;
	};

	class data_registry
	{
		archetype_registry m_archetype_registry;
		vaildref_map<uint64_t, archetype_storage> m_archetypes_storage;
		vaildref_map<uint64_t, taged_archetype_storage> m_taged_archetypes_storage;
		vaildref_map<uint64_t, table> m_tables;
		vaildref_map<uint64_t, component_type_info> m_component_types;

		dense_set<entity> m_entities;
		//require optimisze
		dense_map<entity, sort_key> m_entity_archetype;

		template<typename T>
		component_type_index get_component_type_index()
		{
			if (auto iter = m_component_types.find(typeid(T).hash_code()); iter != m_component_types.end())
				return (*iter).second;
			else
			{
				auto index = m_component_types.emplace(typeid(T).hash_code(), component_type_info::from_template<T>());
				return index;
			}
		}

		auto get_archetype(archetype_index base_arch, append_component components)
		{
			assert(components.size() > 0);
			archetype_index arch = m_archetype_registry.get_archetype(base_arch, components);
			if (auto iter = m_archetypes_storage.find(arch.hash()); iter == m_archetypes_storage.end())
			{
				auto& storage = m_archetypes_storage.emplace(arch.hash(), archetype_storage(arch));
				return storage;
			}
			else return (*iter).second;
		}

		auto get_archetype(archetype_index base_arch, remove_component components)
		{
			assert(components.size() > 0);
			archetype_index arch = m_archetype_registry.get_archetype(base_arch, components);
			if (auto iter = m_archetypes_storage.find(arch.hash()); iter == m_archetypes_storage.end())
			{
				auto& storage = m_archetypes_storage.emplace(arch.hash(), archetype_storage(arch));
				return storage;
			}
			else return (*iter).second;
		}

		archetype_storage& get_entity_storage(entity e)
		{
			...
		}



		template<typename T, typename... Args>
		void add_component(entity e, Args&&... args)
		{
			auto storage = get_entity_storage(e);

		}

		struct entity_batch
		{
			archetype_index archetype;
			vector<entity> entities;
		};

		//for defered add component
		void component_change(
			entity_batch batch,
			array_ref<component_type_index> append_components,
			array_ref<component_type_index> remove_components,
			array_ref<std::function<void* (void*)>> append_constructors
		)
		{
			uint64_t hash = batch.archetype.hash();
			hash = archetype::addition_hash(
				hash,
				std::initializer_list<component_type_index>(
					append_components.begin(), append_components.end()));
			hash = archetype::subtraction_hash(
				hash,
				std::initializer_list<component_type_index>(
					remove_components.begin(), remove_components.end()));

			m_archetype_registry.get_archetype()

		}





	};
}

