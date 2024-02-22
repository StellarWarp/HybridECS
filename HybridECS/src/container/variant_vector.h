#pragma once
#include "../lib/std_lib.h"
namespace hyecs
{
	class variant_vector
	{
		size_t m_size;
		size_t m_capacity;
		uint8_t* m_data;
		//dynamic type info
		uint32_t m_type_size;
	};

	struct container_type_info
	{
		uint32_t type_size;
		uint32_t type_alignment;
		void (*constructor)(void*);
		void (*destructor)(void*);
		void* (*copy_constructor)(void*, const void*);

	};


	class variant_vector_proxy
	{
		uint8_t* m_data;
		size_t m_size;
		size_t m_capacity;
		const container_type_info& m_info;


	public:
		variant_vector_proxy(
			const container_type_info& info,
			uint8_t* data,
			size_t size,
			size_t capacity) :
			m_info(info), m_data(data), m_size(size), m_capacity(capacity) {}

		variant_vector_proxy(const variant_vector_proxy& other) = delete;
		variant_vector_proxy(variant_vector_proxy&& other) = delete;
		variant_vector_proxy& operator=(const variant_vector_proxy& other) = delete;
		variant_vector_proxy& operator=(variant_vector_proxy&& other) = delete;

		constexpr void* data() noexcept
		{
			return m_data;
		}

		constexpr const void* data() const noexcept
		{
			return m_data;
		}

		constexpr size_t size() const noexcept
		{
			return m_size;
		}

		void* operator[](const size_t& index) noexcept
		{
			return m_data + index * m_info.type_size;
		}

		const void* operator[](const size_t& index) const noexcept
		{
			return m_data + index * m_info.type_size;
		}

		void* at(const size_t& index) noexcept
		{
			assert(index < m_size);
			return m_data + index * m_info.type_size;
		}

		const void* at(const size_t& index) const noexcept
		{
			assert(index < m_size);
			return m_data + index * m_info.type_size;
		}

		template<typename T>
		void push_back(const T& value)
		{
			assert(m_size < m_capacity);
			*reinterpret_cast<T*>(at(m_size)) = std::move(value);
			m_size++;
		}

		template<typename T>
		void push_back(T&& value)
		{
			assert(m_size < m_capacity);
			*reinterpret_cast<T*>(at(m_size)) = std::move(value);
			m_size++;
		}

		void push_back()
		{
			assert(m_size < m_capacity);
			if (m_info.constructor)
				m_info.constructor(at(m_size));
			m_size++;
		}

		template<typename T, typename... Args>
		void emplace_back(Args&&... args)
		{
			assert(m_size < m_capacity);
			new(at(m_size)) T(std::forward<Args>(args)...);
			m_size++;
		}

		void pop_back()
		{
			assert(m_size > 0);
			m_size--;
			if (m_info.destructor)
				m_info.destructor(at(m_size));
		}

		void clear()
		{
			if (m_info.destructor)
				for (size_t i = 0; i < m_size; i++)
					m_info.destructor(at(i));
			m_size = 0;
		}



	};
}
