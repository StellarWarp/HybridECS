#pragma once
// temporary replacement cpp20 constexpr string

namespace hyecs
{
    namespace internal
    {
        template<typename>
        struct fnv1a_traits;

        template<>
        struct fnv1a_traits<uint32_t>
        {
            using type = uint32_t;
            static constexpr uint32_t offset = 2166136261;
            static constexpr uint32_t prime = 16777619;
        };

        template<>
        struct fnv1a_traits<uint64_t>
        {
            using type = uint64_t;
            static constexpr uint64_t offset = 14695981039346656037ull;
            static constexpr uint64_t prime = 1099511628211ull;
        };
    }

    template<typename HashT = uint64_t>
    class basic_string_hash
    {
        static constexpr HashT offset = internal::fnv1a_traits<HashT>::offset;
        static constexpr HashT prime = internal::fnv1a_traits<HashT>::prime;
    public:
        HashT _hash;

        constexpr basic_string_hash() : _hash(0) {}

        constexpr basic_string_hash(HashT hash) : _hash(hash) {}

        constexpr basic_string_hash(const basic_string_hash& other) : _hash(other._hash) {}

        template<typename CharT>
        constexpr basic_string_hash(const CharT* str, size_t length) : _hash(0)
        {
            _hash = offset;
            for (size_t i = 0; i < length; ++i)
                _hash = (_hash ^ static_cast<HashT>(str[i])) * prime;
        }

        template<size_t Np1, typename CharT = char>
        constexpr basic_string_hash(const CharT (& str)[Np1]) : basic_string_hash(str, Np1 - 1) {}

        template<size_t Np1, typename CharT = char>
        constexpr basic_string_hash(basic_string_hash prev, const CharT (& str)[Np1]) : basic_string_hash(prev)
        {
            for (size_t i = 0; i < Np1 - 1; ++i)
                _hash = (_hash ^ static_cast<HashT>(str[i])) * prime;
        }

        constexpr operator HashT() const { return _hash; }
    };

    using string_hash = basic_string_hash<uint64_t>;
    using string_hash32 = basic_string_hash<uint32_t>;

    template<size_t Np1, typename CharT = char>
    class static_string
    {
    public:
        CharT _data[Np1];

        friend class static_string;

        constexpr static_string() : _data{0} {}

        constexpr static_string(const static_string& other) = default;

        template<size_t... I>
        constexpr static_string(const CharT* str, std::index_sequence<I...>) : _data{str[I]..., '\0'} {}

        constexpr static_string(const CharT* str) : static_string(str, std::make_index_sequence<Np1 - 1>()) {}

        template<size_t Mp1>
        constexpr static_string(const static_string<Mp1, CharT>& other) : static_string(other._data) {}


        constexpr size_t size() const { return Np1 - 1; }

        template<size_t start, size_t end>
        constexpr auto sub_string() const
        {
            return static_string<end - start + 1, CharT>(_data + start);
        }

        template<size_t Mp1>
        constexpr auto operator+(static_string<Mp1, CharT> other) const
        {
            static_string<Np1 + Mp1 - 1, CharT> result(*this);
            for (size_t i = 0; i < Mp1; ++i) result._data[Np1 - 1 + i] = other._data[i];
            return result;
        }

        template<size_t Mp1>
        constexpr auto operator+(const CharT (& str)[Mp1]) const
        {
            return *this + static_string<Mp1, CharT>(str);
        }

        constexpr CharT& operator[](size_t i) { return _data[i]; }

        constexpr uint64_t hash64() const { return string_hash(_data); }
        constexpr uint32_t hash32() const { return string_hash32(_data); }

        constexpr const CharT* begin() const { return _data; }

        constexpr const CharT* end() const { return _data + Np1 - 1; }

        constexpr const CharT* c_str() const { return _data; }

        constexpr operator const CharT*() const { return c_str(); }

    };

    template<size_t Np1, typename CharT>
    static_string(const CharT (&)[Np1]) -> static_string<Np1, CharT>;


    template<auto functor>
    requires std::invocable<decltype(functor)>
    constexpr auto make_static_string()
    {
        constexpr auto sv = functor();
        static_assert(sv.size() + 1 >= 1);
        return static_string<sv.size() + 1>(sv.data());
    }


}


template<size_t Np1, typename CharT>
std::ostream& operator<<(std::ostream& os, const hyecs::static_string<Np1, CharT>& str)
{
    os << str.c_str();
    return os;
}

template<size_t Np1, typename CharT>
struct std::formatter<hyecs::static_string<Np1, CharT>> : std::formatter<const CharT*>
{
auto format(const hyecs::static_string<Np1, CharT>& str, auto& ctx) const
{
    return std::formatter<const char*>::format(str, ctx);
}
};
