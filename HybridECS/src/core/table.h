#pragma once
#include "../lib/Lib.h"
#include "entity.h"

namespace hyecs
{
	static struct ChunkConfig
	{
		static const size_t DEFAULT_CHUNK_SIZE = 128 * 1024;//128kb
	};


	//template<typename Allocator = std::allocator<uint8_t>>
	class table
	{
		using Allocator = std::allocator<uint8_t>;
		using byte = uint8_t;

		class chunk
		{
		private:
			std::array<byte, ChunkConfig::DEFAULT_CHUNK_SIZE> m_data;
			size_t m_size = 0;
		public:
			size_t size() const { return m_size; }
			byte* data() { return m_data.data(); }

			void increase_size(size_t size) { m_size += size; }
			void decrease_size(size_t size) { m_size -= size; }
		};

		using chunk_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<chunk>;

		[[no_unique_address]] chunk_allocator m_allocator;
		archetype m_notnull_components;
		//todo add allocator for vec?
		vector<component_type_index> m_types;//cache for component types
		vector<chunk*> m_chunks;
		vector<uint32_t> m_offsets;
		size_t m_chunk_capacity;
		struct entity_table_index
		{
			uint32_t chunk_index;
			uint32_t chunk_offset;
		};
		//temporary hole in the table for batch adding removing entities
		stack<entity_table_index> m_free_indices;

	public:

		table(std::initializer_list<component_type_index> components)
			: m_notnull_components(components),
			m_offsets(components.size()),
			m_types(components)
		{
			//size computation
			//for (auto& type : m_types)
			//	type.size = std::bit_ceil(type.size);

			size_t column_size = 0;
			for (auto& type : m_types)
				column_size += type.size();

			size_t offset = 0;
			for (auto& type : m_types)
			{
				m_offsets.push_back(offset);
				offset += type.size();
			}

			m_chunk_capacity = ChunkConfig::DEFAULT_CHUNK_SIZE / column_size;


		}
		~table()
		{
			for (auto& chunk : m_chunks)
			{
				for (int component_index = 0; component_index < m_types.size(); component_index++)
				{
					auto& type = m_types[component_index];
					auto& offset = m_offsets[component_index];
					type.destructor(chunk->data() + offset, chunk->size());
				}
				m_allocator.deallocate(chunk, 1);
			}
		}

		table(const table& other) = delete;
		table(table&& other) = delete;


	private:


		chunk* allocate_chunk()
		{
			chunk* new_chunk = m_allocator.allocate(1);
			m_chunks.push_back(new_chunk);
			uint32_t chunk_index = m_chunks.size() - 1;

			return new_chunk;
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

		void deallocate_entity(uint32_t table_offset)
		{
			deallocate_entity(chunk_index_offset(table_offset));
		}

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
				for (int i = 0; i < m_types.size(); i++)
				{
					auto& type = m_types[i];
					auto& offset = m_offsets[i];
					byte* last_data = component_address(last_chunk, last_chunk_offset, offset, type.size());
					byte* data = component_address(m_chunks[chunk_index], chunk_offset, offset, type.size());
					type.move_constructor(data, last_data);
				}
			}
		}

		byte* component_address(chunk* chunk, uint32_t chunk_offset, uint32_t comp_offset, uint32_t comp_size)
		{
			return chunk->data() + comp_offset + chunk_offset * comp_size;
		}
		byte* component_address(const entity_table_index& index, uint32_t comp_offset, uint32_t comp_size)
		{
			return component_address(m_chunks[index.chunk_index], index.chunk_offset, comp_offset, comp_size);
		}

		byte* component_address(const entity_table_index& index, const uint32_t& comp_index)
		{
			return component_address(m_chunks[index.chunk_index], index.chunk_offset, m_offsets[comp_index], m_types[comp_index].size());
		}

		byte* component_address(chunk* chunk, uint32_t chunk_offset, const uint32_t& comp_index)
		{
			return component_address(chunk, chunk_offset, m_offsets[comp_index], m_types[comp_index].size());
		}




		constexpr entity_table_index chunk_index_offset(uint32_t table_offset)
		{
			uint32_t chunk_index = table_offset / m_chunk_capacity;
			uint32_t chunk_offset = table_offset % m_chunk_capacity;
			return { chunk_index, chunk_offset };
		}

		void all_components(uint32_t table_offset)
		{
			auto [chunk_index, chunk_offset] = chunk_index_offset(table_offset);
			chunk* chunk = m_chunks[chunk_index];
			for (int i = 0; i < m_types.size(); i++)
			{
				byte* data = component_address(chunk, chunk_offset, i);
			}
		}



		void components_address(
			uint32_t table_offset,
			const archetype& component_set,
			sequence_ref<void*> addresses)
		{
			assert(m_notnull_components.contains(component_set));
			auto [chunk_index, chunk_offset] = chunk_index_offset(table_offset);
			chunk* chunk = m_chunks[chunk_index];
			int index = 0;
			auto addr_iter = addresses.begin();
			for (auto& component : component_set)
			{
				if (component != m_notnull_components[index])
				{
					index++;
					continue;
				}
				byte* data = component_address(chunk, chunk_offset, index);
				assert(addr_iter != addresses.end());
				*addr_iter = data;
				addr_iter++;
				index++;
			}
		}

		void components_address(
			uint32_t table_offset,
			const archetype& component_set,
			sequence_ref<void*> match_addresses,
			sequence_ref<void*> unmatch_addresses)
		{
			assert(m_notnull_components.contains(component_set));
			auto [chunk_index, chunk_offset] = chunk_index_offset(table_offset);
			chunk* chunk = m_chunks[chunk_index];
			int index = 0;
			auto addr_iter = match_addresses.begin();
			auto unmatch_addr_iter = unmatch_addresses.begin();
			for (auto& component : component_set)
			{

				byte* data = component_address(chunk, chunk_offset, index);

				if (component != m_notnull_components[index])
				{
					assert(unmatch_addr_iter != unmatch_addresses.end());
					*unmatch_addr_iter = data;
					unmatch_addr_iter++;
				}
				else
				{
					assert(addr_iter != match_addresses.end());
					*addr_iter = data;
					addr_iter++;
				}
				index++;
			}
		}

	public:

		//template<typename SeqIter = uint32_t*>

		class end_iterator {};

		using SeqIter = uint32_t*;
		class raw_accessor
		{
		protected:
			table& m_table;
			sequence_ref<uint32_t, SeqIter> table_offsets;
		public:

			raw_accessor(table& in_table)
				: m_table(in_table)
			{
			}

			raw_accessor(table& in_table, sequence_ref<uint32_t, SeqIter> offsets)
				: m_table(in_table), table_offsets(offsets)
			{
			}


			class component_array_accessor
			{
				raw_accessor* m_accessor;
				component_type_index m_type;
				uint32_t m_component_index;

			public:

				component_array_accessor(uint32_t index, raw_accessor* accessor)
					: m_type(accessor->m_table.m_types[index]), m_component_index(index), m_accessor(accessor)
				{
				}

				component_type_index component_type() const { return m_type; }

				void operator++()
				{
					m_component_index++;
					m_type = m_accessor->m_table.m_types[m_component_index];
				}

				component_array_accessor& operator++(int)
				{
					component_array_accessor copy = *this;
					++(*this);
					return copy;
				}

				auto comparable() const
				{
					return m_type;
				}

				bool operator==(const component_array_accessor& other) const
				{
					return m_type == other.m_type;
				}

				bool operator!=(const component_array_accessor& other) const
				{
					return !(*this == other);
				}

				bool operator==(const end_iterator& other) const
				{
					return m_component_index == m_accessor->m_table.m_types.size();
				}

				bool operator!=(const end_iterator& other) const
				{
					return !(*this == other);
				}


				class iterator
				{
					uint32_t m_comp_offset;
					uint32_t m_type_size;
					raw_accessor* m_accessor;
					typename sequence_ref<uint32_t, SeqIter>::iterator m_offset_iter;

					table& table() const { return m_accessor->m_table; }
					const sequence_ref<uint32_t>& sequence() const { return m_accessor->table_offsets; }
				public:
					iterator(uint32_t component_index,
						raw_accessor* accessor,
						typename sequence_ref<uint32_t, SeqIter>::iterator offset_iter)
						:
						m_comp_offset(accessor->m_table.m_offsets[component_index]),
						m_type_size(accessor->m_table.m_types[component_index].size()),
						m_accessor(accessor),
						m_offset_iter(offset_iter)
					{
					}

					void operator++()
					{
						m_offset_iter++;
					}

					iterator& operator++(int)
					{
						iterator copy = *this;
						++(*this);
						return copy;
					}

					bool operator==(const iterator& other) const
					{
						return m_offset_iter == other.m_offset_iter;
					}

					bool operator!=(const iterator& other) const
					{
						return !(*this == other);
					}

					bool operator==(const end_iterator& other) const
					{
						return m_offset_iter == m_accessor->table_offsets.end();
					}

					bool operator!=(const end_iterator& other) const
					{
						return !(*this == other);
					}

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

		class allocate_accessor : public raw_accessor
		{
			vector<entity_table_index> table_offsets;
		public:
			allocate_accessor(table& in_table, int count) : raw_accessor(in_table)
			{
				table_offsets.reserve(count);
				for (int i = 0; i < count; i++)
					table_offsets.push_back(m_table.allocate_entity());
			}
		};
		template<typename SeqIter>
		raw_accessor get_raw_accessor(sequence_ref<uint32_t, SeqIter> entities_table_offsets)
		{
			return raw_accessor(*this, entities_table_offsets);
		}
		template<typename SeqIter>
		raw_accessor get_raw_accessor(sequence_ref<entity, SeqIter> entities)
		{
			return raw_accessor(*this, entities);
		}
		template<typename SeqIter>
		allocate_accessor get_allocate_accessor(sequence_ref<entity, SeqIter> entities)
		{
			return allocate_accessor(*this, count);
		}



		//void entity_table_move(
		//	table& src, entity_table_index src_index,
		//	sequence_ref<void*> unmatch_addresses)
		//{
		//	auto [chunk_index, chunk_offset] = allocate_entity();
		//	chunk* dest_chunk = m_chunks[chunk_index];
		//	chunk* src_chunk = src.m_chunks[src_index.chunk_index];
		//	auto unmatch_addr_iter = unmatch_addresses.begin();
		//	for (int i = 0; i < m_types.size(); i++)
		//	{
		//		auto& type = m_types[i];

		//		for (int j = 0; j < unmatch_addresses.size(); j++)
		//		{
		//			auto& src_type = src.m_types[j];
		//			if (type != src_type)
		//			{
		//				assert(unmatch_addr_iter != unmatch_addresses.end());
		//				...
		//			}
		//			auto& src_offset = src.m_offsets[j];
		//			auto& dest_offset = m_offsets[i];
		//			byte* src_data = component_address(src_chunk, src_index.chunk_offset, src_offset, type.size());
		//			byte* data = component_address(dest_chunk, chunk_offset, dest_offset, type.size());
		//			type.move_constructor(data, src_data);
		//			...
		//		}
		//	}
		//	src.m_free_indices.push(src_index);
		//}

		void emplace(sorted_sequence_ref<generic::constructor> constructors)
		{
			auto [chunk_index, chunk_offset] = allocate_entity();
			chunk* chunk = m_chunks[chunk_index];
			int type_index = 0;
			for (auto& constructor : constructors)
			{
				if (constructor.type() != m_notnull_components[type_index].type_index())
				{
					type_index++;
					continue;
				}
				byte* data = component_address(chunk, chunk_offset, type_index);
				constructor(data);
				type_index++;
			}
		}

		template<typename CallableConstructor>
		void push_entity(std::initializer_list<CallableConstructor> params)
		{
			assert(params.size() == m_types.size());

			auto [chunk_index, entity_chunk_index] = allocate_entity();
			chunk* chunk = m_chunks[chunk_index];


			for (int i = 0; i < m_types.size(); i++)
			{
				auto& param = params[i];
				byte* data = component_address(chunk, entity_chunk_index, i);
				params[i](data);
			}
		}



		void delete_entity(uint32_t table_offset)
		{
			auto index = chunk_index_offset(table_offset);
			m_free_indices.push(index);
			auto chunk = m_chunks[index.chunk_index];
			for (int i = 0; i < m_types.size(); i++)
			{
				auto& type = m_types[i];
				auto& offset = m_offsets[i];
				byte* data = component_address(chunk, index.chunk_offset, offset, type.size());
				type.destructor(data, 1);
			}
		}





	};
}