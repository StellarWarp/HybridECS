#pragma once

namespace hyecs
{

    template<typename T>
    concept dereferenceable = requires(T t) { *t; };

    template<typename T>
    concept addressable = requires(T t) { &t; };

//
//#define DELCATE_BINARY_OPERATOR_TEST(OP, NAME) \
//	template <typename T, typename U = T> \
//	struct has_operator_##NAME \
//	{ \
//		template <typename V, typename U> \
//		static auto test(V*, U*) -> decltype(std::declval<V>() OP std::declval<U>(), std::true_type()); \
//		template <typename, typename> \
//		static auto test(...) -> std::false_type; \
//		static constexpr bool value = decltype(test<T, U>(nullptr, nullptr))::value; \
//	}; \
//	template <typename T, typename U = T> \
//	constexpr bool has_operator_##NAME##_v = has_operator_##NAME<T, U>::value;
//
//	DELCATE_BINARY_OPERATOR_TEST(+, plus);
//	DELCATE_BINARY_OPERATOR_TEST(-, minus);
//	DELCATE_BINARY_OPERATOR_TEST(*, mul);
//	DELCATE_BINARY_OPERATOR_TEST(/, div);
//	DELCATE_BINARY_OPERATOR_TEST(%, mod);
//	DELCATE_BINARY_OPERATOR_TEST(+=, plus_assign);
//	DELCATE_BINARY_OPERATOR_TEST(-=, minus_assign);
//	DELCATE_BINARY_OPERATOR_TEST(*=, star_assign);
//	DELCATE_BINARY_OPERATOR_TEST(/=, slash_assign);
//	DELCATE_BINARY_OPERATOR_TEST(%=, mod_assign);
//	DELCATE_BINARY_OPERATOR_TEST(==, equal);
//	DELCATE_BINARY_OPERATOR_TEST(!=, not_equal);
//	DELCATE_BINARY_OPERATOR_TEST(<, less);
//	DELCATE_BINARY_OPERATOR_TEST(>, greater);
//	DELCATE_BINARY_OPERATOR_TEST(<=, less_equal);
//	DELCATE_BINARY_OPERATOR_TEST(>=, greater_equal);
//
//#define DELCATE_PREFIX_UNARY_OPERATOR_TEST(OP, NAME) \
//	template <typename T> \
//	struct has_operator_##NAME \
//	{ \
//		template <typename T> \
//		static auto test(T*) -> decltype(OP std::declval<T>(), std::true_type()); \
//		template <typename> \
//		static auto test(...) -> std::false_type; \
//		static constexpr bool value = decltype(test<T>(nullptr))::value; \
//	}; \
//	template <typename T> \
//	constexpr bool has_operator_##NAME##_v = has_operator_##NAME<T>::value;
//
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(--, pre_decrement);
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(++, pre_increment);
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(-, unary_minus);
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(+, unary_plus);
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(!, logical_not);
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(~, bitwise_not);
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(*, dereference);
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(&, address_of);
//	DELCATE_PREFIX_UNARY_OPERATOR_TEST(sizeof, sizeof);
//
//
//#define DELCATE_UNARY_OPERATOR_TEST(NAME, TEST_STATEMENT) \
//	template <typename T> \
//	struct has_operator_##NAME \
//	{ \
//		template <typename V> \
//		static auto test(V*) -> decltype(TEST_STATEMENT, std::true_type()); \
//		template <typename> \
//		static auto test(...) -> std::false_type; \
//		static constexpr bool value = decltype(test<T>(nullptr))::value; \
//	}; \
//	template <typename T> \
//	constexpr bool has_operator_##NAME##_v = has_operator_##NAME<T>::value;
//
//
//	DELCATE_UNARY_OPERATOR_TEST(post_decrement, std::declval<T>()--);
//	DELCATE_UNARY_OPERATOR_TEST(post_increment, std::declval<T>()++);
//	DELCATE_UNARY_OPERATOR_TEST(new, new T());
//	DELCATE_UNARY_OPERATOR_TEST(new_array, new T[1]);
//	DELCATE_UNARY_OPERATOR_TEST(delete, delete std::declval<T>());
//	DELCATE_UNARY_OPERATOR_TEST(delete_array, delete[] std::declval<T>());
//
//	
//
//#define DELCATE_MEMBER_FUNCTION_TEST(NAME) \
//	template <typename T> \
//	struct has_member_function_##NAME \
//	{ \
//		template <typename V> \
//		static auto test(V*) -> decltype(std::declval<V>().NAME(), std::true_type()); \
//		template <typename> \
//		static auto test(...) -> std::false_type; \
//		static constexpr bool value = decltype(test<T>(nullptr))::value; \
//	}; \
//	template <typename T> \
//	constexpr bool has_member_function_##NAME##_v = has_member_function_##NAME<T>::value;
//
//	DELCATE_MEMBER_FUNCTION_TEST(begin);
//	DELCATE_MEMBER_FUNCTION_TEST(end);
//	DELCATE_MEMBER_FUNCTION_TEST(size);
//	DELCATE_MEMBER_FUNCTION_TEST(empty);
//	DELCATE_MEMBER_FUNCTION_TEST(push_back);
//	DELCATE_MEMBER_FUNCTION_TEST(pop_back);
//	DELCATE_MEMBER_FUNCTION_TEST(push_front);
//	DELCATE_MEMBER_FUNCTION_TEST(pop_front);
//	DELCATE_MEMBER_FUNCTION_TEST(insert);
//	DELCATE_MEMBER_FUNCTION_TEST(erase);
//	DELCATE_MEMBER_FUNCTION_TEST(clear);
//	DELCATE_MEMBER_FUNCTION_TEST(resize);
//	DELCATE_MEMBER_FUNCTION_TEST(find);
//	DELCATE_MEMBER_FUNCTION_TEST(rfind);
//	DELCATE_MEMBER_FUNCTION_TEST(at);
//	DELCATE_MEMBER_FUNCTION_TEST(front);
//	DELCATE_MEMBER_FUNCTION_TEST(back);
//	DELCATE_MEMBER_FUNCTION_TEST(append);
//	DELCATE_MEMBER_FUNCTION_TEST(assign);
//	DELCATE_MEMBER_FUNCTION_TEST(compare);
//	DELCATE_MEMBER_FUNCTION_TEST(copy);
//	DELCATE_MEMBER_FUNCTION_TEST(copy_n);
//	DELCATE_MEMBER_FUNCTION_TEST(copy_if);
//	DELCATE_MEMBER_FUNCTION_TEST(copy_backward);
//	DELCATE_MEMBER_FUNCTION_TEST(transform);
//	DELCATE_MEMBER_FUNCTION_TEST(for_each);
//	DELCATE_MEMBER_FUNCTION_TEST(replace);
//	DELCATE_MEMBER_FUNCTION_TEST(replace_if);
//	DELCATE_MEMBER_FUNCTION_TEST(swap);
//	DELCATE_MEMBER_FUNCTION_TEST(reverse);
//	DELCATE_MEMBER_FUNCTION_TEST(unique);
//	DELCATE_MEMBER_FUNCTION_TEST(sort);
//	DELCATE_MEMBER_FUNCTION_TEST(merge);
//	DELCATE_MEMBER_FUNCTION_TEST(rotate);
//	DELCATE_MEMBER_FUNCTION_TEST(random_shuffle);
//	DELCATE_MEMBER_FUNCTION_TEST(fill);
//	DELCATE_MEMBER_FUNCTION_TEST(fill_n);
//	DELCATE_MEMBER_FUNCTION_TEST(count);
//	DELCATE_MEMBER_FUNCTION_TEST(count_if);
//	DELCATE_MEMBER_FUNCTION_TEST(find_if);
//
//	//const auto test = has_member_function_begin_v<std::vector<int>>;
//
//#define DECLATE_MEMBER_TYPE_TEST(TYPE_NAME) \
//	template <typename T> \
//	struct has_member_type_##TYPE_NAME \
//	{ \
//		template <typename T> \
//		static auto test(T*) -> decltype(std::declval<typename T::TYPE_NAME>(), std::true_type()); \
//		template <typename> \
//		static auto test(...) -> std::false_type; \
//		static constexpr bool value = decltype(test<T>(nullptr))::value; \
//	}; \
//	template <typename T> \
//	constexpr bool has_member_type_##TYPE_NAME##_v = has_member_type_##TYPE_NAME<T>::value;
//
//	DECLATE_MEMBER_TYPE_TEST(value_type);
//	DECLATE_MEMBER_TYPE_TEST(size_type);
//	DECLATE_MEMBER_TYPE_TEST(iterator);
//	DECLATE_MEMBER_TYPE_TEST(const_iterator);
//	DECLATE_MEMBER_TYPE_TEST(reverse_iterator);
//	DECLATE_MEMBER_TYPE_TEST(const_reverse_iterator);
//	DECLATE_MEMBER_TYPE_TEST(reference);
//	DECLATE_MEMBER_TYPE_TEST(const_reference);
//	DECLATE_MEMBER_TYPE_TEST(pointer);
//	DECLATE_MEMBER_TYPE_TEST(const_pointer);
//	DECLATE_MEMBER_TYPE_TEST(difference_type);
//	DECLATE_MEMBER_TYPE_TEST(key_type);
//	DECLATE_MEMBER_TYPE_TEST(mapped_type);
//	DECLATE_MEMBER_TYPE_TEST(key_compare);
//	DECLATE_MEMBER_TYPE_TEST(value_compare);
//	DECLATE_MEMBER_TYPE_TEST(hasher);
//	DECLATE_MEMBER_TYPE_TEST(key_equal);
//	DECLATE_MEMBER_TYPE_TEST(allocator_type);
//	DECLATE_MEMBER_TYPE_TEST(traits_type);
//	DECLATE_MEMBER_TYPE_TEST(allocator);
//	DECLATE_MEMBER_TYPE_TEST(allocator_traits);
//	DECLATE_MEMBER_TYPE_TEST(allocator_arg_t);
//	DECLATE_MEMBER_TYPE_TEST(allocator_arg);
//
//
//#define DECLATE_MEMBER_VARIABLE_TEST(VARIABLE_NAME) \
//	template <typename T> \
//	struct has_member_variable_##VARIABLE_NAME \
//	{ \
//		template <typename T> \
//		static auto test(T*) -> decltype(std::declval<T>().VARIABLE_NAME, std::true_type()); \
//		template <typename> \
//		static auto test(...) -> std::false_type; \
//		static constexpr bool value = decltype(test<T>(nullptr))::value; \
//	}; \
//	template <typename T> \
//	constexpr bool has_member_variable_##VARIABLE_NAME##_v = has_member_variable_##VARIABLE_NAME<T>::value;
//
//#define DECLATE_MEMBER_STATIC_VARIABLE_TEST(VARIABLE_NAME) \
//	template <typename T> \
//	struct has_member_static_variable_##VARIABLE_NAME \
//	{ \
//		template <typename T> \
//		static auto test(T*) -> decltype(T::VARIABLE_NAME, std::true_type()); \
//		template <typename> \
//		static auto test(...) -> std::false_type; \
//		static constexpr bool value = decltype(test<T>(nullptr))::value; \
//	}; \
//	template <typename T> \
//	constexpr bool has_member_static_variable_##VARIABLE_NAME##_v = has_member_static_variable_##VARIABLE_NAME<T>::value;




}


