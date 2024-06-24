#pragma once
namespace hyecs
{
	struct entity
	{
	private:
		uint32_t m_id;
		uint32_t m_version;

	public:

		constexpr entity() :
			m_id(std::numeric_limits<uint32_t>::max()),
			m_version(std::numeric_limits<uint32_t>::max()) {}
		constexpr entity(uint32_t m_id, uint32_t version) : m_id(m_id), m_version(version) {}

		uint32_t id() const { return m_id; }
		uint32_t version() const { return m_version; }

		bool operator==(const entity& other) const
		{
			return m_id == other.m_id && m_version == other.m_version;
		}

		bool operator!=(const entity& other) const
		{
			return !operator==(other);
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
