#pragma once
#include "../lib/Lib.h"

namespace hyecs
{
	struct type_hash
	{
		uint64_t hash;
	public:

		type_hash(const std::type_info& info)  noexcept
		{
			hash = info.hash_code();
		}

		type_hash(const type_hash& other) noexcept 
		{
			hash = other.hash;
		}

		type_hash(const type_hash&& other) noexcept
		{
			hash = other.hash;
		}

		type_hash& operator=(const type_hash& other) 
		{
			hash = other.hash;
			return *this;
		}

		type_hash& operator=(const type_hash&& other)
		{
			hash = other.hash;
			return *this;
		}

		bool operator==(const type_hash& other) const
		{
			return hash == other.hash;
		}

		bool operator<(const type_hash& other) const
		{
			return hash < other.hash;
		}

		explicit operator uint64_t() const 
		{
			return hash;
		}
	};

}

template <>
struct std::hash<hyecs::type_hash> {
	size_t operator()(const hyecs::type_hash& info) const
	{
		return info.hash;
	}
};