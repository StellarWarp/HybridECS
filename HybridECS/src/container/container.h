#pragma once

#include "stl_container.h"
#include "generic_container.h"
//#include "entt/dense_map.h"
//#include "entt/dense_set.h"
#include "vaildref_map.h"
#include "fixed_vector.h"
#include "bit_set.h"
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
		using decay_type = std::decay_t<T>;
	public:
		using value_type = T;
		using reference = T&;
		using const_reference = T&;
		using size_type = size_t;

		using iterator = Iter;

	private:
		struct empty_t { empty_t() {} empty_t(size_t) {} };
		//reqiured for non-contiguous iterators whith has no operator-()
		static const bool is_contiguous = has_operator_minus_v<iterator> && has_operator_plus_v<iterator, uint32_t>;
		using optional_size = std::conditional_t<is_contiguous, empty_t, size_t>;
	private:
		iterator m_begin;
		iterator m_end;
		[[no_unique_address]] optional_size m_size;
	public:

		constexpr sequence_ref() noexcept : m_begin(nullptr), m_end(nullptr) {}
		//copy constructor
		constexpr sequence_ref(const sequence_ref& other) noexcept
			: m_begin(other.begin()), m_end(other.end()), m_size(other.size()) {}
		constexpr sequence_ref& operator=(const sequence_ref& other) noexcept {
			m_begin = other.begin();
			m_end = other.end();
			m_size = other.size();
			return *this;
		}
		//move constructor
		constexpr sequence_ref(sequence_ref&& other) noexcept
			: m_begin(other.begin()), m_end(other.end()), m_size(other.size()) {
			other = {};
		}

		template<typename U, typename UIter>
		constexpr sequence_ref(const sequence_ref<U, UIter>& other) noexcept
			: m_begin(other.begin()), m_end(other.end()), m_size(other.size()) {}

		//this include undefined behavior
		template<typename U, typename UIter>
		constexpr sequence_ref(sequence_ref&& other) noexcept
			: m_begin(other.begin()), m_end(other.end()), m_size(other.size()) {
			other.m_begin = nullptr;
			other.m_end = nullptr;
			if constexpr (!std::is_same_v<optional_size, empty_t>)
				other.m_size = 0;
		}

		constexpr sequence_ref(iterator _First_arg, iterator _Last_arg) noexcept
			: m_begin(_First_arg), m_end(_Last_arg), m_size(_Last_arg - _First_arg) {}

		constexpr sequence_ref(iterator _First_arg, iterator _Last_arg, size_t _size) noexcept
			: m_begin(_First_arg), m_end(_Last_arg), m_size(_size) {}


		constexpr sequence_ref(initializer_list<decay_type> list) noexcept
			: m_begin(list.begin()), m_end(list.end()) {}

		constexpr sequence_ref(std::initializer_list<decay_type> list) noexcept
			: m_begin(list.begin()), m_end(list.end()) {}

		template<typename U = decay_type>
		constexpr sequence_ref(vector<U>& vec) noexcept
			: m_begin(vec.data()), m_end(vec.data() + vec.size()) {}

		template<typename U = decay_type>
		constexpr sequence_ref(const vector<std::remove_cv_t<U>>& vec) noexcept
			: m_begin(vec.data()), m_end(vec.data() + vec.size()) {}

		template<typename U = decay_type, size_t N>
		constexpr sequence_ref(std::array<U, N>& arr) noexcept
			: m_begin(arr.data()), m_end(arr.data() + arr.size()) {}

		template<typename U = decay_type, size_t N>
		constexpr sequence_ref(const std::array<std::remove_cv_t<U>, N>& arr) noexcept
			: m_begin(arr.data()), m_end(arr.data() + arr.size()) {}


		template<typename Container>
		constexpr sequence_ref(Container& container) noexcept
			: m_begin(container.begin()), m_end(container.end())
		{
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
			if constexpr (is_contiguous)
				return m_end - m_begin;
			else
				return m_size;
		}

		const T& operator[](size_t index) const {
			static_assert(is_contiguous, "operator[] is only available for contiguous iterators");
			return *(m_begin + index);
		}

		T& operator[](size_t index) {
			static_assert(is_contiguous, "operator[] is only available for contiguous iterators");
			return *(m_begin + index);
		}

		operator initializer_list<decay_type>() const {
			if constexpr (is_contiguous)
				return { m_begin, m_end };
			else
				static_assert(is_contiguous, "operator initializer_list<T>() is only available for contiguous iterators");
		}

		auto as_const() const {
			if constexpr (is_contiguous)
				if constexpr (std::is_pointer_v<iterator>)
					return sequence_ref<const T, const T*>(m_begin, m_end);
				else
					return sequence_ref<const T, iterator>(m_begin, m_end);
			else
				return sequence_ref<const T, iterator>(m_begin, m_end, m_size);
		}


		//operator sequence_ref<const T>() const {
		//	return { m_begin, m_end };
		//}

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
		using container = Container;
	private:
		Container& m_container;

	public:
		constexpr sequence_ref(Container& container) noexcept
			: m_container(container) {}

		sequence_ref<const T, const Container> as_const() const {
			return { m_container };
		}


		[[nodiscard]] constexpr auto begin() const noexcept {
			return m_container.begin();
		}

		[[nodiscard]] constexpr auto end() const noexcept {
			return m_container.end();
		}

		[[nodiscard]] constexpr size_t size() const noexcept {
			return m_container.size();
		}
	};



	template<class Container>
	auto make_sequence_ref(Container& container) {
		//if is vector
		using value_type = typename Container::value_type;
		using vector_type = vector<value_type>;
		using const_vector_type = const vector<value_type>;
		if constexpr (std::is_same_v<vector_type, Container>)//modify for const vector
			return sequence_ref<value_type>(container);
		else if constexpr (std::is_same_v<const_vector_type, Container>)
			return sequence_ref<const value_type>(container);
		else
			return sequence_ref<value_type, Container>(container);
	}

	template<class Iter, typename = std::enable_if_t<has_operator_minus_v<Iter>>>
	auto make_sequence_ref(Iter begin, Iter end) {
		static_assert(has_operator_minus_v<Iter>, "contiguous iterator must have operator-()");
		using value_type = typename Iter::value_type;
		using vector_iter = typename vector<value_type>::iterator;
		using vector_citer = typename vector<value_type>::const_iterator;
		if constexpr (std::is_same_v<Iter, vector_iter>)
			return sequence_ref<value_type>(pointer_from_vector_iterator<value_type>(begin), pointer_from_vector_iterator<value_type>(end));
		else if constexpr (std::is_same_v<Iter, vector_citer>)
			return sequence_ref<const value_type>(pointer_from_vector_iterator<value_type>(begin), pointer_from_vector_iterator<value_type>(end));
		else
			return sequence_ref<typename Iter::value_type, Iter>(begin, end);
	}


	template<class Iter>
	auto make_sequence_ref(Iter begin, Iter end, size_t size) {
		static_assert(!has_operator_minus_v<Iter>, "use make_sequence_ref(Iter begin, Iter end) for contiguous iterators");
		return sequence_ref<typename Iter::value_type, Iter>(begin, end, size);
	}

	template<typename T, typename SeqParam>
	auto sequence_ref_const_convert(sequence_ref<T, SeqParam> seq) {
		return sequence_ref<const T, const SeqParam>(seq);
	}


	template<typename T, typename Option = T*>
	class sorted_sequence_ref : public sequence_ref<T, Option>
	{
	public:
		using sequence_ref<T, Option>::sequence_ref;
	};




	template<typename T, size_t N>
	class sorted_array : public std::array<T, N>
	{
	public:
		template<typename... Args>
		sorted_array(Args&&... args) : std::array<T, N>{std::forward<Args>(args)...}
		{
			std::sort(this->begin(), this->end());
		}
	};
}



