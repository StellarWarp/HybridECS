#pragma once
#include "name_reflect.h"

namespace hyecs
{

	template <typename T>
	constexpr auto type_name = raw_name_of<T>();

	class type_hash
	{
		uint64_t hash;
		constexpr type_hash(uint64_t hash) : hash(hash) {}
	public:
		template<typename T>
		static constexpr type_hash of(){ return type_name<T>.hash();}

        constexpr type_hash() : hash(0) {}
		constexpr type_hash(const type_hash& other) noexcept : hash(other.hash) {}
		constexpr type_hash(type_hash&& other) noexcept : hash(other.hash) { other.hash = 0; };
		constexpr type_hash& operator=(const type_hash& other) noexcept = default;
		constexpr bool operator==(const type_hash& other) const { return hash == other.hash; }
		constexpr bool operator<(const type_hash& other) const { return hash < other.hash; }
		constexpr bool operator>(const type_hash& other) const { return hash > other.hash; }
		constexpr operator uint64_t() const { return hash; }
	};
}

template <>
struct std::hash<hyecs::type_hash> {
	uint64_t operator()(const hyecs::type_hash& info) const
	{
		return info.operator uint64_t();
	}
};

