#pragma once
#include <cstdint>
#include <limits>
#include <memory>
#include "../config.h"

namespace hyecs
{
//    using entity_id_t = uint32_t;
    struct entity_id_t
    {
        uint32_t value ;
        constexpr entity_id_t() : value(std::numeric_limits<uint32_t>::max()) {}
        constexpr entity_id_t(uint32_t value) : value(value) {}
        constexpr operator uint32_t() const{ return value; }
    };
    struct entity_version_t
    {
        uint32_t value ;
        constexpr entity_version_t() : value(std::numeric_limits<uint32_t>::max()) {}
        constexpr entity_version_t(uint32_t value) : value(value) {}
        constexpr operator uint32_t() const{ return value; }
    };
	struct entity
	{
	private:
        entity_id_t m_id;
        entity_version_t m_version;

	public:

		constexpr entity() :m_id(), m_version() {}
		constexpr entity(entity_id_t m_id, entity_version_t version) : m_id(m_id), m_version(version) {}

        entity_id_t id() const { return m_id; }
        entity_version_t version() const { return m_version; }

		bool operator==(const entity& other) const
		{
			return m_id == other.m_id && m_version == other.m_version;
		}

		bool operator<(const entity& other) const
		{
			return m_id < other.m_id || (m_id == other.m_id && m_version < other.m_version);
		}

		bool operator>(const entity& other) const
		{
			return m_id > other.m_id || (m_id == other.m_id && m_version > other.m_version);
		}


		operator uint64_t() const
		{
			return *reinterpret_cast<const uint64_t*>(this);
		}
	};

	inline constexpr entity null_entity = entity();

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
