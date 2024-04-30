#pragma once
#include "data_registry.h"
namespace hyecs
{
	template <auto Identifier>
	class static_data_registry : public data_registry
	{

	public:
		using data_registry::data_registry;

		template <typename... T>
		auto get_sorted_component_types() -> const std::array<std::pair<size_t, component_type_index>, sizeof...(T)>&
		{
			using indexed_type = std::pair<size_t, component_type_index>;
			static const auto cached = sort_range(
				std::array<indexed_type, sizeof...(T)>{
				indexed_type{
					type_list<T...>::template index_of<T>,
					m_component_type_infos.at(type_hash::of<T>)
				}...
			}, [](const indexed_type& lhs, const indexed_type& rhs) {
					return lhs.second < rhs.second;
					});
			return cached;
		}

		template <typename... T>
		auto component_types() -> const std::array<component_type_index, sizeof...(T)>&
		{
			static const auto component_types_info = get_sorted_component_types<T...>();
			static const auto component_types =[&]{
				std::array<component_type_index, sizeof...(T)> res{
					component_types_info[type_list<T...>::template index_of<T>].second ...
				};
				return res;
			}
			();
			return component_types;
		}

		template<typename... T>
		void emplace_static(
			sequence_ref<entity> entities,
			T&&... components
		)
		{
			// static const auto group_division = [&]{
			// 	vector<std::tuple<uint64_t,uint64_t,component_group_id>> division;
			// 	uint64_t begin = 0;
			// 	component_group_id current_group = component_group_id{};
			// 	for_each_arg_indexed([&](auto&& component, size_t index)
			// 	{
			// 		using type = std::decay_t<decltype(component)>;
			// 		const component_type_info& comp_type_info = m_component_type_infos.at(type_hash::of<type>);
			// 		const auto group = comp_type_info.group().id();
			// 		if ( group != current_group)
			// 		{
			// 			division.emplace_back(begin, index, current_group);
			// 			begin = index;
			// 			current_group = group;
			// 		}
			// 	}, std::forward<T>(components)...);
			// 	division.emplace_back(begin, sizeof...(T), current_group);
			//
			// 	//component in the same group need to place intensively
			// 	ASSERTION_CODE(//assert not repeat group(same group with multi division)
			// 		for (size_t i = 1; i < division.size(); ++i)
			// 			for (size_t j = 0; j < i; ++j)
			// 				assert(std::get<component_group_id>(division[i]) != std::get<component_group_id>(division[j]));
			// 	)
			// 	return division;
			// }();

			
			static const auto component_types_info = get_sorted_component_types<T...>();
			static const auto component_types = [&]{
					std::array<component_type_index, sizeof...(T)> res{
						component_types_info[type_list<T...>::template index_of<T>].second ...
					};
					return res;
				}
			();
			
			static const auto order_mapping = [&]{
				std::array<size_t, sizeof...(T)> res;
				for (size_t sorted_loc = 0; sorted_loc < sizeof...(T); ++sorted_loc)
				{
					size_t origin_loc = component_types_info[sorted_loc].first;
					res[origin_loc] = sorted_loc;
				}
				return res;
			}();
			
			const auto constructors = [&](){
					std::array<generic::constructor, sizeof...(T)> res{};
					for_each_arg_indexed([&](auto&& component, size_t index)
					{
						using type = std::decay_t<decltype(component)>;
						res[order_mapping[index]] = generic::constructor(std::forward<type>(component));
					}, std::forward<T>(components)...);
					return res;
				}
			();

			emplace(component_types,constructors, entities);
		}


		template<typename... T>
		void for_each(T&&... args)
		{

		}

		// template<typename Callable,typename... Args, size_t... I>
		// void in_group_for_each_impl(Callable&& func, type_list<Args...>, std::index_sequence<I...>)
		// {
		// 	// using decay_param = typename type_list<Args...>::template cast<std::decay_t>;
		// 	// using unordered_component_param = typename decay_param::template filter_with<is_static_component_v>;
		// 	// using non_component_param = typename decay_param::template filter_without<is_static_component_v>;
		// }
		//
		//
		//
		// template<typename Callable>
		// auto in_group_for_each(Callable&& func)
		// {
		// 	in_group_for_each_impl(std::forward<Callable>(func),
		// 		function_traits<Callable>::args{},
		// 		std::make_index_sequence<function_traits<Callable>::arity>{});
		// }


	};
}