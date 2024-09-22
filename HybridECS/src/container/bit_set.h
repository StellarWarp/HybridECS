#pragma once
#include "../lib/std_lib.h"

namespace hyecs
{
	struct bit_key
	{
		uint32_t section;
		uint32_t mask;
		bit_key(): section(0), mask(0){}
		bit_key(uint32_t index)
			: section(index / 32), mask(1 << (index % 32))
		{}
	};

	template<typename Alloc = std::allocator<uint32_t>>
	class basic_bit_set
	{
		uint32_t* data;
		uint32_t size;
		[[no_unique_address]] Alloc alloc;

	public:
		basic_bit_set(Alloc alloc = Alloc())
			: data(nullptr), size(0), alloc(alloc)
		{
		}

		basic_bit_set(const basic_bit_set& other)
			: size(other.size), alloc(other.alloc)
		{
			data = alloc.allocate(size);
			memcpy(data, other.data, size * sizeof(uint32_t));
		}

		basic_bit_set(basic_bit_set&& other) noexcept
			: size(other.size), alloc(std::move(other.alloc))
		{
			data = other.data;
			other.data = nullptr;
			other.size = 0;
		}

		basic_bit_set& operator = (const basic_bit_set& other)
		{
			if (this == &other)
				return *this;
			alloc.deallocate(data, size);
			size = other.size;
			alloc = other.alloc;
			data = alloc.allocate(size);
			memcpy(data, other.data, size * sizeof(uint32_t));
			return *this;
		}

		~basic_bit_set()
		{
			alloc.deallocate(data, size);
		}

		bool contains(bit_key key) const
		{
			if (key.section >= size)
				return false;
			return (data[key.section] & key.mask) != 0;
		}

		bool contains(const basic_bit_set& other) const
		{
			if (other.size > size)
			{
				for (uint32_t i = size; i < other.size; i++)
				{
					if (other.data[i] != 0)
						return false;
				}
			}
			for (uint32_t i = 0; i < std::min(size, other.size); i++)
			{
				if ((data[i] & other.data[i]) != other.data[i])
					return false;
			}
			return true;
		}

		//true for empty set
		bool contains_any(const basic_bit_set& other) const
		{
			for (uint32_t i = 0; i < std::min(size, other.size); i++)
			{
				if ((data[i] & other.data[i]) != 0)
					return true;
			}
			return false;
		}

		bool none_of(const basic_bit_set& other) const
		{
			return !contains_any(other);
		}

		bool operator == (const basic_bit_set& other) const
		{
			auto [min_size,max_size] = std::minmax(size, other.size);

			if (size != other.size)
			{
				//make sure the extra data is 0
				auto& longer = size > other.size ? *this : other;
				for (uint32_t i = min_size; i < max_size; i++)
					if (longer.data[i] != 0)
						return false;
			}
			for (uint32_t i = 0; i < min_size; i++)
				if (data[i] != other.data[i])
					return false;
			return true;
		}

		void insert(bit_key key)
		{
			if (key.section >= size)
			{
				uint32_t* new_data = alloc.allocate(key.section + 1);
				memset(new_data, 0, (key.section + 1) * sizeof(uint32_t));
				memcpy(new_data, data, size * sizeof(uint32_t));
				alloc.deallocate(data, size);
				data = new_data;
				size = key.section + 1;
			}
			data[key.section] |= key.mask;
		}

		void erase(bit_key key)
		{
			data[key.section] &= ~key.mask;
		}

	};

	using bit_set = basic_bit_set<>;


}