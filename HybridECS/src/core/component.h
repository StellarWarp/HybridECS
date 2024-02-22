#pragma once
#include "rtref.h"


namespace hyecs
{
	class bit_key
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
				for (int i = size; i < other.size; i++)
				{
					if (other.data[i] != 0)
						return false;
				}
			}
			for (int i = 0; i < std::min(size, other.size); i++)
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
					for (int i = size; i < other.size; i++)
						if (other.data[i] != 0)
							return false;
						else
							for (int i = other.size; i < size; i++)
								if (data[i] != 0)
									return false;
			}
			for (int i = 0; i < size; i++)
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



	class component_type_info
	{
		uint64_t m_hash;
		bit_key m_bit_id;
		size_t m_size;
		generic::move_constructor_ptr m_move_constructor;
		generic::destructor_ptr m_destructor;
		uint8_t m_is_tag : 1;


		static inline seqence_allocator<component_type_info> seqence_index_allocator;


	public:
		template<typename T>
		static component_type_info from_template(bool is_tag)
		{
			component_type_info info;
			info.m_hash = typeid(T).hash_code();
			info.m_bit_id = seqence_index_allocator.allocate();
			info.m_size = std::is_empty_v<T> ? 0 : sizeof(T);
			info.m_move_constructor = generic::copy_constructor<T>;
			info.m_destructor = generic::nullable_destructor<T>();
			info.m_is_tag = is_tag;
			return info;
		}

		template<typename T>
		static component_type_info from_template()
		{
			return from_template<T>(std::is_empty_v<T>);
		}

		bool is_tag() const
		{
			return m_is_tag;
		}

		bool is_empty() const
		{
			return m_size == 0;
		}


		uint64_t hash() const
		{
			return m_hash;
		}

		bit_key bit_key() const
		{
			return m_bit_id;
		}

		size_t size() const
		{
			return m_size;
		}

		generic::move_constructor_ptr move_constructor() const
		{
			return m_move_constructor;
		}

		generic::destructor_ptr destructor() const
		{
			return m_destructor;
		}


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

		const component_type_info& get_info() const
		{
			return *info;
		}

		 uint64_t hash() const
		{
			return info->hash();
		}

		 bit_key bit_key() const
		{
			return info->bit_key();
		}

		 size_t size() const
		{
			return info->size();
		}

		bool is_tag() const
		{
			return info->is_tag();
		}

		const generic::move_constructor_ptr move_constructor() const
		{
			return info->move_constructor();
		}

		const generic::destructor_ptr destructor() const
		{
			return info->destructor();
		}

		operator const component_type_info& () const
		{
			return *info;
		}

		bool operator == (const component_type_index& other) const
		{
			return info->hash() == other.info->hash();
		}

		bool operator != (const component_type_index& other) const
		{
			return !(*this == other);
		}

		bool operator < (const component_type_index& other) const
		{
			return info->hash() < other.info->hash();
		}

		bool operator > (const component_type_index& other) const
		{
			return info->hash() > other.info->hash();
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