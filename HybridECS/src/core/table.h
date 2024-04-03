#pragma once
#include "../lib/Lib.h"
#include "archetype.h"
#include "entity.h"
#include "storage_key_registry.h"

namespace hyecs
{
	static struct ChunkConfig
	{
		static const size_t DEFAULT_CHUNK_SIZE = 128 * 1024;//128kb
	};


	//template<typename Allocator = std::allocator<uint8_t>>
	using byte_differ_t = uint32_t;
	class table
	{
		using Allocator = std::allocator<uint8_t>;
		using byte = uint8_t;

		using table_index_t = storage_key::table_index_t;
		using table_offset_t = storage_key::table_offset_t;


		class chunk
		{
		private:
			std::array<entity, ChunkConfig::DEFAULT_CHUNK_SIZE> m_entities;
			std::array<byte, ChunkConfig::DEFAULT_CHUNK_SIZE> m_data;
			size_t m_size = 0;
		public:
			size_t size() const { return m_size; }
			byte* data() { return m_data.data(); }
			auto& entities() { return m_entities; }
			const auto& entities() const { return m_entities; }

			void increase_size(size_t size) { m_size += size; }
			void decrease_size(size_t size) { m_size -= size; }
		};

		using chunk_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<chunk>;

		[[no_unique_address]] chunk_allocator m_allocator;
		struct table_comp_type_info : public cached_component_type_index
		{
		private:
			uint32_t m_offset;
		public:
			table_comp_type_info() = default;
			table_comp_type_info(component_type_index type) : cached_component_type_index(type) {}
			table_comp_type_info(component_type_index type, uint32_t offset) : cached_component_type_index(type), m_offset(offset) {}

			void set_offset(uint32_t offset) { m_offset = offset; }
			uint32_t offset() const { return m_offset; }
		};
		vector<table_comp_type_info> m_notnull_components;
		//todo add allocator for vec?
		vector<chunk*> m_chunks;
		size_t m_chunk_capacity;
		uint32_t m_chunk_offset_bits;
		table_index_t m_table_index;


		struct entity_table_index
		{
			uint32_t chunk_index;
			uint32_t chunk_offset;
		};
		//temporary hole in the table for batch adding removing entities
		stack<entity_table_index> m_free_indices;



	public:
		table(initializer_list<component_type_index> components)
		{
			//size computation
			//for (auto& type : m_types)
			//	type.size = std::bit_ceil(type.size);

			size_t column_size = 0;
			size_t offset = 0;

			for (auto& type : components)
			{
				if (type.size() == 0) continue;
				column_size += type.size();
			}

			m_chunk_capacity = ChunkConfig::DEFAULT_CHUNK_SIZE / column_size;
			//how many bits needed to store chunk offset
			m_chunk_offset_bits = std::bit_width(m_chunk_capacity - 1);
			auto comp_iter = components.begin();
			m_notnull_components.reserve(components.size());
			for (auto& type : m_notnull_components)
			{
				if (comp_iter->size() == 0) continue;
				m_notnull_components.emplace_back(*comp_iter, offset);
				offset += type.size() * m_chunk_capacity;
				comp_iter++;
			}
		}
		~table()
		{
			for (auto& chunk : m_chunks)
			{
				deallocate_chunk(chunk);
			}
		}

		table(const table& other) {};
		table(table&& other) :
			m_allocator(std::move(other.m_allocator)),
			m_notnull_components(std::move(other.m_notnull_components)),
			m_chunks(std::move(other.m_chunks)),
			m_chunk_capacity(std::move(other.m_chunk_capacity)),
			m_free_indices(std::move(other.m_free_indices))
		{
		}


	private:


		chunk* allocate_chunk()
		{
			chunk* new_chunk = new (m_allocator.allocate(1)) chunk();
			m_chunks.push_back(new_chunk);
			uint32_t chunk_index = m_chunks.size() - 1;

			return new_chunk;
		}
		void deallocate_chunk(chunk* _chunk)
		{
			for (int component_index = 0; component_index < m_notnull_components.size(); component_index++)
			{
				auto& comp_type = m_notnull_components[component_index];
				comp_type.destructor(_chunk->data() + comp_type.offset(), _chunk->size());
			}
			m_allocator.deallocate(_chunk, 1);
		}

		entity_table_index allocate_entity()
		{
			if (!m_free_indices.empty())
			{
				auto index = m_free_indices.top();
				m_free_indices.pop();
				return index;
			}
			if (m_chunks.empty() || m_chunks.back()->size() == m_chunk_capacity)
				allocate_chunk();

			uint32_t chunk_index = m_chunks.size() - 1;
			uint32_t chunk_offset = m_chunks.back()->size();

			return { chunk_index, chunk_offset };
		}

		void deallocate_entity(entity_table_index index)
		{
			m_free_indices.push(index);
		}

		void deallocate_entity(table_offset_t table_offset)
		{
			deallocate_entity(chunk_index_offset(table_offset));
		}

	public:
		//todo can be optimized to in-chunk swap
		//call after all addition and removal were done
		void phase_swap_back()
		{
			while (!m_free_indices.empty())
			{
				auto [chunk_index, chunk_offset] = m_free_indices.top();
				m_free_indices.pop();
				auto last_chunk = m_chunks.back();
				if (last_chunk->size() == 0)
				{
					m_chunks.pop_back();
					m_allocator.deallocate(last_chunk, 1);
					if (m_chunks.empty()) return;
					last_chunk = m_chunks.back();
				}
				uint32_t last_chunk_offset = last_chunk->size() - 1;
				last_chunk->decrease_size(1);
				for (auto& type : m_notnull_components)
				{
					byte* last_data = component_address(last_chunk, last_chunk_offset, type);
					byte* data = component_address(m_chunks[chunk_index], chunk_offset, type);
					type.move_constructor(data, last_data);
				}
			}
		}
	private:
		static byte* component_address(chunk* chunk, uint32_t chunk_offset, uint32_t comp_offset, uint32_t comp_size)
		{
			return chunk->data() + comp_offset + chunk_offset * comp_size;
		}
		byte* component_address(const entity_table_index& index, uint32_t comp_offset, uint32_t comp_size)
		{
			return component_address(m_chunks[index.chunk_index], index.chunk_offset, comp_offset, comp_size);
		}
		static byte* component_address(chunk* chunk, uint32_t chunk_offset, const table_comp_type_info& type)
		{
			return chunk->data() + type.offset() + chunk_offset * type.size();
		}
		byte* component_address(const entity_table_index& index, const table_comp_type_info& type)
		{
			return component_address(m_chunks[index.chunk_index], index.chunk_offset, type);
		}
		byte* component_address(const entity_table_index& index, const uint32_t& comp_index)
		{
			auto& type = m_notnull_components[comp_index];
			return component_address(m_chunks[index.chunk_index], index.chunk_offset, type);
		}

		byte* component_address(chunk* chunk, uint32_t chunk_offset, const uint32_t& comp_index)
		{
			auto& type = m_notnull_components[comp_index];
			return component_address(chunk, chunk_offset, type);
		}




		constexpr entity_table_index chunk_index_offset(const table_offset_t& table_offset) const
		{
			//high bits
			uint32_t chunk_index = static_cast<uint32_t>(table_offset) >> m_chunk_offset_bits;
			//low bits
			uint32_t chunk_offset = static_cast<uint32_t>(table_offset) & ((1 << m_chunk_offset_bits) - 1);
			return { chunk_index, chunk_offset };
		}

		constexpr table_offset_t table_offset(const entity_table_index& index) const
		{
			return static_cast<table_offset_t>(index.chunk_index << m_chunk_offset_bits | index.chunk_offset);
		}

		void all_components(table_offset_t table_offset)
		{
			auto [chunk_index, chunk_offset] = chunk_index_offset(table_offset);
			chunk* chunk = m_chunks[chunk_index];
			for (int i = 0; i < m_notnull_components.size(); i++)
			{
				byte* data = component_address(chunk, chunk_offset, i);
			}
		}

	public:

		//template<typename SeqIter = uint32_t*>

		using end_iterator = nullptr_t;


		class raw_accessor
		{
		protected:
			using table_offset_seq = sequence_ref<const table_offset_t>;
			table& m_table;
			table_offset_seq table_offsets;
		public:

			raw_accessor(table& in_table, table_offset_seq offsets)
				: m_table(in_table), table_offsets(offsets)
			{
			}

			raw_accessor(const raw_accessor&) = default;
			raw_accessor& operator=(const raw_accessor&) = default;
			raw_accessor(raw_accessor&& other) : m_table(other.m_table), table_offsets(other.table_offsets)
			{
				other.table_offsets = table_offset_seq{};
			}
			//template<typename SeqParam>
			//raw_accessor(table& in_table, sequence_ref<entity, SeqParam> entities)
			//	: m_table(in_table)
			//{
			//	//todo
			//}


			class component_array_accessor
			{
			protected:
				raw_accessor* m_accessor;
				const table_comp_type_info* m_type;
				uint32_t m_component_index;
				uint32_t m_table_component_count;

			public:

				component_array_accessor(uint32_t index, raw_accessor* accessor)
					: m_type(&accessor->m_table.m_notnull_components[index]),
					m_component_index(index),
					m_table_component_count(accessor->m_table.m_notnull_components.size()),
					m_accessor(accessor)
				{
				}

				component_type_index component_type() const { return *m_type; }

				component_array_accessor& operator++()
				{
					m_component_index++;
					m_type = &m_accessor->m_table.m_notnull_components[m_component_index];
					return *this;
				}

				component_array_accessor operator++(int)
				{
					component_array_accessor copy = *this;
					++(*this);
					return copy;
				}

				auto comparable() const
				{
					return *m_type;
				}

				bool operator==(const component_array_accessor& other) const { return *m_type == *other.m_type; }
				bool operator!=(const component_array_accessor& other) const { return !(*this == other); }
				bool operator==(const end_iterator& other) const { return m_component_index == m_table_component_count; }
				bool operator!=(const end_iterator& other) const { return !(*this == other); }


				class iterator
				{
				protected:
					uint32_t m_comp_offset;
					uint32_t m_type_size;
					raw_accessor* m_accessor;
					typename table_offset_seq::iterator m_offset_iter;

					table& table() const { return m_accessor->m_table; }
					const table_offset_seq& sequence() const { return m_accessor->table_offsets; }
				public:
					iterator(uint32_t component_index,
						raw_accessor* accessor,
						typename table_offset_seq::iterator offset_iter)
						:
						m_comp_offset(accessor->m_table.m_notnull_components[component_index].offset()),
						m_type_size(accessor->m_table.m_notnull_components[component_index].size()),
						m_accessor(accessor),
						m_offset_iter(offset_iter)
					{
					}

					iterator& operator++()
					{
						m_offset_iter++; return *this;
					}

					iterator operator++(int)
					{
						iterator copy = *this;
						++(*this);
						return copy;
					}

					bool operator==(const iterator& other) const { return m_offset_iter == other.m_offset_iter; }
					bool operator!=(const iterator& other) const { return !operator==(other); }
					bool operator==(const end_iterator& other) const { return m_offset_iter == m_accessor->table_offsets.end(); }
					bool operator!=(const end_iterator& other) const { return !operator==(other); }

					void* operator*()
					{
						return table().component_address(
							table().chunk_index_offset(*m_offset_iter),
							m_comp_offset, m_type_size);
					}
				};

				iterator begin() const
				{
					return iterator(m_component_index, m_accessor, m_accessor->table_offsets.begin());
				}

				end_iterator end() const
				{
					return end_iterator{};
				}
			};

			component_array_accessor begin()
			{
				return component_array_accessor(0u, this);
			}

			end_iterator end()
			{
				return end_iterator{};
			}

		};

		raw_accessor get_raw_accessor(sequence_ref<const table_offset_t> entities_table_offsets)
		{
			return raw_accessor(*this, entities_table_offsets);
		}



		class complete_raw_accessor
		{
			const table* m_table;
		public:
			complete_raw_accessor(const table* table) : m_table(table) {}
			complete_raw_accessor(const complete_raw_accessor&) = default;
			complete_raw_accessor& operator=(const complete_raw_accessor&) = default;
			complete_raw_accessor(complete_raw_accessor&& other) = default;

			class component_array_accessor
			{
				using chunk_seq = sequence_ref<chunk* const>;
			protected:
				const table* m_table;
				const table_comp_type_info* m_type;
				uint32_t m_component_index;
				uint32_t m_table_component_count;

			public:
				using value_type = entity;
				component_array_accessor(const table* table, uint32_t index) :
					m_table(table),
					m_type(&table->m_notnull_components[index]),
					m_component_index(index),
					m_table_component_count(table->m_notnull_components.size())
				{
				}
				const component_type_index component_type() const { return *m_type; }
				auto comparable() const { return *m_type; }
				component_array_accessor& operator++()
				{
					m_component_index++;
					m_type = &m_table->m_notnull_components[m_component_index];
					return *this;
				}

				component_array_accessor operator++(int)
				{
					component_array_accessor copy = *this;
					++(*this);
					return copy;
				}
				bool operator==(const component_array_accessor& other) const { return *m_type == *other.m_type; }
				bool operator!=(const component_array_accessor& other) const { return !(*this == other); }
				bool operator==(const end_iterator& other) const { return m_component_index == m_table_component_count; }
				bool operator!=(const end_iterator& other) const { return !(*this == other); }

				class iterator
				{
				protected:
					const table_comp_type_info& m_type;
					chunk_seq::iterator m_chunk_iter;
					chunk_seq::iterator m_chunk_end;
					chunk* m_chunk;
					uint32_t m_offset;

				public:
					iterator(chunk_seq::iterator chunk_iter, chunk_seq::iterator chunk_end, const table_comp_type_info& type) :
						m_chunk_iter(chunk_iter),
						m_chunk_end(chunk_end),
						m_chunk(*chunk_iter),
						m_offset(0),
						m_type(type)
					{
					}

					iterator& operator++()
					{
						m_offset++;
						if (m_offset == m_chunk->size())
						{
							m_chunk_iter++;
							if (m_chunk_iter == m_chunk_end)
							{
								m_chunk = nullptr;
								return *this;
							}
							m_chunk = *m_chunk_iter;
							m_offset = 0;
						}
						return *this;
					}

					iterator operator++(int)
					{
						iterator copy = *this;
						++(*this);
						return copy;
					}

					void* operator*()
					{
						return table::component_address(m_chunk, m_offset, m_type);
					}

					bool operator==(const iterator& other) const { return m_chunk == other.m_chunk && m_offset == other.m_offset; }
					bool operator!=(const iterator& other) const { return !operator==(other); }
					bool operator==(const end_iterator& other) const { return m_chunk_iter == m_chunk_end; }
					bool operator!=(const end_iterator& other) const { return !operator==(other); }
				};

				iterator begin() const
				{
					chunk_seq chunks = make_sequence_ref(m_table->m_chunks);
					return iterator(chunks.begin(), chunks.end(), *m_type);
				}

				end_iterator end() const
				{
					return end_iterator{};
				}
			};

			component_array_accessor begin() { return component_array_accessor(m_table, 0u); }
			end_iterator end() { return end_iterator{}; }

		};

		complete_raw_accessor get_raw_accessor()
		{
			return complete_raw_accessor(this);
		}
		//template<typename SeqParam>
		//raw_accessor get_raw_accessor(sequence_ref<const entity, SeqParam> entities)
		//{
		//	return raw_accessor(*this, entities);
		//}
		template<typename SeqParam = const entity*>
		class allocate_accessor : public raw_accessor
		{
			using entity_seq = sequence_ref<const entity, SeqParam>;
			entity_seq entities;
			vector<table_offset_t> entity_table_offsets;
		public:
			template<typename StorageKeyBuilder>
			allocate_accessor(table& in_table, entity_seq entities, StorageKeyBuilder builder) :
				raw_accessor(in_table,{}), entities(entities)
			{
				uint32_t count = entities.size();
				auto entity_iter = entities.begin();
				entity_table_offsets.reserve(count);
				for (uint32_t i = 0; i < count; i++, entity_iter++)
				{
					auto chunk_index_offset = m_table.allocate_entity();
					auto& [chunk_index, chunk_offset] = chunk_index_offset;
					chunk* chunk = m_table.m_chunks[chunk_index];
					chunk->entities()[chunk_offset] = *entity_iter;
					auto table_offset = m_table.table_offset(chunk_index_offset);
					entity_table_offsets.push_back(table_offset);
					builder(*entity_iter, storage_key{ m_table.m_table_index, table_offset });
				}
				raw_accessor::table_offsets = make_sequence_ref(entity_table_offsets);
			}
		};

		template<typename SeqParam, typename StorageKeyBuilder>
		auto get_allocate_accessor(sequence_ref<const entity, SeqParam> entities, StorageKeyBuilder builder)
		{
			static_assert(std::is_invocable_v<StorageKeyBuilder, entity, storage_key>);
			return allocate_accessor(*this, entities, builder);
		}

		class deallocate_accessor : public raw_accessor
		{
		public:
			using raw_accessor::raw_accessor;

			~deallocate_accessor()
			{
				for (auto& offset : table_offsets) m_table.deallocate_entity(offset);
			}


		};

		deallocate_accessor get_deallocate_accessor(sequence_ref<const table_offset_t> entities_table_offsets)
		{
			return deallocate_accessor(*this, entities_table_offsets);
		}

		//[[nodiscard]]
		//table_offset_t emplace(entity e, sorted_sequence_ref<const generic::constructor> constructors)
		//{
		//	auto [chunk_index, chunk_offset] = allocate_entity();
		//	chunk* chunk = m_chunks[chunk_index];
		//	chunk->entities()[chunk_offset] = e;
		//	int type_index = 0;
		//	for (auto& constructor : constructors)
		//	{
		//		if (constructor.type() != m_notnull_components[type_index])
		//		{
		//			type_index++;
		//			continue;
		//		}
		//		byte* data = component_address(chunk, chunk_offset, type_index);
		//		constructor(data);
		//		type_index++;
		//	}
		//	return table_offset({ chunk_index, chunk_offset });
		//}


		void delete_entity(table_offset_t table_offset)
		{
			auto index = chunk_index_offset(table_offset);
			m_free_indices.push(index);
			auto chunk = m_chunks[index.chunk_index];
			for (auto& type : m_notnull_components)
			{
				byte* data = component_address(chunk, index.chunk_offset, type);
				type.destructor(data, 1);
			}
		}

	private:
		//using Callable = std::function<void(entity, sequence_ref<void*>)>;
		template<typename Callable>
		void dynamic_for_each(sequence_ref<const uint32_t> component_indices, Callable func)
		{
			using params = typename function_traits<Callable>::args;
			constexpr bool query_entity = params::template contains<entity>;
			constexpr bool query_storage_key = params::template contains<storage_key>;
			vector<void*> addresses(component_indices.size());
			for (uint32_t chunk_index = 0; chunk_index < m_chunks.size(); chunk_index++)
			{
				auto chunk = m_chunks[chunk_index];
				for (uint32_t chunk_offset = 0; chunk_offset < chunk->size(); chunk_offset++)
				{
					for (uint32_t i : component_indices)
					{
						auto& type = m_notnull_components[i];
						byte* data = component_address(chunk, chunk_offset, type.offset(), type.size());
						addresses[i] = data;
					}
					if constexpr (query_entity)
					{
						const auto& e = chunk->entities()[chunk_offset];
						if constexpr (query_storage_key)
						{
							storage_key key(m_table_index, table_offset({ chunk_index, chunk_offset }));
							func(e, key, addresses);
						}
						else
						{
							func(e, addresses);
						}
					}
					else
					{
						if constexpr (query_storage_key)
						{
							storage_key key(m_table_index, table_offset({ chunk_index, chunk_offset }));
							func(addresses, key);
						}
						else
						{
							func(addresses);
						}
					}
				}
			}
		}

	public:
		void dynamic_for_each(sequence_ref<const uint32_t> component_indices,
			std::function<void(entity, sequence_ref<void*>)> func)
		{
			dynamic_for_each(component_indices, func);
		}

		void dynamic_for_each(sequence_ref<const uint32_t> component_indices,
			std::function<void(sequence_ref<void*>)> func)
		{
			dynamic_for_each(component_indices, func);
		}

		void dynamic_for_each(sequence_ref<const uint32_t> component_indices,
			std::function<void(entity, storage_key, sequence_ref<void*>)> func)
		{
			dynamic_for_each(component_indices, func);
		}

		void dynamic_for_each(sequence_ref<const uint32_t> component_indices,
			std::function<void(storage_key, sequence_ref<void*>)> func)
		{
			dynamic_for_each(component_indices, func);
		}

		void get_components_offsets(
			sorted_sequence_ref<component_type_index> components,
			sequence_ref<uint32_t> _out)
		{
			auto target_comp_iter = components.begin();
			auto matching_comp_iter = m_notnull_components.begin();
			auto out_iter = _out.begin();
			while (target_comp_iter != components.end())
			{
				auto& target_comp = *target_comp_iter;
				auto& matching_comp = *matching_comp_iter;
				if (target_comp == matching_comp)
				{
					*out_iter = matching_comp.offset();
					out_iter++;
					target_comp_iter++;
					matching_comp_iter++;
				}
				else if (target_comp < matching_comp)
				{
					target_comp_iter++;
				}
				else //if(target_comp > matching_comp)
				{
					matching_comp_iter++;
				}
			}

		}

		class entity_accessor
		{
			using chunk_seq = sequence_ref<const chunk* const>;
		protected:
			chunk_seq m_chunks;

		public:
			using value_type = entity;
			entity_accessor(const table* table) : m_chunks(make_sequence_ref(table->m_chunks)) {}

			class iterator
			{
			protected:
				chunk_seq::iterator m_chunk_iter;
				chunk_seq::iterator m_chunk_end;
				const chunk* m_chunk;
				uint32_t m_offset;

			public:
				iterator(chunk_seq::iterator chunk_iter, chunk_seq::iterator chunk_end) :
					m_chunk_iter(chunk_iter),
					m_chunk_end(chunk_end),
					m_chunk(*chunk_iter),
					m_offset(0)
				{
				}

				iterator& operator++()
				{
					m_offset++;
					if (m_offset == m_chunk->size())
					{
						m_chunk_iter++;
						if (m_chunk_iter == m_chunk_end)
						{
							m_chunk = nullptr;
							return *this;
						}
						m_chunk = *m_chunk_iter;
						m_offset = 0;
					}
					return *this;
				}

				iterator operator++(int)
				{
					iterator copy = *this;
					++(*this);
					return copy;
				}

				entity operator*()
				{
					return m_chunk->entities()[m_offset];
				}


				bool operator==(const iterator& other) const { return m_chunk == other.m_chunk && m_offset == other.m_offset; }
				bool operator!=(const iterator& other) const { return !operator==(other); }
				bool operator==(const end_iterator& other) const { return m_chunk_iter == m_chunk_end; }
				bool operator!=(const end_iterator& other) const { return !operator==(other); }
			};

			iterator begin() const
			{
				return iterator(m_chunks.begin(), m_chunks.end());
			}

			end_iterator end() const
			{
				return end_iterator{};
			}

			size_t size() const
			{
				size_t size = 0;
				for (auto& chunk : m_chunks)
				{
					size += chunk->size();
				}
				return size;
			}
		};

		class table_offset_accessor : public entity_accessor
		{
			const table* m_table;
		public:
			table_offset_accessor(const table* table) : entity_accessor(table), m_table(table) {}
			class iterator : public entity_accessor::iterator
			{
				const table* m_table;
			public:
				iterator(chunk_seq::iterator chunk_iter, chunk_seq::iterator chunk_end, const table* table) :
					entity_accessor::iterator(chunk_iter, chunk_end), m_table(table)
				{
				}

				table_offset_t operator*()
				{
					return m_table->table_offset({ (uint32_t)(m_chunk_iter - m_table->m_chunks.data()), m_offset });
				}
			};
			iterator begin() const
			{
				return iterator(m_chunks.begin(), m_chunks.end(), m_table);
			}
			end_iterator end() const
			{
				return end_iterator{};
			}

		};


		entity_accessor get_entities()
		{
			return entity_accessor(this);
		}

		table_offset_accessor get_table_offsets()
		{
			return table_offset_accessor(this);
		}

	};
}