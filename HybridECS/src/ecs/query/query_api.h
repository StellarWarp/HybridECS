#pragma once
#include "ecs/type/entity.h"
#include "core/hyecs_core.h"

namespace hyecs::query_parameter
{
    namespace
    {
        struct wo_component_param {};
        struct descriptor_param {};
        struct filter_param : descriptor_param {};
        struct all_param : filter_param {};
        struct any_param : filter_param {};
        struct none_param : filter_param {};
        struct variant_param : descriptor_param {};
        struct optional_param : descriptor_param {};
        struct relation_param : descriptor_param {};
        struct entity_relation_param : descriptor_param {};
        struct entity_multi_relation_param : descriptor_param {};
    }

    namespace internal
    {
        template<typename T>
        concept decayed_type = std::is_same_v<std::decay_t<T>, T>;

        template<typename T>
        concept is_query_descriptor = std::is_base_of_v<descriptor_param, T>;
    }

    template<typename T>
            requires (internal::decayed_type<T> && !std::is_base_of_v<descriptor_param, T>)
    using read = const T&;

    template<typename T>
            requires (internal::decayed_type<T> && !std::is_base_of_v<descriptor_param, T>)
    using read_copy = T;

    template<typename T>
            requires (internal::decayed_type<T> && !std::is_base_of_v<descriptor_param, T>)
    using read_write = T&;

    template<typename T>
    requires (internal::decayed_type<T> && !std::is_base_of_v<descriptor_param, T>)
    struct write : wo_component_param
    {
        using type = T;
        void operator=(const T& other) { value = other; }
        write(T& val) : value(val) {}
    private:
        T& value;
    };

    //template<typename T>
    //using input = RO<T>;
    //template<typename T>
    //using output = WO<T>;
    //template<typename T>
    //using inout = RW<T>;

    template<internal::decayed_type... T>
    struct all_of : all_param { using types = type_list<T...>; };

    template<internal::decayed_type... T>
    struct any_of : any_param { using types = type_list<T...>; };

    template<internal::decayed_type... T>
    struct none_of : none_param { using types = type_list<T...>; };

    template<typename... T>
    struct variant : variant_param { using types = type_list<T...>; };

    template<typename T>
    struct optional : optional_param { using type = T; };

    template<typename...>
    struct relation;

    template<typename First, typename Second>
    struct relation<First, Second> : relation_param
    {
        static_assert(!std::is_same_v<First, entity> && !std::is_same_v<Second, entity>);
        using first = First;
        using second = Second;
    };

    namespace internal
    {
        template<typename T>
        struct is_filter : std::is_base_of<filter_param, T> {};
    }

    template<typename First, typename... QueryParam>
    struct relation<First(QueryParam...)> : entity_relation_param
    {
        using relation_tag = First;
        using query_param = type_list<QueryParam...>;
        using accessable = query_param::template filter_without<internal::is_filter>;
        entity e;
        accessable::tuple_t components;

    };

    template<typename...>
    struct multi_relation;

    template<typename First, typename... QueryParam>
    struct multi_relation<First(QueryParam...)> : entity_multi_relation_param
    {
        using relation_tag = First;
        using query_param = type_list<QueryParam...>;
        using accessable = query_param::template filter_without<internal::is_filter>;
        entity e;
        vector<typename accessable::tuple_t> components;
    };

    namespace
    {
        struct begin_rel_scope_param{};
        struct relation_ref_param{};
    }

    template<typename Tag, static_string Name>
    struct begin_rel_scope : begin_rel_scope_param{
        using relation_tag = Tag;
        static constexpr static_string name = Name;
    };
    struct end_rel_scope{};

    template<static_string Name>
    struct relation_ref : relation_ref_param{
        static constexpr static_string name = Name;
    };
}
