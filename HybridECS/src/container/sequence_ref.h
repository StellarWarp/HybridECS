#pragma once
#include "../meta/meta_utils.h"
#include "stl_container.h"

namespace hyecs
{
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

	template <typename>
	struct is_array_iterator {
		static constexpr bool value = false;
	};

	template <typename Iter>
	struct is_vector_iterator
	{
		static constexpr bool test()
		{
			if constexpr (std::is_pointer_v<Iter>)
				return false;
			else if constexpr (std::is_same_v<typename vector<typename Iter::value_type>::iterator, Iter>
				|| std::is_same_v<typename vector<typename Iter::value_type>::const_iterator, Iter>)
				return true;
			return false;
		}

		static constexpr bool value = test();
	};

	template <typename Iter>
	static constexpr bool is_vector_or_array_iterator_v = is_vector_iterator<Iter>::value;
	template <typename Iter>
	static constexpr bool is_general_iterator_v =
		has_member_type_value_type_v<Iter> && has_operator_dereference_v<Iter> && !is_vector_or_array_iterator_v<Iter>;


	template <typename T, typename Iter = T*, typename = void>
	class sequence_ref;

#define HYECS_sequence_ref_template_guides(sequence_ref_class)\
	template <typename T>\
	sequence_ref_class(vector<T>) -> sequence_ref_class<T>;\
	template <typename T, size_t N>\
	sequence_ref_class(std::array<T, N>) -> sequence_ref_class<T>;\
	\
	template <typename Container, std::enable_if_t<has_member_type_value_type_v<Container>, int> = 0>\
	sequence_ref_class(Container&) -> sequence_ref_class<typename Container::value_type, Container>;\
	\
	template <typename Iter, std::enable_if_t<is_general_iterator_v<Iter>, int> = 0>\
	sequence_ref_class(Iter, Iter) -> sequence_ref_class<typename Iter::value_type, Iter>;\
	template <typename Iter, std::enable_if_t<std::is_pointer_v<Iter>, int> = 0>\
	sequence_ref_class(Iter, Iter) -> sequence_ref_class<std::decay_t<std::remove_pointer_t<Iter>>, Iter>;\
	template <typename Iter, std::enable_if_t<is_vector_or_array_iterator_v<Iter>, int> = 0>\
	sequence_ref_class(Iter, Iter) -> sequence_ref_class<typename Iter::value_type>;

	HYECS_sequence_ref_template_guides(sequence_ref)
	
	// template <typename T>
	// sequence_ref(vector<T>) -> sequence_ref<T>;
	// template <typename T, size_t N>
	// sequence_ref(std::array<T, N>) -> sequence_ref<T>;
	// template <typename Container, std::enable_if_t<has_member_type_value_type_v<Container>, int>  = 0>
	// sequence_ref(Container&) -> sequence_ref<typename Container::value_type, Container>;
	// template <typename Iter, std::enable_if_t<is_general_iterator_v<Iter>, int>  = 0>
	// sequence_ref(Iter, Iter) -> sequence_ref<typename Iter::value_type, Iter>;
	// template <typename Iter, std::enable_if_t<std::is_pointer_v<Iter>, int>  = 0>
	// sequence_ref(Iter, Iter) -> sequence_ref<std::decay_t<std::remove_pointer_t<Iter>>, Iter>;
	// template <typename Iter, std::enable_if_t<is_vector_or_array_iterator_v<Iter>, int>  = 0>
	// sequence_ref(Iter, Iter) -> sequence_ref<typename Iter::value_type>;

	template <typename T, typename SeqParam = const T*>
	class sequence_cref : public sequence_ref<const T, SeqParam>
	{
		using base = sequence_ref<const T, SeqParam>;

	public:
		using base::base;

		//sequence_cref(const base& other) : base(other)
		//{
		//}
	};

	HYECS_sequence_ref_template_guides(sequence_cref)

	// template <typename T>
	// sequence_cref(vector<T>) -> sequence_cref<T>;
	// template <typename T, size_t N>
	// sequence_cref(std::array<T, N>) -> sequence_cref<T>;
	// template <typename Container, std::enable_if_t<has_member_type_value_type_v<Container>, int>  = 0>
	// sequence_cref(Container&) -> sequence_cref<typename Container::value_type, Container>;
	// template <typename Iter, std::enable_if_t<is_general_iterator_v<Iter>, int>  = 0>
	// sequence_cref(Iter, Iter) -> sequence_cref<typename Iter::value_type, Iter>;
	// template <typename Iter, std::enable_if_t<std::is_pointer_v<Iter>, int>  = 0>
	// sequence_cref(Iter, Iter) -> sequence_cref<std::decay_t<std::remove_pointer_t<Iter>>, Iter>;
	// template <typename Iter, std::enable_if_t<is_vector_or_array_iterator_v<Iter>, int>  = 0>
	// sequence_cref(Iter, Iter) -> sequence_cref<typename Iter::value_type>;


	template <typename T, typename Iter>
	class sequence_ref<T, Iter, std::enable_if_t<std::is_convertible_v<decltype(*std::declval<Iter>()), T>>>
	{
	protected:
		// static_assert(!std::is_const_v<T>);
		using decay_type = std::decay_t<T>;

	public:
		using value_type = T;
		using reference = T&;
		using const_reference = T&;
		using size_type = size_t;

		using iterator = Iter;

	protected:
		struct empty_t
		{
			empty_t()
			{
			}

			empty_t(size_t)
			{
			}
		};

		//reqiured for non-contiguous iterators whith has no operator-()
		static const bool is_contiguous = has_operator_minus_v<iterator> && has_operator_plus_v<iterator, uint32_t>;
		using optional_size = std::conditional_t<is_contiguous, empty_t, size_t>;

	protected:
		iterator m_begin;
		iterator m_end;
		[[no_unique_address]] optional_size m_size;

	public:
		constexpr sequence_ref() noexcept : m_begin(nullptr), m_end(nullptr)
		{
		}

		//copy constructor
		constexpr sequence_ref(const sequence_ref& other) noexcept
			: m_begin(other.begin()), m_end(other.end()), m_size(other.size())
		{
		}
		//move constructor
		constexpr sequence_ref(sequence_ref&& other) noexcept
			: m_begin(other.begin()), m_end(other.end()), m_size(other.size())
		{
			other.m_begin = nullptr;
			other.m_end = nullptr;
			if constexpr (!is_contiguous)
				other.m_size = 0;
		}
		constexpr sequence_ref& operator=(const sequence_ref& other) noexcept
		{
			new(this) sequence_ref(other);
			return *this;
		}
		constexpr sequence_ref& operator=(sequence_ref&& other) noexcept
		{
			new(this) sequence_ref(std::move(other));
			return *this;
		}
		

		template <typename U, typename UIter>
		constexpr sequence_ref(const sequence_ref<U, UIter>& other) noexcept
			: m_begin(other.begin()), m_end(other.end()), m_size(other.size())
		{
		}

		//this include undefined behavior
		template <typename U, typename UIter>
		constexpr sequence_ref(sequence_ref&& other) noexcept
			: m_begin(other.begin()), m_end(other.end()), m_size(other.size())
		{
			other.m_begin = nullptr;
			other.m_end = nullptr;
			if constexpr (!std::is_same_v<optional_size, empty_t>)
				other.m_size = 0;
		}

		constexpr sequence_ref(iterator _First_arg, iterator _Last_arg) noexcept
			: m_begin(_First_arg), m_end(_Last_arg), m_size(_Last_arg - _First_arg)
		{
		}

		constexpr sequence_ref(iterator _First_arg, iterator _Last_arg, size_t _size) noexcept
			: m_begin(_First_arg), m_end(_Last_arg), m_size(_size)
		{
		}

		constexpr sequence_ref(const typename vector<decay_type>::iterator& begin, const typename vector<decay_type>::iterator& end) noexcept
			: m_begin(pointer_from_vector_iterator<decay_type>(begin)), m_end(pointer_from_vector_iterator<decay_type>(end))
		{
		}

		constexpr sequence_ref(const typename vector<decay_type>::const_iterator& begin, const typename vector<decay_type>::const_iterator& end) noexcept
			: m_begin(pointer_from_vector_iterator<decay_type>(begin)), m_end(pointer_from_vector_iterator<decay_type>(end))
		{
		}

		template <size_t N>
		constexpr sequence_ref(const typename std::array<decay_type, N>::iterator& begin, const typename std::array<decay_type, N>::iterator& end) noexcept
			: m_begin(pointer_from_array_iterator<decay_type, N>(begin)), m_end(pointer_from_array_iterator<decay_type, N>(end))
		{
		}

		template <size_t N>
		constexpr sequence_ref(const typename std::array<decay_type, N>::const_iterator& begin, const typename std::array<decay_type, N>::const_iterator& end) noexcept
			: m_begin(pointer_from_array_iterator<decay_type, N>(begin)), m_end(pointer_from_array_iterator<decay_type, N>(end))
		{
		}


		constexpr sequence_ref(initializer_list<decay_type> list) noexcept
			: m_begin(list.begin()), m_end(list.end())
		{
		}

		constexpr sequence_ref(std::initializer_list<decay_type> list) noexcept
			: m_begin(list.begin()), m_end(list.end())
		{
		}

		template <typename U = decay_type>
		constexpr sequence_ref(vector<U>& vec) noexcept
			: m_begin(vec.data()), m_end(vec.data() + vec.size())
		{
		}

		template <typename U = decay_type>
		constexpr sequence_ref(const vector<U>& vec) noexcept
			: m_begin(vec.data()), m_end(vec.data() + vec.size())
		{
		}

		template <typename U = decay_type, size_t N>
		constexpr sequence_ref(std::array<U, N>& arr) noexcept
			: m_begin(arr.data()), m_end(arr.data() + arr.size())
		{
		}

		template <typename U = decay_type, size_t N>
		constexpr sequence_ref(const std::array<U, N>& arr) noexcept
			: m_begin(arr.data()), m_end(arr.data() + arr.size())
		{
		}


		template <typename Container>
		constexpr sequence_ref(Container& container) noexcept
			: m_begin(container.begin()), m_end(container.end())
		{
			if constexpr (!std::is_same_v<optional_size, empty_t>)
				m_size = container.size();
		}

		[[nodiscard]] constexpr iterator begin() const noexcept
		{
			return m_begin;
		}

		[[nodiscard]] constexpr iterator end() const noexcept
		{
			return m_end;
		}

		[[nodiscard]] constexpr size_t size() const noexcept
		{
			if constexpr (is_contiguous)
				return m_end - m_begin;
			else
				return m_size;
		}

		const T& operator[](size_t index) const
		{
			static_assert(is_contiguous, "operator[] is only available for contiguous iterators");
			assert(index < size());
			return *(m_begin + index);
		}

		T& operator[](size_t index)
		{
			static_assert(is_contiguous, "operator[] is only available for contiguous iterators");
			assert(index < size());
			return *(m_begin + index);
		}

		operator initializer_list<decay_type>() const
		{
			if constexpr (is_contiguous)
				return {m_begin, m_end};
			else
				static_assert(is_contiguous, "operator initializer_list<T>() is only available for contiguous iterators");
		}

		auto as_const() const
		{
			if constexpr (is_contiguous)
				if constexpr (std::is_pointer_v<iterator>)
					return sequence_cref<T>(m_begin, m_end);
				else
					return sequence_cref<T, iterator>(m_begin, m_end);
			else
				return sequence_cref<T, iterator>(m_begin, m_end, m_size);
		}

		template <typename... Args, size_t... I>
		auto cast_tuple(std::index_sequence<I...>) { return std::tuple<Args...>{reinterpret_cast<Args>(*(m_begin + I))...}; }

		template <typename... Args>
		auto cast_tuple() { return cast_tuple<Args...>(std::index_sequence_for<Args...>{}); }

		// operator sequence_ref<const T>() const
		// {
		// 	return {m_begin, m_end};
		// }
	};


	template <typename T, typename Container>
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
			: m_container(container)
		{
		}

		sequence_cref<T, const Container> as_const() const
		{
			return {m_container};
		}


		[[nodiscard]] constexpr auto begin() const noexcept
		{
			return m_container.begin();
		}

		[[nodiscard]] constexpr auto end() const noexcept
		{
			return m_container.end();
		}

		[[nodiscard]] constexpr size_t size() const noexcept
		{
			return m_container.size();
		}
	};


	template <typename T, typename Option = T*>
	class sorted_sequence_ref : public sequence_ref<T, Option>
	{
	public:
		explicit sorted_sequence_ref(sequence_ref<T, Option> seq) : sequence_ref<T, Option>(seq)
		{
		};

		template <typename Iter>
		sorted_sequence_ref(const Iter& bgein, const Iter& end) : sequence_ref<T, Option>(bgein, end)
		{
		};
	};
	

	template <typename T, typename Option = const T*>
	class sorted_sequence_cref : public sequence_cref<T, Option>
	{
	public:
		explicit sorted_sequence_cref(sequence_cref<T, Option> seq) : sequence_cref<T, Option>(seq)
		{
		};

		template <typename Iter>
		sorted_sequence_cref(const Iter& bgein, const Iter& end) : sequence_cref<T, Option>(bgein, end)
		{
		};
	};

	// template <typename T>
	// sorted_sequence_cref(vector<T>) -> sorted_sequence_cref<T>;
	// template <typename T, size_t N>
	// sorted_sequence_cref(std::array<T, N>) -> sorted_sequence_cref<T>;
	// template <typename Container, std::enable_if_t<has_member_type_value_type_v<Container>, int>  = 0>
	// sorted_sequence_cref(Container&) -> sorted_sequence_cref<typename Container::value_type, Container>;
	// template <typename Iter, std::enable_if_t<has_member_type_value_type_v<Iter>, int>  = 0>
	// sorted_sequence_cref(Iter, Iter) -> sorted_sequence_cref<typename Iter::value_type, Iter>;
	// template <typename Iter, std::enable_if_t<std::is_pointer_v<Iter>, int>  = 0>
	// sorted_sequence_cref(Iter, Iter) -> sorted_sequence_cref<std::decay_t<std::remove_pointer_t<Iter>>, Iter>;

	HYECS_sequence_ref_template_guides(sorted_sequence_ref)
	HYECS_sequence_ref_template_guides(sorted_sequence_cref)


	template <typename T, size_t N>
	class sorted_array : public std::array<T, N>
	{
	public:
		template <typename... Args>
		sorted_array(Args&&... args) : std::array<T, N>{std::forward<Args>(args)...}
		{
			std::sort(this->begin(), this->end());
		}
	};

	template <typename Range, typename Compare = std::less<>>
	Range sort_range(Range&& range, Compare compare = Compare{})
	{
		auto sorted = range;
		std::sort(sorted.begin(), sorted.end(), compare);
		return sorted;
	}

	template <typename T>
	void get_sub_sequence_indices(
		sorted_sequence_cref<T> sequence,
		sorted_sequence_cref<T> sub_sequence,
		sequence_ref<uint32_t> out_index)
	{
		auto sub_iter = sub_sequence.begin();
		auto write_iter = out_index.begin();
		uint32_t index = 0;
		while (sub_iter != sub_sequence.end())
		{
			assert(index <= sequence.size());
			const auto& elem = *sub_iter;
			if (elem < sequence[index])
			{
				sub_iter++;
				write_iter++;
			}
			else if (elem == sequence[index])
			{
				*write_iter = index;
				write_iter++;
				sub_iter++;
				index++;
			}
			else
			{
				index++;
			}
		}
	}

	template <typename T>
	void get_sub_sequence_indices(
		sorted_sequence_cref<T> sequence,
		sequence_cref<T> sub_sequence,
		sequence_ref<uint32_t> out_index)
	{
		//search each element in sub_sequence in sequence
		for (size_t i = 0; i < sub_sequence.size(); i++)
		{
			auto iter = std::lower_bound(sequence.begin(), sequence.end(), sub_sequence[i]);
			if (iter != sequence.end() && *iter == sub_sequence[i])
			{
				out_index[i] = iter - sequence.begin();
			}
		}
	}
}
