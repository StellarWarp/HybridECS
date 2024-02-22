#pragma once
#include "../lib/Lib.h"
namespace hyecs
{
	struct entity
	{
		uint32_t id;
		uint32_t version;

		entity() : id(0), version(0) {}
		entity(uint32_t id, uint32_t version) : id(id), version(version) {}

		bool operator==(const entity& other) const
		{
			return id == other.id && version == other.version;
		}
	};

	template<typename T>
	struct is_entity : std::is_same<T, entity> {};
}

template <> //function-template-specialization
struct std::hash<hyecs::entity> {
	size_t operator()(const hyecs::entity& val) const
	{
		return *reinterpret_cast<const uint64_t*>(&val);
	}
};
