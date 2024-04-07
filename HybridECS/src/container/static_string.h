#pragma once
// temporary replacement cpp20 constexpr string

namespace hyecs
{
	template <size_t Np1, typename CharT = char>
	class static_string
	{
		CharT data[Np1];
		friend class static_string;
	public:
		constexpr static_string() : data{ 0 } {}
		constexpr static_string(const CharT(&str)[Np1]) : static_string(str, std::make_index_sequence<Np1 - 1>{}) {}
		template<size_t... I>
		constexpr static_string(const CharT* str, std::index_sequence < I...>) : data{ str[I]..., '\0' } {}

		template<size_t Mp1>
		constexpr static_string(const static_string<Mp1, CharT>& other) : static_string(other.data, std::make_index_sequence<Mp1 - 1>{}) {}

		constexpr size_t size() const { return Np1 - 1; }

		template<size_t start, size_t end>
		constexpr auto sub_string() const
		{
			return static_string<end - start + 1, CharT>(data + start, std::make_index_sequence<end - start>{});
		}

		template<size_t Mp1>
		constexpr auto operator+(static_string<Mp1, CharT> other) const
		{
			static_string<Np1 + Mp1 - 1, CharT> result(other);
			for (size_t i = 0; i < Mp1; ++i) result.data[Np1 - 1 + i] = other.data[i];
			return result;
		}
		template<size_t Mp1>
		constexpr auto operator+(const CharT(&str)[Mp1]) const
		{
			return *this + static_string<Mp1, CharT>(str);
		}

		constexpr uint64_t hash() const
		{
			constexpr uint64_t fnv_offset_basis = 14695981039346656037ULL;
			constexpr uint64_t fnv_prime = 1099511628211ULL;
			uint64_t hash = fnv_offset_basis;
			for (size_t i = 0; i < Np1 - 1; ++i)
			{
				hash ^= data[i];
				hash *= fnv_prime;
			}
			return hash;
		}

		const CharT* begin() const { return data; }
		const CharT* end() const { return data + Np1 - 1; }
		const CharT* c_str() const { return data; }
		operator const CharT* () const { return c_str(); }
	};
}