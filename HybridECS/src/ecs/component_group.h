#pragma once
#include "../lib/std_lib.h"
#include "../meta/meta_utils.h"
#include "../container/static_string.h"

namespace hyecs
{

	//this is not for muti-thread
	template<typename ForType, typename Integer = uint64_t>
	struct seqence_allocator
	{
		static Integer allocate() {
			static Integer next = 0;
			return next++;
		}
	};

	struct tag_component { static constexpr bool is_tag = true; };

    template<typename T>
    concept has_member_static_variable_is_tag = requires { T::is_tag; };
    template<typename T>
    concept has_member_static_variable_static_group_id = requires { T::static_group_id; };

	struct component_group_id
	{
		uint32_t id;
	public:
		constexpr component_group_id() : id(0) {}
		template <size_t Np1, typename CharT = char>
		constexpr component_group_id(const CharT (&str)[Np1])
			:id (string_hash32(str)){}
		constexpr component_group_id(const component_group_id& id) : id(id.id) {}
		component_group_id(const std::string& name)
		{
			id = string_hash32(name.c_str(), name.size());
		}
		component_group_id(const volatile component_group_id& other) : id(other.id) {}
        constexpr auto operator <=> (const component_group_id& other) const = default;
	};


	//specialize this and add static_group_id to override the default behavior
	template<typename T>
	struct component_static_group_override{};
	//specialize this and add is_tag and static_group_id to override the default behavior
	template<typename T>
	struct component_traits_override{/*static_group_id*/};
	template<typename T>
	struct component_register_static : std::false_type{};

	template<typename T>
	class component_traits
	{
		static constexpr bool is_static_type_helper()
		{
			if constexpr (component_register_static<T>::value)
				return true;
			else if constexpr (has_member_static_variable_static_group_id<component_static_group_override<T>>)
				return true;
			else if constexpr (has_member_static_variable_static_group_id<component_traits_override<T>>)
				return true;
			else if constexpr (has_member_static_variable_static_group_id<T>)
				return true;
			else
			return false;
		}
		static constexpr bool is_tag_helper()
		{
			if constexpr (has_member_static_variable_is_tag<component_traits_override<T>>)
				return component_traits_override<T>::is_tag;
			else if constexpr (has_member_static_variable_is_tag<T>)
				return T::is_tag;
			else
				return std::is_empty_v<T>;
		}
		static constexpr component_group_id static_group_id_helper()
		{
			if constexpr (has_member_static_variable_static_group_id<component_static_group_override<T>>)
				return component_static_group_override<T>::static_group_id;
			else if constexpr (has_member_static_variable_static_group_id<component_traits_override<T>>)
				return component_traits_override<T>::static_group_id;
			else if constexpr (has_member_static_variable_static_group_id<T>)
				return T::static_group_id;
			else return component_group_id();
		}
	public:
		static constexpr bool is_static = is_static_type_helper();
		static constexpr bool is_tag = is_tag_helper();
		static constexpr component_group_id static_group_id{static_group_id_helper()};
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


namespace hyecs
{
	class ecs_type_register_context
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
		unordered_map<component_group_id, component_group_register_info> m_groups;
	public:
		template <size_t Np1, typename CharT = char>
		component_group_id add_group(const CharT(&name)[Np1])
		{
			component_group_id id(name);
			component_group_register_info info{ id, name, {} };
			m_groups[id] = info;
			return component_group_id(name);
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
					seqence_allocator<component_register_info,uint32_t>::allocate(),
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

	struct ecs_static_register_context
	{
		inline static ecs_type_register_context register_context;
	};


#define REGISTER_HYECS_COMPONENT(T, group_id) \
namespace internal_component_register { \
	inline volatile ecs_component_static_register<T> __ecs_component_register_##T(group_id); \
}\
\
template<>\
struct component_static_group_override<T>{\
static constexpr component_group_id static_group_id{group_id};\
};\
template<>\
struct component_register_static<T> : std::true_type{};

	template<typename T>
	struct ecs_component_static_register
	{
		ecs_component_static_register(component_group_id group_id, bool is_tag)
		{
			ecs_static_register_context::register_context.add_component<T>(group_id, is_tag);
		}
		ecs_component_static_register(component_group_id group_id)
		{
			ecs_static_register_context::register_context.add_component<T>(group_id, component_traits<T>::is_tag);
		}

	};


	struct ecs_component_group_static_register
	{
		template <size_t Np1, typename CharT = char>
		ecs_component_group_static_register(const CharT(&name)[Np1])
		{
			ecs_static_register_context::register_context.add_group(name);
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



