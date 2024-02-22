#pragma once
#include "../lib/std_lib.h"
namespace hyecs
{
	/// <summary>
	/// std::array like container for dynamic types
	/// </summary>
	class variant_array
	{
		size_t m_size;
		uint8_t* m_data;
		//dynamic type info
		uint32_t m_type_size;

	public:
		variant_array(size_t type_size, size_t size) noexcept
			: m_size(size), m_type_size(type_size)
		{
			m_data = new uint8_t[m_type_size * m_size];
		}

		~variant_array()
		{
			delete[] m_data;
		}

		variant_array(const variant_array& other) noexcept
			: m_size(other.m_size), m_type_size(other.m_type_size)
		{
			m_data = new uint8_t[m_type_size * m_size];
			memcpy(m_data, other.m_data, m_type_size * m_size);
		}

		variant_array(variant_array&& other) noexcept
			: m_size(other.m_size), m_type_size(other.m_type_size)
		{
			m_data = other.m_data;
			other.m_data = nullptr;
		}

		variant_array& operator=(const variant_array& other) noexcept
		{
			m_size = other.m_size;
			m_type_size = other.m_type_size;
			m_data = new uint8_t[m_type_size * m_size];
			memcpy(m_data, other.m_data, m_type_size * m_size);
			return *this;
		}

		variant_array& operator=(variant_array&& other) noexcept
		{
			m_size = other.m_size;
			m_type_size = other.m_type_size;
			m_data = other.m_data;
			other.m_data = nullptr;
			return *this;
		}





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

		constexpr size_t type_size() const noexcept
		{
			return m_type_size;
		}

		void* operator[](size_t index) noexcept
		{
			return m_data + index * m_type_size;
		}

		const void* operator[](size_t index) const noexcept
		{
			return m_data + index * m_type_size;
		}

	};
}
