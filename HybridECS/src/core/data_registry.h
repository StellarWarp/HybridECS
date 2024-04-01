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
		storage_key_registry m_storage_key_registry;

		vaildref_map<uint64_t, component_storage> m_component_storages;
		vaildref_map<uint64_t, archetype_storage> m_archetypes_storage;
		vaildref_map<uint64_t, taged_archetype_storage> m_taged_archetypes_storage;
		//using storage_variant = std::variant<component_storage, archetype_storage, taged_archetype_storage>;
		//vaildref_map <archetype::hash_type, storage_variant> m_storages;
		vaildref_map<uint64_t, table> m_tables;

		dense_set<entity> m_entities;

		class entity_allocator
		{
			uint32_t m_next_id;
			dense_map<uint32_t, uint32_t> m_reuseable_entity_versions{};

		public:

			entity_allocator()
				: m_next_id(0)
			{
			}
			entity allocate()
			{
				entity e(m_next_id, 0);
				m_next_id++;
				return e;
			}

			void deallocate(entity e)
			{
				m_reuseable_entity_versions[e.id()] = e.version();
			}

		};

		entity_allocator m_entity_allocator;

		entity allocate_entity()
		{
			return m_entity_allocator.allocate();
		}

		void deallocate_entity(entity e)
		{
			m_entity_allocator.deallocate(e);
		}

		void allocate_entity(sequence_ref<entity> entities)
		{
			for (uint32_t i = 0; i < entities.size(); i++)
			{
				entity e = m_entity_allocator.allocate();
				entities[i] = e;
				m_entities.insert(e);
			}
		}

		void deallocate_entity(sequence_ref<entity> entities)
		{
			for (uint32_t i = 0; i < entities.size(); i++)
			{
				deallocate_entity(entities[i]);
				m_entities.erase(entities[i]);
			}
		}

		entity emplace(
			sorted_sequence_ref<const component_type_index> components,
			sorted_sequence_ref<const generic::constructor> constructors,
			uint32_t count = 1)
		{
			auto group_begin = components.begin();
			auto group_end = components.begin();

			vector<entity> entities(count);
			allocate_entity(entities);

			while(group_begin != components.end())
			{
				group_end = std::adjacent_find(group_begin, components.end(),
					[](const component_type_index& lhs, const component_type_index& rhs) {
						return lhs.group() != rhs.group();
					});

				archetype_index arch = m_archetype_registry.get_archetype(append_component(group_begin, group_end));
				
				if (arch.is_tagged())
				{
					auto& storage = m_taged_archetypes_storage.at(arch.hash());
					storage.emplace_entity(entities, constructors);
				}
				else
				{
					if (arch.component_count() != 1)
					{
						auto& storage = m_archetypes_storage.at(arch.hash());
						storage.emplace_entity(entities, constructors);
					}
					else
					{
						auto& storage = m_component_storages.at(arch.hash());
						storage.emplace_entity(entities, constructors);
					}
				}


				group_begin = group_end;
			}
		}





	};
}

