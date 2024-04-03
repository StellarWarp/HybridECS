#pragma once
#include "../lib/std_lib.h"

namespace hyecs
{

	namespace internal
	{
		constexpr uint64_t static_fnv1a_hash(std::string_view str)
		{
			constexpr uint64_t fnv_offset_basis = 14695981039346656037ULL;
			constexpr uint64_t fnv_prime = 1099511628211ULL;
			uint64_t hash = fnv_offset_basis;
			for (auto c : str)
			{
				hash ^= c;
				hash *= fnv_prime;
			}
			return hash;
		}

		template <typename T>
		constexpr auto type_name_sv()
		{
			std::string_view name;
#if __GNUC__ || __clang__
			name = __PRETTY_FUNCTION__;
			std::size_t start = name.find('=') + 2;
			std::size_t end = name.size() - 1;
			name = std::string_view{ name.data() + start, end - start };
			start = name.rfind("::");
#elif _MSC_VER
			name = __FUNCSIG__;
			std::size_t start = name.find('<') + 1;
			std::size_t end = name.rfind(">(");
			name = std::string_view{ name.data() + start, end - start };
			start = name.rfind("::");
#endif
			return start == std::string_view::npos ? name : std::string_view{
					name.data() + start + 2, name.size() - start - 2
			};
		}

		template <typename T>
		constexpr auto static_type_hash()
		{
			return static_fnv1a_hash(type_name_sv<T>());
		}


		template <typename T, size_t N>
		struct type_name_holder
		{
			const char data[N + 1];
			constexpr type_name_holder(const char* str) : type_name_holder(str, std::make_index_sequence<N>{}) {}
			template <size_t... I>
			constexpr type_name_holder(const char* str, std::index_sequence<I...>) : data{ str[I]..., '\0' } {}
		};

		template <typename T>
		constexpr auto type_name()
		{
			constexpr std::string_view sv = type_name_sv<T>();
			constexpr size_t len = sv.size();
			constexpr const char* name = sv.data();
			constexpr type_name_holder<T, len> holder(name);
			return holder;
		}

		template <typename T>
		auto type_name(T t)
		{
			return type_name<T>();
		}
	}

	class static_type_info
	{

	};


	//template <typename T>
	//const char* type_name = internal::type_name<T>();

	template <typename T>
	constexpr std::string_view type_name_sv = internal::type_name_sv<T>();

	class type_hash
	{
		uint64_t hash;
		constexpr type_hash(uint64_t hash) : hash(hash) {}
	public:
		template<typename T>
		static constexpr type_hash hash_code = internal::static_type_hash<T>();

		constexpr type_hash(const type_hash& other) noexcept : hash(other.hash) {}
		constexpr type_hash(type_hash&& other) noexcept : hash(other.hash) { other.hash = 0; };
		constexpr type_hash& operator=(const type_hash& other) noexcept { hash = other.hash; return *this; }
		constexpr bool operator==(const type_hash& other) const { return hash == other.hash; }
		constexpr bool operator<(const type_hash& other) const { return hash < other.hash; }
		constexpr bool operator>(const type_hash& other) const { return hash > other.hash; }
		constexpr explicit operator uint64_t() const { return hash; }
	};

}

template <>
struct std::hash<hyecs::type_hash> {
	uint64_t operator()(const hyecs::type_hash& info) const
	{
		return info.operator uint64_t();
	}
};


namespace static_string_concat
{
	enum { StaticStringLiteral, StaticStringConcat };

	template <size_t N, int type>
	struct static_string;

	template <size_t N>
	struct static_string<N, StaticStringLiteral>
	{
		constexpr static_string(const wchar_t(&s)[N + 1]) : s(s)
		{
		}

		const wchar_t(&s)[N + 1];
	};

	template <size_t N>
	struct static_string<N, StaticStringConcat>
	{
		template <size_t N1, int T1, size_t N2, int T2>
		constexpr static_string(const static_string<N1, T1>& s1,
			const static_string<N2, T2>& s2)
			: static_string(s1,
				std::make_index_sequence<N1>{},
				s2,
				std::make_index_sequence<N2>{})
		{
		}

		template <size_t N1, int T1, size_t... I1, size_t N2, int T2, size_t... I2>
		constexpr static_string(const static_string<N1, T1>& s1,
			std::index_sequence<I1...>,
			const static_string<N2, T2>& s2,
			std::index_sequence<I2...>)
			: s{ s1.s[I1]..., s2.s[I2]..., '\0' }
		{
			static_assert(N == N1 + N2, "static_string length error");
		}

		wchar_t s[N + 1];
	};

	template <size_t N>
	constexpr auto string_literal(const wchar_t(&s)[N])
	{
		return static_string<N - 1, StaticStringLiteral>(s);
	}

	template <size_t N1, int T1, size_t N2, int T2>
	constexpr auto string_concat(const static_string<N1, T1>& s1,
		const static_string<N2, T2>& s2)
	{
		return static_string<N1 + N2, StaticStringConcat>(s1, s2);
	}
}

