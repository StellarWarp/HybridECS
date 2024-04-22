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
		vector<component_type_index> m_notnull_components;
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
			sorted_sequence_ref<component_storage*> component_storages,
			storage_key_registry::group_key_accessor key_registry)
			: m_index(index),
			m_table(sparse_table(component_storages)),
			m_key_registry(key_registry)
		{
			m_notnull_components.reserve(component_storages.size());
			for (uint64_t i = 0; i < component_storages.size(); ++i)
			{
				m_notnull_components.push_back(component_storages[i]->component_type());
			}
		}

		archetype_index archetypex() const
		{
			return m_index;
		}

	public:
		archetype_index archetype() const
		{
			return m_index;
		}
		void get_component_indices(
			sorted_sequence_ref<const component_type_index> types,
			sequence_ref<uint32_t> component_indices) const
		{
			auto comp_iter = types.begin();
			auto write_iter = component_indices.begin();
			uint32_t index = 0;
			while (comp_iter != types.end())
			{
				const auto& comp = *comp_iter;
				if (comp < m_notnull_components[index])
				{
					comp_iter++; 
					write_iter++;
				}
				else if (comp == m_notnull_components[index])
				{
					*write_iter = index;
					write_iter++;
					comp_iter++;
					index++;
				}
				else
				{
					index++;
				}
			}
		}


		void entity_change_archetype(
			sequence_ref<const entity> entities,
			archetype_storage* dest_archetype,
			sorted_sequence_ref<const generic::constructor> adding_constructors)
		{
			auto constructors_iter = adding_constructors.begin();

			using src_accessor_variant = std::variant<table::deallocate_accessor, sparse_table::deallocate_accessor>;
			using dest_accessor_variant = std::variant<table::allocate_accessor<>, sparse_table::allocate_accessor<>>;
			auto& src = m_table;
			auto& dest = dest_archetype->m_table;
			vector<storage_key::table_offset_t> keys;
			if (src.index() == 0)
			{
				keys.reserve(entities.size());
				for (auto e : entities) keys.push_back(m_key_registry.at(e).get_table_offset());
			}
			auto src_accessor_var = std::visit([&](auto& t) -> src_accessor_variant {
				using table_type = std::decay_t<decltype(t)>;
				if constexpr (std::is_same_v<table_type, table>)
					return t.get_deallocate_accessor(keys);
				else if constexpr (std::is_same_v<table_type, sparse_table>)
					return t.get_deallocate_accessor(entities);
				else
					static_assert(!std::is_same_v<table_type, table_type>, "unhandled type");
				}, src);
			auto dest_accessor_var = std::visit([&](auto& t) -> dest_accessor_variant {
				return t.get_allocate_accessor(entities, [&](entity e, storage_key s) {
					using table_type = std::decay_t<decltype(t)>;
					if constexpr (std::is_same_v<table_type, table>)
					{
						m_key_registry.insert(e, s);
					}
					else if constexpr (std::is_same_v<table_type, sparse_table>) {}
					else static_assert(!std::is_same_v<table_type, table_type>, "unhandled type");
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
							auto& constructor = *constructors_iter;
							for (void* addr : dest_component_accessors)
								constructor(addr);
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

			auto src_accessor = sparse_table_ptr->get_raw_accessor();
			auto entity_seq = make_sequence_ref(entities.begin(), entities.end());
			auto dest_accessor = std::get<table>(m_table).get_allocate_accessor(entity_seq, [=](entity e, storage_key s) {
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
			auto entities_seq = make_sequence_ref(entities).as_const();
			auto src_accessor = table_ptr->get_raw_accessor();
			auto dest_accessor = std::get<sparse_table>(m_table).get_allocate_accessor(entities_seq, [](entity e, storage_key s) {
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
		template<typename SeqParam>
		class allocate_accessor
		{
			using sparse_allocate_accessor = sparse_table::allocate_accessor<SeqParam>;
			using chunk_allocate_accessor = table::allocate_accessor<SeqParam>;
			std::variant<sparse_allocate_accessor, chunk_allocate_accessor> m_accessor;

		public:
			allocate_accessor(sparse_allocate_accessor accessor)
				: m_accessor(accessor) {}
			allocate_accessor(chunk_allocate_accessor accessor)
				: m_accessor(accessor) {}

			class component_array_accessor
			{
				using sparse_component_array_accessor = typename sparse_allocate_accessor::component_array_accessor;
				using chunk_component_array_accessor = typename chunk_allocate_accessor::component_array_accessor;
				std::variant<sparse_component_array_accessor, chunk_component_array_accessor> m_accessor;

			public:
				component_array_accessor(sparse_component_array_accessor accessor)
					: m_accessor(accessor) {}

				component_array_accessor(chunk_component_array_accessor accessor)
					: m_accessor(accessor) {}

				component_array_accessor(const component_array_accessor& other)
					: m_accessor(other.m_accessor) {}

				component_array_accessor& operator++() { std::visit([](auto& accessor) {++accessor; }, m_accessor); return*this; }
				component_array_accessor operator++(int) { std::visit([](auto& accessor) {accessor++; }, m_accessor); return *this; }

				component_type_index component_type() const { return std::visit([](auto& accessor) {return accessor.component_type(); }, m_accessor); }

				class iterator
				{
					using sparse_iterator = typename sparse_component_array_accessor::iterator;
					using chunk_iterator = typename chunk_component_array_accessor::iterator;
					std::variant<sparse_iterator, chunk_iterator, end_iterator> m_iterator;

				public:
					iterator(sparse_iterator iter)
						: m_iterator(iter) {}

					iterator(chunk_iterator iter)
						: m_iterator(iter) {}

					iterator(end_iterator)
						: m_iterator(nullptr) {}

					void* operator*()
					{
						return std::visit([](auto& iter) -> void* {
							using iter_type = std::decay_t<decltype(iter)>;
							if constexpr (has_operator_dereference_v<iter_type>)return *iter;
							else return nullptr;
							}, m_iterator);
					}

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
				iterator begin() { return std::visit([](auto& accessor) {return iterator(accessor.begin()); }, m_accessor); }
				iterator end() { return std::visit([](auto& accessor) {return iterator(accessor.end()); }, m_accessor); }

				bool operator==(const component_array_accessor& other) const { return m_accessor == other.m_accessor; }
				bool operator!=(const component_array_accessor& other) const { return m_accessor != other.m_accessor; }
				bool operator==(const end_iterator& other) const { return std::visit([](auto& accessor) {return accessor == nullptr; }, m_accessor); }
				bool operator!=(const end_iterator& other) const { return std::visit([](auto& accessor) {return accessor != nullptr; }, m_accessor); }
			};

			component_array_accessor begin() { return std::visit([](auto& accessor) {return component_array_accessor(accessor.begin()); }, m_accessor); }
			end_iterator end() { return {}; }

		};

		template<typename SeqParam>
		auto get_allocate_accessor(sequence_ref<const entity, SeqParam> entities)
		{
			return std::visit([&](auto& t)
				{
					using table_type = std::decay_t<decltype(t)>;
					return allocate_accessor(t.get_allocate_accessor(entities, [this](entity e, storage_key s) {
						if constexpr (std::is_same_v<table_type, table>)
						{
							m_key_registry.insert(e, s);
						}
						else if constexpr (std::is_same_v<table_type, sparse_table>)
						{
							//todo
						}
						}));
				}, m_table);
		}


		void dynamic_for_each(sequence_ref<const uint32_t> component_indices, std::function<void(entity, sequence_ref<void*>)> func)
		{
			std::visit([&](auto& t)
				{
					t.dynamic_for_each(component_indices, func);
				}, m_table);
		}





		//storage_key copy_construct_entity(entity e, sequence_ref<void*> component_data)
		//{
		//	storage_key key;
		//	switch (get)
		//	{
		//	case storage_type::Sparse:
		//		//key = m_sparse_table.copy_construct_entity(e, component_data);
		//		break;
		//	case storage_type::Chunk:
		//		//key = m_table->copy_construct_entity(e, component_data);
		//		break;
		//	}

		//	for (auto& on_add : m_on_entity_add)
		//	{
		//		on_add(e, key);
		//	}

		//	return key;
		//}



		//template<typename Callable>
		//void for_each_entity(Callable&& callable)
		//{
		//	switch (m_storage_type)
		//	{
		//	case storage_type::Sparse:
		//		//m_sparse_table.for_each_entity(callable);
		//		break;
		//	case storage_type::Chunk:
		//		//m_table->for_each_entity(callable);
		//		break;
		//	}
		//}




	};


}
