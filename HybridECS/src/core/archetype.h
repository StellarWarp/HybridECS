#pragma once
#include "component.h"
#include "../meta/meta_utils.h"
#include "../lib/std_lib.h"
namespace hyecs
{

	struct append_component :public initializer_list<component_type_index> {
		using initializer_list<component_type_index>::initializer_list;
	};

	struct remove_component :public initializer_list<component_type_index> {
		using initializer_list<component_type_index>::initializer_list;
	};

	//enum class archetype_hash_t : uint64_t
	//{
	//	empty = 0,
	//};

	class archetype
	{
	public:
		using hash_type = uint64_t;
	private:
		hash_type m_hash;
		hash_type m_untag_hash;
		//uint32_t m_tag_count;
		component_bit_set m_component_mask;
		vector<component_type_index> m_component_types;
		component_group_index m_group;
		ASSERTION_CODE(
			static inline unordered_map<uint64_t, component_bit_set> debug_archetype_hash;
		)
	public:

		static hash_type addition_hash(hash_type base_hash,
			initializer_list<component_type_index> addition_components)
		{
			auto hash = base_hash;
			for (auto& type : addition_components)
				hash += (uint64_t)type.hash();
			return hash;
		}

		static hash_type subtraction_hash(hash_type base_hash,
			initializer_list<component_type_index> removal_components)
		{
			auto hash = base_hash;
			for (auto& type : removal_components)
				hash -= (uint64_t)type.hash();
			return hash;
		}


	private:

#ifdef HYECS_DEBUG
		void print_mask(const component_bit_set& mask) const
		{
			for (uint32_t i = 0; i < 32; i++)
			{
				if (static_cast<bit_set>(mask).contains(bit_key(i))) printf("1");
				else printf("0");
			}
			printf("\n");
		}

		void hash_collision_assertion() const
		{
			//full part assertion
			if (auto it = debug_archetype_hash.find(m_hash); it != debug_archetype_hash.end()) {
				assert(it->second == m_component_mask);// hash collision
			}
			else debug_archetype_hash.insert({ m_hash, m_component_mask });

			//untag part assertion
			component_bit_set exclusive_mask = m_component_mask;
			for (auto& type : m_component_types)
			{
				if (type.is_tag()) exclusive_mask.erase(type);
			}

			if (auto it = debug_archetype_hash.find(m_untag_hash); it != debug_archetype_hash.end()) {
				assert(it->second == exclusive_mask);// hash collision
			}
			else debug_archetype_hash.insert({ m_untag_hash, exclusive_mask });

		}

		void test_group_identity() const
		{
			if (!m_group.valid()) return;
			for (auto& type : m_component_types)
			{
				if (type.group() != m_group)
				{
					assert(false);//group identity error
				}
			}
		}
#endif // HYECS_DEBUG

		void hash_append_component(std::initializer_list<component_type_index> addition_components)
		{
			for (auto& type : addition_components)
			{
				m_hash += (uint64_t)type.hash();
				m_component_mask.insert(type);
				if (!type.is_tag()) m_untag_hash += (uint64_t)type.hash();
				//else m_tag_count++;
			}
			ASSERTION_CODE(hash_collision_assertion());
			//group code
			ASSERTION_CODE(test_group_identity());
			if (m_component_types.size() != 0) m_group = m_component_types[0].group();
		}
		void hash_removed_component(std::initializer_list<component_type_index> removal_components)
		{
			for (auto& type : removal_components)
			{
				m_hash -= (uint64_t)type.hash();
				m_component_mask.erase(type);
				if (!type.is_tag()) m_untag_hash -= (uint64_t)type.hash();
				//else m_tag_count--;
			}
			ASSERTION_CODE(hash_collision_assertion());
		}

	public:

		archetype() : m_hash(0), m_untag_hash(0), m_group() {}
		archetype(std::initializer_list<component_type_index> components)
			: m_hash(0), m_component_types(components), m_untag_hash(0)
		{
			std::sort(m_component_types.begin(), m_component_types.end());
			hash_append_component(components);
		}
		archetype(const archetype& base, append_component components)
			: archetype(base)
		{
			m_component_types.reserve(components.size() + m_component_types.size());
			for (auto& component : components)
				m_component_types.push_back(component);

			std::sort(m_component_types.begin(), m_component_types.end());
			hash_append_component(components);
		}
		archetype(const archetype& base, remove_component components)
			: archetype(base)
		{
			for (auto& component : components)
				m_component_types.erase(std::remove(m_component_types.begin(), m_component_types.end(), component), m_component_types.end());
			hash_removed_component(components);
		}
		archetype(const archetype& base, append_component adding, remove_component removings)
			: archetype(base)
		{
			for (auto& component : removings)
				m_component_types.erase(std::remove(m_component_types.begin(), m_component_types.end(), component), m_component_types.end());
			for (auto& component : adding)
				m_component_types.push_back(component);
			std::sort(m_component_types.begin(), m_component_types.end());
			hash_append_component(adding);
			hash_removed_component(removings);
		}

		bool contains(component_type_index type) const { return m_component_mask.contains(type); }
		bool contains(const archetype& other) const { return m_component_mask.contains(other.m_component_mask); }
		bool contains(const component_bit_set& mask) const { return m_component_mask.contains(mask); }
		const component_bit_set& component_mask() const { return m_component_mask; }
		bool operator==(const archetype& other) const { return m_hash == other.m_hash; }
		uint64_t hash() const { return m_hash; }
		uint64_t excludsive_hash() const { return m_untag_hash; }
		component_type_index operator[](size_t index) const { return m_component_types[index]; }
		const component_type_index* begin() const { return m_component_types.data(); }
		const component_type_index* end() const { return m_component_types.data() + m_component_types.size(); }
		bool is_tag() const { return m_untag_hash != m_hash; }
		size_t component_count() const { return m_component_types.size(); }
		component_group_index group() const { return m_group; }

		//size_t untag_component_count() const
		//{
		//	return m_component_types.size() - m_tag_count;
		//}

		//size_t tag_component_count() const
		//{
		//	return m_tag_count;
		//}


	};


	class archetype_index
	{
		const archetype* m_archetype;

	public:
		inline static const archetype empty_archetype;

		archetype_index() : m_archetype(&empty_archetype) {}
		archetype_index(const archetype& archetype) : m_archetype(&archetype) {}

		const archetype& get_info() const { return *m_archetype; }
		const uint64_t hash() const { return m_archetype->hash(); }
		const uint64_t exclusive_hash() const { return m_archetype->excludsive_hash(); }
		const uint64_t hash(append_component addition_components) const { return archetype::addition_hash(m_archetype->hash(), addition_components); }
		const uint64_t hash(remove_component removal_components) const { return archetype::subtraction_hash(m_archetype->hash(), removal_components); }
		bool is_tag() const { return m_archetype->is_tag(); }
		bool contains(component_type_index type) const { return m_archetype->contains(type); }
		bool contains(const archetype_index& other) const { return m_archetype->contains(other.get_info()); }
		bool contains(const component_bit_set& mask) const { return m_archetype->contains(mask); }
		const component_bit_set& component_mask() const { return m_archetype->component_mask(); }

		bool operator==(const archetype_index& other) const
		{
			ASSERTION_CODE(
				if (m_archetype != other.m_archetype && m_archetype != nullptr && other.m_archetype != nullptr)
					assert(m_archetype->hash() != other.m_archetype->hash());//multiple location of same archetype
			);
			return m_archetype == other.m_archetype;
		}

		bool operator!=(const archetype_index& other) const { return m_archetype != other.m_archetype; }
		auto begin() const { return m_archetype->begin(); }
		auto end() const { return m_archetype->end(); }
		uint32_t component_count() const { return m_archetype->component_count(); }

		component_type_index operator[](size_t index) const { return (*m_archetype)[index]; }
	};
}

template<>
struct std::hash<hyecs::archetype>
{
	size_t operator()(const hyecs::archetype& arch) const
	{
		return arch.hash();
	}
};

template<>
struct std::hash<hyecs::archetype_index>
{
	size_t operator()(const hyecs::archetype_index& arch) const
	{
		return arch.hash();
	}
};


