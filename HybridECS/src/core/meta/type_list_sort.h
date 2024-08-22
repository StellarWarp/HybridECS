#pragma once

#include "type_list.h"
#include "type_hash.h"

namespace hyecs
{
//    namespace internal
//    {
//        template <typename... T, size_t... I>
//        constexpr auto static_sort_mapping(std::index_sequence < I...>)
//        {
//            struct indexed_type { size_t index; type_hash hash; };
//            constexpr std::array<indexed_type, sizeof...(T)> indexed =
//                    internal::sort(std::array<indexed_type, sizeof...(T)>{ indexed_type{ I, type_hash::of<T> }... },
//                                   [](auto& a, auto& b) { return a.hash < b.hash; });
//            using unsort_list = type_list<T...>;
//            return type_list<typename unsort_list::template get<indexed[I].index>...>{};
//        }
//    }
//
//    template<typename... T>
//    using sorted_type_list = decltype(internal::static_sort_mapping<T...>(std::make_index_sequence<sizeof...(T)>{}));
//
//    template<typename... T>
//    constexpr auto type_hash_array(type_list<T...>)
//    {
//        return type_indexed_array<type_hash, type_list<T...>>{ type_hash::of<T>... };
//    }
}
