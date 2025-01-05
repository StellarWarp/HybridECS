#pragma once

#include <cassert>
#include "ut.hpp"

template<
    auto IDENT,
    bool AllowMultiDestroy = false,
    auto NewObjectCallback = nullptr,
    auto DestroyObjectCallback = nullptr>
struct managed_object_tester
{
    static constexpr bool has_new_object = !std::is_same_v<std::decay_t<decltype(NewObjectCallback)>, nullptr_t>;
    static constexpr bool has_destroy_object = !std::is_same_v<std::decay_t<decltype(DestroyObjectCallback)>, nullptr_t>;

    static inline size_t object_counter = 0;
    static inline size_t from_default_counter = 0;
    static inline size_t from_custom_counter = 0;
    static inline size_t from_copy_counter = 0;
    static inline size_t from_move_counter = 0;
    static inline size_t construct_call = 0;
    static inline size_t move_call = 0;
    static inline size_t copy_call = 0;
    static inline size_t destroy_call = 0;
    static inline size_t id_counter = 0;

    enum state_flags
    {
        none = 0,
        construct_from_custom = 1 << 0,
        construct_from_copy = 1 << 1,
        construct_from_move = 1 << 2,
        move_assigned = 1 << 3,
        copy_assigned = 1 << 4,
        destroyed = 1 << 5,
        moved = 1 << 6,
        moved_dynamic = 1 << 7,
        default_construct = 1 << 8,
        multiple_destroyed = 1 << 9
    };

    static void check_log()
    {
        boost::ut::log << "object_counter: " << object_counter << "\n"
                << "from_default_counter: " << from_default_counter << "\n"
                << "from_custom_counter: " << from_custom_counter << "\n"
                << "from_copy_counter: " << from_copy_counter << "\n"
                << "from_move_counter: " << from_move_counter << "\n"
                << "construct_call: " << construct_call << "\n"
                << "move_call: " << move_call << "\n"
                << "copy_call: " << copy_call << "\n"
                << "destroy_call: " << destroy_call << "\n";
        fflush(stdout);
    }

    void new_object_callback()
    {
        if constexpr (has_new_object)
            NewObjectCallback(*this);
    }

    void destroy_object_callback()
    {
        if constexpr (has_destroy_object)
            DestroyObjectCallback(*this);
    }

    int a, b, c, d;
    size_t id;
    state_flags flags = none;

    friend state_flags& operator|=(state_flags& lhs, state_flags rhs)
    {
        return lhs = static_cast<state_flags>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    friend state_flags operator|(state_flags lhs, state_flags rhs)
    {
        return static_cast<state_flags>(static_cast<int>(lhs) | static_cast<int>(rhs));
    }

    managed_object_tester()
        : a(0), b(0), c(0), d(0)
    {
        id = id_counter++;
        object_counter++;
        construct_call++;
        flags |= default_construct;
        from_default_counter++;
        new_object_callback();
    }

    managed_object_tester(int a, int b, int c, int d) : a(a), b(b), c(c), d(d)
    {
        id = id_counter++;
        object_counter++;
        construct_call++;
        flags |= construct_from_custom;
        from_custom_counter++;
        new_object_callback();
    }

    managed_object_tester(const managed_object_tester& other) : a(other.a), b(other.b), c(other.c), d(other.d)
    {
        id = id_counter++;
        object_counter++;
        copy_call++;
        flags |= construct_from_copy;
        from_copy_counter++;
        new_object_callback();
    }

    managed_object_tester(managed_object_tester&& other) : a(other.a), b(other.b), c(other.c), d(other.d)
    {
        id = id_counter++;
        object_counter++;
        if constexpr (!hyecs::DESTROY_MOVED_COMPONENTS)
        {
            if (hyecs::generic::g_debug_dynamic_move_signature)
            {
                object_counter--;
                other.flags |= moved_dynamic;
            }
        }
        move_call++;
        flags |= construct_from_move;
        other.flags |= moved;
        from_move_counter++;
        new_object_callback();
    }

    managed_object_tester& operator=(const managed_object_tester& other)
    {
        flags |= copy_assigned;
        a = other.a;
        b = other.b;
        c = other.c;
        d = other.d;
        return *this;
    }

    ~managed_object_tester()
    {
        if constexpr (!hyecs::DESTROY_MOVED_COMPONENTS)
            assert((flags & moved_dynamic) == 0);
        assert(object_counter != 0);
        if (flags & destroyed)
        {
            flags |= multiple_destroyed;
            if (!AllowMultiDestroy)
                assert(false);
        }
        flags |= destroyed;
        a = b = c = d = 0;
        object_counter--;
        destroy_call++;
        if (flags & default_construct)
            from_default_counter--;
        else if (flags & construct_from_custom)
            from_custom_counter--;
        else if (flags & construct_from_copy)
            from_copy_counter--;
        else if (flags & construct_from_move)
            from_move_counter--;
        destroy_object_callback();
    }

    bool operator==(const managed_object_tester& o) const
    {
        return a == o.a && b == o.b && c == o.c && d == o.d;
    }
};

constexpr size_t managed_object_tester_size = sizeof(managed_object_tester<0>);
