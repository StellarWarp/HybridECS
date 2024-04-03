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
	class bit_set
	{
		uint32_t* data;
		uint32_t size;
		[[no_unique_address]] Alloc alloc;

	public:
		bit_set(uint32_t default_size = 4, Alloc alloc = Alloc())
			: size(default_size), alloc(alloc)
		{
			data = alloc.allocate(default_size);
			memset(data, 0, default_size * sizeof(uint32_t));
		}

		bit_set(const bit_set& other)
			: size(other.size), alloc(other.alloc)
		{
			data = alloc.allocate(size);
			memcpy(data, other.data, size * sizeof(uint32_t));
		}

		bit_set(bit_set&& other) noexcept
			: size(other.size), alloc(std::move(other.alloc))
		{
			data = other.data;
			other.data = nullptr;
			other.size = 0;
		}

		bit_set& operator = (const bit_set& other)
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

		~bit_set()
		{
			alloc.deallocate(data, size);
		}

		bool has(bit_key key) const
		{
			if (key.section >= size)
				return false;
			return (data[key.section] & key.mask) != 0;
		}

		//todo reqire test
		bool contains(const bit_set& other) const
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

		bool operator == (const bit_set& other) const
		{
			if (size != other.size)
			{
				//make sure the extra data is 0
				if (size < other.size)
					for (uint32_t i = size; i < other.size; i++)
						if (other.data[i] != 0)
							return false;
						else
							for (uint32_t i = other.size; i < size; i++)
								if (data[i] != 0)
									return false;
			}
			for (uint32_t i = 0; i < size; i++)
			{
				if (data[i] != other.data[i])
					return false;
			}
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


}