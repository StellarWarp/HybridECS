#pragma once

#include "stl_container.h"
#include "generic_container.h"
//#include "entt/dense_map.h"
//#include "entt/dense_set.h"
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
	using dense_map = unordered_map<Key, Value, Hash, Equal, Alloc>;

	template <
		class T,
		typename Hash = std::hash<T>,
		typename Equal = std::equal_to<T>,
		typename Alloc = std::allocator<T>
	>
	using dense_set = unordered_set<T, Hash, Equal, Alloc>;


	template<typename T, typename = void>
	struct has_iterator : std::false_type {};

	template<typename T>
	struct has_iterator<T, std::void_t<typename T::iterator>> : std::true_type {};

	template<typename T, typename = void>
	struct iterator_of
	{
		using type = T*;
	};

	template<typename T>
	struct iterator_of<T, std::enable_if_t<has_iterator<T>::value>>
	{
		using type = typename T::iterator;
	};


	template<typename T, typename Iter = typename iterator_of<T>::type>
	class sequence_ref
	{
	private:
		Iter m_begin;
		Iter m_end;
	public:
		using value_type = T;
		using reference = T&;
		using const_reference = T&;
		using size_type = size_t;

		using iterator = Iter;
		//using const_iterator = Iter;

		constexpr sequence_ref() noexcept : m_begin(nullptr), m_end(nullptr) {}

		constexpr sequence_ref(Iter _First_arg, Iter _Last_arg) noexcept
			: m_begin(_First_arg), m_end(_Last_arg) {}

		constexpr sequence_ref(initializer_list<T> list) noexcept
			: m_begin(list.begin()), m_end(list.end()) {}

		constexpr sequence_ref(std::initializer_list<T> list) noexcept
			: m_begin(list.begin()), m_end(list.end()) {}

		//template<typename... Args>
		//constexpr sequence_ref(T first, Args... args) noexcept
		//{
		//	std::initializer_list<T> list{ first, args... };
		//	m_begin = list.begin();
		//	m_end = list.end();
		//}

		[[nodiscard]] constexpr Iter begin() const noexcept {
			return m_begin;
		}

		[[nodiscard]] constexpr Iter end() const noexcept {
			return m_end;
		}

		[[nodiscard]] constexpr size_t size() const noexcept {
			return static_cast<size_t>(m_end - m_begin);
		}
	};

	template<typename T, typename Iter = typename iterator_of<T>::type>
	class sorted_sequence_ref : public sequence_ref<T, Iter>
	{
	public:
		using sequence_ref<T, Iter>::sequence_ref;
	};



};

