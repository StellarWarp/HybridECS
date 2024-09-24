#pragma once
#include "lib/std_lib.h"
#include "core/hyecs_core.h"

namespace hyecs
{

    struct component_group_id
    {
        uint32_t id;
    public:
        constexpr component_group_id() : id(0) {}
        template <size_t Np1, typename CharT = char>
        constexpr explicit component_group_id(const CharT (&str)[Np1])
                :id (string_hash32(str)){}
        constexpr component_group_id(const component_group_id& id) : id(id.id) {}
        component_group_id(const std::string& name)
        {
            id = string_hash32(name.c_str(), name.size());
        }
        component_group_id(const volatile component_group_id& other) : id(other.id) {}
        constexpr auto operator <=> (const component_group_id& other) const = default;
    };

    template<class>
    struct component_info_mappings {
        friend auto constexpr get_component_static_info(component_info_mappings);
        template <auto SetValue>
        struct set {
            friend auto constexpr get_component_static_info(component_info_mappings) { return SetValue; }
        };
    };

    struct index_mapping_tag;

    template<class, size_t>
    struct mapping_ {
        friend constexpr auto get_(mapping_);
        template <auto SetValue>
        struct set {
            friend constexpr auto get_(mapping_) { return SetValue; }
        };
    };

    template<class T,
            size_t N = 0,
            auto tag [[maybe_unused]]= []{}>
    static consteval size_t mapping_count()
    {
#ifdef __INTELLISENSE__
        return 0;
#else
        if constexpr (requires { get_(mapping_<T, N>{}); })
            return mapping_count<T, N + 1>();
        else
            return N;
#endif
    }

    template<typename T,
            component_group_id Group,
            bool IsTag>
    struct component_static_info
    {
        using type = T;
        static constexpr auto group_id = Group;
        static constexpr bool is_tag = IsTag;
    };

    template<typename T,
            component_group_id Group,
            bool IsTag>
    struct component_reflect_impl
    {
        template<size_t N = 0>
        static constexpr size_t get_index()
        {
            if constexpr (requires { get_(mapping_<index_mapping_tag, N>{}); })
                return get_index<N + 1>();
            else
                return N;
        }
        static constexpr auto index = get_index();

        using info_t = component_static_info<T, Group, IsTag>;
        using setter_t = component_info_mappings<T>::template set<info_t{}>;
        using index_setter_t = mapping_<index_mapping_tag, index>::template set<info_t{}>;
        static constexpr auto _0 [[maybe_unused]] = setter_t();
        static constexpr auto _1 [[maybe_unused]] = index_setter_t();
    };

    template<auto...>
    struct component_register_helper;

    template<typename T>
    constexpr component_group_id default_group_id()
    {
        if constexpr ( requires { T::static_group_id; } )
            return T::static_group_id;
        else
            return component_group_id();
    }
    template<typename T>
    constexpr bool default_is_tag()
    {
        if constexpr ( requires { T::is_tag; } )
            return T::is_tag;
        else if constexpr ( std::is_empty_v<T> )
            return true;
        else
            return false;
    }

    template<typename T,
            component_group_id Group = default_group_id<T>(),
            bool IsTag = default_is_tag<T>()>
    using register_ecs_component = component_register_helper<
            component_reflect_impl<T,Group,IsTag>{}
    >;



	//this is not for multi-thread
	template<typename ForType, typename Integer = uint64_t>
	struct sequence_allocator
	{
		static Integer allocate() {
			static Integer next = 0;
			return next++;
		}
	};

	struct tag_component { static constexpr bool is_tag = true; };

	template<typename T>
	class component_traits
	{
	public:
        static constexpr bool is_static = requires { get_component_static_info(component_info_mappings<T>{}); };
		static constexpr bool is_tag = get_component_static_info(component_info_mappings<T>{}).is_tag;
		static constexpr component_group_id static_group_id = get_component_static_info(component_info_mappings<T>{}).group_id;
	};
	template<typename T>
	struct is_static_component { static constexpr bool value = component_traits<std::decay_t<T>>::is_static; };
	template<typename T>
	struct is_tag_component { static constexpr bool value = component_traits<std::decay_t<T>>::is_tag; };

}


template<>
struct std::hash<hyecs::component_group_id>
{
	std::size_t operator()(const hyecs::component_group_id& id) const
	{
		return id.id;
	}
};

//rtti
namespace hyecs
{
	class ecs_rtti_context
	{
	public:
		struct component_register_info
		{
			generic::type_index type;
			bit_key component_key;
			uint8_t is_tag : 1;
		};

		struct component_group_register_info
		{
			component_group_id id;
			std::string name;
			unordered_map<generic::type_index, component_register_info> components;
		};
	private:
		unordered_map<component_group_id, component_group_register_info> m_groups{};
	public:
		template <size_t Np1, typename CharT = char>
		void add_group(const CharT(&name)[Np1])
		{
			component_group_id id(name);
			component_group_register_info info{ id, name, {} };
            if (m_groups.contains(id)) return;
            else
			    m_groups.insert({ id, info });
		}


		void add_component(component_group_id group_id, generic::type_index type, bool is_tag)
		{
			auto it = m_groups.find(group_id);
			if (it == m_groups.end()) throw std::runtime_error("group not found");
			auto& group = it->second;
			if (group.components.contains(type)) throw std::runtime_error("component already exists");
			group.components.insert({ 
				type, 
				component_register_info{
                        type,
                        sequence_allocator<component_register_info,uint32_t>::allocate(),
                        is_tag
				} 
				});
		}

		template<typename T>
		void add_component(component_group_id group_id, bool is_tag)
		{
			add_component(group_id, generic::type_info::of<T>(), is_tag);
		}
		template<typename T>
		void add_component(component_group_id group_id)
		{
			add_component<T>(group_id, component_traits<T>::is_tag);
		}

		const auto& groups() const
		{
			return m_groups;
		}

	};

	struct ecs_global_rtti_context
	{
		static ecs_rtti_context& register_context()
        {
            static ecs_rtti_context context;
            return context;
        }
	};



    template<typename T, component_group_id GroupId, bool IsTag = default_is_tag<T>()>
    struct ecs_rtti_register
    {
        using _ = register_ecs_component<T, GroupId, IsTag>;
        ecs_rtti_register(){
            ecs_global_rtti_context::register_context().add_component<T>(GroupId, IsTag);
        }
    };

	struct ecs_rtti_group_register
	{
		template <size_t Np1, typename CharT = char>
		ecs_rtti_group_register(const CharT(&name)[Np1])
		{
			ecs_global_rtti_context::register_context().add_group(name);
		}
	};


    struct component_type_index;
	struct component_group_info
	{
		component_group_id id;
		std::string name;
		vector<component_type_index> component_types;
	};

	class component_group_index
	{
		const component_group_info* info;

	public:
		component_group_index()
			: info(nullptr)
		{}

		component_group_index(const component_group_info& info)
			: info(&info) {}

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
		bool valid() const { return info != nullptr; }
		const vector<component_type_index>& component_types() const { return info->component_types; }
        auto operator <=> (const component_group_index& other) const { return info->id <=> other.info->id; }
        bool operator == (const component_group_index& other) const { return info->id == other.info->id; }
	};
};


template<>
struct std::hash<hyecs::component_group_index>
{
	std::size_t operator()(const hyecs::component_group_index& index) const
	{
		return std::hash<hyecs::component_group_id>{}(index.id());
	}
};



