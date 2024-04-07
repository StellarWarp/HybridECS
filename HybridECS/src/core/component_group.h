#pragma once
#include "../lib/std_lib.h"
#include "../meta/meta_utils.h"

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

	DECLATE_MEMBER_STATIC_VARIABLE_TEST(is_tag);

	template<typename T>
	class component_traits
	{

		static constexpr bool is_tag_helper()
		{
			if constexpr (has_member_static_variable_is_tag_v<T>) return T::is_tag;
			else return std::is_empty_v<T>;
		}
	public:
		static constexpr bool is_tag = is_tag_helper();
	};

	struct component_group_id
	{
		uint32_t id;
	public:
		component_group_id() : id(0) {}
		component_group_id(const std::string& name)
		{
			id = static_cast<uint32_t>(std::hash<std::string>{}(name));
		}
		component_group_id(const char* name)
		{
			id = static_cast<uint32_t>(std::hash<std::string>{}(name));
		}
		component_group_id(const volatile component_group_id& other) : id(other.id) {}
		bool operator == (const component_group_id& other) const { return id == other.id; }
		bool operator != (const component_group_id& other) const { return id != other.id; }
		bool operator < (const component_group_id& other) const { return id < other.id; }
		bool operator > (const component_group_id& other) const { return id > other.id; }
	};
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

		component_group_id add_group(const std::string& name)
		{
			component_group_id id(name);
			component_group_register_info info{ id, name, {} };
			m_groups[id] = info;
			return id;
		}

		void add_component(component_group_id group_id, generic::type_index type, bool is_tag)
		{
			auto it = m_groups.find(group_id);
			if (it == m_groups.end()) throw std::runtime_error("group not found");
			auto& group = it->second;
			if (group.components.contains(type)) throw std::runtime_error("component already exists");
			group.components[type] = component_register_info{
				type,
				seqence_allocator<component_register_info,uint32_t>::allocate(),
				is_tag
			};
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
	inline static volatile ecs_component_static_register<T> __ecs_component_register_##T(group_id); \
}

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
		ecs_component_group_static_register(const char* name)
		{
			ecs_static_register_context::register_context.add_group(name);
		}
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
		const vector<component_type_index>& component_types() const { return info->component_types; }
		bool operator == (const component_group_index& other) const { return info->id == other.info->id; }
		bool operator != (const component_group_index& other) const { return !operator==(other); }
		bool operator < (const component_group_index& other) const { return info->id < other.info->id; }
		bool operator > (const component_group_index& other) const { return info->id > other.info->id; }
	};
};




