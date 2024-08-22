#pragma once
#include "../container/container.h"
#include "core/runtime_type/generic_type.h"
#include "component_group.h"

namespace hyecs
{
	class component_type_info : public generic::type_index_interface
	{
        friend struct generic::type_index_interface;
		generic::type_index_container_cached m_type_index;
        size_t m_size;
        size_t m_alignment;
        generic::copy_constructor_ptr_t m_copy_constructor;
        generic::move_constructor_ptr_t m_move_constructor;
        generic::destructor_ptr_t m_destructor;
		type_hash m_hash;
		bit_key m_bit_id;
		component_group_index m_group;//todo
		uint8_t m_is_tag : 1;

		static inline seqence_allocator<component_type_info, uint32_t> seqence_index_allocator;
	public:
		component_type_info(
			const generic::type_index& type_index,
			component_group_index group,
			bool is_tag)
			: m_type_index(type_index),
              m_alignment(type_index.alignment()),
              m_copy_constructor(type_index.copy_constructor_ptr()),
              m_move_constructor(type_index.move_constructor_ptr()),
              m_destructor(type_index.destructor_ptr()),
              m_hash(type_index.hash()),
              m_bit_id(seqence_index_allocator.allocate()),
              m_is_tag(is_tag),
              m_group(group){}

        const char* name() const { return m_type_index.name(); }
        bool is_tag() const { return m_is_tag; }
        bit_key bit_key() const { return m_bit_id; }
		component_group_index group() const { return m_group; }
		explicit operator const generic::type_index() const { return m_type_index; }
	};


    struct component_type_index_interface
    {

        size_t size(this auto&& self) requires (requires { self.m_size; })
        {
            return self.m_size;
        }
        size_t size(this auto&& self) requires (!requires { self.m_size; }) && (requires { self.m_info; })
        {
            return self.m_info->size();
        }
        size_t alignment(this auto&& self) requires (requires { self.m_alignment; })
        {
            return self.m_alignment;
        }
        size_t alignment(this auto&& self) requires (!requires { self.m_alignment; }) && (requires { self.m_info; })
        {
            return self.m_info->alignment();
        }
        bool is_empty(this auto&& self) requires (requires { self.size(); })
        { return self.size() == 0; }
        type_hash hash(this auto&& self) requires (requires { self.m_hash; })
        {
            return self.m_hash;
        }
        type_hash hash(this auto&& self) requires (!requires { self.m_hash; }) && (requires { self.m_info; })
        {
            return self.m_info->hash();
        }
        const char* name(this auto&& self) requires (requires { self.m_name; })
        {
            return self.m_name;
        }
        const char* name(this auto&& self) requires (!requires { self.m_name; }) && (requires { self.m_info; })
        {
            return self.m_info->name();
        }


        auto copy_constructor_ptr(this auto&& self) requires (requires { self.m_copy_constructor; })
        {
            return self.m_copy_constructor;
        }
        auto copy_constructor_ptr(this auto&& self) requires (!requires { self.m_copy_constructor; }) && (requires { self.m_info; })
        {
            return self.m_info->copy_constructor_ptr();
        }
        auto move_constructor_ptr(this auto&& self) requires (requires { self.m_move_constructor; })
        {
            return self.m_move_constructor;
        }
        auto move_constructor_ptr(this auto&& self) requires (!requires { self.m_move_constructor; }) && (requires { self.m_info; })
        {
            return self.m_info->move_constructor_ptr();
        }
        auto destructor_ptr(this auto&& self) requires (requires { self.m_destructor; })
        {
            return self.m_destructor;
        }
        auto destructor_ptr(this auto&& self) requires (!requires { self.m_destructor; }) && (requires { self.m_info; })
        {
            return self.m_info->destructor_ptr();
        }

    public:

        void* copy_constructor(this auto&& self, void* dest, const void* src) requires (requires { self.copy_constructor_ptr(); })
        {
            return self.copy_constructor_ptr()(dest, src);
        }
        void* move_constructor(this auto&& self, void* dest, void* src) requires (requires { self.move_constructor_ptr(); })
        {
            return self.move_constructor_ptr()(dest, src);
        }
        void destructor(this auto&& self, void* addr) requires (requires { self.destructor_ptr(); })
        {
            if (self.destructor_ptr() == nullptr) return;
            return self.destructor_ptr()(addr);
        }
        void destructor(this auto&& self, void* addr, size_t count) requires (requires { self.destructor_ptr(); self.size(); })
        {
            if (self.destructor_ptr() == nullptr) return;
            for (size_t i = 0; i < count; i++)
            {
                self.destructor_ptr()(addr);
                addr = (uint8_t*)addr + self.size();
            }
        }
        void destructor(this auto&& self, void* begin, void* end) requires (requires { self.destructor_ptr(); self.size(); })
        {
            if (self.destructor_ptr() == nullptr) return;
            for (; begin != end; (uint8_t*)begin + self.size())
            {
                self.destructor_ptr()(begin);
            }
        }

        bool is_trivially_destructible(this auto&& self) requires (requires { self.destructor_ptr(); })
        {
            return self.destructor_ptr() == nullptr;
        }
        bool is_reallocable(this auto&& self) requires (requires { self.move_constructor_ptr(); })
        {
            return self.move_constructor_ptr() == nullptr;
        }

    public:
        struct bit_key bit_key(this auto&& self) requires (requires { self.m_bit_id; })
        {
            return self.m_bit_id;
        }
        struct bit_key bit_key(this auto&& self) requires (!requires { self.m_bit_id; }) && (requires { self.m_info; })
        {
            return self.m_info->bit_key();
        }
        bool is_tag(this auto&& self) requires (requires { self.m_is_tag; })
        {
            return self.m_is_tag;
        }
        bool is_tag(this auto&& self) requires (!requires { self.m_is_tag; }) && (requires { self.m_info; })
        {
            return self.m_info->is_tag();
        }
        component_group_index group(this auto&& self) requires (requires { self.m_group; })
        {
            return self.m_group;
        }
        component_group_index group(this auto&& self) requires (!requires { self.m_group; }) && (requires { self.m_info; })
        {
            return self.m_info->group();
        }
    };

    template<typename T1, typename T2>
    requires std::derived_from<std::decay_t<T1>,component_type_index_interface> &&
             std::derived_from<std::decay_t<T2>,component_type_index_interface>
    std::strong_ordering operator <=>(T1&& a, T2&& b) requires (requires { a.hash(); b.hash(); })
    {
        std::strong_ordering group_comp = a.group() <=> b.group();
        if (group_comp != std::strong_ordering::equivalent) return group_comp;
        std::strong_ordering is_tag_comp = a.is_tag() <=> b.is_tag();
        if (is_tag_comp != std::strong_ordering::equivalent) return is_tag_comp;
        std::strong_ordering hash_comp = a.hash() <=> b.hash();
        return hash_comp;
    }
    template<typename T1, typename T2>
    requires std::derived_from<std::decay_t<T1>,component_type_index_interface> &&
             std::derived_from<std::decay_t<T2>,component_type_index_interface>
    bool operator == (T1&& a, T2&& b) requires (requires { a.hash(); b.hash(); })
    {
        return a.hash() == b.hash();
    }
    template<typename T1, typename T2>
    requires std::derived_from<std::decay_t<T1>,component_type_index_interface> &&
             std::derived_from<std::decay_t<T2>,generic::type_index_interface>
    bool operator == (T1&& a, T2&& b) requires (requires { a.hash(); b.hash(); })
    {
        return a.hash() == b.hash();
    }

    class component_type_index : public component_type_index_interface
	{
        friend struct component_type_index_interface;
		const component_type_info* m_info;
	public:
		component_type_index()
			: m_info(nullptr)
		{}
		component_type_index(const component_type_info& info)
			: m_info(&info)
		{}
		component_type_index(const component_type_index& other) = default;
        component_type_index(component_type_index&& other): m_info(other.m_info)
        {
             other.m_info = nullptr;
        }
		component_type_index& operator = (const component_type_index& other)
		{
            m_info = other.m_info;
			return *this;
		}
	};

	class cached_component_type_index: public component_type_index
	{
        friend struct component_type_index_interface;

        size_t m_size;
        size_t m_alignment;
        type_hash m_hash;
	public:
        cached_component_type_index() = default;

		explicit cached_component_type_index(component_type_index index)
			: component_type_index(index),
            m_size(index.size()),
            m_alignment(index.alignment()),
            m_hash(index.hash())
		{}
        cached_component_type_index& operator = (const cached_component_type_index& other) = default;
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
		component_bit_set(sequence_cref< component_type_index> indices)
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

template<std::derived_from<hyecs::component_type_index_interface> T>
struct std::hash<T>
{
	size_t operator()(const hyecs::component_type_index& index) const
	{
		return (size_t)index.hash();
	}
};


namespace hyecs
{
	namespace internal
	{
		template<typename... T, size_t... I>
		auto sorted_static_components_helper(std::index_sequence<I...>)
		{
			//only support for static type
			static_assert((component_traits<std::decay_t<T>>::is_static && ...), "only support for static type");
			//static sort
			struct type_sort_info
			{
				int original_index;
				component_group_id group;
				bool is_tag;
				type_hash hash;
			};

			static constexpr std::array<type_sort_info,sizeof...(T)> order_mapping =
				std::ranges::sort(std::array<type_sort_info,sizeof...(T)>{
				type_sort_info{
					I,
					component_traits<T>::static_group_id,
					component_traits<T>::is_tag,
					type_hash::of<T>()
				}...
				},[](const type_sort_info& lhs, const type_sort_info& rhs)
			{
				if(lhs.group != rhs.group)
					return lhs.group < rhs.group;
				if(lhs.is_tag != rhs.is_tag)
					return !lhs.is_tag;
				return lhs.hash < rhs.hash;
			});

			using unordered_types = type_list<T...>;
			using sorted_types = type_list<
				typename unordered_types::template get<
					order_mapping[I].original_index
			>...
			>;

			return sorted_types{};
		}
	}

	template<typename... T>
	using sorted_static_components = decltype(internal::sorted_static_components_helper<T...>(std::make_index_sequence<sizeof...(T)>{}));
}