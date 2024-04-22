#pragma once
// temporary replacement cpp20 constexpr string

namespace hyecs
{
	namespace internal
	{
		template<typename>
		struct fnv1a_traits;

		template<>
		struct fnv1a_traits<uint32_t> {
			using type = uint32_t;
			static constexpr uint32_t offset = 2166136261;
			static constexpr uint32_t prime = 16777619;
		};

		template<>
		struct fnv1a_traits<uint64_t> {
			using type = uint64_t;
			static constexpr uint64_t offset = 14695981039346656037ull;
			static constexpr uint64_t prime = 1099511628211ull;
		};
	}

	template<typename HashT = uint64_t>
	class basic_string_hash
	{
		HashT m_hash;
		constexpr HashT offset = internal::fnv1a_traits<HashT>::offset;
		constexpr HashT prime = internal::fnv1a_traits<HashT>::prime;
	public:
		constexpr basic_string_hash() : m_hash(0) {}
		constexpr basic_string_hash(HashT hash) : m_hash(hash) {}
		constexpr basic_string_hash(const basic_string_hash& other) : m_hash(other.m_hash) {}
		template<typename CharT>
		constexpr basic_string_hash(const CharT* str, size_t length) : m_hash(0)
		{
			m_hash = offset;
			for (size_t i = 0; i < length; ++i)
				m_hash = (m_hash ^ static_cast<HashT>(str[i])) * prime;
		}
		template<size_t Np1, typename CharT = char>
		constexpr basic_string_hash(const CharT(&str)[Np1]) : basic_string_hash(str, Np1 - 1) {}
		template<size_t Np1, typename CharT = char>
		constexpr basic_string_hash(basic_string_hash prev, const CharT(&str)[Np1]) : basic_string_hash(prev)
		{
			for (size_t i = 0; i < Np1 - 1; ++i)
				m_hash = (m_hash ^ static_cast<HashT>(str[i])) * prime;
		}

		constexpr basic_string_hash
			constexpr operator HashT() const { return m_hash; }


	};

	using string_hash = basic_string_hash<uint64_t>;




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
			static_string<Np1 + Mp1 - 1, CharT> result(*this);
			for (size_t i = 0; i < Mp1; ++i) result.data[Np1 - 1 + i] = other.data[i];
			return result;
		}
		template<size_t Mp1>
		constexpr auto operator+(const CharT(&str)[Mp1]) const
		{
			return *this + static_string<Mp1, CharT>(str);
		}

		constexpr uint64_t hash() const { return string_hash(data); }

		const CharT* begin() const { return data; }
		const CharT* end() const { return data + Np1 - 1; }
		const CharT* c_str() const { return data; }
		operator const CharT* () const { return c_str(); }
	};
}