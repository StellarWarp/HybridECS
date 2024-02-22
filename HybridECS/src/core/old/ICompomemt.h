#pragma once
#include "Reflection.h"
#include "Entity.h"
namespace hyecs
{

	template<typename T>
	struct Singleton
	{
		using type = std::remove_cvref_t<T>;
		using ref_type = std::conditional_t<std::is_reference_v<T>, T&, const T&>;
		ref_type value;

		operator ref_type() const
		{
			return value;
		}
	};

	template<typename T>
	struct is_singleton { static constexpr bool value = false; };

	template<typename T>
	struct is_singleton<Singleton<T>> { static constexpr bool value = true; };

	template<typename T, typename = void>
	struct component_self_resourse_allocation : std::false_type {};

	template<typename T>
	struct component_self_resourse_allocation<T, std::enable_if_t<T::resourse_allocation>> : std::true_type {};

	template<typename T, typename = void>
	struct in_place_delete : std::bool_constant<!(std::is_move_constructible_v<T>&& std::is_move_assignable_v<T>)> {};

	template<>
	struct in_place_delete<void> : std::false_type {};

	template<typename T>
	struct in_place_delete<T, std::enable_if_t<T::in_place_delete>>
		: std::true_type {};

	template<typename T, typename = void>
	struct reference_effective : std::false_type {};

	template<typename T>
	struct reference_effective<T, std::enable_if_t<T::reference_effective>> : std::true_type {};

	struct min_type_info
	{
		size_t size{};
		//void* (*constructor)(void*) {};
		void (*destructor)(void*) {};
		void* (*move_constructor)(void*, void*) {};
	};


	template<typename T>
	struct conponent_traits
	{
		using type = std::remove_cvref_t<T>;
		using ref_type = type&;
		using cref_type = const type&;
		static constexpr size_t size = std::is_empty_v<T> ? 0 : sizeof(T);
		static constexpr bool reference_effective = ::hyecs::reference_effective<T>::value;
		static constexpr bool in_place_delete = ::hyecs::in_place_delete<T>::value;
		static constexpr bool resourse_allocation = ::hyecs::component_self_resourse_allocation<T>::value || in_place_delete;
		static constexpr bool is_empty = std::is_empty_v<T>;

		static constexpr min_type_info get_min_type_info()
		{
			static_assert(!(conponent_traits<T>::reference_effective && conponent_traits<T>::in_place_delete),
				"duplication register component type");
			min_type_info info;

			info.size = conponent_traits<T>::size;
			if constexpr (conponent_traits<T>::resourse_allocation)
			{
				//info.constructor = [](void* ptr)->void* {return new(ptr) T(); };
				info.destructor = [](void* ptr) {((T*)ptr)->~T(); };
				if constexpr (!conponent_traits<T>::in_place_delete && !conponent_traits<T>::reference_effective)
					info.move_constructor = [](void* ptr, void* other)->void* {return new(ptr) T(std::move(*(T*)other)); };
			}
			return info;
		}

		static inline const min_type_info type_info = get_min_type_info();
	};


//#define conponent_config(cfg) static constexpr bool cfg = true
//
//
//	struct Test {
//		conponent_config(reference_effective);
//	};
//	void test() {
//
//		conponent_traits<Test>::resourse_allocation;
//		conponent_traits<Test>::in_place_delete;
//		conponent_traits<Test>::reference_effective;
//		conponent_traits<Test>::size;
//		conponent_traits<Test>::type_info;
//
//	}



}






