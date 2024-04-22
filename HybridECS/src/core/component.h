#pragma once
#include "../lib/std_lib.h"
#include "../container/container.h"
#include "../container/generic_type.h"
#include "component_group.h"
namespace hyecs
{

	class component_type_info
	{
		generic::type_index_container_cached m_type_index;
		type_hash m_hash;
		bit_key m_bit_id;
		component_group_index m_group;//todo
		uint8_t m_is_tag : 1;


		static inline seqence_allocator<component_type_info, uint32_t> seqence_index_allocator;

	public:


		component_type_info(
			const generic::type_index_container_cached& type_index,
			component_group_index group,
			bool is_tag)
			: m_type_index(type_index),
			m_hash(type_index.hash()),
			m_bit_id(seqence_index_allocator.allocate()),
			m_is_tag(is_tag),
			m_group(group)
		{}

        const char* name() const { return m_type_index.name(); }
        bool is_tag() const { return m_is_tag; }
        bool is_empty() const { return m_type_index.size() == 0; }
		type_hash hash() const { return m_hash; }
        bit_key bit_key() const { return m_bit_id; }
        size_t size() const { return m_type_index.size(); }
		component_group_index group() const { return m_group; }
        void* copy_constructor(void* dest, const void* src) const { return m_type_index.copy_constructor(dest, src); }
        void* move_constructor(void* dest, void* src) const { return m_type_index.move_constructor(dest, src); }
        void destructor(void* addr) const { m_type_index.destructor(addr); }
        void destructor(void* addr, size_t count) const { m_type_index.destructor(addr, count); }
		const generic::type_index_container_cached& type_index() const { return m_type_index; }
		operator const generic::type_index_container_cached& () const { return m_type_index; }
		operator const generic::type_index() const { return m_type_index; }
	};

	class component_type_index
	{
		const component_type_info* info;
	public:
		component_type_index()
			: info(nullptr)
		{}
		component_type_index(const component_type_info& info)
			: info(&info)
		{}
		component_type_index(const component_type_index& other)
			: info(other.info)
		{}
		component_type_index& operator = (const component_type_index& other)
		{
			info = other.info;
			return *this;
		}

		generic::type_index type_index() const { return info->type_index(); }
		const component_type_info& get_info() const { return *info; }
		const char* name() const { return info->name(); }
		type_hash hash() const { return info->hash(); }
		bit_key bit_key() const { return info->bit_key(); }
		size_t size() const { return info->size(); }
		bool is_empty() const { return info->is_empty(); }
		bool is_tag() const { return info->is_tag(); }
		component_group_index group() const { return info->group(); }

		void* move_constructor(void* dest, void* src) const { return info->move_constructor(dest, src); }
		void* copy_constructor(void* dest, const void* src) const { return info->copy_constructor(dest, src); }
		void destructor(void* addr) const { info->destructor(addr); }
		void destructor(void* addr, size_t count) const { info->destructor(addr, count); }

		operator const component_type_info& () const { return *info; }
		operator const generic::type_index_container_cached& () const { return info->type_index(); }
		operator const generic::type_index() const { return info->type_index(); }
		bool operator == (const component_type_index& other) const { return info->hash() == other.info->hash(); }
		bool operator != (const component_type_index& other) const { return !(*this == other); }
		bool operator < (const component_type_index& other) const {
			return info->group() < other.info->group() || info->hash() < other.info->hash(); }
		bool operator > (const component_type_index& other) const { return info->hash() > other.info->hash(); }
	};

	class cached_component_type_index
	{
	protected:
		component_type_index index;
		generic::type_index_container_cached type_index_cache;
	public:
		cached_component_type_index(component_type_index type_index_cache)
			: index(type_index_cache), type_index_cache(type_index_cache)
		{
		}

		size_t size() const { return type_index_cache.size(); }
		void* move_constructor(void* dest, void* src) const { return type_index_cache.move_constructor(dest, src); }
		void* copy_constructor(void* dest, const void* src) const { return type_index_cache.copy_constructor(dest, src); }
		void destructor(void* addr) const { return type_index_cache.destructor(addr); }
		void destructor(void* addr, size_t count) const { return type_index_cache.destructor(addr, count); }
		operator component_type_index() const { return index; }
		operator generic::type_index() const { return index; }
		bool operator==(const cached_component_type_index& other) const { return type_index_cache == other.type_index_cache; }
		bool operator!=(const cached_component_type_index& other) const { return type_index_cache != other.type_index_cache; }
		bool operator<(const cached_component_type_index& other) const { return type_index_cache < other.type_index_cache; }
		bool operator>(const cached_component_type_index& other) const { return type_index_cache > other.type_index_cache; }
	};


	class component_constructor
	{
		component_type_index type_index;
		std::function<void* (void*)> constructor;

	public:
		component_constructor(component_type_index type_index, std::function<void* (void*)> constructor)
			: type_index(type_index), constructor(constructor)
		{}

		void* operator()(void* src) const
		{
			return constructor(src);
		}

		component_type_index type() const
		{
			return type_index;
		}
	};

	class component_bit_set :public bit_set
	{
	public:
		using bit_set::bit_set;
		component_bit_set(sequence_ref<component_type_index> indices)
		{
			for (auto& index : indices)
			{
				insert(index);
			}
		}
        void insert(const component_type_index& index) { bit_set::insert(index.bit_key()); }
        void erase(const component_type_index& index) { bit_set::erase(index.bit_key()); }
        bool contains(const component_type_index& index) const { return bit_set::contains(index.bit_key()); }
        bool contains(const component_bit_set& other) const { return bit_set::contains(other); }
		bool contains_any(const component_bit_set& other) const { return bit_set::contains_any(other); }
	};

}

template<>
struct std::hash<hyecs::component_type_index>
{
	size_t operator()(const hyecs::component_type_index& index) const
	{
		return (size_t)index.get_info().hash();
	}
};