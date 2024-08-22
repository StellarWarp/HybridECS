#pragma once
#include "table.h"
#include "sparse_table.h"
#include "storage_key_registry.h"

namespace hyecs
{
	class archetype_storage : non_movable
	{
		friend class tag_archetype_storage;

		enum class storage_type
		{
			Sparse,
			Chunk,
		};

		archetype_index m_index;
		vector<component_type_index> m_notnull_components; //cache from m_component_storages
		vector<component_storage*> m_component_storages; //store for chunk to sparse convert
		std::variant<table, sparse_table> m_table;
		storage_type get_storage_type() const { return m_table.index() == 0 ? storage_type::Chunk : storage_type::Sparse; }
		storage_key_registry::group_key_accessor m_key_registry;

		//event
		//add and remove event are inside the m_table
		// vector<std::function<void(entity, storage_key)>> m_on_entity_add;
		// vector<std::function<void(entity, storage_key)>> m_on_entity_remove;
		vector<std::function<void(entity, storage_key, storage_key)>> m_on_entity_move; //from, to
		vector<std::function<void()>> m_on_sparse_to_chunk;
		vector<std::function<void()>> m_on_chunk_to_sparse;

		uint32_t sparse_to_chunk_convert_limit;
		uint32_t chunk_to_sparse_convert_limit;

	public:
		archetype_storage(
			archetype_index index,
			sorted_sequence_cref<component_storage*> component_storages,
			storage_key_registry::group_key_accessor key_registry)
			: m_index(index),
			  m_component_storages(component_storages),
			  m_table(sparse_table(component_storages)),
			  m_key_registry(key_registry)
		{
			m_notnull_components.reserve(component_storages.size());
			for (uint64_t i = 0; i < component_storages.size(); ++i)
			{
				m_notnull_components.push_back(component_storages[i]->component_type());
			}
			size_t components_size = sizeof(entity);
			for (auto& comp : index)
				components_size += comp.size();

			if (component_storages.size() > 1)
			{
				sparse_to_chunk_convert_limit = component_table_chunk_traits::size / components_size;
				chunk_to_sparse_convert_limit = sparse_to_chunk_convert_limit / 2;
			}
			else
			{
				sparse_to_chunk_convert_limit = std::numeric_limits<uint32_t>::max();
				chunk_to_sparse_convert_limit = 0;
			}
		}

	public:
		archetype_index archetype() const
		{
			return m_index;
		}

		size_t entity_count() const
		{
			return std::visit([](auto& t)
			{
				return t.entity_count();
			}, m_table);
		}

		const storage_key_registry::group_key_accessor& get_key_registry() const
		{
			return m_key_registry;
		}

		table* get_table()
		{
			return std::get_if<table>(&m_table);
		}

		const vector<component_type_index>& get_accessible_components() const
		{
			return m_notnull_components;
		}

		const vector<component_storage*>& get_component_storages() const
		{
			return m_component_storages;
		}

		void get_component_indices(
			sorted_sequence_cref<component_type_index> types,
			sequence_ref<uint32_t> component_indices) const
		{
			get_sub_sequence_indices<component_type_index>(
				sorted_sequence_cref(m_notnull_components),
				types,
				component_indices);
		}

		void get_component_indices(
			sequence_cref<component_type_index> types,
			sequence_ref<uint32_t> component_indices) const
		{
			get_sub_sequence_indices<component_type_index>(
				sorted_sequence_cref(m_notnull_components),
				types,
				component_indices);
		}


		void add_callback_on_entity_add(std::function<void(entity, storage_key)> callback)
		{
			std::visit([&](auto& t)
			{
				t.add_callback_on_entity_add(callback);
			}, m_table);
		}

		void add_callback_on_entity_remove(std::function<void(entity, storage_key)> callback)
		{
			std::visit([&](auto& t)
			{
				t.add_callback_on_entity_remove(callback);
			}, m_table);
		}


		void entity_change_archetype(
			sequence_cref<entity> entities,
			archetype_storage* dest_archetype,
			sorted_sequence_cref<generic::constructor> adding_constructors)
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
			auto src_accessor_var = std::visit([&](auto& t) -> src_accessor_variant
			{
				using table_type = std::decay_t<decltype(t)>;
				if constexpr (std::is_same_v<table_type, table>)
					return t.get_deallocate_accessor(keys);
				else if constexpr (std::is_same_v<table_type, sparse_table>)
					return t.get_deallocate_accessor(entities);
				else
					static_assert(!std::is_same_v<table_type, table_type>, "unhandled type");
			}, src);
			auto dest_accessor_var = std::visit([&](auto& t) -> dest_accessor_variant
			{
				return t.get_allocate_accessor(entities, [&](entity e, storage_key s)
				{
					using table_type = std::decay_t<decltype(t)>;
					if constexpr (std::is_same_v<table_type, table>)
					{
						m_key_registry.insert(e, s);
					}
					else if constexpr (std::is_same_v<table_type, sparse_table>)
					{
					}
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
						assert(constructors_iter->type() == dest_component_accessors.component_type());
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
            sorted_sequence_cref<component_type_index> components(m_index.begin(), m_index.end());
			m_table.emplace<table>(components);
			auto& entities = sparse_table_ptr->get_entities();

			auto src_accessor = sparse_table_ptr->get_raw_accessor();
			auto entity_seq = sequence_cref(entities.begin(), entities.end());
			auto dest_accessor = std::get<table>(m_table).get_allocate_accessor(entity_seq,
			                                                                    [this](entity e, storage_key s)
			                                                                    {
				                                                                    m_key_registry.insert(e, s);
			                                                                    });

			auto src_component_accessors = src_accessor.begin();
			auto dest_component_accessors = dest_accessor.begin();
			while (src_component_accessors != src_accessor.end())
			{
				auto src_comp_iter = src_component_accessors.begin();
				auto dest_comp_iter = dest_component_accessors.begin();
				component_type_index type = src_component_accessors.component_type();
				assert(type == dest_component_accessors.component_type());
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

			dest_accessor.notify_move_construct_finish();
		}

		void chunk_convert_to_sparse()
		{
			table* table_ptr = &std::get<table>(m_table);
			auto sparse_ptr = std::make_unique<sparse_table>(sorted_sequence_cref(m_component_storages));
			auto entities = table_ptr->get_entities();
			auto src_accessor = table_ptr->get_raw_accessor();
			auto dest_accessor = sparse_ptr->get_allocate_accessor(
				sequence_cref(entities),
				[this](entity e, storage_key s)
				{
					//todo may need erase key
					// m_key_registry.insert(e, s);//need to set the key to null
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

			m_table = std::move(*sparse_ptr);

			for (auto& on_chunk_to_sparse : m_on_chunk_to_sparse)
			{
				on_chunk_to_sparse();
			}

			dest_accessor.notify_move_construct_finish();
		}

		void add_callback_on_sparse_to_chunk(std::function<void()>&& callback)
		{
			m_on_sparse_to_chunk.emplace_back(callback);
		}

		void add_callback_on_chunk_to_sparse(std::function<void()>&& callback)
		{
			m_on_chunk_to_sparse.emplace_back(callback);
		}

		// void emplace_entity(
		// 	sequence_cref< entity> entities,
		// 	sorted_sequence_ref<const generic::constructor> constructors,
		// 	sequence_ref<storage_key> keys)
		// {
		// 	std::visit([&](auto& table)
		// 		{
		// 			auto accessor = table.get_allocate_accessor(entities, [](entity e, storage_key s) {
		// 				//todo
		// 				});
		// 			auto component_accessors = accessor.begin();
		// 			for (auto constructor : constructors)
		// 			{
		// 				for (void* addr : component_accessors)
		// 				{
		// 					constructor(addr);
		// 				}
		// 				component_accessors++;
		// 			}
		// 		}, m_table);
		//
		// 	for (auto& on_add : m_on_entity_add)
		// 	{
		// 		auto key_iter = keys.begin();
		// 		for (entity e : entities)
		// 		{
		// 			on_add(e, *key_iter);
		// 			key_iter++;
		// 		}
		// 	}
		// }

		using end_iterator = nullptr_t;

		//a warp for sparse_table and table allocate_accessor
		template <typename SeqParam>
		class allocate_accessor
		{
			using sparse_allocate_accessor = sparse_table::allocate_accessor<SeqParam>;
			using chunk_allocate_accessor = table::allocate_accessor<SeqParam>;
			std::variant<sparse_allocate_accessor, chunk_allocate_accessor> m_accessor;

		public:
			allocate_accessor()
			{
			};

			allocate_accessor(sparse_allocate_accessor&& accessor)
				: m_accessor(std::move(accessor))
			{
			}

			allocate_accessor(chunk_allocate_accessor&& accessor)
				: m_accessor(std::move(accessor))
			{
			}

			const chunk_allocate_accessor* get_chunk_allocate_accessor() const
			{
				return std::get_if<chunk_allocate_accessor>(&m_accessor);
			}

			void notify_construct_finish()
			{
				std::visit([](auto& accessor) { accessor.notify_construct_finish(); }, m_accessor);
			}

			class component_array_accessor
			{
				using sparse_component_array_accessor = typename sparse_allocate_accessor::component_array_accessor;
				using chunk_component_array_accessor = typename chunk_allocate_accessor::component_array_accessor;
				std::variant<sparse_component_array_accessor, chunk_component_array_accessor> m_accessor;

			public:
				component_array_accessor(sparse_component_array_accessor accessor)
					: m_accessor(accessor)
				{
				}

				component_array_accessor(chunk_component_array_accessor accessor)
					: m_accessor(accessor)
				{
				}

				component_array_accessor(const component_array_accessor& other) = delete;

				component_array_accessor(component_array_accessor&& other) noexcept
					: m_accessor(std::move(other.m_accessor))
				{
				}


				component_array_accessor& operator++()
				{
					std::visit([](auto& accessor) { ++accessor; }, m_accessor);
					return *this;
				}

				component_array_accessor operator++(int)
				{
					std::visit([](auto& accessor) { accessor++; }, m_accessor);
					return *this;
				}

				component_type_index component_type() const { return std::visit([](auto& accessor) { return accessor.component_type(); }, m_accessor); }

				class iterator
				{
					using sparse_iterator = typename sparse_component_array_accessor::iterator;
					using chunk_iterator = typename chunk_component_array_accessor::iterator;
					std::variant<sparse_iterator, chunk_iterator> m_iterator;

				public:
					iterator(sparse_iterator iter)
						: m_iterator(iter)
					{
					}

					iterator(chunk_iterator iter)
						: m_iterator(iter)
					{
					}

					void* operator*()
					{
						return std::visit([](auto& iter) -> void* {
							using iter_type = std::decay_t<decltype(iter)>;
							if constexpr (dereferenceable<iter_type>)return *iter;
							else return nullptr;
						}, m_iterator);
					}

					void operator++()
					{
						std::visit([](auto& iterator) { iterator++; }, m_iterator);
					}

					void operator++(int)
					{
						operator++();
					}

					//can't use variant operator== and operator!=
					bool operator==(const iterator& other) const
					{
						return std::visit(
							[&](auto& accessor)
							{
								using iter_type = std::decay_t<decltype(accessor)>;
								if constexpr (std::is_same_v<iter_type, sparse_iterator>)
									return accessor == std::get<sparse_iterator>(other.m_iterator);
								else if constexpr (std::is_same_v<iter_type, chunk_iterator>)
									return accessor == std::get<chunk_iterator>(other.m_iterator);
								else
									static_assert(!std::is_same_v<iter_type, iter_type>, "unhandled type");
							}, m_iterator);
					}

					bool operator==(const end_iterator&) const { return std::visit([](auto& accessor) { return accessor == nullptr; }, m_iterator); }
				};

				iterator begin() { return std::visit([](auto& accessor) { return iterator(accessor.begin()); }, m_accessor); }
				end_iterator end() { return nullptr; }

				bool operator==(const component_array_accessor& other) const { return m_accessor == other.m_accessor; }
				bool operator==(const end_iterator& other) const { return std::visit([](auto& accessor) { return accessor == nullptr; }, m_accessor); }
			};

			component_array_accessor begin() { return std::visit([](auto& accessor) { return component_array_accessor(accessor.begin()); }, m_accessor); }
			end_iterator end() { return {}; }
		};

		template <typename SeqParam>
		auto get_allocate_accessor(sequence_cref<entity, SeqParam> entities)
		{
			std::visit([&](auto& t)
			{
				using table_type = std::decay_t<decltype(t)>;
				if constexpr (std::is_same_v<table_type, sparse_table>)
				{
					if (t.entity_count() + entities.size() > sparse_to_chunk_convert_limit)
						sparse_convert_to_chunk();
				}
			}, m_table);

			return std::visit([&]<typename table_type>(table_type& t)
			{
				return allocate_accessor(t.get_allocate_accessor(entities, [this](entity e, storage_key s)
				{
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

		void dynamic_for_each(sequence_cref<uint32_t> component_indices, std::function<void(entity, sequence_ref<void*>)> func)
		{
			std::visit([&](auto& t)
			{
				t.dynamic_for_each(component_indices, func);
			}, m_table);
		}

		template <typename Callable>
		void for_each(Callable&& func, sequence_cref<uint32_t> component_indices)
		{
			std::visit([&](auto& t)
			{
				t.template for_each<Callable>(std::forward<Callable>(func), component_indices);
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
