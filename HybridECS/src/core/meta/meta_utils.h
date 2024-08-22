#pragma once
#include "function_traits.h"
#include "type_indexed_array.h"
#include "others.h"
#include "operator_test.h"
#include "name_reflect.h"
#include "type_hash.h"


namespace hyecs
{
    struct non_copyable
    {
        non_copyable() = default;
        non_copyable(const non_copyable&) = delete;
        non_copyable& operator=(const non_copyable&) = delete;
        non_copyable(non_copyable&&) = default;
        non_copyable& operator=(non_copyable&&) = default;
    };

    struct non_movable
    {
        non_movable() = default;
        non_movable(const non_movable&) = delete;
        non_movable& operator=(const non_movable&) = delete;
        non_movable(non_movable&&) = delete;
        non_movable& operator=(non_movable&&) = delete;
    };
}





