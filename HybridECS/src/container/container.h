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
		typename Alloc = std::allocator<std::pair<Key, Value>>
	>
	using dense_map = unordered_map<Key, Value, Hash, Equal, Alloc>;

	template <
		class T,
		typename Hash = std::hash<T>,
		typename Equal = std::equal_to<T>,
		typename Alloc = std::allocator<T>
	>
	using dense_set = unordered_set<T, Hash, Equal, Alloc>;


	//template<typename T, typename = void>
	//struct iterator_of
	//{
	//	using type = T*;
	//};

	//template<typename T>
	//struct iterator_of<T, std::enable_if_t<has_member_type_iterator_v<T>>>
	//{
	//	using type = typename T::iterator;
	//};
	

	template<typename T, typename Iter = T*, typename = void>
	class sequence_ref;

	template<typename T, typename Iter>
	class sequence_ref<T, Iter, std::enable_if_t<std::is_convertible_v<decltype(*std::declval<Iter>()), T>>>
	{
	public:
		using value_type = T;
		using reference = T&;
		using const_reference = T&;
		using size_type = size_t;

		using iterator = Iter;

	private:
		struct empty_t { empty_t() {} empty_t(size_t) {} };
		//reqiured for non-contiguous iterators whith has no operator-()
		using optional_size = std::conditional_t<has_operator_minus_v<iterator>, empty_t, size_t>;
	private:
		iterator m_begin;
		iterator m_end;
		[[no_unique_address]] optional_size m_size;
	public:

		constexpr sequence_ref() noexcept : m_begin(nullptr), m_end(nullptr) {}

		constexpr sequence_ref(iterator _First_arg, iterator _Last_arg) noexcept
			: m_begin(_First_arg), m_end(_Last_arg), m_size(_Last_arg - _First_arg) {}

		constexpr sequence_ref(iterator _First_arg, iterator _Last_arg, size_t _size) noexcept
			: m_begin(_First_arg), m_end(_Last_arg), m_size(_size) {}


		constexpr sequence_ref(initializer_list<T> list) noexcept
			: m_begin(list.begin()), m_end(list.end()) {}

		constexpr sequence_ref(std::initializer_list<T> list) noexcept
			: m_begin(list.begin()), m_end(list.end()) {}


		constexpr sequence_ref(vector<T>& vec) noexcept
			: m_begin(vec.data()), m_end(vec.data() + vec.size()) {}

		template<typename Container>
		constexpr sequence_ref(Container& container) noexcept
			: m_begin(container.begin()), m_end(container.end()) {
			if constexpr (!std::is_same_v<optional_size, empty_t>)
				m_size = container.size();
		}

		[[nodiscard]] constexpr iterator begin() const noexcept {
			return m_begin;
		}

		[[nodiscard]] constexpr iterator end() const noexcept {
			return m_end;
		}

		[[nodiscard]] constexpr size_t size() const noexcept {
			if constexpr (has_operator_minus_v<iterator>)
				return m_end - m_begin;
			else
				return m_size;
		}
	};

	template<typename T, typename Container>
	class sequence_ref<T, Container, std::enable_if_t<
		has_member_type_iterator_v<Container> && !has_operator_dereference_v<Container>>>
	{
	public:
		using value_type = T;
		using reference = T&;
		using const_reference = T&;
		using size_type = size_t;

		using iterator = typename Container::iterator;

	private:
		Container& m_container;
	public:
		constexpr sequence_ref(Container& container) noexcept
			: m_container(container) {}

		[[nodiscard]] constexpr iterator begin() const noexcept {
			return m_container.begin();
		}

		[[nodiscard]] constexpr iterator end() const noexcept {
			return m_container.end();
		}

		[[nodiscard]] constexpr size_t size() const noexcept {
			return m_container.size();
		}
	};



	template<class Container>
	auto make_sequence_ref(Container& container) {
		//if is vector
		if constexpr (std::is_same_v<Container, vector<typename Container::value_type>>)
			return sequence_ref<typename Container::value_type>(container);
		else
			return sequence_ref<typename Container::value_type, Container>(container);
	}

	template<class Iter, typename = std::enable_if_t<has_operator_minus_v<Iter>>>
	auto make_sequence_ref(Iter begin, Iter end) {
		static_assert(has_operator_minus_v<Iter>, "contiguous iterator must have operator-()");
		return sequence_ref<typename Iter::value_type, Iter>(begin, end);
	}

	template<class Iter>
	auto make_sequence_ref(Iter begin, Iter end, size_t size) {
		static_assert(!has_operator_minus_v<Iter>, "use make_sequence_ref(Iter begin, Iter end) for contiguous iterators");
		return sequence_ref<typename Iter::value_type, Iter>(begin, end, size);
	}


	template<typename T, typename Option = T*>
	class sorted_sequence_ref : public sequence_ref<T, Option>
	{
	public:
		using sequence_ref<T, Option>::sequence_ref;
	};



};

