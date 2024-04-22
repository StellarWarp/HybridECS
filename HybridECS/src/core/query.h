#pragma once
#include "taged_archetype_storage.h"
#include "query_condition.h"
namespace hyecs
{


	class query	
	{
		query_condition m_condition;
		vector<archetype_storage*> m_archetype_storages;

		//todo add support for sparse table access optimization

		struct access_info
		{
		private:
			friend query;
			access_info(sequence_ref<component_type_index> access_list)
				: access_list(access_list) {}
			struct archetype_access_info
			{
				archetype_storage* storage;
				sequence_ref<const uint32_t> component_indices;
			};
			vector<component_type_index> access_list;
			vector<archetype_access_info> archetype_access_infos;
			vector<uint32_t> indices_storage;
			//vector<component_storage*> component_storages;//for sparse table access

			void on_archetype_add(archetype_storage* storage)
			{
				size_t old_size = indices_storage.size();
				size_t new_size = old_size + access_list.size();
				indices_storage.resize(new_size);
				auto component_indices_seq = sequence_ref<uint32_t>(
					indices_storage.data() + old_size,
					indices_storage.data() + new_size);
				archetype_access_infos.push_back({ storage,  component_indices_seq });
				storage->get_component_indices(access_list, component_indices_seq);
			}
			auto begin() const { return archetype_access_infos.begin(); }
			auto end() const { return archetype_access_infos.end(); }
		};

		map<uint64_t, access_info> m_access_infos;

	public:
		query(query_condition condition) : m_condition(condition) {}

		void on_archetype_add(archetype_storage* storage)
		{
			m_archetype_storages.push_back(storage);
			for (auto& [_, access_info] : m_access_infos)
			{
				access_info.on_archetype_add(storage);
			}
		}

		//input is filtered by condition.all 
		void on_unfiltered_archetype_add(archetype_storage* storage)
		{
			auto arch = storage->archetype();
			bool match = false;
			for (component_type_index comp : m_condition.any)
			{
				if (arch.contains(comp))
				{
					match = true;
					break;
				}
			}
			if (!match) return;
			for (component_type_index comp : m_condition.none)
				if (arch.contains(comp))return;

			on_archetype_add(storage);
		}

		const access_info& get_access_info(sequence_ref<component_type_index> access_list)
		{
			auto hash = archetype::addition_hash(0, append_component(access_list));
			if (auto iter = m_access_infos.find(hash); iter != m_access_infos.end())
				return iter->second;

			auto [iter, _] = m_access_infos.insert({
					hash, access_info{access_list}
				});
			auto& info = iter->second;
			info.indices_storage.reserve(access_list.size() * m_archetype_storages.size());
			info.archetype_access_infos.reserve(m_archetype_storages.size());
			for (auto storage : m_archetype_storages)
				info.on_archetype_add(storage);
			return info;
		}

		void dynamic_for_each(
			const access_info& info,
			std::function<void(entity, sequence_ref<void*>)> func)
		{
			for (const auto& i : info)
			{
				i.storage->dynamic_for_each(i.component_indices, [&](entity e, sequence_ref<void*> components)
					{
						func(e, components);
					});
			}
		}
	};

	class taged_table_query
	{

		archetype_index m_archetype_index;
		std::vector<taged_archetype_storage> m_storages;
		dense_set<entity> m_entities;
		dense_set<storage_key> m_sort_keys;
		table* m_table;
	};

	class taged_query
	{
		query_condition m_condition;
		vector<taged_table_query*> m_table_queries;
	};


}