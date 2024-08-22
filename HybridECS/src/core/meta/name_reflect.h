#pragma once

#include <type_traits>
#include <string_view>
#include "static_string.h"


#if defined __clang__ || defined __GNUC__
#   define INLINE_REFLECT_FUNCTION_NAME (__PRETTY_FUNCTION__)
#   define INLINE_REFLECT_FUNCTION_PREFIX '='
#   define INLINE_REFLECT_FUNCTION_SUFFIX ']'
#elif defined _MSC_VER
#   define INLINE_REFLECT_FUNCTION_NAME (__FUNCSIG__)
#   define INLINE_REFLECT_FUNCTION_PREFIX '<'
#   define INLINE_REFLECT_FUNCTION_SUFFIX '>'
#else
staic_assert(false, "Unsupported compiler");
#endif

namespace hyecs
{
    namespace details
    {
        template<typename T>
        constexpr auto raw_name_of()
        {
            std::string_view name = INLINE_REFLECT_FUNCTION_NAME;
            std::size_t start = name.find_first_not_of(' ',name.find_first_of(INLINE_REFLECT_FUNCTION_PREFIX) + 1);
            std::size_t end = name.find_last_of(INLINE_REFLECT_FUNCTION_SUFFIX);
            return name.substr(start, end - start);
        }

        template<auto Value>
        constexpr auto raw_name_of()
        {
            std::string_view name = INLINE_REFLECT_FUNCTION_NAME;
            std::size_t start = name.find_first_not_of(' ',name.find_first_of(INLINE_REFLECT_FUNCTION_PREFIX) + 1);
            std::size_t end = name.find_last_of(INLINE_REFLECT_FUNCTION_SUFFIX);
            return name.substr(start, end - start);
        }

        template<auto Value>
        constexpr auto enum_name_of()
        {
            std::string_view name = raw_name_of<Value>();
            std::size_t start = name.rfind("::") + 2;
            std::size_t end = name.size();
            return name.substr(start, end - start);
        }

        template <typename T>
        struct Wrapper//this fix for clang bug
        {
            T a;
            constexpr Wrapper(T value) : a(value) {}
        };

        template<Wrapper Field>
        constexpr auto field_name_of()
        {
            std::string_view name = raw_name_of<Field>();
            std::size_t start = name.rfind('{') + 1;
            std::size_t end = name.rfind('}');
            std::string_view name1 = name.substr(start, end - start);
            start = name1.rfind("::") + 2;
            end = name1.size();
            return name1.substr(start, end - start);
        }

        template<Wrapper Method>
        constexpr auto method_name_of()
        {
            std::string_view name = raw_name_of<Method>();
            std::size_t start = name.rfind('{') + 1;
            std::size_t end = name.rfind('}');
            std::string_view name1 = name.substr(start, end - start);
            end = name1.find_last_of('(');
            start = name1.substr(0, end).rfind("::") + 2;
            return name1.substr(start, end - start);
        }

    }

    template<typename T>
    constexpr auto raw_name_of()
    {
        return make_static_string<details::raw_name_of<T>>();
    }

    template<auto T>
    constexpr auto raw_name_of()
    {
        return make_static_string<details::raw_name_of<T>>();
    }

    template<auto T>
    constexpr auto enum_name_of()
    {
        return make_static_string<details::enum_name_of<T>>();
    }

    template<details::Wrapper T>
    constexpr auto field_name_of()
    {
        return make_static_string<details::field_name_of<T>>();
    }

    template<details::Wrapper T>
    constexpr auto method_name_of()
    {
        return make_static_string<details::method_name_of<T>>();
    }
}

#undef INLINE_REFLECT_FUNCTION_NAME
#undef INLINE_REFLECT_FUNCTION_PREFIX
#undef INLINE_REFLECT_FUNCTION_SUFFIX
