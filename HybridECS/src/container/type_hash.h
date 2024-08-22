#pragma once
#include "../lib/std_lib.h"
#include "../utils/utils.h"
#include "static_string.h"

namespace hyecs
{

	namespace internal
	{

#ifdef ENTT_STANDARD_CPP
#    define ENTT_NONSTD false
#else
#    define ENTT_NONSTD true
#    if defined __clang__ || defined __GNUC__
#        define ENTT_PRETTY_FUNCTION __PRETTY_FUNCTION__
#        define ENTT_PRETTY_FUNCTION_PREFIX '='
#        define ENTT_PRETTY_FUNCTION_SUFFIX ']'
#    elif defined _MSC_VER
#        define ENTT_PRETTY_FUNCTION __FUNCSIG__
#        define ENTT_PRETTY_FUNCTION_PREFIX '<'
#        define ENTT_PRETTY_FUNCTION_SUFFIX '>'
#    endif
#endif

		template <typename T>
		constexpr auto type_name_sv()
		{
#if defined ENTT_PRETTY_FUNCTION
			std::string_view pretty_function{ ENTT_PRETTY_FUNCTION };
			auto first = pretty_function.find_first_not_of(' ', pretty_function.find_first_of(ENTT_PRETTY_FUNCTION_PREFIX) + 1);
			auto value = pretty_function.substr(first, pretty_function.find_last_of(ENTT_PRETTY_FUNCTION_SUFFIX) - first);
			return value;
#else
			return std::string_view{ "" };
#endif
		}
		//wait for cpp 20 constexpr string
		template <typename T>
		constexpr auto type_static_name()
		{
			constexpr std::string_view sv = type_name_sv<T>();
			constexpr size_t len = sv.size();
			constexpr const char* name = sv.data();
			return static_string<len + 1>(name, std::make_index_sequence<len>{});;
		}

		//template <typename T>
		//const char* type_name()
		//{
		//	static const auto name = type_static_name<T>();
		//	return name.c_str();
		//}
	}

	template <typename T>
	constexpr auto type_name = internal::type_static_name<T>();

	class type_hash
	{
		uint64_t hash;
		constexpr type_hash(uint64_t hash) : hash(hash) {}
	public:
		template<typename T>
		static constexpr type_hash of(){ return type_name<T>.hash();}

        constexpr type_hash() : hash(0) {}
		constexpr type_hash(const type_hash& other) noexcept : hash(other.hash) {}
		constexpr type_hash(type_hash&& other) noexcept : hash(other.hash) { other.hash = 0; };
		constexpr type_hash& operator=(const type_hash& other) noexcept = default;
		constexpr bool operator==(const type_hash& other) const { return hash == other.hash; }
		constexpr bool operator<(const type_hash& other) const { return hash < other.hash; }
		constexpr bool operator>(const type_hash& other) const { return hash > other.hash; }
		constexpr operator uint64_t() const { return hash; }
	};

	namespace internal
	{
		template <typename... T, size_t... I>
		constexpr auto static_sort_mapping(std::index_sequence < I...>)
		{
			struct indexed_type { size_t index; type_hash hash; };
			constexpr std::array<indexed_type, sizeof...(T)> indexed =
				internal::sort(std::array<indexed_type, sizeof...(T)>{ indexed_type{ I, type_hash::of<T> }... },
					[](auto& a, auto& b) { return a.hash < b.hash; });
			using unsort_list = type_list<T...>;
			return type_list<typename unsort_list::template get<indexed[I].index>...>{};
		}
	}

	template<typename... T>
	using sorted_type_list = decltype(internal::static_sort_mapping<T...>(std::make_index_sequence<sizeof...(T)>{}));

	template<typename... T>
	constexpr auto type_hash_array(type_list<T...>)
	{
		return type_indexed_array<type_hash, type_list<T...>>{ type_hash::of<T>... };
	}

}

template <>
struct std::hash<hyecs::type_hash> {
	uint64_t operator()(const hyecs::type_hash& info) const
	{
		return info.operator uint64_t();
	}
};

