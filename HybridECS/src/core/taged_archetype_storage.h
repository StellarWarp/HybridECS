#pragma once
#include "archetype_storage.h"
#include "component_storage.h"

namespace hyecs
{
	class taged_archetype_storage
	{
		archetype_index m_index;
		dense_set<entity> m_entities;
		dense_set<storage_key> m_sort_keys;
		table* m_table;
		archetype_storage* m_untaged_storage;
		vector<component_storage*> m_taged_storages;

		//event
		std::function<void(entity, storage_key)> m_on_entity_add;
		std::function<void(entity, storage_key)> m_on_entity_remove;


	private:
		void entity_change_archetype(
			sequence_ref<entity> entities,
			taged_archetype_storage* dest_archetype,
			remove_component taged_remove_components,
			sorted_sequence_ref<generic::constructor> untaged_adding_constructors,
			sorted_sequence_ref<generic::constructor> taged_adding_constructors)
		{
			m_untaged_storage->entity_change_archetype(entities, dest_archetype->m_untaged_storage, untaged_adding_constructors);

			auto src_taged_storages_iter = m_taged_storages.begin();
			auto dest_taged_storages_iter = dest_archetype->m_taged_storages.begin();
			auto src_end = m_taged_storages.end();
			auto constructors_iter = taged_adding_constructors.begin();
			for (; src_taged_storages_iter != src_end; ++src_taged_storages_iter, ++dest_taged_storages_iter)
			{
				auto& src_taged_storages = **src_taged_storages_iter;
				auto& dest_taged_storages = **dest_taged_storages_iter;
				if (src_taged_storages.component_type() < dest_taged_storages.component_type())
				{
					//remove
					src_taged_storages.erase(entities);
				}
				else if (src_taged_storages.component_type() > dest_taged_storages.component_type())
				{
					//add
					dest_taged_storages.emplace(entities, *constructors_iter);
					constructors_iter++;
				}
					
			}
		}
	};
}