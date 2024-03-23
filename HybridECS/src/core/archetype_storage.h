#pragma once
#include "archetype.h"
#include "table.h"
#include "sparse_table.h"
#include "entity.h"
#include "storage_key_registry.h"

namespace hyecs
{




	class archetype_storage
	{
		friend class taged_archetype_storage;

		enum class storage_type
		{
			Sparse,
			Chunk,
		};

		archetype_index m_index;
		storage_type m_storage_type;
		std::variant<table, sparse_table> m_table;

		//event

		vector<std::function<void(entity, storage_key)>> m_on_entity_add;
		vector<std::function<void(entity, storage_key)>> m_on_entity_remove;
		vector<std::function<void(entity, storage_key, storage_key)>> m_on_entity_move;//from, to
		vector<std::function<void()>> m_on_sparse_to_chunk;
		vector<std::function<void()>> m_on_chunk_to_sparse;

	public:
		archetype_storage(archetype_index index, vector<component_storage*> component_storages)
			: m_index(index),
			m_storage_type(storage_type::Sparse),
			m_table(sparse_table(component_storages))
		{
		}

		archetype_index archetypex() const
		{
			return m_index;
		}

	private:
		void entity_change_archetype(
			sequence_ref<entity> entities,
			archetype_storage* dest_archetype,
			sorted_sequence_ref<generic::constructor> adding_constructors)
		{
			sorted_sequence_ref<generic::constructor>::iterator constructors_iter = adding_constructors.begin();

			//discard src unmatch components and construct dest unmatch components, move match components
			std::visit([&](auto& src, auto& dest)
				{
					using Tsrc = std::decay_t<decltype(src)>;
					using Tdest = std::decay_t<decltype(dest)>;
					auto src_accessor = src.get_raw_accessor(entities);
					auto dest_accessor = dest.get_allocate_accessor(entities, [](entity e, storage_key s) {
						//todo
						});
					auto src_component_accessors = src_accessor.begin();
					auto dest_component_accessors = dest_accessor.begin();
					while (src_component_accessors != src_accessor.end())
						if (src_component_accessors.comparable() < dest_component_accessors.comparable())
						{
							component_type_index type = src_component_accessors.component_type();
							for (void* addr : src_component_accessors)
								type.destructor(addr);
							src_component_accessors++;
						}
						else if (src_component_accessors.comparable() > dest_component_accessors.comparable())
						{
							assert(constructors_iter->type() == dest_component_accessors.component_type().type_index());
							for (void* addr : dest_component_accessors)
								(*constructors_iter)(addr);
							dest_component_accessors++;
							constructors_iter++;
						}
						else
						{
							auto src_comp_iter = src_component_accessors.begin();
							auto dest_comp_iter = dest_component_accessors.begin();
							component_type_index type = src_component_accessors.component_type();
							while (src_comp_iter != src_component_accessors.end())
							{
								type.move_constructor(*dest_comp_iter, *src_comp_iter);
								src_comp_iter++;
								dest_comp_iter++;
							}
							src_component_accessors++;
							dest_component_accessors++;
						}

				}, m_table, dest_archetype->m_table);

		}

		void sparse_convert_to_chunk()
		{
			auto sparse_table_ptr = std::make_unique<sparse_table>(std::move(std::get<sparse_table>(m_table)));
			m_storage_type = storage_type::Chunk;
			initializer_list<component_type_index> components(m_index.begin(), m_index.end());
			m_table.emplace<table>(components);
			auto& entities = sparse_table_ptr->get_entities();
			
			auto src_accessor = sparse_table_ptr->get_raw_accessor(entities);
			auto dest_accessor = std::get<table>(m_table).get_allocate_accessor(entities, [](entity e, storage_key s) {
				//todo
				});

			auto src_component_accessors = src_accessor.begin();
			auto dest_component_accessors = dest_accessor.begin();
			while (src_component_accessors != src_accessor.end())
			{
				auto src_comp_iter = src_component_accessors.begin();
				auto dest_comp_iter = dest_component_accessors.begin();
				component_type_index type = src_component_accessors.component_type();
				while (src_comp_iter != src_component_accessors.end())
				{
					type.move_constructor(*dest_comp_iter, *src_comp_iter);
					src_comp_iter++;
					dest_comp_iter++;
				}
				src_component_accessors++;
				dest_component_accessors++;
			}

			for (auto& on_sparse_to_chunk : m_on_sparse_to_chunk)
			{
				on_sparse_to_chunk();
			}
		}

		void chunk_convert_to_sparse()
		{
			auto table_ptr = std::make_unique<table>(std::move(std::get<table>(m_table)));
			m_storage_type = storage_type::Sparse;
			auto entities = table_ptr->get_entities();
			auto src_accessor = table_ptr->get_raw_accessor(make_sequence_ref(entities));
			auto dest_accessor = std::get<sparse_table>(m_table).get_allocate_accessor(entities, [](entity e, storage_key s) {
				//todo
				});

			auto src_component_accessors = src_accessor.begin();
			auto dest_component_accessors = dest_accessor.begin();
			while (src_component_accessors != src_accessor.end())
			{
				auto src_comp_iter = src_component_accessors.begin();
				auto dest_comp_iter = dest_component_accessors.begin();
				component_type_index type = src_component_accessors.component_type();
				while (src_comp_iter != src_component_accessors.end())
				{
					type.move_constructor(*dest_comp_iter, *src_comp_iter);
					src_comp_iter++;
					dest_comp_iter++;
				}
				src_component_accessors++;
				dest_component_accessors++;
			}

			for (auto& on_chunk_to_sparse : m_on_chunk_to_sparse)
			{
				on_chunk_to_sparse();
			}
		}



		storage_key add_entity(entity e, sequence_ref<generic::constructor> constructors)
		{
			storage_key key;
			switch (m_storage_type)
			{
			case storage_type::Sparse:
				break;
			case storage_type::Chunk:
				break;
			}

			for (auto& on_add : m_on_entity_add)
			{
				on_add(e, key);
			}

			return key;
		}

		storage_key copy_construct_entity(entity e, sequence_ref<void*> component_data)
		{
			storage_key key;
			switch (m_storage_type)
			{
			case storage_type::Sparse:
				//key = m_sparse_table.copy_construct_entity(e, component_data);
				break;
			case storage_type::Chunk:
				//key = m_table->copy_construct_entity(e, component_data);
				break;
			}

			for (auto& on_add : m_on_entity_add)
			{
				on_add(e, key);
			}

			return key;
		}



		template<typename Callable>
		void for_each_entity(Callable&& callable)
		{
			switch (m_storage_type)
			{
			case storage_type::Sparse:
				//m_sparse_table.for_each_entity(callable);
				break;
			case storage_type::Chunk:
				//m_table->for_each_entity(callable);
				break;
			}
		}




	};


}
