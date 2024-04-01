#pragma once
#include "../lib/std_lib.h"
#include "../container/generic_type.h"
namespace hyecs
{
	struct bit_key
	{
		uint32_t section;
		uint32_t mask;

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

	//this is not for muti-thread
	template<typename ForType, typename Integer = uint64_t>
	struct seqence_allocator
	{
		static Integer allocate() {
			static Integer next = 0;
			return next++;
		}
	};


	class ecs_registry_context;
	struct component_group_id
	{
		uint32_t id;

		component_group_id(const std::string& name)
		{
			id = static_cast<uint32_t>(std::hash<std::string>{}(name));
		}
		component_group_id(const char* name)
		{
			id = static_cast<uint32_t>(std::hash<std::string>{}(name));
		}

		static component_group_id register_group(const std::string& name, ecs_registry_context* context)
		{
			auto id = component_group_id(name);
			//...

			return id;
		}

		bool operator == (const component_group_id& other) const { return id == other.id; }
	};

	class component_type_index;
	struct component_group_info
	{
		component_group_id id;
		std::string name;
		vector<component_type_index> component_types;

	};

	class component_group_index
	{
		component_group_info* info;

	public:
		component_group_index()
			: info(nullptr)
		{}

		component_group_index(component_group_info* info)
			: info(info)
		{}

		component_group_index(const component_group_index& other)
			: info(other.info)
		{}

		component_group_index& operator = (const component_group_index& other)
		{
			info = other.info;
			return *this;
		}

		component_group_id id() const { return info->id; }
		const std::string& name() const { return info->name; }
		const vector<component_type_index>& component_types() const { return info->component_types; }
		bool operator == (const component_group_index& other) const { return info->id == other.info->id; }
		bool operator != (const component_group_index& other) const { return !operator==(other); }
	};




	class component_type_info
	{
		generic::type_index_container_cached m_type_index;
		uint64_t m_hash;
		bit_key m_bit_id;
		uint8_t m_is_tag : 1;
		component_group_index m_group;//todo


		static inline seqence_allocator<component_type_info, uint32_t> seqence_index_allocator;

		component_type_info(
			const generic::type_index_container_cached& type_index,
			uint64_t hash,
			uint32_t seqence_index,
			bool is_tag)
			: m_type_index(type_index),
			m_hash(hash),
			m_bit_id(seqence_index),
			m_is_tag(is_tag)
		{}

	public:
		template<typename T>
		static const component_type_info& from_template(bool is_tag)
		{
			static component_type_info info(
				generic::type_index_container_cached{ generic::type_info::from_template<T>() },
				typeid(T).hash_code(),
				seqence_index_allocator.allocate(),
				is_tag
			);

			return info;
		}

        template<typename T> static component_type_info from_template() { return from_template<T>(std::is_empty_v<T>); }
        const char* name() const { return m_type_index.name(); }
        bool is_tag() const { return m_is_tag; }
        bool is_empty() const { return m_type_index.size() == 0; }
        uint64_t hash() const { return m_hash; }
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
		uint64_t hash() const { return info->hash(); }
		bit_key bit_key() const { return info->bit_key(); }
		size_t size() const { return info->size(); }
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
		bool operator < (const component_type_index& other) const { return info->hash() < other.info->hash(); }
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

}

template<>
struct std::hash<hyecs::component_type_index>
{
	size_t operator()(const hyecs::component_type_index& index) const
	{
		return index.get_info().hash();
	}
};