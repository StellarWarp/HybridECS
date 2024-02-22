#pragma once
#include "Query.h"
#include "EntityManager.h"
namespace hyecs
{
	template<typename...>
	class QueryBuilder;

	//template<typename AllList, typename AnyList, typename NoneList, typename AccList>
	//class QueryBuilder;

	template<typename... All_t, typename... Any_t, typename... None_t, typename... Acc_t>
	class QueryBuilder<
		type_list<All_t...>,
		type_list<Any_t...>,
		type_list<None_t...>,
		type_list<Acc_t...>>
	{
	public:
		template<typename ...T>
		constexpr auto All()
		{
			return QueryBuilder<type_list<All_t..., T...>, type_list<Any_t...>, type_list<None_t...>, type_list<Acc_t...>>();
		}

		template<typename ...T>
		constexpr auto Any()
		{
			return QueryBuilder<type_list<All_t...>, type_list<Any_t..., T...>, type_list<None_t...>, type_list<Acc_t...>>();
		}

		template<typename ...T>
		constexpr auto None()
		{
			return QueryBuilder<type_list<All_t...>, type_list<Any_t...>, type_list<None_t..., T...>, type_list<Acc_t...>>();
		}

		template<typename ...T>
		constexpr auto Access()
		{
			return QueryBuilder<type_list<All_t...>, type_list<Any_t...>, type_list<None_t...>, type_list<Acc_t..., T...>>();
		}

		constexpr auto Build(EntityManager& em)
		{
			if constexpr (sizeof...(Acc_t) == 0)
			{
				return QueryBuilder<type_list<All_t...>, type_list<Any_t...>, type_list<None_t...>, type_list<All_t...>>().Build(em);
			}
			else
			{
				using type = Query<type_list<All_t...>, type_list<Any_t...>, type_list<None_t...>, type_list<Acc_t...>>;
				auto query = new type();
				em.RegisterQuery(*query);
				return std::unique_ptr<type>(query);
			}
		}
	};

	template<>
	struct QueryBuilder<> : QueryBuilder<type_list<>, type_list<>, type_list<>, type_list<>>
	{
	};
}
