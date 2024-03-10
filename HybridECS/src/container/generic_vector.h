#include "generic_type.h"
#include <memory>

namespace hyecs
{

	class generic_vector
	{
	private:
		uint8_t* m_begin;
		uint8_t* m_end;
		uint8_t* m_capacity_end;
		generic::type_index_container_cached m_type;

	public:
		generic_vector(generic::type_index type_info, size_t inital_capacity = 4) noexcept
			: m_type(type_info)
		{
			size_t capacity_byte_size = m_type.size() * inital_capacity;
			m_begin = new uint8_t[capacity_byte_size];
			m_end = m_begin;
			m_capacity_end = m_end;
		}

		~generic_vector()
		{
			clear();
			delete[] m_begin;
		}

		generic_vector(const generic_vector& other) noexcept
			: m_type(other.m_type),
			m_begin(new uint8_t[other.byte_capacity()]),
			m_end(m_begin + other.byte_size()),
			m_capacity_end(m_begin + other.byte_capacity())
		{
			std::memcpy(m_begin, other.m_begin, other.byte_size());
		}

		generic_vector(generic_vector&& other) noexcept
			: m_type(other.m_type),
			m_begin(other.m_begin),
			m_end(other.m_end),
			m_capacity_end(other.m_capacity_end)
		{
			other.m_begin = nullptr;
			other.m_end = nullptr;
			other.m_capacity_end = nullptr;
		}

		generic_vector& operator=(const generic_vector& other) noexcept
		{
			assert(this != &other);
			if (this->byte_capacity() < other.byte_size())
			{
				delete[] m_begin;
				m_type = other.m_type;
				m_begin = new uint8_t[other.byte_capacity()];
				m_end = m_begin + other.byte_size();
				m_capacity_end = m_begin + other.byte_capacity();
			}
			std::memcpy(m_begin, other.m_begin, other.byte_size());

		}

		generic_vector& operator=(generic_vector&& other) noexcept
		{
			assert(this != &other);
			m_begin = other.m_begin;
			m_end = other.m_end;
			m_capacity_end = other.m_capacity_end;
			m_type = other.m_type;
			other.m_begin = nullptr;
			other.m_end = nullptr;
			other.m_capacity_end = nullptr;
		}

		constexpr void* data() noexcept
		{
			return m_begin;
		}

		constexpr const void* data() const noexcept
		{
			return m_begin;
		}

		void resize(size_t new_size) noexcept
		{
			size_t new_capacity_byte_size = m_type.size() * new_size;
			size_t copy_btye_size = std::min(new_capacity_byte_size, byte_size());
			uint8_t* new_data = new uint8_t[new_capacity_byte_size];
			std::memcpy(new_data, m_begin, copy_btye_size);
			delete[] m_begin;
			m_begin = new_data;
			m_end = m_begin + copy_btye_size;
			m_capacity_end = m_begin + new_capacity_byte_size;
		}

		constexpr size_t capacity() const noexcept
		{
			return (m_capacity_end - m_begin) / m_type.size();
		}

		constexpr size_t size() const noexcept
		{
			return (m_end - m_begin) / m_type.size();
		}

		constexpr size_t type_size() const noexcept
		{
			return m_type.size();
		}

		constexpr size_t byte_size() const noexcept
		{
			return m_capacity_end - m_begin;
		}

		constexpr size_t byte_capacity() const noexcept
		{
			return m_capacity_end - m_begin;
		}

		void clear()
		{
			m_type.destructor(m_begin, m_end);
			m_end = m_begin;
		}

		void* at(const size_t& index) noexcept
		{
			assert(index < size());
			return m_begin + index * m_type.size();
		}

		const void* at(const size_t& index) const noexcept
		{
			assert(index < size());
			return m_begin + index * m_type.size();
		}

		void* operator[](const size_t& index) noexcept
		{
			assert(index < size());
			return m_begin + index * m_type.size();
		}

		const void* operator[](const size_t& index) const noexcept
		{
			assert(index < size());
			return m_begin + index * m_type.size();
		}

	private:
		void capacity_extend()
		{
			resize(size() * 2);
		}
	public:

		template<typename T>
		void push_back(const T& value)
		{
			assert(typeid(T).hash_code() == m_type.hash());
			if (m_end == m_capacity_end)
				capacity_extend();
			*reinterpret_cast<T*>(m_end) = std::move(value);
			m_end += m_type.size();
		}

		template<typename T>
		void push_back(T&& value)
		{
			assert(typeid(T).hash_code() == m_type.hash());
			if (m_end == m_capacity_end)
				capacity_extend();
			*reinterpret_cast<T*>(m_end) = std::move(value);
			m_end += m_type.size();
		}

		template<typename T, typename... Args>
		void emplace_back(Args&&... args)
		{
			assert(typeid(T).hash_code() == m_type.hash());
			if (m_end == m_capacity_end)
				capacity_extend();
			new(m_end) T(std::forward<Args>(args)...);
			m_end += m_type.size();
		}

		void pop_back()
		{
			assert(m_end > m_begin);
			m_end -= m_type.size();
			m_type.destructor(m_end);
		}


		class iterator
		{
			using iterator_category = std::random_access_iterator_tag;
			using value_type = void;
			using difference_type = std::ptrdiff_t;
			using pointer = void*;

			uint8_t* m_ptr;
			size_t m_type_size;

		public:

			iterator(pointer ptr) : m_ptr((uint8_t*)ptr) {}

			iterator& operator++()
			{
				m_ptr += m_type_size;
				return *this;
			}

			iterator operator++(int)
			{
				iterator temp = *this;
				m_ptr += m_type_size;
				return temp;
			}

			iterator& operator--()
			{
				m_ptr -= m_type_size;
				return *this;
			}

			iterator operator--(int)
			{
				iterator temp = *this;
				m_ptr -= m_type_size;
				return temp;
			}

			iterator& operator+=(difference_type n)
			{
				m_ptr += n * m_type_size;
				return *this;
			}

			iterator& operator-=(difference_type n)
			{
				m_ptr -= n * m_type_size;
				return *this;
			}

			iterator operator+(difference_type n) const
			{
				return iterator(m_ptr + n * m_type_size);
			}

			iterator operator-(difference_type n) const
			{
				return iterator(m_ptr - n * m_type_size);
			}

			difference_type operator-(const iterator& other) const
			{
				return (m_ptr - other.m_ptr) / m_type_size;
			}

			pointer operator*() const
			{
				return m_ptr;
			}

			bool operator==(const iterator& other) const
			{
				return m_ptr == other.m_ptr;
			}

			bool operator!=(const iterator& other) const
			{
				return m_ptr != other.m_ptr;
			}

			bool operator<(const iterator& other) const
			{
				return m_ptr < other.m_ptr;
			}

			bool operator>(const iterator& other) const
			{
				return m_ptr > other.m_ptr;
			}

			bool operator<=(const iterator& other) const
			{
				return m_ptr <= other.m_ptr;
			}

			bool operator>=(const iterator& other) const
			{
				return m_ptr >= other.m_ptr;
			}
		};

		iterator begin() noexcept
		{
			return iterator(m_begin);
		}

		iterator end() noexcept
		{
			return iterator(m_end);
		}


		
	};
}
