#pragma once
#include "archetype_registry.h"
#include "archetype_storage.h"
#include "tag_archetype_storage.h"
#include "query.h"

namespace hyecs
{
	struct sort_key
	{
	};

	struct scope_output_color
	{
		inline static const char* red = "\033[0;31m";
		inline static const char* green = "\033[0;32m";
		inline static const char* yellow = "\033[0;33m";
		inline static const char* blue = "\033[0;34m";
		inline static const char* magenta = "\033[0;35m";
		inline static const char* cyan = "\033[0;36m";
		inline static const char* white = "\033[0;37m";

		scope_output_color(const char* color)
		{
			std::cout << color;
		}
		~scope_output_color()
		{
			std::cout << "\033[0m";
		}
	};

	inline std::ostream& operator<<(std::ostream& os, const component_type_index& index)
	{
		std::string_view name = index.name();
		os << name.substr(7);
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const archetype_index& index)
	{
		const auto& arch = index.get_info();
		for (size_t i = 0; i < arch.component_count(); i++)
		{
			const auto& comp = arch[i];
			os << comp;
			if (i != arch.component_count() - 1)
			{
				os << " ";
			}
		}
		return os;
	}

	inline std::ostream& operator<<(std::ostream& os, const query_condition& condition)
	{
		if (!condition.all().empty())
		{
			os << "[";
			for (size_t i = 0; i < condition.all().size(); i++) {
				auto comp = condition.all()[i];
				os << comp;
				if (i == condition.all().size() - 1) break;
				os << " ";
			}
			os << "]";
		}
		if (!condition.anys().empty())
		{
			os << "( ";
			for (size_t i = 0; i < condition.anys().size(); i++)
			{
				auto& any = condition.anys()[i];
				os << "(";
				for (size_t j = 0; j < any.size(); j++)
				{
					auto comp = any[j];
					os << comp;
					if (j == any.size() - 1) break;
					os << "|";
				}
				os << ")";
				if (i == condition.anys().size() - 1) break;
				os << "|";
			}
			os << " )";
		}
		if (!condition.none().empty())
		{
			os << " - (";
			for (size_t i = 0; i < condition.none().size(); i++) {
				auto comp = condition.none()[i];
				os << comp;
				if (i == condition.none().size() - 1) break;
				os << " ";
			}
			os << ")";
		}
		return os;
	}


	class data_registry
	{
	public:
		archetype_registry m_archetype_registry;
		storage_key_registry m_storage_key_registry;

		vaildref_map<uint64_t, component_storage> m_component_storages;
		vaildref_map<uint64_t, archetype_storage> m_archetypes_storage;
		vaildref_map<uint64_t, tag_archetype_storage> m_tag_archetypes_storage;
		//using storage_variant = std::variant<component_storage, archetype_storage, tag_archetype_storage>;
		//vaildref_map <archetype::hash_type, storage_variant> m_storages;

		vaildref_map<uint64_t, component_type_info> m_component_type_infos;
		vaildref_map<component_group_id, component_group_info> m_component_group_infos;

		vaildref_map<uint64_t, query> m_queries;
		vaildref_map<uint64_t, table_tag_query> m_table_queries;

		dense_set<entity> m_entities;

		class entity_allocator
		{
			uint32_t m_next_id;
			dense_map<uint32_t, uint32_t> m_reuseable_entity_versions{};

		public:
			entity_allocator()
				: m_next_id(0)
			{
			}

			entity allocate()
			{
				entity e(m_next_id, 0);
				m_next_id++;
				return e;
			}

			void deallocate(entity e)
			{
				m_reuseable_entity_versions[e.id()] = e.version();
			}
		};

		entity_allocator m_entity_allocator;

		entity allocate_entity()
		{
			return m_entity_allocator.allocate();
		}

		void deallocate_entity(entity e)
		{
			m_entity_allocator.deallocate(e);
		}

		void allocate_entity(sequence_ref<entity> entities)
		{
			for (uint32_t i = 0; i < entities.size(); i++)
			{
				entity e = m_entity_allocator.allocate();
				entities[i] = e;
				m_entities.insert(e);
			}
		}

		void deallocate_entity(sequence_ref<entity> entities)
		{
			for (uint32_t i = 0; i < entities.size(); i++)
			{
				deallocate_entity(entities[i]);
				m_entities.erase(entities[i]);
			}
		}


		void add_untag_archetype(archetype_index arch)
		{
			scope_output_color c(scope_output_color::cyan);

			assert(!m_archetypes_storage.contains(arch.hash()));
			vector<component_storage*> storages;
			storages.reserve(arch.component_count());
			for (auto& component : arch)
			{
				if (!component.is_empty())
					storages.push_back(&m_component_storages.at(component.hash()));
			}
			//todo process for single component arch
			m_archetypes_storage.emplace_value(arch.hash(), arch,
				sorted_sequence_cref(storages),
				m_storage_key_registry.get_group_key_accessor());

			std::cout << "add untag archetype " << arch << std::endl;
		}

		void add_tag_archetype(archetype_index arch, archetype_index base_arch)
		{
			scope_output_color c(scope_output_color::cyan);
			assert(!m_tag_archetypes_storage.contains(arch.hash()));
			assert(m_archetypes_storage.contains(base_arch.hash()));
			archetype_storage* base_storage = &m_archetypes_storage.at(base_arch.hash());
			vector<component_storage*> tag_storages;
			for (auto& component : arch)
			{
				if (component.is_tag() && !component.is_empty())
					tag_storages.push_back(&m_component_storages.at(component.hash()));
			}
			m_tag_archetypes_storage.emplace_value(arch.hash(), arch, base_storage, sorted_sequence_cref(tag_storages));
			std::cout << "add tag archetype " << arch << std::endl;
		}

		void add_table_query(const archetype_registry::archetype_query_addition_info& info)
		{
			scope_output_color c(scope_output_color::green);
			auto& [
				query_index,
					base_archetype_index,
					tag_condition,
					is_full_set,
					is_direct_set,
					notify_adding_tag,
					notify_partial_convert] = info;
			assert(!m_table_queries.contains(query_index));
			assert(m_archetypes_storage.contains(base_archetype_index.hash()));

			archetype_storage* base_storage = &m_archetypes_storage.at(base_archetype_index.hash());
			vector<component_storage*> tag_storages;
			for (auto& component : tag_condition.all())
			{
				assert(component.is_tag());
				if (!component.is_empty())
					tag_storages.push_back(&m_component_storages.at(component.hash()));
			}
			table_tag_query& table_query = m_table_queries.emplace_value(
				query_index, base_storage, tag_storages, is_full_set, is_direct_set ASSERTION_CODE(, tag_condition));
			notify_adding_tag = [this, &table_query](archetype_index arch)
			{
				tag_archetype_storage* tag_storage = &m_tag_archetypes_storage.at(arch.hash());
				table_query.notify_tag_archetype_add(tag_storage);
			};
			notify_partial_convert = [this, &table_query]()
			{
				table_query.notify_partial_convert();
			};

			std::cout << "add table query [" << base_archetype_index << "] + " << tag_condition;
			if (is_full_set)
				if (is_direct_set) std::cout << " direct set";
				else std::cout << " full set";
			else std::cout << " partial set";

			std::cout << std::endl;
		}

		void add_query(const archetype_registry::query_addition_info& info)
		{
			scope_output_color c(scope_output_color::yellow);
			assert(!m_queries.contains(info.index));
			query& q = m_queries.emplace_value(info.index, info.condition);
			info.archetype_query_addition_callback = [this, &q](query_index table_query_index)
			{
				table_tag_query* table_query = &m_table_queries.at(table_query_index);
				q.notify_table_query_add(table_query);
				std::cout << "query::" << q.condition_debug()
					<< " add table query " << "[" << table_query->base_archetype() << "] "
					<< table_query->tag_condition()
					<< std::endl;
			};
			std::cout << "add query " << info.condition << std::endl;
		}

		component_group_info& register_component_group(component_group_id id, std::string name)
		{
			return m_component_group_infos.emplace(id, component_group_info{ id, name, {} });
		}

		component_type_index register_component_type(generic::type_index type, component_group_info& group, bool is_tag)
		{
			component_type_index component_index = m_component_type_infos.emplace(type.hash(), component_type_info(type, group, is_tag));

			group.component_types.push_back(component_index);
			m_archetype_registry.register_component(component_index);
			if (!type.is_empty())
				m_component_storages.emplace_value(type.hash(), component_index);
			return component_index;
		}

	public:
		void register_type(const ecs_type_register_context& context)
		{
			for (auto& [_, group] : context.groups())
			{
				component_group_info& group_info = register_component_group(group.id, group.name);

				for (auto& [_, component] : group.components)
				{
					register_component_type(component.type, group_info, component.is_tag);
				}
			}
		}

		data_registry()
		{
			m_archetype_registry.bind_untag_archetype_addition_callback(
				[this](archetype_index arch)
			{
				add_untag_archetype(arch);
			}
			);
			m_archetype_registry.bind_tag_archetype_addition_callback(
				[this](archetype_index arch, archetype_index base_arch)
			{
				add_tag_archetype(arch, base_arch);
			}
			);

			m_archetype_registry.bind_archetype_query_addition_callback(
				[this](const archetype_registry::archetype_query_addition_info& info)
			{
				add_table_query(info);
			}
			);

			m_archetype_registry.bind_query_addition_callback(
				[this](const archetype_registry::query_addition_info& info)
			{
				return add_query(info);
			}
			);
		}

		data_registry(const ecs_type_register_context& context) : data_registry()
		{
			register_type(context);
		}


		~data_registry()
		{
		}

		void emplace(
			sorted_sequence_cref<component_type_index> components,
			sorted_sequence_cref<generic::constructor> constructors,
			sequence_ref<entity> entities)
		{
			assert(components.size() == constructors.size());
			auto group_begin = components.begin();
			auto group_end = components.begin();

			allocate_entity(entities);

			while (group_begin != components.end())
			{
				group_end = std::adjacent_find(
					group_begin, components.end(),
					[](const component_type_index& lhs, const component_type_index& rhs)
				{
					return lhs.group() != rhs.group();
				});

				archetype_index arch = m_archetype_registry.get_archetype(append_component(group_begin, group_end));

				auto constructors_begin = constructors.begin() + (group_begin - components.begin());
				auto constructors_end = constructors.begin() + (group_end - components.begin());

				emplace_in_group(arch, entities, sorted_sequence_cref(constructors_begin, constructors_end));

				group_begin = group_end;
			}
		}

		void emplace_in_group(archetype_index arch,
			sequence_cref<entity> entities,
			sorted_sequence_cref<generic::constructor> constructors)
		{
			if (arch.component_count() == 1)
			{
				auto& storage = m_component_storages.at(arch[0].hash());
				auto component_accessor = storage.allocate(entities);
				for (void* addr : component_accessor)
				{
					constructors[0](addr);
				}
			}
			else
			{
				auto construct_process = [&](auto&& allocate_accessor)
				{
					auto constructor_iter = constructors.begin();

					auto component_accessor = allocate_accessor.begin();
					while (component_accessor != allocate_accessor.end())
					{
						assert(component_accessor.component_type() == constructor_iter->type());
						for (void* addr : component_accessor)
						{
							(*constructor_iter)(addr);
						}
						++constructor_iter;
						++component_accessor;
					}
					allocate_accessor.notify_construct_finish();
				};
				if (arch.is_tag())
				{
					auto& storage = m_tag_archetypes_storage.at(arch.hash());
					construct_process(storage.allocate(entities));
				}
				else
				{
					auto& storage = m_archetypes_storage.at(arch.hash());
					construct_process(storage.get_allocate_accessor(entities));
				}
			}
		}


		auto& get_query(const query_condition& condition)
		{
			const query_index index = m_archetype_registry.get_query(condition);
			return m_queries.at(index);
		}

		component_type_index get_component_index(type_hash hash)
		{
			return m_component_type_infos.at(hash);
		}




	};

	namespace query_parameter
	{
		struct wo_component_param {};
		struct descriptor_param {};
		struct filter_param : descriptor_param {};
		struct all_param : filter_param {};
		struct any_param : filter_param {};
		struct none_param : filter_param {};
		struct variant_param : descriptor_param {};
		struct optional_param : descriptor_param {};
		struct relation_param : descriptor_param {};
		struct entity_relation_param : descriptor_param {};
		struct entity_multi_relation_param : descriptor_param {};

		namespace internal
		{
			template<typename T>
			concept decayed_type = std::is_same_v<std::decay_t<T>, T>;

			template<typename T>
			concept is_query_descriptor = std::is_base_of_v<descriptor_param, T>;
		}

		template<typename T> 
			requires (internal::decayed_type<T> && !std::is_base_of_v<descriptor_param, T>)
		using read = const T&;

		template<typename T>
			requires (internal::decayed_type<T> && !std::is_base_of_v<descriptor_param, T>)
		using read_copy = T;

		template<typename T>
			requires (internal::decayed_type<T> && !std::is_base_of_v<descriptor_param, T>)
		using read_write = T&;

		template<typename T>
			requires (internal::decayed_type<T> && !std::is_base_of_v<descriptor_param, T>)
		struct write : wo_component_param
		{
			using type = T;
			void operator=(const T& other) { value = other; }
			write(T& val) : value(val) {}
		private:
			T& value;
		};

		//template<typename T>
		//using input = RO<T>;
		//template<typename T>
		//using output = WO<T>;
		//template<typename T>
		//using inout = RW<T>;

		template<internal::decayed_type... T>
		struct all_of : all_param { using types = type_list<T...>; };

		template<internal::decayed_type... T>
		struct any_of : any_param { using types = type_list<T...>; };

		template<internal::decayed_type... T>
		struct none_of : none_param { using types = type_list<T...>; };

		template<typename... T>
		struct variant : variant_param { using types = type_list<T...>; };

		template<typename T>
		struct optional : optional_param { using type = T; };

		template<typename...>
		struct relation;

		template<typename First, typename Second>
		struct relation<First, Second> : relation_param
		{
			static_assert(!std::is_same_v<First, entity> && !std::is_same_v<Second, entity>);
			using first = First;
			using second = Second;
		};

		namespace internal
		{
			template<typename T>
			struct is_filter : std::is_base_of<filter_param, T> {};
		}

		template<typename First, typename... QueryParam>
		struct relation<First(QueryParam...)> : entity_relation_param
		{
			using relation_tag = First;
			using query_param = type_list<QueryParam...>;
			using accessable = query_param::template filter_without<internal::is_filter>;
			entity e;
			accessable::tuple_t components;

		};

		template<typename...>
		struct multi_relation;

		template<typename First, typename... QueryParam>
		struct multi_relation<First(QueryParam...)> : entity_multi_relation_param
		{
			using relation_tag = First;
			using query_param = type_list<QueryParam...>;
			using accessable = query_param::template filter_without<internal::is_filter>;
			entity e;
			vector<typename accessable::tuple_t> components;
		};


	}

	struct query_descriptor
	{


		enum class relation_category
		{
			single, multi
		};


		struct access_info
		{
			enum read_write_tag {
				ro, wo, rw
			};

			type_hash hash;
			//uint32_t param_index;
			read_write_tag read_write;

		};

		small_vector<type_hash> cond_all;
		small_vector<type_hash> cond_none;
		vector<small_vector<type_hash>> cond_anys;

		vector<std::tuple<type_hash, relation_category, query_descriptor>> relation;

		small_vector<access_info> access_components;
		small_vector<access_info> optional_access_components;
		small_vector<small_vector<access_info>> variant_access_components;
		bool entity_access;
		bool storage_key_access;

		//template<template <typename...> typename T, typename... U>
		//static auto cast_to_type_list_helper(T<U...>){
		//	return type_list<U...>{};
		//};
		//template<typename T>
		//using cast_to_type_list = decltype(cast_to_type_list_helper(std::declval<T>()));

		//using comp_list = cast_to_type_list<query_parameter::any_of<int, float>>;

		template<typename T>
		auto access_info_from()
		{
			using decayed = std::decay_t<T>;
			static constexpr bool is_const = std::is_const_v<T>;
			static constexpr bool is_ref = std::is_reference_v<T>;
			static constexpr bool is_wo = std::is_base_of_v<query_parameter::wo_component_param, T>;

			std::cout << type_name<T> << std::endl;


			if constexpr (is_wo)
			{
				static_assert(!std::is_empty_v<typename T::type>, "empty component is not allowed to access");
				return access_info{ type_hash::of<typename T::type>(), access_info::wo };
			}
			else
			{
				static_assert(!std::is_empty_v<decayed>, "empty component is not allowed to access");

				if constexpr (is_const)
					return access_info{ type_hash::of<decayed>(), access_info::ro };
				else
					if constexpr (is_ref)
						return access_info{ type_hash::of<decayed>(), access_info::rw };
					else
						return access_info{ type_hash::of<decayed>(), access_info::wo };
			}
		};

		template<typename...Args>
		query_descriptor(type_list<Args...> list)
		{
			for_each_type([&]<typename T>(type_wrapper<T>) {
				if constexpr (std::is_base_of_v<query_parameter::descriptor_param, T>)
				{
					if constexpr (std::is_base_of_v<query_parameter::all_param, T>)
					{
						for_each_type([&]<typename U>(type_wrapper<U>) {
							cond_all.push_back(type_hash::of<U>());
						}, typename T::types{});
					}
					else if constexpr (std::is_base_of_v<query_parameter::any_param, T>)
					{
						auto& cond_any = cond_anys.emplace_back();
						for_each_type([&]<typename U>(type_wrapper<U>) {
							cond_any.push_back({ type_hash::of<U>() });
						}, typename T::types{});
					}
					else if constexpr (std::is_base_of_v<query_parameter::none_param, T>)
					{
						for_each_type([&]<typename U>(type_wrapper<U>) {
							cond_none.push_back(type_hash::of<U>());
						}, typename T::types{});
					}
					else if constexpr (std::is_base_of_v<query_parameter::variant_param, T>)
					{
						auto& cond_any = cond_anys.emplace_back();
						auto& access_any = variant_access_components.emplace_back();
						for_each_type([&]<typename U>(type_wrapper<U>) {
							auto info = access_info_from<U>();
							cond_any.push_back(info.hash);
							access_any.push_back(info);
						}, typename T::types{});
					}
					else if constexpr (std::is_base_of_v<query_parameter::optional_param, T>)
					{
						auto info = access_info_from<typename T::type>();
						optional_access_components.push_back(info);
					}
					else if constexpr (std::is_base_of_v<query_parameter::relation_param, T>)
					{

					}
					else if constexpr (std::is_base_of_v<query_parameter::entity_relation_param, T>)
					{
						using relation_tag = typename T::relation_tag;
						using query_param = typename T::query_param;
						relation.emplace_back(
							type_hash::of<relation_tag>(),
							relation_category::single,
							query_descriptor(query_param{}));
					}
					else if constexpr (std::is_base_of_v<query_parameter::entity_multi_relation_param, T>)
					{
						using relation_tag = typename T::relation_tag;
						using query_param = typename T::query_param;
						relation.emplace_back(
							type_hash::of<relation_tag>(),
							relation_category::multi,
							query_descriptor(query_param{}));
					}
					else
					{
						static_assert(false, "unknown type");
					}
				}
				else if constexpr (is_static_component<T>::value || std::is_base_of_v<query_parameter::wo_component_param, T>)
				{
					access_info info = access_info_from<T>();
					cond_all.push_back(info.hash);
					access_components.push_back(info);
				}
				else if constexpr (std::is_same_v<std::decay_t<T>, entity>)
				{
					static_assert(std::is_same_v<T, entity>, "entity must be value parameter");
					std::cout << type_name<T> << std::endl;
					entity_access = true;
				}
				else if constexpr (std::is_same_v<std::decay_t<T>, storage_key>)
				{
					static_assert(std::is_same_v<T, storage_key>, "storage_key must be value parameter");
					std::cout << type_name<T> << std::endl;
					storage_key_access = true;
				}
				else
				{
					//std::cout << "unknown type " << type_name<T> << std::endl;
					static_assert(false, "unknown type");
				}

			}, list);

		}
	};

	class executer_builder
	{
		template<typename T>
		struct is_query_descriptor {
			static constexpr bool value = std::is_base_of_v<query_parameter::descriptor_param, T>;
		};

		data_registry& m_registry;

	public:
		executer_builder(data_registry& registry) : m_registry(registry) {}

		template <typename Callable>
		auto register_executer(Callable&& callable)
		{
			using params = typename function_traits<std::decay_t<Callable>>::args;

			auto descriptor = query_descriptor(params{});

			small_vector<component_type_index> cond_all;
			small_vector<component_type_index> cond_none;
			small_vector<small_vector<component_type_index>> cond_anys;
			cond_all.reserve(descriptor.cond_all.size());
			cond_none.reserve(descriptor.cond_none.size());
			cond_anys.reserve(descriptor.cond_anys.size());

			std::ranges::sort(descriptor.cond_all);
			std::ranges::sort(descriptor.cond_none);

			for (auto hash : descriptor.cond_all)
				cond_all.push_back(m_registry.get_component_index(hash));
			for (auto hash : descriptor.cond_none)
				cond_none.push_back(m_registry.get_component_index(hash));
			for (auto& vec : descriptor.cond_anys)
			{
				auto& cond_any = cond_anys.emplace_back();
				cond_any.reserve(vec.size());
				for (auto hash : vec)
					cond_any.push_back(m_registry.get_component_index(hash));
			}

			//todo enhance the assertion
			ASSERTION_CODE(

				auto is_unique = [](auto& sorted_range)->bool {
				if (sorted_range.size() <= 1) return true;
				for (int i = 1; i < sorted_range.size(); ++i)
					if (sorted_range[i] == sorted_range[i - 1])
						return false;
				return true;
				};

				assert(is_unique(descriptor.cond_all));
				assert(is_unique(descriptor.cond_none));
				for (auto& vec : descriptor.cond_anys)
					assert(is_unique(vec));

				);





			query_condition cond(cond_all, cond_anys, cond_none);


			const auto& q = m_registry.get_query(cond);

			q.condition_debug();


			//vector<component_type_index> access_components = std::ranges::views::transform(descriptor.access_components,
			//	[&](access_info info) { return m_registry.get_component_index(info.hash); });

			//vector<component_type_index> variant_access_components = std::ranges::views::transform(descriptor.variant_access_components,
			//	[&](auto&& vec) { return std::ranges::views::transform(vec,
			//		[&](access_info info) { return m_registry.get_component_index(info.hash); }); });
			//vector<component_type_index> optional_access_components = std::ranges::views::transform(descriptor.optional_access_components,
			//	[&](access_info info) { return m_registry.get_component_index(info.hash); });




		}
	};

}
