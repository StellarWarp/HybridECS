#pragma once

#include "container/container.h"
#include "ecs/type/entity.h"
#include "core/marco.h"

namespace hyecs
{
#if defined(_MSC_VER)
#define no_unique_address msvc::no_unique_address
#endif

    namespace details
    {
        struct version_value_pair_base
        {
            entity_version_t version{};

            ~version_value_pair_base()
            {
                version = null_entity.version();
            }
        };

        template<typename T>
        struct version_value_pair_default : version_value_pair_base
        {
            struct empty_t {};
            using value_type = std::conditional_t<std::is_void_v<T>, empty_t, T>;
            [[no_unique_address]] value_type _value;//not init
            value_type& value() { return _value; }

            using reference_type = value_type&;
            using pointer_type = value_type*;

            version_value_pair_default() = default;
            template<typename... Args>
            version_value_pair_default(entity_version_t v, Args&& ... args) :
                    version_value_pair_base{v},
                    _value(std::forward<Args>(args)...) {}
        };
    }

    // sparse container dont support for iteration
    // if iteration is needed, use dense container
    template<typename T,
            typename VersionValuePair = details::version_value_pair_default<T>
    > requires std::is_trivially_destructible_v<T> or std::is_void_v<T>
    class entity_sparse_table
    {
        //using T = void*;

        struct empty_t {};

        using value_type = std::conditional_t<std::is_void_v<T>, empty_t, T>;
        static constexpr bool is_map = !std::is_empty_v<value_type>;
        static constexpr bool is_set = std::is_empty_v<value_type>;

        struct entity_value_pair
        {
            entity e;
            [[no_unique_address]] value_type value;
        };
        static_assert(!std::is_void_v<T> || sizeof(entity_value_pair) == sizeof(entity));

        using version_value_pair = VersionValuePair;

        //static constexpr size_t page_byte_size = 4096;
        //static constexpr size_t page_capacity = page_byte_size / sizeof(entity_value_pair);
        static constexpr size_t max_page_byte_size = 2048;
        //choose a page_capacity of power of 2
        static constexpr size_t page_capacity = []() -> size_t
        {
            for (size_t capacity = 1;; capacity *= 2)
                if (capacity * sizeof(version_value_pair) > max_page_byte_size)
                    return capacity / 2;
            return 0;
        }();
        static constexpr size_t page_byte_size = page_capacity * sizeof(version_value_pair);

        static_assert(page_capacity != 0);
        static_assert(std::has_single_bit(page_capacity), "for optimization");

        using table_offset_t = uint32_t;

        struct table_location
        {
            uint32_t page_index;
            uint32_t page_offset;

            table_location(table_offset_t offset)
                    : page_index(offset / page_capacity), page_offset(offset % page_capacity) {}
        };


        struct page
        {
        private:
            version_value_pair data[page_capacity];
            uint8_t padding[page_byte_size - sizeof(data)];
        public:
            page() = default;//entity_version_t default inited

            version_value_pair& at(uint32_t offset)
            {
                assert(offset < page_capacity);
                return data[offset];
            }

            template<class... Args>
            version_value_pair& emplace(uint32_t offset, entity_version_t version, Args&& ... args)
            {
                auto& pair = at(offset);
                return *new(&pair) version_value_pair{version, std::forward<Args>(args)...};;
            }

            void erase(uint32_t offset)
            {
                auto& pair = at(offset);
                pair.~version_value_pair();
            }
        };

        static_assert(sizeof(page) == page_byte_size);

        vector<page*> pages;

        page* alloc_page()
        {
            return new page();
        }

        void dealloc_page(page* p)
        {
            delete p;
        }

    public:

        entity_sparse_table() {};

        entity_sparse_table(const entity_sparse_table& other)
        {
            pages.resize(other.pages.size());
            for (size_t i = 0; i < other.pages.size(); i++)
            {
                if (other.pages[i])
                {
                    auto p = pages[i] = alloc_page();
                    new(p) page(*other.pages[i]);
                }
            }
        }

        entity_sparse_table(entity_sparse_table&& other) = default;

        entity_sparse_table& operator=(const entity_sparse_table& other) = default;

        entity_sparse_table& operator=(entity_sparse_table&& other) = default;

        ~entity_sparse_table()
        {
            for (auto page: pages)
                dealloc_page(page);
        }

        void clear()
        {
            this->~entity_sparse_table();
            pages.clear();
        }

    private:


        page* page_for_index(uint32_t page_index)
        {
            if (pages.size() <= page_index)
                pages.resize(page_index + 1);
            if (!pages[page_index])
                pages[page_index] = alloc_page();
            return pages[page_index];
        }


        version_value_pair& pair_for_location(table_location loc)
        {
            return page_for_index(loc.page_index)->at(loc.page_offset);
        }

        template<typename U>
        struct add_ref_t { using type = U; };
        template<>
        struct add_ref_t<void> { using type = void; };
        template<typename U> using add_ref = typename add_ref_t<U>::type;
    public:

        template<typename... Args>
        add_ref<T> emplace(entity e, Args&& ... args)
        {
            //e is first argument
            auto [page_index, page_offset] = table_location(e.id());
            auto& pair = page_for_index(page_index)->emplace(page_offset, e.version(), std::forward<Args>(args)...);
            return pair.value();
        }

        void insert(const entity_value_pair& pair) requires is_map
        {
            emplace(pair.e, pair.value);
        }

        void insert(entity_value_pair&& pair) requires is_map
        {
            emplace(pair.e, std::move(pair.value));
        }

        void insert(entity e) requires is_set
        {
            emplace(e);
        }

        void erase(entity e)
        {
            auto [page_index, page_offset] = table_location(e.id());
            assert(pages.size() > page_index && pages[page_index]);
            pages[page_index]->erase(page_offset);
        }

        bool contains(entity e) const
        {
            auto [page_index, page_offset] = table_location(e.id());
            if (pages.size() <= page_index || !pages[page_index])
                return false;
            auto& pair = pages[page_index]->at(page_offset);
            //entity is reused without remove from table
            assert(pair.version == e.version() || pair.version == null_entity.version());
            return pair.version != null_entity.version();
        }

    private:
        using value_ref_t = version_value_pair::reference_type;
        using value_pointer_t = version_value_pair::pointer_type;

        const value_ref_t at_(entity e) const
        {
            auto [page_index, page_offset] = table_location(e.id());
            auto& pair = pages[page_index]->at(page_offset);
            assert(pair.version == e.version());
            return pair.value();
        }

    public:
        value_ref_t at(entity e) { return (value_ref_t) at_(e); }

        const value_ref_t at(entity e) const { return at_(e); }

        value_pointer_t find(entity e)
        {
            auto [page_index, page_offset] = table_location(e.id());
            auto& pair = pages[page_index]->at(page_offset);
            if (pair.version == null_entity.version())
                return nullptr;
            //not allow entity reuse without remove from table
            assert(pair.version == e.version());
            return &pair.value();
        }

        value_ref_t operator[](entity e)
        {
            auto& pair = pair_for_location(table_location(e.id()));
            //entity is reused without remove from table
            assert(pair.version == e.version() || pair.version == null_entity.version());
            return pair.value();
        }


    };


    class entity_sparse_set : protected entity_sparse_table<void>
    {
        using super = entity_sparse_table<void>;
    public:
        using super::super;
        using super::contains;
        using super::insert;
        using super::erase;
    };

    template<typename T>
    using entity_sparse_map = entity_sparse_table<T>;

    class entity_dense_set
    {
        entity_sparse_map<uint32_t> m_sparse;
        vector<entity> m_dense;

    public:

        entity_dense_set() = default;

        ~entity_dense_set() = default;

        void clear()
        {
            m_sparse.clear();
            m_dense.clear();
        }

        bool contains(entity e)
        {
            return m_sparse.contains(e);
        }

        void insert(entity e)
        {
            assert(not contains(e));
            uint32_t
                    index = (uint32_t)
                    m_dense.size();
            m_dense.push_back(e);
            m_sparse.emplace(e, index);
        }

        void erase(entity e)
        {
            assert(contains(e));
            uint32_t index = m_sparse.at(e);
            entity replaced = m_dense[index] = m_dense.back();
            m_dense.pop_back();
            m_sparse.at(replaced) = index;
            m_sparse.erase(e);
        }

        auto begin(this auto&& self) { return self.m_dense.begin(); }

        auto end(this auto&& self) { return self.m_dense.end(); }

        [[nodiscard]] const auto& entities() const { return m_dense; }

        [[nodiscard]] size_t size() const { return m_dense.size(); }

    };

    template<typename T>
    class entity_dense_map
    {
        entity_sparse_map<uint32_t> m_sparse;
        vector<std::pair<entity, T>> m_dense;

    public:

        entity_dense_map() = default;

        ~entity_dense_map() = default;

        bool contains(entity e) const
        {
            return m_sparse.contains(e);
        }

        auto find(entity e)
        {
            uint32_t* ptr = m_sparse.find(e);
            if (!ptr) return end();
            uint32_t index = *ptr;
            return m_dense.begin() + index;
        }

        const T& at(entity e) const
        {
            return m_dense[m_sparse.at(e)].second;
        }


        T& at(entity e)
        {
            return m_dense[m_sparse.at(e)].second;
        }

        std::pair<entity, T>& emplace(entity e, auto&& value)
        {
            assert(not contains(e));
            uint32_t index = (uint32_t) m_dense.size();
            auto& ref = m_dense.emplace_back(e, std::forward<decltype(value)>(value));
            m_sparse.emplace(e, index);
            return ref;
        }

        void insert(std::pair<entity, T>&& pair)
        {
            assert(not contains(pair.first));
            uint32_t
                    index = (uint32_t)
                    m_dense.size();
            m_dense.push_back(std::move(pair));
            m_sparse.emplace(pair.first, index);
        }

        void erase(entity e)
        {
            assert(contains(e));
            uint32_t index = m_sparse.at(e);
            auto& replaced_pair = m_dense[index] = m_dense.back();
            entity replaced = replaced_pair.first;
            m_dense.pop_back();
            m_sparse.at(replaced) = index;
            m_sparse.erase(e);
        }

        size_t size() const { return m_dense.size(); }

        auto begin(this auto&& self) { return self.m_dense.begin(); }

        auto end(this auto&& self) { return self.m_dense.end(); }

        auto entities() const { return m_dense; }

    };

//    template<
//            typename Key,
//            typename Value,
//            typename Hash = std::hash<Key>,
//            typename Equal = std::equal_to<Key>,
//            typename Alloc = std::allocator<std::pair<Key, Value>>
//    >
//    struct dense_map : unordered_map<Key, Value, Hash, Equal, Alloc>{};
//
//    template <
//            class T,
//            typename Hash = std::hash<T>,
//            typename Equal = std::equal_to<T>,
//            typename Alloc = std::allocator<T>
//    >
//    struct dense_set : unordered_set<T, Hash, Equal, Alloc>{};

    template<
            typename Key,
            typename Value,
            typename Hash = std::hash<Key>,
            typename Equal = std::equal_to<Key>,
            typename Alloc = std::allocator<std::pair<Key, Value>>
    >
    struct dense_map;

    template<
            class T,
            typename Hash = std::hash<T>,
            typename Equal = std::equal_to<T>,
            typename Alloc = std::allocator<T>
    >
    struct dense_set;

    template<typename Value>
    struct dense_map<entity, Value> : entity_dense_map<Value> {};

    template<>
    struct dense_set<entity> : entity_dense_set {};


    class raw_entity_dense_map
    {
        struct version_value_pair : details::version_value_pair_base
        {
            raw_segmented_vector::index_t index;
            void* ptr;

            using value_t = std::pair<void*&, raw_segmented_vector::index_t&>;

            value_t value()
            {
                return {ptr, index};
            }

            using reference_type = value_t;
            using pointer_type = value_t*;

            version_value_pair() = default;
            version_value_pair(entity_version_t v, void* ptr, raw_segmented_vector::index_t index) :
                    version_value_pair_base{v},
                    index(index),
                    ptr(ptr){}
        };


        entity_sparse_table<std::pair<void*, raw_segmented_vector::index_t>, version_value_pair> m_sparse;
        raw_segmented_vector m_dense;// element type <entity, value>

        using no_deleter = decltype([](void*) {});
    public:

        struct entity_value
        {
            entity& e;
            void* value;

            entity_value(void* ptr) :
                    e(*(entity*) ptr),
                    value((uint8_t*) ptr + sizeof(entity)) {}
        };

        raw_entity_dense_map(size_t value_size, size_t alignment) :
                m_sparse(),
                m_dense(sizeof(entity) + value_size, std::max(alignment, alignof(entity)))
        {
            assert(alignment <= sizeof(entity));//todo support for lager alignment
        }

        ~raw_entity_dense_map() = default; //warn that the need to delete value outside

        template<typename Deleter = no_deleter>
        void clear(Deleter deleter = {})
        {
            for (entity_value pair: m_dense)
            {
                deleter(pair.value);
            }
            m_sparse.clear();
            m_dense.clear();
        }

        void* allocate_value(entity e)
        {
            auto info = m_dense.allocate_value();
            m_sparse.emplace(e, info.first, info.second);
            entity_value ref(info.first);
            ref.e = e;
            return ref.value;
        }

        template<typename Mover = std::nullptr_t,
                typename Deleter = no_deleter,
                typename MovedDeleter = std::nullptr_t>
        void deallocate_value(entity e,
                              Mover&& mover = {},
                              Deleter&& deleter = {},
                              MovedDeleter&& moved_deleter = {}//default using deleter
        )
        {
            static constexpr bool has_mover = !std::is_same_v<Mover, std::nullptr_t>;
            static constexpr bool has_moved_deleter = !std::is_same_v<MovedDeleter, std::nullptr_t>;
            const auto& [ptr, index] = m_sparse.at(e);
            m_dense.deallocate_value(
                    ptr, index, [&]//remapping on swap
                    {
                        const entity& swap_remap = entity_value(ptr).e;
                        auto [s_ptr, s_index] = m_sparse.at(swap_remap);
                        s_ptr = ptr;
                        s_index = index;
                    },
                    [&]//move
                    {
                        if constexpr (has_mover)
                            return [&](void* dest, void* src)
                            {
                                auto dest_ = entity_value(dest);
                                auto src_ = entity_value(src);
                                dest_.e = src_.e;
                                mover(entity_value(dest).value, entity_value(src).value);
                            };
                        else
                            return std::nullptr_t{};
                    }(),
                    [&](void* o)//delete
                    {
                        deleter(entity_value(o).value);
                    },
                    [&]
                    {//moved deleter
                        if constexpr (has_moved_deleter)
                            return [&](void* o)//delete
                            {
                                moved_deleter(entity_value(o).value);
                            };
                        else
                            return std::nullptr_t{};
                    }()
            );
            m_sparse.erase(e);
        }

        size_t size() const { return m_dense.size(); }

        bool contains(entity e)
        {
            return m_sparse.contains(e);
        }

        void* at(entity e)
        {
            return entity_value(m_sparse.at(e).first).value;
        }


        class iterator : public raw_segmented_vector::iterator
        {
            using super = raw_segmented_vector::iterator;
        public:
            iterator(super&& it) : super(std::move(it)) {}

            entity_value operator*()
            {
                return entity_value(raw_segmented_vector::iterator::operator*());
            }
        };


        using end_iterator = raw_segmented_vector::end_iterator;

        iterator begin()
        {
            return iterator{m_dense.begin()};
        }

        end_iterator end()
        {
            return m_dense.end();
        }
    };

}
