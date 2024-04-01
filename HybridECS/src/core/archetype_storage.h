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
		std::variant<table, sparse_table> m_table;
		storage_type get_storage_type() const { return m_table.index() == 0 ? storage_type::Chunk : storage_type::Sparse; }
		storage_key_registry::group_key_accessor m_key_registry;

		//event

		vector<std::function<void(entity, storage_key)>> m_on_entity_add;
		vector<std::function<void(entity, storage_key)>> m_on_entity_remove;
		vector<std::function<void(entity, storage_key, storage_key)>> m_on_entity_move;//from, to
		vector<std::function<void()>> m_on_sparse_to_chunk;
		vector<std::function<void()>> m_on_chunk_to_sparse;

	public:
		archetype_storage(
			archetype_index index,
			vector<component_storage*> component_storages,
			storage_key_registry::group_key_accessor key_registry)
			: m_index(index),
			m_table(sparse_table(component_storages)),
			m_key_registry(key_registry)
		{
		}

		archetype_index archetypex() const
		{
			return m_index;
		}

	private:
		void entity_change_archetype(
			sequence_ref<const entity> entities,
			archetype_storage* dest_archetype,
			sorted_sequence_ref<generic::constructor> adding_constructors)
		{
			sorted_sequence_ref<generic::constructor>::iterator constructors_iter = adding_constructors.begin();

			using raw_accessor_variant = std::variant<table::raw_accessor, sparse_table::raw_accessor>;
			using allocate_accessor_variant = std::variant<table::allocate_accessor, sparse_table::allocate_accessor>;
			auto src = m_table;
			auto dest = dest_archetype->m_table;
			auto src_accessor_var = std::visit([&](auto& table) -> raw_accessor_variant {
				return table.get_raw_accessor(entities); 
				}, src);
			auto dest_accessor_var = std::visit([&](auto& table) -> allocate_accessor_variant {
				return table.get_allocate_accessor(entities, [&](entity e, storage_key s) {
				if constexpr (std::is_same_v<decltype(table), table>)
				{
					m_key_registry.
				}
				}); 
				}, dest);

			//discard src unmatch components and construct dest unmatch components, move match components
			std::visit([&](auto& src_accessor, auto& dest_accessor)
				{

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

				}, src_accessor_var, dest_accessor_var);

		}

		void sparse_convert_to_chunk()
		{
			auto sparse_table_ptr = std::make_unique<sparse_table>(std::move(std::get<sparse_table>(m_table)));
			initializer_list<component_type_index> components(m_index.begin(), m_index.end());
			m_table.emplace<table>(components);
			auto& entities = sparse_table_ptr->get_entities();

			auto src_accessor = sparse_table_ptr->get_raw_accessor(entities);
			auto dest_accessor = std::get<table>(m_table).get_allocate_accessor(entities, [=](entity e, storage_key s) {
				m_key_registry.insert(e, s);
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
			auto entities = table_ptr->get_entities();
			auto src_accessor = table_ptr->get_raw_accessor(make_sequence_ref(entities));
			auto dest_accessor = std::get<sparse_table>(m_table).get_allocate_accessor(entities, [](entity e, storage_key s) {
				//todo
				//no action for sparse table as entity is the only key for sparse table
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



		storage_key add_entity(entity e, sorted_sequence_ref<const generic::constructor> constructors)
		{
			storage_key key;
			switch (get_storage_type())
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

		void emplace_entity(
			sequence_ref<const entity> entities,
			sorted_sequence_ref<const generic::constructor> constructors,
			sequence_ref<storage_key> keys)
		{
			std::visit([&](auto& table)
				{
					auto accessor = table.get_allocate_accessor(entities, [](entity e, storage_key s) {
						//todo
						});
					auto component_accessors = accessor.begin();
					for (auto constructor : constructors)
					{
						for (void* addr : component_accessors)
						{
							constructor(addr);
						}
						component_accessors++;
					}
				}, m_table);

			for (auto& on_add : m_on_entity_add)
			{
				auto key_iter = keys.begin();
				for (entity e : entities)
				{
					on_add(e, *key_iter);
					key_iter++;
				}
			}
		}

		using end_iterator = nullptr_t;

		//a warp for sparse_table and table allocate_accessor
		class allocate_accessor
		{
			std::variant<sparse_table::allocate_accessor, table::allocate_accessor> m_accessor;

		public:
			allocate_accessor(sparse_table::allocate_accessor accessor)
				: m_accessor(accessor) {}
			allocate_accessor(table::allocate_accessor accessor)
				: m_accessor(accessor) {}

			class component_array_accessor
			{
				std::variant<sparse_table::allocate_accessor::component_array_accessor, table::allocate_accessor::component_array_accessor> m_accessor;

			public:
				component_array_accessor(sparse_table::allocate_accessor::component_array_accessor accessor)
					: m_accessor(accessor) {}

				component_array_accessor(table::allocate_accessor::component_array_accessor accessor)
					: m_accessor(accessor) {}

				component_array_accessor(const component_array_accessor& other)
					: m_accessor(other.m_accessor) {}

				component_array_accessor& operator++() { std::visit([](auto& accessor) {++accessor; }, m_accessor); return*this; }
				component_array_accessor operator++(int) { std::visit([](auto& accessor) {accessor++; }, m_accessor); return *this; }


				class iterator
				{
					std::variant<sparse_table::allocate_accessor::component_array_accessor::iterator, table::allocate_accessor::component_array_accessor::iterator> m_iterator;

				public:
					iterator(sparse_table::allocate_accessor::component_array_accessor::iterator iterator)
						: m_iterator(iterator) {}

					iterator(table::allocate_accessor::component_array_accessor::iterator iterator)
						: m_iterator(iterator) {}

					void* operator*() { return std::visit([](auto& iterator) {return *iterator; }, m_iterator); }

					iterator& operator++()
					{
						std::visit([](auto& iterator) {iterator++; }, m_iterator);
						return *this;
					}
					iterator& operator++(int)
					{
						std::visit([](auto& iterator) {iterator++; }, m_iterator);
						return *this;
					}

					bool operator==(const iterator& other) const { return m_iterator == other.m_iterator; }
					bool operator!=(const iterator& other) const { return m_iterator != other.m_iterator; }
				};
				iterator begin() { return std::visit([](auto& accessor) {return accessor.begin(); }, m_accessor); }
				iterator end() { return std::visit([](auto& accessor) {return accessor.end(); }, m_accessor); }

				bool operator==(const component_array_accessor& other) const { return m_accessor == other.m_accessor; }
				bool operator!=(const component_array_accessor& other) const { return m_accessor != other.m_accessor; }
				bool operator==(const end_iterator& other) const { return std::visit([](auto& accessor) {return accessor == nullptr; }, m_accessor); }
				bool operator!=(const end_iterator& other) const { return std::visit([](auto& accessor) {return accessor != nullptr; }, m_accessor); }
			};

			component_array_accessor begin() { return std::visit([](auto& accessor) {return component_array_accessor(accessor.begin()); }, m_accessor); }
			end_iterator end() { return {}; }

		};

		allocate_accessor get_allocate_accessor(sequence_ref<entity> entities)
		{
			return std::visit([&](auto& table)
				{
					return allocate_accessor(table.get_allocate_accessor(entities, [](entity e, storage_key s) {
						//todo
						}));
				}, m_table);
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
