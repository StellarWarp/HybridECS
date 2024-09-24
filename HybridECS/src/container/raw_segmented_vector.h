#pragma once

#include "sequence_ref.h"

namespace hyecs
{

    class raw_segmented_vector : non_copyable
    {
    public:
        using index_t = uint32_t;

    private:

        static constexpr size_t default_chunk_byte_capacity = 1024 * 2 - sizeof(size_t);

        struct segment_location
        {
            uint32_t chunk_index;
            uint32_t chunk_offset;
        };


        struct chunk
        {
            uint8_t data[default_chunk_byte_capacity];
            size_t element_count = 0;

            ASSERTION_CODE(chunk(){std::memset(data,0xFFFF'FFFF,default_chunk_byte_capacity);})
        };
        vector<chunk*> m_chunks;
        stack<std::pair<chunk*, uint32_t>> m_free_chunks;
        uint32_t m_type_size;
        generic::allocator m_allocator;
        uint32_t chunk_element_capacity;
        uint32_t element_count;

        uint32_t m_chunk_offset_bits;
        index_t m_chunk_index_mask;
        index_t m_chunk_offset_mask;


    public:
        raw_segmented_vector(size_t type_size, size_t alignment) noexcept
                : m_type_size(type_size), m_allocator(alignment), element_count(0)
        {
            chunk_element_capacity = default_chunk_byte_capacity / type_size;
            m_chunk_offset_bits = std::bit_width(chunk_element_capacity - 1);
            m_chunk_offset_mask = (1 << m_chunk_offset_bits) - 1;
            m_chunk_index_mask = ~m_chunk_offset_mask;
        }
        ~raw_segmented_vector()
        {
            for (auto& chunk : m_chunks)
            {
                deallocate_chunk(chunk);
            }
        }

        void clear()
        {
            this->~raw_segmented_vector();
            element_count = 0;
            while (!m_free_chunks.empty()) m_free_chunks.pop();
        }


    private:
        chunk* allocate_chunk()
        {
            return new(m_allocator.allocate(sizeof(chunk))) chunk();
        }

        void deallocate_chunk(chunk* ptr)
        {
            m_allocator.deallocate((std::byte*) ptr, sizeof(chunk));
        }


        segment_location chunk_index_offset(const index_t& vector_offset) const
        {
            //high bits
            uint32_t chunk_index = (vector_offset & m_chunk_index_mask) >> m_chunk_offset_bits;
            //low bits
            uint32_t chunk_offset = vector_offset & m_chunk_offset_mask;
            return {chunk_index, chunk_offset};
        }

        index_t table_offset(const segment_location& index) const
        {
            return static_cast<index_t>(index.chunk_index << m_chunk_offset_bits | index.chunk_offset);
        }

        void* at_(chunk* c, uint32_t offset) const noexcept
        {
            assert(offset < chunk_element_capacity);
            return c->data + offset * m_type_size;
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
            return at_(m_chunks[chunk_index], chunk_offset);
        }

        const void* at(const size_t& index) const noexcept
        {
            auto [chunk_index, chunk_offset] = chunk_index_offset(index);
            return at_(m_chunks[chunk_index], chunk_offset);
        }

        /// this container is not designed for consistent index access

//        void* operator[](const size_t& index) noexcept
//        {
//            return at(index);
//        }
//
//        const void* operator[](const size_t& index) const noexcept
//        {
//            return at(index);
//        }


    public:

        //the index is a internal allocate non-consistent index
        //note that value is not inited
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
                ptr = at_(chunk, chunk_offset);
                chunk->element_count++;
                if (chunk->element_count == chunk_element_capacity)
                    m_free_chunks.pop();
            } else
            {
                m_chunks.push_back(allocate_chunk());
                chunk = m_chunks.back();
                chunk_offset = 0;
                chunk_index = m_chunks.size() - 1;
                ptr = chunk->data;
                m_free_chunks.push({chunk, chunk_index});
                assert(chunk->element_count < chunk_element_capacity);
                chunk->element_count++;
            }

            element_count++;
            return {ptr, table_offset({chunk_index, chunk_offset})};
        }

        template<std::invocable<> OnSwapBack,
                typename Mover = std::nullptr_t,
                typename Deleter = std::nullptr_t,
                typename MovedDeleter = std::nullptr_t>
        void deallocate_value(
                void* ptr, index_t index,
                OnSwapBack&& on_swap_back,
                Mover&& mover = {},
                Deleter&& deleter = {},
                MovedDeleter&& moved_deleter = {}//default using deleter
        )
        {
            static constexpr bool has_mover = !std::is_same_v<Mover, std::nullptr_t>;
            static constexpr bool has_deleter = !std::is_same_v<Deleter, std::nullptr_t>;
            static constexpr bool has_moved_deleter = !std::is_same_v<MovedDeleter, std::nullptr_t>;
            auto [chunk_index, chunk_offset] = chunk_index_offset(index);
            assert(chunk_index < m_chunks.size());
            assert(chunk_offset < chunk_element_capacity);
            chunk* chunk = m_chunks[chunk_index];
            assert(at_(chunk, chunk_offset) == ptr);
            if constexpr (has_deleter) deleter(ptr);

            if (chunk->element_count == chunk_element_capacity)
                m_free_chunks.push({chunk, chunk_index});
            bool swap_element = chunk_offset != chunk->element_count - 1;
            if (swap_element)
            {
                void* last_element_ptr = at_(chunk, chunk->element_count - 1);
                if constexpr (std::same_as<std::nullptr_t, std::decay_t<decltype(mover)>>)
                    std::memcpy(ptr, last_element_ptr, m_type_size);
                else
                    mover(ptr, last_element_ptr);
                if constexpr (DESTROY_MOVED_COMPONENTS)
                {
                    if constexpr (has_moved_deleter)
                        moved_deleter(last_element_ptr);
                    else if constexpr (has_deleter)
                        deleter(last_element_ptr);
                }
                on_swap_back();
            }

            chunk->element_count--;
            element_count--;
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

            //todo chunk element count should not be 0
            iterator(sequence_ref<chunk*> chunks, size_t type_size) noexcept
                    : m_chunks(chunks), m_type_size(type_size), m_chunk_index(0), m_chunk_offset(0)
            {
                while (m_chunk_index < m_chunks.size() && m_chunks[m_chunk_index]->element_count == 0)
                {
                    m_chunk_index++;
                }
            }

            //todo and here
            iterator& operator++()
            {
                m_chunk_offset++;
                while (m_chunk_offset == m_chunks[m_chunk_index]->element_count)
                {
                    m_chunk_index++;
                    m_chunk_offset = 0;
                    if(m_chunk_index == m_chunks.size()) break;
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

            bool operator==(end_iterator) const
            {
                return m_chunk_index == m_chunks.size();
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
