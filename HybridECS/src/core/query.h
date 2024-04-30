#pragma once
#include "tag_archetype_storage.h"
#include "query_condition.h"

namespace hyecs
{
	//no need condition info for tag query, condition is processed by archetype_registry
	class table_tag_query
	{
	public:
		enum access_type
		{
			full_set_access,
			mixed_access,
			sparse_access
		};

	private:
		archetype_storage* m_archetype_storage;
		vector<tag_archetype_storage*> m_tag_storages; //unordered no need to init added by notify_tag_archetype_add

		table* m_table;
		vector<component_storage*> m_component_storages; //corresponding to m_access_components
		vector<component_type_index> m_access_components; //cache for component_type of m_comp_storages

		dense_map<entity, storage_key> m_entities;

		access_type m_query_type;
		bool m_is_direct_query;

		ASSERTION_CODE(query_condition m_tag_condition);

	public:
		table_tag_query(
			archetype_storage* base_storage,
			sequence_ref<component_storage*> tag_comp_storages,
			bool is_full_set,
			bool is_direct_set
			ASSERTION_CODE(, const query_condition& tag_condition)
		)
			: m_archetype_storage(base_storage),
			  m_table(base_storage->get_table()),
			  m_query_type(full_set_access),
			  m_is_direct_query(is_direct_set)
			  ASSERTION_CODE(, m_tag_condition(tag_condition))
		{
			m_component_storages.reserve(
				m_archetype_storage->get_component_storages().size() + tag_comp_storages.size());

			for (auto comp_storage : m_archetype_storage->get_component_storages())
				m_component_storages.push_back(comp_storage);
			for (auto comp_storage : tag_comp_storages)
				m_component_storages.push_back(comp_storage);

			std::sort(
				m_component_storages.begin(), m_component_storages.end(),
				[](const component_storage* a, const component_storage* b)
				{
					return a->component_type() < b->component_type();
				});

			m_access_components.reserve(m_component_storages.size());
			for (const auto comp_storage : m_component_storages)
			{
				m_access_components.push_back(comp_storage->component_type());
			}


			if (!is_full_set) notify_partial_convert();
			else
				assert(tag_comp_storages.size() == 0);
			assert(is_direct_set?m_query_type==full_set_access:true);
		}

		table_tag_query(const table_tag_query&) = delete;
		table_tag_query& operator=(const table_tag_query&) = delete;

#ifdef HYECS_DEBUG
		//debug code
		archetype_index base_archetype() const { return m_archetype_storage->archetype(); }
		//debug code
		const query_condition& tag_condition() const { return m_tag_condition; }
#endif

		void get_component_indices(
			sorted_sequence_ref<const component_type_index> types,
			sequence_ref<uint32_t> component_indices) const
		{
			get_sub_sequence_indices<component_type_index>(
				m_access_components, types, component_indices);
		}

		bool is_direct_query() const
		{
			return m_is_direct_query;
		}

		archetype_storage* get_direct_query_archetype_storage() const
		{
			assert(m_is_direct_query);
			return m_archetype_storage;
		}

	private:
		bool is_chunk_storage() const
		{
			return m_table != nullptr;
		}

	private:
		void notify_storage_chunk_convert()
		{
			assert(m_query_type == sparse_access);
			m_query_type = mixed_access;
			m_table = m_archetype_storage->get_table();
			assert(m_table);
			auto key_registry = m_archetype_storage->get_key_registry();
			for (auto& [e, key] : m_entities)
			{
				static_assert(std::is_reference_v<decltype((key))>);
				key = key_registry.at(e);
			}
			//todo
		}

		void notify_storage_sparse_convert()
		{
			//todo
			assert(m_query_type == mixed_access);
			m_query_type = sparse_access;
			m_table = nullptr;
			//todo if this needed?
			// for (auto& [e, key] : m_entities)
			// {
			// 	key = storage_key{};
			// }
		}

		void notify_entity_add(entity e, storage_key key)
		{
			m_entities.insert({e, key});
		}

		void notify_entity_remove(entity e)
		{
			m_entities.erase(e);
		}

	public:
		void notify_partial_convert()
		{
			m_archetype_storage->add_callback_on_sparse_to_chunk(
				[&]()
				{
					notify_storage_chunk_convert();
				});
			m_archetype_storage->add_callback_on_chunk_to_sparse(
				[&]()
				{
					notify_storage_sparse_convert();
				});
			m_table = m_archetype_storage->get_table();
			//todo init m_entities
			if (m_table)
			{
				m_query_type = mixed_access;
				auto key_registry = m_archetype_storage->get_key_registry();
				for (const auto tag_storage : m_tag_storages)
				{
					auto storage_entity = tag_storage->entities();
					for (auto [e,_] : storage_entity)
					{
						m_entities.insert({e, key_registry.at(e)});
					}
				}
			}
			else
			{
				m_query_type = sparse_access;
				for (const auto tag_storage : m_tag_storages)
				{
					auto storage_entity = tag_storage->entities();
					for (auto e : storage_entity)
					{
						m_entities.insert({e, {}});
					}
				}
			}
		}

		void notify_tag_archetype_add(tag_archetype_storage* storage)
		{
			m_tag_storages.push_back(storage);
			if (m_query_type == full_set_access) return;

			storage->add_callback_on_entity_add(
				//auto add all entities
				[&](entity e, storage_key key)
				{
					notify_entity_add(e, key);
				});
			storage->add_callback_on_entity_remove(
				[&](entity e, storage_key key)
				{
					notify_entity_remove(e);
				});
		}

	public:
		//explain : same query condition can have different access component list
		struct access_info
		{
		private:
			friend table_tag_query;

			access_info(sequence_ref<component_type_index> access_list)
			// : access_list(access_list)
			{
			}

			vector<uint32_t> table_access_indices;
			vector<uint32_t> tag_access_indices;
			vector<uint32_t> table_access_order_map;
			vector<uint32_t> tag_access_order_map;
			vector<uint32_t> full_access_indices;
		};

	private:
		using access_hash = uint64_t; //arch hash of access component list
		map<access_hash, access_info> m_access_infos;

	public:
		const access_info& get_access_info(sequence_ref<component_type_index> access_list)
		{
			access_hash hash = archetype::addition_hash(0, append_component(access_list));
			if (auto iter = m_access_infos.find(hash); iter != m_access_infos.end())
				return iter->second;

			auto [iter, _] = m_access_infos.insert(
				{
					hash, access_info{access_list}
				});
			auto& info = iter->second;

			vector<component_type_index> table_access_list;
			size_t tag_count = 0;
			table_access_list.reserve(access_list.size());
			for (const auto& comp : access_list)
				if (comp.is_tag()) tag_count++;
				else table_access_list.push_back(comp);

			info.table_access_indices.resize(table_access_list.size());
			info.full_access_indices.resize(access_list.size());
			info.tag_access_indices.reserve(tag_count);
			info.tag_access_order_map.reserve(tag_count);
			info.table_access_order_map.reserve(table_access_list.size());

			m_archetype_storage->get_component_indices(table_access_list, info.table_access_indices);
			get_component_indices(access_list, info.full_access_indices);
			for (uint32_t i = 0; i < access_list.size(); i++)
			{
				if (access_list[i].is_tag())
				{
					info.tag_access_indices.push_back(info.full_access_indices[i]);
					info.tag_access_order_map.push_back(i);
				}
				else
					info.table_access_order_map.push_back(i);
			}
			return info;
		}

		void dynamic_for_each(
			const access_info& info,
			std::function<void(entity, sequence_ref<void*>)> func)
		{
			switch (m_query_type)
			{
			case full_set_access:
				m_archetype_storage->dynamic_for_each(
					info.table_access_indices,
					[&](entity e, sequence_ref<void*> components)
					{
						func(e, components);
					});
				break;
			case mixed_access:
				{
					const auto full_component_count = info.full_access_indices.size();
					const auto table_component_count = info.table_access_indices.size();
					vector<void*> cache(table_component_count + full_component_count); //todo this allocation can be optimized
					sequence_ref<void*> table_components(cache.data(), cache.data() + table_component_count);
					sequence_ref<void*> addresses(cache.data() + table_component_count, cache.data() + cache.size());

					for (const auto& [entity,st_key] : m_entities)
					{
						m_table->components_addresses(st_key, info.table_access_indices, table_components);
						for (size_t i = 0; i < info.table_access_indices.size(); i++)
						{
							addresses[info.table_access_order_map[i]] = table_components[i];
						}
						for (size_t i = 0; i < info.tag_access_indices.size(); i++)
						{
							addresses[info.tag_access_order_map[i]] = m_component_storages[i]->at(entity);
						}
						func(entity, addresses);
					}
				}
				break;
			case sparse_access:
				{
					auto& component_indices = info.full_access_indices;
					vector<void*> addresses(component_indices.size()); //todo this allocation can be optimized
					for (const auto& [entity,_] : m_entities)
					{
						for (size_t i = 0; i < component_indices.size(); i++)
						{
							addresses[i] = m_component_storages[component_indices[i]]->at(entity);
						}
						func(entity, addresses);
					}
				}
				break;
			}
		}
	};

	class query
	{
		vector<archetype_storage*> m_archetype_storages;

		vector<table_tag_query*> m_tag_table_queries;

		//todo add support for sparse table access optimization

		//explain : same query condition can have different access component list
		struct access_info
		{
		private:
			friend query;

			access_info(sequence_ref<const component_type_index> access_list)
				: access_list(access_list)
			{
			}

			struct archetype_access_info
			{
				archetype_storage* storage;
				sequence_ref<const uint32_t> component_indices;
			};

			struct table_query_access_info
			{
				table_tag_query* query;
				table_tag_query::access_info access_info;
			};

			vector<component_type_index> access_list;
			vector<archetype_access_info> archetype_access_infos;
			vector<uint32_t> indices_storage;
			//vector<component_storage*> component_storages;//for sparse table access
			vector<table_query_access_info> table_query_access_infos;

			void on_archetype_add(archetype_storage* storage)
			{
				size_t old_size = indices_storage.size();
				size_t new_size = old_size + access_list.size();
				indices_storage.resize(new_size);
				auto component_indices_seq = sequence_ref<uint32_t>(
					indices_storage.data() + old_size,
					indices_storage.data() + new_size);
				archetype_access_infos.push_back({storage, component_indices_seq});
				storage->get_component_indices(access_list, component_indices_seq);
			}

			void on_table_query_add(table_tag_query* query)
			{
				table_query_access_infos.push_back(
					{
						query,
						query->get_access_info(access_list)
					});
			}
		};

		using access_hash = uint64_t; //arch hash of access component list
		map<access_hash, access_info> m_access_infos;

		ASSERTION_CODE(query_condition m_condition);

	public:
		query(const query_condition& condition) ASSERTION_CODE(: m_condition(condition))
		{
		}
		query(const query&) = delete;
		query& operator=(const query&) = delete;

#ifdef HYECS_DEBUG
		const query_condition& condition() const { return m_condition; }
#  endif
		void notify_table_query_add(table_tag_query* query)
		{
			if (query->is_direct_query())
			{
				archetype_storage* storage = query->get_direct_query_archetype_storage();
				m_archetype_storages.push_back(storage);
				for (auto& [_, access_info] : m_access_infos)
				{
					access_info.on_archetype_add(storage);
				}
			}
			else
			{
				m_tag_table_queries.push_back(query);
				for (auto& [_, access_info] : m_access_infos)
				{
					access_info.on_table_query_add(query);
				}
			}
		}

		const access_info& get_access_info(sequence_ref<const component_type_index> access_list)
		{
			access_hash hash = archetype::addition_hash(0, append_component(access_list));
			if (auto iter = m_access_infos.find(hash); iter != m_access_infos.end())
				return iter->second;

			auto [iter, _] = m_access_infos.insert(
				{
					hash, access_info{access_list}
				});
			auto& info = iter->second;
			info.indices_storage.reserve(access_list.size() * m_archetype_storages.size());
			info.archetype_access_infos.reserve(m_archetype_storages.size());
			for (auto storage : m_archetype_storages)
				info.on_archetype_add(storage);
			for (auto query : m_tag_table_queries)
				info.on_table_query_add(query);
			return info;
		}

		void dynamic_for_each(
			const access_info& acc_info,
			std::function<void(entity, sequence_ref<void*>)> func)
		{
			for (const auto& info : acc_info.archetype_access_infos)
			{
				info.storage->dynamic_for_each(
					info.component_indices, [&](entity e, sequence_ref<void*> components)
					{
						func(e, components);
					});
			}
			for (const auto& info : acc_info.table_query_access_infos)
			{
				info.query->dynamic_for_each(
					info.access_info, [&](entity e, sequence_ref<void*> components)
					{
						func(e, components);
					});
			}
		}

		template<typename... T>
		void for_each(T&&... args)
		{
			
		}
	};
}
