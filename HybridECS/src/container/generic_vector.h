#pragma once

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
		generic_vector(generic::type_index type, size_t inital_capacity = 4) noexcept
			: m_type(type)
		{
			size_t capacity_byte_size = m_type.size() * inital_capacity;
			m_begin = new uint8_t[capacity_byte_size];
			m_end = m_begin;
			m_capacity_end = m_end + capacity_byte_size;
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

		generic::type_index type() const noexcept
		{
			return m_type;
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

		void* allocate_element()
		{
			if (m_end == m_capacity_end)
				capacity_extend();
			void* addr = m_end;
			m_end += m_type.size();
			return addr;
		}

		void deallocate_element(void* ptr)
		{
			assert(ptr >= m_begin && ptr < m_end);
			uint8_t* erase_ptr = (uint8_t*)ptr;
			uint8_t* last_ptr = m_end - m_type.size();
			m_type.move_constructor(erase_ptr, last_ptr);
			m_end = last_ptr;
		}

		template<typename T>
		void* push_back(const T& value)
		{
			assert(typeid(T).hash_code() == m_type.hash());
			T* addr = (T*)allocate_element();
			new(addr) T(std::forward<T>(value));
			return addr;
		}

		template<typename T>
		void* push_back(T&& value)
		{
			using type = std::remove_reference_t<T>;
			using pointer = type*;
			assert(typeid(type).hash_code() == m_type.hash());
			pointer addr = (pointer)allocate_element();
			new(addr) type(std::forward<T>(value));
			return addr;
		}

		template<typename T, typename... Args>
		void* emplace_back(Args&&... args)
		{
			assert(typeid(T).hash_code() == m_type.hash());
			T* addr = (T*)allocate_element();
			new(addr) T(std::forward<Args>(args)...);
			return addr;
		}

		void* emplace_back(generic::constructor constructor)
		{
			assert(constructor.type() == m_type);
			return constructor(allocate_element());
		}

		void pop_back()
		{
			assert(m_end > m_begin);
			m_end -= m_type.size();
			m_type.destructor(m_end);
		}

		void swap_back_erase(void* ptr)
		{
			assert(ptr >= m_begin && ptr < m_end);
			uint8_t* erase_ptr = (uint8_t*)ptr;
			uint8_t* last_ptr = m_end - m_type.size();
			m_type.destructor(erase_ptr);
			m_type.move_constructor(erase_ptr, last_ptr);
			m_end = last_ptr;
		}

		void* front()
		{
			return m_begin;
		}

		void* back()
		{
			return m_end - m_type.size();
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

			iterator(uint8_t* ptr, size_t type_size) noexcept
				: m_ptr(ptr),
				m_type_size(type_size)
			{}

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
				return iterator(m_ptr + n * m_type_size, m_type_size);
			}

			iterator operator-(difference_type n) const
			{
				return iterator(m_ptr - n * m_type_size, m_type_size);
			}

			difference_type operator-(const iterator& other) const
			{
				return (m_ptr - other.m_ptr) / m_type_size;
			}

			operator pointer() const
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
			return iterator(m_begin, m_type.size());
		}

		iterator end() noexcept
		{
			return iterator(m_end, m_type.size());
		}

		const iterator begin() const noexcept
		{
			return iterator(m_begin, m_type.size());
		}

		const iterator end() const noexcept
		{
			return iterator(m_end, m_type.size());
		}

		iterator get_iterator(void* ptr) noexcept
		{
			return iterator((uint8_t*)ptr, m_type.size());
		}



	};
}
