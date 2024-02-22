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
			size_t size() { return m_size; }
			byte* data() { return m_data.data(); }


		};

		using chunk_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<chunk>;

		[[no_unique_address]] chunk_allocator m_allocator;
		archetype m_notnull_components;
		vector<generic::type_info, Allocator> m_types;
		vector<chunk*, Allocator> m_chunks;
		vector<uint32_t, Allocator> m_offsets;
		size_t m_chunk_capacity;
		struct entity_table_index
		{
			uint32_t chunk_index;
			uint32_t chunk_offset;
		};
		stack<entity_table_index> m_free_indices;

	public:

		table(std::initializer_list<component_type_index> components)
			: m_notnull_components(components), m_offsets(components.size())
		{
			for (auto& component : components)
				m_types.push_back(generic::type_info{
					.size = component.size(),
					.copy_constructor = nullptr,
					.move_constructor = component.move_constructor(),
					.destructor = component.destructor()
					});
			//size computation
			for (auto& type : m_types)
				type.size = std::bit_ceil(type.size);

			size_t column_size = 0;
			for (auto& type : m_types)
				column_size += type.size;

			size_t offset = 0;
			for (auto& type : m_types)
			{
				m_offsets.push_back(offset);
				offset += type.size;
			}

			m_chunk_capacity = ChunkConfig::DEFAULT_CHUNK_SIZE / column_size;


		}
		~table()
		{

		}

		table(const table& other) = delete;
		table(table&& other) = delete;


	private:


		chunk* allocate_chunk()
		{
			chunk* new_chunk = m_allocator.allocate(1);
			m_chunks.push_back(new_chunk);
			uint32_t chunk_index = m_chunks.size() - 1;
			for (int i = m_chunk_capacity - 1; i >= 0; i--)
				m_free_indices.emplace(chunk_index, i);

			return new_chunk;
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
				auto& type = m_types[i];
				auto& offset = m_offsets[i];
				byte* data = chunk->data() + offset + chunk_offset * type.size;
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

				auto& type = m_types[index];
				auto& offset = m_offsets[index];
				byte* data = chunk->data() + offset + chunk_offset * type.size;
				assert(addr_iter != addresses.end());
				*addr_iter = data;
				addr_iter++;
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

				auto& type = m_types[index];
				auto& offset = m_offsets[index];
				byte* data = chunk->data() + offset + chunk_offset * type.size;
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
				auto& type = m_types[index];
				auto& offset = m_offsets[index];
				byte* data = chunk->data() + offset + chunk_offset * type.size;

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
			if (m_free_indices.empty())
				allocate_chunk();

			uint32_t index = m_free_indices.top();
			m_free_indices.pop();

			auto [chunk_index, entity_chunk_index] = chunk_index_offset(index);
			chunk* chunk = m_chunks[chunk_index];


			for (int i = 0; i < m_types.size(); i++)
			{
				auto& type = m_types[i];
				auto& param = params[i];
				auto& offset = m_offsets[i];
				byte* data = chunk->data() + offset + entity_chunk_index * type.size;
				params[i](data);
			}
		}


	};
}