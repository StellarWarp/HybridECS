#pragma once
#include "data_registry.h"

namespace hyecs
{
	template <auto Identifier>
	class immediate_data_registry : public data_registry
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
					m_component_type_infos.at(type_hash::of<T>())
				}...
			}, [](const indexed_type& lhs, const indexed_type& rhs) {
					return lhs.second < rhs.second;
					});
			return cached;
		}

		template <typename... T>
		auto component_types(type_list<T...> = {}) -> const std::array<component_type_index, sizeof...(T)>&
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

		template <typename... T>
		auto unsorted_component_types(type_list<T...> = {}) -> const std::array<component_type_index, sizeof...(T)>&
		{
			static const std::array<component_type_index, sizeof...(T)> component_types =  {
				m_component_type_infos.at(type_hash::of<T>()) ...
			};
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
					for_each_arg_indexed([&]<typename type>(type&& component, size_t index)
					{
						res[order_mapping[index]] = generic::constructor(std::forward<type>(component));
					}, std::forward<T>(components)...);
					return res;
				}
			();

			emplace(sorted_sequence_cref(component_types),sorted_sequence_cref(constructors), entities);
		}
		


		template <typename Callable>
		auto in_group_for_each(Callable&& func)
		{
			using params = typename function_traits<Callable>::args;
			using component_param = typename params::template filter_with<is_static_component>;
			using non_component_param = typename params::template filter_without<is_static_component>;
			using decayed_component_param = typename component_param::template cast<std::decay_t>;
			
			std::cout << type_name<component_param> << std::endl;
			std::cout << type_name<non_component_param> << std::endl;

			static query& q = get_query({
				{component_types(decayed_component_param{})},
				{}
				});

			static auto access_info = q.get_access_info(unsorted_component_types(decayed_component_param{}));

			q.for_each<Callable>(std::forward<Callable>(func),access_info);
		}

		template <typename ...>
		struct static_condition;
		template<typename... All, typename... Any, typename... None>
		struct static_condition<type_list<All...>, type_list<Any...>, type_list<None...>>
		{
			using all = type_list<All...>;
			using any = type_list<Any...>;
			using none = type_list<None...>;
		};

		template <typename...>
		struct query_builder;

		struct builder_context
		{
			immediate_data_registry& registry;
		};

		

		template<typename... All, typename... Any, typename... None>
		struct query_builder<type_list<All...>, type_list<Any...>, type_list<None...>>
		{
			builder_context context;

			query_builder(builder_context ctx)
				: context(ctx)
			{
			}
			
			template<typename... T, std::enable_if_t<(is_static_component<T>::value && ...), int> = 0>
			auto all() const { return query_builder<type_list<All..., T...>, type_list<Any...>, type_list<None...>>{context}; }
			template<typename... T, std::enable_if_t<(is_static_component<T>::value && ...), int> = 0>
			auto any() const { return query_builder<type_list<All...>, type_list<Any..., T...>, type_list<None...>>{context}; }
			template<typename... T, std::enable_if_t<(is_static_component<T>::value && ...), int> = 0>
			auto none() const { return query_builder<type_list<All...>, type_list<Any...>, type_list<None..., T...>>{context}; }

		

			template<size_t I, size_t N, typename Test, typename TrueRet, typename FinalRet>
			constexpr auto for_each_index(Test test_func, TrueRet true_func, FinalRet final_func)
			{
				if constexpr (I < N)
				{
					if constexpr (test_func(std::integral_constant<size_t, I>{}))
						return true_func();
					else
						return for_each_index<I + 1,N>(test_func, true_func, final_func);
				}
				else
				{
					return final_func();
				}
			}
			
			template<typename... T,size_t... Gourp>
			constexpr auto get_group_set(std::index_sequence<Gourp...> current, type_list<T...> = {})
			{
				if constexpr (sizeof...(T) == 0)
				{
					return current;
				}
				else
				{
					constexpr component_group_id group_id = component_traits<typename type_list<T...>::template get<0>>::static_group_id;
					constexpr size_t N = sizeof...(Gourp);
					return for_each_index<0,N>(
						[&](auto i)
						{
							return std::get<i>(std::index_sequence<Gourp...>{}) == static_cast<size_t>(group_id.id);;
						},
						[&] {return get_group_set(current, typename type_list<T...>::pop_front{}); },
						[&]
						{
							using next = std::index_sequence<Gourp..., static_cast<size_t>(group_id.id)>;
							return get_group_set(next{}, typename type_list<T...>::pop_front{});
						});
				}
				
			}
			
			auto group_condition()
			{
		
				
			}


			template <typename Callable>
			auto for_each(Callable&& func)
			{
				using params = typename function_traits<Callable>::args;
				using component_param = typename params::template filter_with<is_static_component>;
				using non_component_param = typename params::template filter_without<is_static_component>;
				using decayed_component_param = typename component_param::template cast<std::decay_t>;
				static_assert(decayed_component_param::template contains_in<type_list<All...>>);

				std::cout << type_name<component_param> << std::endl;
				std::cout << type_name<non_component_param> << std::endl;

				group_condition();



				immediate_data_registry& registry = context.registry;

				static query& q = registry.get_query({
					{registry.component_types<All...>()},
					{registry.component_types<Any...>()},
					{registry.component_types<None...>()}
					});

				static auto access_info = q.get_access_info(registry.unsorted_component_types(decayed_component_param{}));

				q.for_each<Callable>(std::forward<Callable>(func),access_info);
			}
		};

		auto query_with()
		{
			return query_builder<type_list<>, type_list<>, type_list<>>( {*this} );
		}


	};
}