#pragma once
#include "type_list.h"
#include "function_traits.h"

namespace hyecs
{
	template<typename T, typename Types>
	struct type_indexed_array;

	template<typename T, typename... Us>
	struct type_indexed_array<T, type_list<Us...>>
	{
	private:
		std::array<T, sizeof...(Us)> arr;

		template<typename U, size_t I>
		constexpr auto get()
		{
			if constexpr (std::is_same_v<U, std::tuple_element_t<I, std::tuple<Us...>>>)
				return arr[I];
			else
				return get<U, I + 1>();
		}

		template<typename U, size_t I>
		constexpr auto get() const
		{
			if constexpr (std::is_same_v<U, std::tuple_element_t<I, std::tuple<Us...>>>)
				return arr[I];
			else
				return get<U, I + 1>();
		}


	public:
		template<typename... Ts>
		constexpr type_indexed_array(Ts&&... args) : arr{ std::forward<Ts>(args)... } {}

		constexpr type_indexed_array(const std::array<T, sizeof...(Us)>& arr) : arr{ arr } {}


		template<typename U>
		constexpr auto get()
		{
			return get<U, 0>();
		}

		template<typename U>
		constexpr auto get() const
		{
			return get<U, 0>();
		}

		constexpr size_t index_of(const T& val) const
		{
			return std::find(arr.begin(), arr.end(), val) - arr.begin();
		}

		constexpr auto data() { return arr.data(); }

		constexpr auto operator[](size_t i) const { return arr[i]; }
		constexpr auto begin() const { return arr.begin(); }
		constexpr auto end() const { return arr.end(); }
		constexpr auto size() const { return arr.size(); }

		template<typename Callable, size_t I, typename V, typename... Vs>
		constexpr void for_each(Callable&& func) const
		{
			if constexpr (sizeof...(Vs) > 0)
			{
				func(arr[I], I, type_wrapper<V>{});
				for_each<Callable, I + 1, Vs...>(std::forward<decltype(func)>(func));
			}
			else if constexpr (sizeof...(Vs) == 0)
			{
				func(arr[I], I, type_wrapper<V>{});
			}
		}

		template<typename Callable>
		constexpr void for_each(Callable&& func) const
		{
			for_each<Callable,0, Us...>(std::forward<decltype(func)>(func));
		}

		constexpr const std::array<T, sizeof...(Us)>& as_array() const
		{
			return arr;
		}
		constexpr std::array<T, sizeof...(Us)>& as_array()
		{
			return arr;
		}

		template<typename Callable>
		constexpr auto cast(Callable&& func) const
		{
			return type_indexed_array<
				std::invoke_result_t<Callable, const T&, type_wrapper<nullptr_t>>,
				type_list<Us...>
			>{
				func(arr[type_list<Us...>::template index_of<Us>], type_wrapper<Us>{})...
			};
		}

	};

}
