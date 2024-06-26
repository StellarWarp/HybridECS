#pragma once

#include "sequence_ref.h"

namespace hyecs
{

	class raw_segmented_vector
	{
	public:
		using index_t = uint32_t;

	private:

		static constexpr size_t default_chunk_byte_capacity = 1024 * 2;

		struct segment_location
		{
			uint32_t chunk_index;
			uint32_t chunk_offset;
		};



		struct chunk
		{
			uint8_t data[default_chunk_byte_capacity];
			uint32_t element_count;
		};
		vector<chunk*> m_chunks;
		stack<std::pair<chunk*, uint32_t>> m_free_chunks;
		uint32_t m_type_size;
		uint32_t chunk_element_capacity;
		uint32_t element_count;

		uint32_t m_chunk_offset_bits;
		uint32_t m_chunk_index_mask;
		uint32_t m_chunk_offset_mask;


	public:
		raw_segmented_vector(size_t type_size) noexcept
			: m_type_size(type_size), element_count(0)

		{
			chunk_element_capacity = default_chunk_byte_capacity / type_size;
			m_chunk_offset_bits = std::bit_width(chunk_element_capacity - 1);
			m_chunk_offset_mask = (1 << m_chunk_offset_bits) - 1;
			m_chunk_index_mask = ~m_chunk_offset_mask;
		}



	private:
		chunk* allocate_chunk()
		{
			return new chunk();
		}
		chunk* deallocate_chunk(chunk* ptr)
		{
			delete ptr;
		}




		segment_location chunk_index_offset(const index_t& vector_offset) const
		{
			//high bits
			uint32_t chunk_index = (vector_offset & m_chunk_index_mask) >> m_chunk_offset_bits;
			//low bits
			uint32_t chunk_offset = vector_offset & m_chunk_offset_mask;
			return { chunk_index, chunk_offset };
		}

		index_t table_offset(const segment_location& index) const
		{
			return static_cast<index_t>(index.chunk_index << m_chunk_offset_bits | index.chunk_offset);
		}




	public:

		size_t capacity() const noexcept
		{
			return m_chunks.size() * chunk_element_capacity;
		}

		size_t size() const noexcept
		{
			return element_count;
		}

		size_t type_size() const noexcept
		{
			return m_type_size;
		}

		void* at(const size_t& index) noexcept
		{
			auto [chunk_index, chunk_offset] = chunk_index_offset(index);
			return m_chunks[chunk_index]->data + chunk_offset * m_type_size;
		}

		const void* at(const size_t& index) const noexcept
		{
			auto [chunk_index, chunk_offset] = chunk_index_offset(index);
			return m_chunks[chunk_index]->data + chunk_offset * m_type_size;
		}

		void* operator[](const size_t& index) noexcept
		{
			return at(index);
		}

		const void* operator[](const size_t& index) const noexcept
		{
			return at(index);
		}


	public:

		std::pair<void*, index_t> allocate_value()
		{
			chunk* chunk;
			uint32_t chunk_offset;
			uint32_t chunk_index;
			void* ptr;

			if (!m_free_chunks.empty())
			{
				auto& info = m_free_chunks.top();
				chunk = info.first;
				chunk_index = info.second;
				chunk_offset = chunk->element_count;
				ptr = chunk->data + chunk_offset * m_type_size;
				chunk->element_count++;
				if (chunk->element_count == chunk_element_capacity)
					m_free_chunks.pop();
			}
			else
			{
				m_chunks.push_back(allocate_chunk());
				chunk = m_chunks.back();
				chunk_offset = 0;
				chunk_index = m_chunks.size() - 1;
				ptr = chunk->data;
				m_free_chunks.push({ chunk, chunk_index });
				chunk->element_count++;

			}

			element_count++;
			return { ptr, table_offset({ chunk_index, chunk_offset }) };
		}

		[[nodiscard]] bool deallocate_value(void* ptr, index_t index)
		{
			auto [chunk_index, chunk_offset] = chunk_index_offset(index);
			assert(chunk_index < m_chunks.size());
			assert(chunk_offset < chunk_element_capacity);
			chunk* chunk = m_chunks[chunk_index];
			if (chunk->element_count == chunk_element_capacity)
				m_free_chunks.push({ chunk, chunk_index });
			bool swap_element = chunk_offset != chunk->element_count - 1;
			if (swap_element)
			{
				uint32_t last_element_offset = (chunk->element_count - 1) * m_type_size;
				void* last_element_ptr = chunk->data + last_element_offset;
				std::memcpy(ptr, last_element_ptr, m_type_size);
			}
			
			chunk->element_count--;
			element_count--;
			return swap_element;
		}



		using end_iterator = nullptr_t;
		class iterator
		{
			using iterator_category = std::random_access_iterator_tag;
			using value_type = void;
			using difference_type = std::ptrdiff_t;
			using pointer = void*;

			sequence_ref<chunk*> m_chunks;
			size_t m_type_size;
			uint32_t m_chunk_index;
			uint32_t m_chunk_offset;


		public:

			iterator(sequence_ref<chunk*> chunks, size_t type_size) noexcept
				: m_chunks(chunks), m_type_size(type_size), m_chunk_index(0), m_chunk_offset(0)
			{
			}

			iterator& operator++()
			{
				m_chunk_offset++;
				if (m_chunk_offset == m_chunks[m_chunk_index]->element_count)
				{
					m_chunk_index++;
					m_chunk_offset = 0;
				}
				return *this;
			}

			iterator operator++(int)
			{
				iterator temp = *this;
				++(*this);
				return temp;
			}

			iterator& operator--()
			{
				if (m_chunk_offset == 0)
				{
					m_chunk_index--;
					m_chunk_offset = m_chunks[m_chunk_index]->element_count;
				}
				m_chunk_offset--;
				return *this;
			}

			iterator operator--(int)
			{
				iterator temp = *this;
				--(*this);
				return temp;
			}

			void* operator*() const
			{
				return m_chunks[m_chunk_index]->data + m_chunk_offset * m_type_size;
			}

			bool operator==(const iterator& other) const
			{
				assert(m_chunks.begin() == other.m_chunks.begin() && m_chunks.end() == other.m_chunks.end());
				return m_chunk_index == other.m_chunk_index && m_chunk_offset == other.m_chunk_offset;
			}

			bool operator!=(const iterator& other) const
			{
				return !(*this == other);
			}

			bool operator==(end_iterator) const
			{
				return m_chunk_index == m_chunks.size();
			}

			bool operator!=(end_iterator) const
			{
				return m_chunk_index != m_chunks.size();
			}
		};

		iterator begin() noexcept
		{
			return iterator(m_chunks, m_type_size);
		}

		end_iterator end() noexcept
		{
			return nullptr;
		}




	};
}
