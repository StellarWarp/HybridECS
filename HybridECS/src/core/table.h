#pragma once
#include "../lib/Lib.h"
#include "archetype_registry.h"

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
		vector<component_type_index, Allocator> m_types;//cache for component types
		vector<chunk*, Allocator> m_chunks;
		vector<uint32_t, Allocator> m_offsets;
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

		byte* component_address(chunk* chunk, uint32_t chunk_offset, uint32_t comp_offset, uint32_t comp_size)
		{
			return chunk->data() + comp_offset + chunk_offset * comp_size;
		}

		byte* component_address(const entity_table_index& index, const uint32_t& comp_index)
		{
			return component_address(m_chunks[index.chunk_index], index.chunk_offset, m_offsets[comp_index], m_types[comp_index].size());
		}

		byte* component_address(chunk* chunk, uint32_t chunk_offset, const uint32_t& comp_index)
		{
			return component_address(chunk, chunk_offset, m_offsets[comp_index], m_types[comp_index].size());
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
			array_ref<void*> addresses)
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
			array_ref<void*> match_addresses,
			array_ref<void*> unmatch_addresses)
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

		template<typename CallableConstructor>
		void push_entity(std::initializer_list<CallableConstructor> params)
		{
			assert(params.size() == m_types.size());

			auto [chunk_index, entity_chunk_index] = allocate_entity(index);
			chunk* chunk = m_chunks[chunk_index];


			for (int i = 0; i < m_types.size(); i++)
			{
				auto& param = params[i];
				byte* data = component_address(chunk, entity_chunk_index, i);
				params[i](data);
			}
		}

		void remove_entity(uint32_t table_offset)
		{
			auto [chunk_index, chunk_offset] = chunk_index_offset(table_offset);
			m_free_indices.push({ chunk_index, chunk_offset });
		}

		void delete_entity(uint32_t table_offset)
		{
			auto index = chunk_index_offset(table_offset);
			m_free_indices.push(index);
			auto chunk = m_chunks[index.chunk_index];
			for(int i=0;i<m_types.size();i++)
			{
				auto& type = m_types[i];
				auto& offset = m_offsets[i];
				byte* data = component_address(chunk, index.chunk_offset, offset, type.size());
				type.destructor(data, 1);
			}
		}





	};
}