#pragma once

#include "../delegate/multicast_delegate.h"
#include "../auto_reference/auto_reference.h"


namespace auto_delegate
{
    using namespace auto_reference;

    template<typename Func>
    using auto_delegate = delegate<Func, weak_reference<generic_ref_reflector, generic_ref_reflector>>;
    template<typename Func>
    using weak_delegate = delegate<Func, std::weak_ptr<void>>;
    template<typename Func>
    using shared_delegate = delegate<Func, std::shared_ptr<void>>;


#pragma region auto_delegate


    template<typename AutoRefProtocol = generic_ref_protocol,
            typename DelegateHandle = void,
            typename InverseHandle = delegate_handle_traits<DelegateHandle>::inverse_handle_type
    >
    class auto_delegate_container : public array_ref_charger<AutoRefProtocol, std::tuple<void*, InverseHandle>>
    {
        using tuple_t = std::tuple<void*, InverseHandle>;
        using super = array_ref_charger<AutoRefProtocol, std::tuple<void*, InverseHandle>>;
    public:
        using delegate_handle_t = delegate_handle_traits<DelegateHandle>::delegate_handle_type;
        using delegate_handle_t_ref = delegate_handle_traits<DelegateHandle>::delegate_handle_reference;
        using inverse_handle_t = delegate_handle_traits<DelegateHandle>::inverse_handle_type;
        using inverse_handle_t_ref = delegate_handle_traits<DelegateHandle>::inverse_handle_reference;
        static constexpr bool enable_delegate_handle = delegate_handle_traits<DelegateHandle>::enable_delegate_handle;

    private:


        delegate_handle_t bind_handle(inverse_handle_t_ref handle_ref)
        {
            if constexpr (requires { delegate_handle_t(this, &handle_ref); })
                return delegate_handle_t(this, &handle_ref);
            else if constexpr (requires { delegate_handle_t(& handle_ref); })
                return delegate_handle_t(&handle_ref);
            else
                return;
        }

        template<size_t Idx, class Tuple>
        constexpr size_t tuple_element_offset()
        {
            using ElementType = typename std::tuple_element<Idx, Tuple>::type;
            alignas(Tuple) char storage[sizeof(Tuple)];//for eliminate warning
            Tuple* t_ptr = reinterpret_cast<Tuple*>(&storage);
            ElementType* elem_ptr = &std::get<Idx>(*t_ptr);
            return reinterpret_cast<char*>(elem_ptr) - reinterpret_cast<char*>(t_ptr);
        }

    public:
        delegate_handle_t bind(const super::pointer_t& obj, void* invoker)
        {
            assert(obj);
            auto& [fn, handle_ref] = super::bind(obj);
            fn = invoker;
            return bind_handle(handle_ref);
        }

        delegate_handle_t bind(nullptr_t, void* invoker) requires enable_delegate_handle
        {
            auto& [fn, handle_ref] = super::bind(nullptr_t{});
            fn = invoker;
            return bind_handle(handle_ref);
        }

        void unbind(delegate_handle_t_ref handle) requires enable_delegate_handle
        {
            if constexpr (!enable_delegate_handle) return;
            inverse_handle_t* inv_handle = inverse_handle_t::get(&handle);
            assert(inv_handle);
            intptr_t offset = offsetof(typename super::element_t, data) + tuple_element_offset<1, tuple_t>();
            auto* h = (typename super::ref_handle_t*) (intptr_t(inv_handle) - offset);
            super::notify_reference_removed(h);
        }

        void unbind(const super::pointer_t& obj) requires (not enable_delegate_handle)
        {
            assert(obj);
            super::unbind(obj);
        }

        auto_delegate_container() = default;

        auto_delegate_container(auto_delegate_container&& other) noexcept
                : super(std::move(other))
        {
            if constexpr (enable_delegate_handle)
                if constexpr (delegate_handle_t::container_reference)
                {
                    for (auto& ref: *this)
                    {
                        ref.data.inv_handle.notify_container_moved(this);
                    }
                }
        }
    };

    template<typename Func>
    using multicast_auto_delegate = multicast_delegate<Func, auto_delegate_container<generic_ref_protocol, delegate_handle>>;

    template<typename Func>
    using multicast_auto_delegate_extern_ref = multicast_delegate<Func, auto_delegate_container<reference_reflector_ref_protocol>>;

#pragma endregion

#pragma region weak_delegate

    template<typename DelegateHandle = delegate_handle>
    class weak_delegate_container
    {
    public:
        using delegate_handle_t = delegate_handle_traits<DelegateHandle>::delegate_handle_type;
        using delegate_handle_t_ref = delegate_handle_traits<DelegateHandle>::delegate_handle_reference;
        using inverse_handle_t = delegate_handle_traits<DelegateHandle>::inverse_handle_type;
        using inverse_handle_t_ref = delegate_handle_traits<DelegateHandle>::inverse_handle_reference;
        static constexpr bool enable_delegate_handle = delegate_handle_traits<DelegateHandle>::enable_delegate_handle;


    private:
        struct delegate_object
        {
            std::weak_ptr<void> ptr;
            void* invoker;
            [[DELEGATE_no_unique_address]] inverse_handle_t inv_handle;
        };

        std::vector<delegate_object> objects;

        template<typename T, typename U>
        inline bool equals(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
        {
            return !t.owner_before(u) && !u.owner_before(t);
        }

    public:

        auto size() { return objects.size(); }

        bool empty() { return objects.empty(); }

        void clear() { objects.clear(); }

        delegate_handle_t bind(const std::weak_ptr<void>& obj, void* invoker)
        {
            assert(!obj.expired());
            auto& [ptr, fn, handle_ref] = objects.emplace_back(obj, invoker, inverse_handle_t{});
            if constexpr (requires { delegate_handle_t(this, &handle_ref); })
                return delegate_handle_t(this, &handle_ref);
            else if constexpr (requires { delegate_handle_t(& handle_ref); })
                return delegate_handle_t(&handle_ref);
            else
                return;
        }

        void unbind(delegate_handle_t_ref handle) requires enable_delegate_handle
        {
            if constexpr (!enable_delegate_handle) return;
            inverse_handle_t* inv_handle = inverse_handle_t::get(&handle);
            intptr_t handle_ref_element_offset = offsetof(delegate_object, inv_handle);
            auto* o = (delegate_object*) (intptr_t(inv_handle) - handle_ref_element_offset);
            int64_t index = o - objects.data();
            objects[index] = std::move(objects.back());
            objects.pop_back();
        }

        void unbind(const std::weak_ptr<void>& obj) requires (not enable_delegate_handle)
        {
            assert(!obj.expired());
            for (int i = 0; i < objects.size(); ++i)
            {
                if (equals(objects[i].handle.get(), obj))
                {
                    objects[i] = std::move(objects.back());
                    objects.pop_back();
                    return;
                }
            }
            assert(false);
        }


        weak_delegate_container() = default;

        weak_delegate_container(weak_delegate_container&& other) noexcept
                : objects(std::move(other.objects))
        {
            if constexpr (enable_delegate_handle)
                if constexpr (delegate_handle_t::reqiure_container)
                {
                    for (auto& ref: objects)
                    {
                        ref.inv_handle.notify_container_moved(this);
                    }
                }
        }

        class iterator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using value_type = delegate_object;
            using difference_type = std::ptrdiff_t;
            using pointer = value_type*;
            using reference = value_type&;
        private:
            std::vector<delegate_object>::iterator it;
            std::vector<delegate_object>::iterator end_it;
            std::vector<delegate_object>& vec;

        public:
            iterator(std::vector<delegate_object>& vec)
                    : it(vec.begin()), end_it(vec.end()), vec(vec) {}

            void operator++() { ++it; }

            void operator++(int) { operator++(); }

            reference operator*() { return *it; }

            bool operator==(const iterator& other) { return it == other.it; }

            bool operator==(nullptr_t)
            {
                if (it == end_it)
                {
                    vec.resize(end_it - vec.begin());
                    return true;
                }
                while (it->ptr.expired())
                {
                    end_it--;
                    if (it == end_it)
                    {
                        vec.resize(end_it - vec.begin());
                        return true;
                    }
                    *it = std::move(*end_it);
                }
                return false;
            }
        };

        iterator begin() { return iterator(objects); }

        nullptr_t end() { return {}; }
    };

    template<typename Func>
    using multicast_weak_delegate = multicast_delegate<Func, weak_delegate_container<>>;

#pragma endregion

}