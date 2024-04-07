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
	public:

		archetype_registry m_archetype_registry;
		storage_key_registry m_storage_key_registry;

		vaildref_map<uint64_t, component_storage> m_component_storages;
		vaildref_map<uint64_t, archetype_storage> m_archetypes_storage;
		vaildref_map<uint64_t, taged_archetype_storage> m_taged_archetypes_storage;
		//using storage_variant = std::variant<component_storage, archetype_storage, taged_archetype_storage>;
		//vaildref_map <archetype::hash_type, storage_variant> m_storages;
		vaildref_map<uint64_t, table> m_tables;

		vaildref_map<uint64_t, component_type_info> m_component_type_infos;
		vaildref_map<component_group_id, component_group_info> m_component_group_infos;

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


		void add_untaged_archetype(archetype_index arch)
		{
			vector<component_storage*> storages;
			storages.reserve(arch.component_count());
			for (auto& component : arch)
			{
				storages.push_back(&m_component_storages.at(component.hash()));
			}
			m_archetypes_storage.emplace(arch.hash(), archetype_storage(arch, storages, m_storage_key_registry.get_group_key_accessor()));
		}

		void add_taged_archetype(archetype_index arch, archetype_index base_arch)
		{
			assert(m_archetypes_storage.contains(base_arch.hash()));
			archetype_storage* base_storage = &m_archetypes_storage.at(base_arch.hash());
			vector<component_storage*> taged_storages;
			for (auto& component : arch)
			{
				if (component.is_tag()) taged_storages.push_back(&m_component_storages.at(component.hash()));
			}
			m_taged_archetypes_storage.emplace(arch.hash(), taged_archetype_storage(arch, base_storage, taged_storages));
		}

		component_group_info& register_component_group(component_group_id id, std::string name)
		{
			return m_component_group_infos.emplace(id, component_group_info{ id, name , {} });
		}

		component_type_index register_component_type(generic::type_index type, component_group_info& group, bool is_tag)
		{
			component_type_index component_index = m_component_type_infos.emplace(type.hash(), component_type_info(type, group, is_tag));

			group.component_types.push_back(component_index);
			m_archetype_registry.register_component(component_index);
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
			m_archetype_registry.bind_untaged_archetype_addition_callback(
				[this](archetype_index arch) {
					add_untaged_archetype(arch);
				}
			);
			m_archetype_registry.bind_taged_archetype_addition_callback(
				[this](archetype_index arch, archetype_index base_arch) {
					add_taged_archetype(arch, base_arch);
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

		template <typename... T>
		auto get_component_types()
		{
			using input_sequence = type_list<T...>;
			using type_order = sorted_type_list<T...>;
			constexpr auto type_hashes = type_hash_array(type_order{});

			auto cached = type_indexed_array<component_type_index, type_order>{
				m_component_type_infos.at(type_hashes[input_sequence::template index_of<T>])...
			};
			return cached;
		}




		void emplace(
			sorted_sequence_ref<const component_type_index> components,
			sorted_sequence_ref<const generic::constructor> constructors,
			sequence_ref<entity> entities)
		{
			auto group_begin = components.begin();
			auto group_end = components.begin();

			allocate_entity(entities);

			while (group_begin != components.end())
			{
				group_end = std::adjacent_find(group_begin, components.end(),
					[](const component_type_index& lhs, const component_type_index& rhs) {
						return lhs.group() != rhs.group();
					});

				archetype_index arch = m_archetype_registry.get_archetype(append_component(group_begin, group_end));

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
					auto construct_process = [&](auto allocate_accessor) {
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
						};
					if (arch.is_tagged())
					{
						auto& storage = m_taged_archetypes_storage.at(arch.hash());
						construct_process(storage.allocate(entities));
					}
					else
					{
						auto& storage = m_archetypes_storage.at(arch.hash());
						construct_process(storage.get_allocate_accessor(entities.as_const()));
					}
				}

				group_begin = group_end;
			}
		}

		template<typename... T>
		void emplace_static(
			sequence_ref<entity> entities,
			T&&... components
		)
		{
			static auto component_types = get_component_types<T...>();
			static auto constructors = type_indexed_array<generic::constructor, type_list<T...>>{ generic::constructor(components)... };
			emplace(component_types.as_array(), constructors.as_array(), entities);
		}









	};
}

