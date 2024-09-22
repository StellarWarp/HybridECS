#pragma once
#include "bit_set.h"
#include <bitset>

namespace hyecs
{

	class segmented_bit_set
	{
		static constexpr size_t default_page_bytes = 4096;

		using bit_page = std::bitset<default_page_bytes * 8>;

		vector<bit_page*> pages;

	public:
		segmented_bit_set()
		{
		}

		segmented_bit_set(const segmented_bit_set& other)
		{
			pages.reserve(other.pages.size());
			for (auto page : other.pages)
				pages.push_back(new bit_page(*page));
		}
		segmented_bit_set& operator = (const segmented_bit_set& other)
		{
			pages.reserve(other.pages.size());
			for (auto page : other.pages)
				pages.push_back(new bit_page(*page));
		}

		segmented_bit_set(segmented_bit_set&& other) noexcept = default;


		~segmented_bit_set()
		{
			for (auto page : pages)
				delete page;
		}


		void insert(uint64_t index)
		{
			auto page_index = index / default_page_bytes;
			if (page_index >= pages.size())
				pages.resize(page_index + 1);
			if (pages[page_index] == nullptr)
				pages[page_index] = new bit_page();
			pages[page_index]->set(index % default_page_bytes);

		}

		void erase(uint64_t index)
		{
			auto page_index = index / default_page_bytes;
			assert(page_index < pages.size());
			assert(pages[page_index] != nullptr);
			pages[page_index]->reset(index % default_page_bytes);
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
				for (Unit i = size; i < other.size; i++)
				{
					if (other.data[i] != 0)
						return false;
				}
			}
			for (Unit i = 0; i < std::min(size, other.size); i++)
			{
				if ((data[i] & other.data[i]) != other.data[i])
					return false;
			}
			return true;
		}

		//true for empty set
		bool contains_any(const basic_bit_set& other) const
		{
			for (Unit i = 0; i < std::min(size, other.size); i++)
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
			auto [min_size, max_size] = std::minmax(size, other.size);

			if (size != other.size)
			{
				//make sure the extra data is 0
				auto& longer = size > other.size ? *this : other;
				for (Unit i = min_size; i < max_size; i++)
					if (longer.data[i] != 0)
						return false;
			}
			for (Unit i = 0; i < min_size; i++)
				if (data[i] != other.data[i])
					return false;
			return true;
		}


	};



}