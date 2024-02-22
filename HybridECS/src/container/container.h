#pragma once

#include "stl_container.h"
#include "generic_container.h"
#include "entt/dense_map.h"
#include "entt/dense_set.h"
#include "vaildref_map.h"
#include "fixed_vector.h"
namespace hyecs
{

	template<
		typename Key,
		typename Value,
		typename Hash = std::hash<Key>,
		typename Equal = std::equal_to<Key>,
		typename Alloc = std::allocator<std::pair<const Key, Value>>
	>
	using dense_map = entt::dense_map<Key, Value, Hash, Equal, Alloc>;

	template <
		class T,
		typename Hash = std::hash<T>,
		typename Equal = std::equal_to<T>,
		typename Alloc = std::allocator<T>
	>
	using dense_set = entt::dense_set<T, Hash, Equal, Alloc>;


	template<typename T>
	class array_ref
	{
	private:
		const T* _First;
		const T* _Last;
	public:
		using value_type = T;
		using reference = const T&;
		using const_reference = const T&;
		using size_type = size_t;

		using iterator = const T*;
		using const_iterator = const T*;

		constexpr array_ref() noexcept : _First(nullptr), _Last(nullptr) {}

		constexpr array_ref(const T* _First_arg, const T* _Last_arg) noexcept
			: _First(_First_arg), _Last(_Last_arg) {}

		_NODISCARD constexpr T* begin() const noexcept {
			return _First;
		}

		_NODISCARD constexpr T* end() const noexcept {
			return _Last;
		}

		_NODISCARD constexpr size_t size() const noexcept {
			return static_cast<size_t>(_Last - _First);
		}
	};
}