#pragma once
#include "type_list.h"

namespace hyecs
{
	template<typename T, typename... Us>
	struct type_indexed_array
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
		constexpr type_indexed_array(Ts... args) : arr{ (T)args... } {}

		constexpr type_indexed_array(const std::array<T, sizeof...(Us)>& arr) : arr{ arr } {}

		constexpr type_indexed_array(const std::array<T, sizeof...(Us)>& arr, type_list<Us...>) : arr{ arr } {}

		constexpr type_indexed_array(const std::array<T, sizeof...(Us)> arr, type_list<Us...>) : arr{ arr } {}

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

		constexpr auto data() { return arr.data(); }
	};

}
