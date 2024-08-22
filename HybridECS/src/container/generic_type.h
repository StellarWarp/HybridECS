#pragma once

#include "vaildref_map.h"
#include "../meta/meta_utils.h"
#include "type_hash.h"

namespace hyecs
{
    namespace generic
    {
        using default_constructor_ptr = void* (*)(void*);
        using copy_constructor_ptr_t = void* (*)(void*, const void*);
        using move_constructor_ptr_t = void* (*)(void*, void*);
        using destructor_ptr_t = void (*)(void*);

        template<typename T>
        constexpr void* default_constructor(void* addr)
        {
            return (T*) new(addr) T();
        }

        template<typename T>
        constexpr void* move_constructor(void* dest, void* src)
        {
            return (T*) new(dest) T(std::move(*(T*) src));
        }

        template<typename T>
        constexpr void* copy_constructor(void* dest, const void* src)
        {
            return (T*) new(dest) T(*(T*) src);
        }

        template<typename T>
        constexpr void destructor(void* addr)
        {
            ((T*) addr)->~T();
        }

        template<typename T>
        constexpr auto nullable_move_constructor()
        {
            if constexpr (std::is_move_constructible_v<T> || std::is_copy_constructible_v<T>)
                return move_constructor<T>;
            else
                return nullptr;
        }

        template<typename T>
        constexpr auto nullable_copy_constructor()
        {
            if constexpr (std::is_copy_constructible_v<T>)
                return copy_constructor<T>;
            else
                return nullptr;
        }

        template<typename T>
        constexpr auto nullable_destructor()
        {
            if constexpr (!std::is_trivially_destructible_v<T>)
                return destructor<T>;
            else
                return nullptr;
        }


        struct type_info
        {
            size_t size;
            size_t alignment;
            copy_constructor_ptr_t copy_constructor;
            move_constructor_ptr_t move_constructor;
            destructor_ptr_t destructor;
            type_hash hash;
            const char* name;

            type_info(size_t size,
                      size_t align,
                      copy_constructor_ptr_t copy_constructor,
                      move_constructor_ptr_t move_constructor,
                      destructor_ptr_t destructor,
                      type_hash hash,
                      const char* name)
                    : size(size),
                      alignment(align),
                      copy_constructor(copy_constructor),
                      move_constructor(move_constructor),
                      destructor(destructor),
                      hash(hash),
                      name(name)
            {
            }

            type_info() = delete;

            type_info(const type_info&) = delete;

            type_info(type_info&&) = delete;

            type_info& operator=(const type_info&) = delete;

            template<typename T>
            static const type_info& of()
            {
                static_assert(std::is_same_v<T, std::decay_t<T>>, "T must be a decayed type");
                const static type_info info{
                        std::is_empty_v<T> ? 0 : sizeof(T),
                        std::is_empty_v<T> ? 0 : alignof(T),
                        nullable_copy_constructor<T>(),
                        nullable_move_constructor<T>(),
                        nullable_destructor<T>(),
                        type_hash::of<T>(),
                        type_name<T>
                };
                return info;
            }
        };


        struct type_index_interface
        {

            size_t size(this auto&& self) requires (requires { self.m_size; })
            {
                return self.m_size;
            }

            size_t size(this auto&& self) requires (!requires { self.m_size; })

            && (requires { self.m_info; })
            {
                return self.m_info->size;
            }

            size_t alignment(this auto&& self) requires (requires { self.m_alignment; })
            {
                return self.m_alignment;
            }

            size_t alignment(this auto&& self) requires (!requires { self.m_alignment; })

            && (requires { self.m_info; })
            {
                return self.m_info->alignment;
            }

            bool is_empty(this auto&& self) requires (requires { self.size(); }) { return self.size() == 0; }

            type_hash hash(this auto&& self) requires (requires { self.m_hash; })
            {
                return self.m_hash;
            }

            type_hash hash(this auto&& self) requires (!requires { self.m_hash; })

            && (requires { self.m_info; })
            {
                return self.m_info->hash;
            }

            const char* name(this auto&& self) requires (requires { self.m_name; })
            {
                return self.m_name;
            }

            const char* name(this auto&& self) requires (!requires { self.m_name; })

            && (requires { self.m_info; })
            {
                return self.m_info->name;
            }


            auto copy_constructor_ptr(this auto&& self) requires (requires { self.m_copy_constructor; })
            {
                return self.m_copy_constructor;
            }

            auto copy_constructor_ptr(this auto&& self) requires (!requires { self.m_copy_constructor; })

            && (requires { self.m_info; })
            {
                return self.m_info->copy_constructor;
            }

            auto move_constructor_ptr(this auto&& self) requires (requires { self.m_move_constructor; })
            {
                return self.m_move_constructor;
            }

            auto move_constructor_ptr(this auto&& self)requires (!requires { self.m_move_constructor; })

            && (requires { self.m_info; })
            {
                return self.m_info->move_constructor;
            }

            auto destructor_ptr(this auto&& self) requires (requires { self.m_destructor; })
            {
                return self.m_destructor;
            }

            auto destructor_ptr(this auto&& self) requires (!requires { self.m_destructor; })

            && (requires { self.m_info; })
            {
                return self.m_info->destructor;
            }

        public:

            void* copy_constructor(this auto&& self, void* dest, const void* src) requires (requires { self.copy_constructor_ptr(); })
            {
                return self.copy_constructor_ptr()(dest, src);
            }

            void* move_constructor(this auto&& self, void* dest, void* src) requires (requires { self.move_constructor_ptr(); })
            {
                return self.move_constructor_ptr()(dest, src);
            }

            void destructor(this auto&& self, void* addr) requires (requires { self.destructor_ptr(); })
            {
                if (self.destructor_ptr() == nullptr) return;
                return self.destructor_ptr()(addr);
            }

            void destructor(this auto&& self, void* addr, size_t count) requires (requires { self.destructor_ptr(); self.size(); })
            {
                if (self.destructor_ptr() == nullptr) return;
                for (size_t i = 0; i < count; i++)
                {
                    self.destructor_ptr()(addr);
                    addr = (uint8_t*) addr + self.size();
                }
            }

            void destructor(this auto&& self, void* begin, void* end) requires (requires { self.destructor_ptr(); self.size(); })
            {
                if (self.destructor_ptr() == nullptr) return;
                for (; begin != end; (uint8_t*) begin + self.size())
                {
                    self.destructor_ptr()(begin);
                }
            }

            bool is_trivially_destructible(this auto&& self) requires (requires { self.destructor_ptr(); })
            {
                return self.destructor_ptr() == nullptr;
            }

            bool is_reallocable(this auto&& self) requires (requires { self.move_constructor_ptr(); })
            {
                return self.move_constructor_ptr() == nullptr;
            }

        };

        template<typename T1, typename T2>
        requires std::derived_from<std::decay_t<T1>, type_index_interface> &&
                 std::derived_from<std::decay_t<T2>, type_index_interface>
        std::strong_ordering operator<=>(T1&& a, T2&& b) requires (requires { a.hash(); b.hash(); })
        {
            return a.hash() <=> b.hash();
        }
//        template<typename T1, typename T2>
//        requires std::derived_from<std::decay_t<T1>,type_index_interface> &&
//                 std::derived_from<std::decay_t<T2>,type_index_interface>
//        bool operator ==(T1&& a, T2&& b) requires (requires { a.hash(); b.hash(); })
//        {
//            return a.hash() == b.hash();
//        }

        class type_index : public type_index_interface
        {
            friend struct type_index_interface;
            const type_info* m_info;

        public:
            //type_index() : m_info(nullptr) {}
            type_index(const type_info& info) : m_info(&info)
            {
            }

            bool operator==(const type_index& other) const noexcept
            {
                ASSERTION_CODE(
                        if (m_info != nullptr && other.m_info != nullptr && m_info != other.m_info)
                            assert(m_info->hash != other.m_info->hash); //multiple location of same type is not allowed
                )
                return m_info == other.m_info; //a type should only have one instance of type_info
            }
        };

        class type_index_container_cached : public type_index
        {
            friend struct type_index_interface;
            //cached info
            size_t m_size;
            copy_constructor_ptr_t m_copy_constructor;
            move_constructor_ptr_t m_move_constructor;
            destructor_ptr_t m_destructor;

        public:
            type_index_container_cached(const type_info& info)
                    : type_index(info),
                      m_size(info.size),
                      m_copy_constructor(info.copy_constructor),
                      m_move_constructor(info.move_constructor),
                      m_destructor(info.destructor)
            {
            }

            type_index_container_cached(type_index index)
                    : type_index(index),
                      m_size(index.size()),
                      m_copy_constructor(index.copy_constructor_ptr()),
                      m_move_constructor(index.move_constructor_ptr()),
                      m_destructor(index.destructor_ptr())
            {
            }

        };

        class constructor
        {
            type_index m_type;
            std::function<void*(void*)> m_constructor;

            // template <typename T>
            // struct lambda_forward_wrapper
            // {
            // 	using type = std::decay_t<T>;
            // 	uint8_t data[std::is_empty_v<type> ? 0 : sizeof(type)];
            // 	lambda_forward_wrapper(T&& val)
            // 	{
            // 		if constexpr (!std::is_empty_v<type>)
            // 			new(reinterpret_cast<std::decay_t<T>*>(data)) std::decay_t<T>(std::forward<T>(val));
            // 	}
            // 	T&& forward()
            // 	{
            // 		if constexpr (!std::is_empty_v<type>)
            // 			return std::forward<T>(*reinterpret_cast<std::decay_t<T>*>(data));
            // 		else return T{};
            // 	}
            // };

        public:
            constructor() : m_type(type_info::of<nullptr_t>())
            {
            }

            constructor(type_index type, std::function<void*(void*)> constructor)
                    : m_type(type), m_constructor(constructor)
            {
            }

            template<typename T, std::enable_if_t<!std::is_same_v<std::decay_t<T>, constructor>, int>  = 0>
            constructor(T&& src)
                    :m_type(type_info::of<std::decay_t<T>>())
            {
                if constexpr (!std::is_empty_v<std::decay_t<T>>)
                    m_constructor = [src = std::forward<T>(src)](void* ptr) mutable
                    {
                        return new(ptr) std::decay_t<T>(src);
                    };
            }

            void* operator()(void* ptr) const
            {
                if (m_constructor) return m_constructor(ptr);
                return ptr;
            }

            type_index type() const { return m_type; }

            template<typename T, typename... Args>
            static constructor of(Args&& ... args)
            {
                return constructor(type_info::of<T>(), [args...](void* ptr) { return new(ptr) T(args...); });
            }
        };


        class allocator
        {
            uint32_t alignment_p2;
        public:
            using value_type = std::byte;
            using const_pointer = const value_type*;
            using reference = value_type&;
            using const_reference = const value_type&;
            using difference_type = std::ptrdiff_t;

            allocator() : alignment_p2(std::bit_width(__STDCPP_DEFAULT_NEW_ALIGNMENT__) - 1) {}

            allocator(uint32_t alignment) : alignment_p2(std::bit_width(alignment) - 1) {
                assert(1 << alignment_p2 == alignment);
            }

            void set_alignment(uint32_t alignment)
            {
                alignment_p2 = std::bit_width(alignment) - 1;
                assert(1 << alignment_p2 == alignment);
            }

        private:
            template<auto align>
            struct alignas(align) align_type {};

            auto switch_allocator(auto&& func)
            {
#define ALIGN_FOR(p2) case p2: return func(std::allocator<align_type<(1<<p2)>>{}); break;
                switch (alignment_p2)
                {
                    ALIGN_FOR(0);
                    ALIGN_FOR(1);
                    ALIGN_FOR(2);
                    ALIGN_FOR(3);
                    ALIGN_FOR(4);
                    ALIGN_FOR(5);
                    ALIGN_FOR(6);
                    ALIGN_FOR(7);
                    ALIGN_FOR(8);
                    ALIGN_FOR(9);
                    ALIGN_FOR(10);
                    ALIGN_FOR(11);
                    ALIGN_FOR(12);
                    ALIGN_FOR(13);
                    default:
                        return func(std::allocator<align_type<4>>{});
                }
#undef ALIGN_FOR
            }
        public:
            value_type* allocate(std::size_t bytes)
            {
                auto n = bytes >> alignment_p2;
                assert(bytes == n << alignment_p2);
                return switch_allocator([&](auto&& alloc)
                                        {
                                            return reinterpret_cast<value_type*>(alloc.allocate(n));
                                        });
            }

            void deallocate(value_type* ptr, std::size_t bytes)
            {
                auto n = bytes >> alignment_p2;
                assert(bytes == n << alignment_p2);
                return switch_allocator([&](auto&& alloc)
                                        {
                                            using ptr_t = std::decay_t<decltype(alloc)>::value_type*;
                                            alloc.deallocate((ptr_t) ptr, n);
                                        });
            }

        };
    }


}

template<>
struct std::hash<hyecs::generic::type_index>
{
    size_t operator()(const hyecs::generic::type_index& index) const noexcept
    {
        return index.hash();
    }
};
