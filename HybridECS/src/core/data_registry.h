#pragma once
#include "archetype_registry.h"
#include "archetype_storage.h"
#include "tag_archetype_storage.h"
#include "query.h"

namespace hyecs
{
	struct sort_key
	{
	};

	std::ostream& operator<<(std::ostream& os, const component_type_index& index)
	{
		os << index.get_info().name();
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const archetype_index& index)
	{
		const auto& arch = index.get_info();
		for (auto& comp : arch)
		{
			os << comp << " ";
		}
		return os;
	}

	std::ostream& operator<<(std::ostream& os, const query_condition& condition)
	{
		if (!condition.all().empty())
		{
			os << "[";
			for (auto& comp : condition.all())
			{
				os << comp << " ";
			}
			os << "]";
		}
		if (!condition.any().empty())
		{
			os << "(";
			for (auto& comp : condition.any())
			{
				os << comp << " | ";
			}
			os << ")";
		}
		if (!condition.none().empty())
		{
			for (auto& comp : condition.none())
			{
				os << "/" << comp << " ";
			}
		}
		return os;
	}


	class data_registry
	{
	public:
		archetype_registry m_archetype_registry;
		storage_key_registry m_storage_key_registry;

		vaildref_map<uint64_t, component_storage> m_component_storages;
		vaildref_map<uint64_t, archetype_storage> m_archetypes_storage;
		vaildref_map<uint64_t, tag_archetype_storage> m_tag_archetypes_storage;
		//using storage_variant = std::variant<component_storage, archetype_storage, tag_archetype_storage>;
		//vaildref_map <archetype::hash_type, storage_variant> m_storages;

		vaildref_map<uint64_t, component_type_info> m_component_type_infos;
		vaildref_map<component_group_id, component_group_info> m_component_group_infos;

		vaildref_map<uint64_t, query> m_queries;
		vaildref_map<uint64_t, table_tag_query> m_table_queries;

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


		void add_untag_archetype(archetype_index arch)
		{
			assert(!m_archetypes_storage.contains(arch.hash()));
			vector<component_storage*> storages;
			storages.reserve(arch.component_count());
			for (auto& component : arch)
			{
				if (!component.is_empty())
					storages.push_back(&m_component_storages.at(component.hash()));
			}
			//todo process for single component arch
			m_archetypes_storage.emplace_value(arch.hash(), arch, 
				sorted_sequence_cref(storages),
				m_storage_key_registry.get_group_key_accessor());

			std::cout << "add untag archetype " << arch << std::endl;
		}

		void add_tag_archetype(archetype_index arch, archetype_index base_arch)
		{
			assert(!m_tag_archetypes_storage.contains(arch.hash()));
			assert(m_archetypes_storage.contains(base_arch.hash()));
			archetype_storage* base_storage = &m_archetypes_storage.at(base_arch.hash());
			vector<component_storage*> tag_storages;
			for (auto& component : arch)
			{
				if (component.is_tag() && !component.is_empty())
					tag_storages.push_back(&m_component_storages.at(component.hash()));
			}
			m_tag_archetypes_storage.emplace_value(arch.hash(), arch, base_storage, sorted_sequence_cref(tag_storages));
			std::cout << "add tag archetype " << arch << std::endl;
		}

		void add_table_query(const archetype_registry::archetype_query_addition_info& info)
		{
			auto& [
				query_index,
				base_archetype_index,
				tag_condition,
				is_full_set,
				is_direct_set,
				notify_adding_tag,
				notify_partial_convert] = info;
			assert(!m_table_queries.contains(query_index));
			assert(m_archetypes_storage.contains(base_archetype_index.hash()));

			archetype_storage* base_storage = &m_archetypes_storage.at(base_archetype_index.hash());
			vector<component_storage*> tag_storages;
			for (auto& component : tag_condition.all())
			{
				assert(component.is_tag());
				if (!component.is_empty())
					tag_storages.push_back(&m_component_storages.at(component.hash()));
			}
			table_tag_query& table_query = m_table_queries.emplace_value(
				query_index, base_storage, tag_storages, is_full_set, is_direct_set ASSERTION_CODE(, tag_condition));
			notify_adding_tag = [this, &table_query](archetype_index arch)
			{
				tag_archetype_storage* tag_storage = &m_tag_archetypes_storage.at(arch.hash());
				table_query.notify_tag_archetype_add(tag_storage);
			};
			notify_partial_convert = [this, &table_query]()
			{
				table_query.notify_partial_convert();
			};

			std::cout << "add table query [" << base_archetype_index << "] + " << tag_condition;
			if (is_full_set)
				if (is_direct_set) std::cout << " direct set";
				else std::cout << " full set";
			else std::cout << " partial set";

			std::cout << std::endl;
		}

		void add_query(const archetype_registry::query_addition_info& info)
		{
			assert(!m_queries.contains(info.index));
			query& q = m_queries.emplace_value(info.index, info.condition);
			info.archetype_query_addition_callback = [this, &q](query_index table_query_index)
			{
				table_tag_query* table_query = &m_table_queries.at(table_query_index);
				q.notify_table_query_add(table_query);
				std::cout << "query ::" << q.condition()
					<< " add table query " << "{" << table_query->base_archetype() << "} "
					<< table_query->tag_condition()
					<< std::endl;
			};
			std::cout << "add query " << info.condition << std::endl;
		}

		component_group_info& register_component_group(component_group_id id, std::string name)
		{
			return m_component_group_infos.emplace(id, component_group_info{id, name, {}});
		}

		component_type_index register_component_type(generic::type_index type, component_group_info& group, bool is_tag)
		{
			component_type_index component_index = m_component_type_infos.emplace(type.hash(), component_type_info(type, group, is_tag));

			group.component_types.push_back(component_index);
			m_archetype_registry.register_component(component_index);
			if (!type.is_empty())
				m_component_storages.emplace(type.hash(), component_storage(component_index));
			return component_index;
		}

	public:
		void register_type(const ecs_type_register_context& context)
		{
			for (auto& [_, group] : context.groups())
			{
				component_group_info& group_info = register_component_group(group.id, group.name);

				for (auto& [_, component] : group.components)
				{
					register_component_type(component.type, group_info, component.is_tag);
				}
			}
		}

		data_registry()
		{
			m_archetype_registry.bind_untag_archetype_addition_callback(
				[this](archetype_index arch)
				{
					add_untag_archetype(arch);
				}
			);
			m_archetype_registry.bind_tag_archetype_addition_callback(
				[this](archetype_index arch, archetype_index base_arch)
				{
					add_tag_archetype(arch, base_arch);
				}
			);

			m_archetype_registry.bind_archetype_query_addition_callback(
				[this](const archetype_registry::archetype_query_addition_info& info)
				{
					add_table_query(info);
				}
			);

			m_archetype_registry.bind_query_addition_callback(
				[this](const archetype_registry::query_addition_info& info)
				{
					return add_query(info);
				}
			);
		}

		data_registry(const ecs_type_register_context& context) : data_registry()
		{
			register_type(context);
		}


		~data_registry()
		{
		}

		void emplace(
			sorted_sequence_cref<component_type_index> components,
			sorted_sequence_cref<generic::constructor> constructors,
			sequence_ref<entity> entities)
		{
			assert(components.size() == constructors.size());
			auto group_begin = components.begin();
			auto group_end = components.begin();

			allocate_entity(entities);

			while (group_begin != components.end())
			{
				group_end = std::adjacent_find(
					group_begin, components.end(),
					[](const component_type_index& lhs, const component_type_index& rhs)
					{
						return lhs.group() != rhs.group();
					});

				archetype_index arch = m_archetype_registry.get_archetype(append_component(group_begin, group_end));

				auto constructors_begin = constructors.begin() + (group_begin - components.begin());
				auto constructors_end = constructors.begin() + (group_end - components.begin());

				emplace_in_group(arch, entities, sorted_sequence_cref(constructors_begin, constructors_end));

				group_begin = group_end;
			}
		}

		void emplace_in_group(archetype_index arch,
			sequence_cref<entity> entities,
			sorted_sequence_cref<generic::constructor> constructors)
		{
			if (arch.component_count() == 1)
			{
				auto& storage = m_component_storages.at(arch[0].hash());
				auto component_accessor = storage.allocate(entities);
				for (void* addr : component_accessor)
				{
					constructors[0](addr);
				}
			}
			else
			{
				auto construct_process = [&](auto&& allocate_accessor)
				{
					auto constructor_iter = constructors.begin();

					auto component_accessor = allocate_accessor.begin();
					while (component_accessor != allocate_accessor.end())
					{
						assert((generic::type_index)component_accessor.component_type() == constructor_iter->type());
						for (void* addr : component_accessor)
						{
							(*constructor_iter)(addr);
						}
						++constructor_iter;
						++component_accessor;
					}
					allocate_accessor.notify_construct_finish();
				};
				if (arch.is_tag())
				{
					auto& storage = m_tag_archetypes_storage.at(arch.hash());
					construct_process(storage.allocate(entities));
				}
				else
				{
					auto& storage = m_archetypes_storage.at(arch.hash());
					construct_process(storage.get_allocate_accessor(entities));
				}
			}
		}
		

		auto& get_query(const query_condition& condition)
		{
			const query_index index = m_archetype_registry.get_query(condition);
			return m_queries.at(index);
		}
	};
}
