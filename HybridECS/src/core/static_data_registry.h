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
		auto get_sorted_component_types() -> std::array<std::pair<size_t, component_type_index>, sizeof...(T)>
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

		template<typename... T>
		void emplace_static(
			sequence_ref<entity> entities,
			T&&... components
		)
		{
			static const auto component_types_info = get_sorted_component_types<T...>();
			static const auto component_types =[&]{
					std::array<component_type_index, sizeof...(T)> res{
						component_types_info[type_list<T...>::template index_of<T>].second ...
					};
					return res;
				}
			();

			static const auto constructors =[&]{
					auto unsorted_constructors = std::array<generic::constructor, sizeof...(T)>{ generic::constructor::of<T>()... };
					std::array<generic::constructor, sizeof...(T)> res{
						unsorted_constructors[component_types_info[type_list<T...>::template index_of<T>].first] ...
					};
					return res;
				}
			();

			emplace(component_types, constructors, entities);
		}



	};
}