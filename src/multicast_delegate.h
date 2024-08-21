#pragma once

#include <memory>
#include <ranges>
#include "auto_reference.h"
#include "function_traits.h"
#include "delegate.h"

#if defined(_MSC_VER)
#define no_unique_address msvc::no_unique_address
#endif

#pragma region delegate_handle

template<typename DelegateHandle>
struct delegate_handle_traits
{
public:
    using delegate_handle_type = DelegateHandle;
    using delegate_handle_reference = delegate_handle_type&;
    using inverse_handle_type = typename DelegateHandle::inverse_handle_type;
    using inverse_handle_reference = inverse_handle_type&;
    static const bool enable_delegate_handle = true;
    static const bool require_container = DelegateHandle::container_reference;

};

template<>
struct delegate_handle_traits<void>
{
private:
    struct empty_t
    {
    };
public:
    using delegate_handle_type = void;
    using delegate_handle_reference = empty_t;
    using inverse_handle_type = empty_t;
    using inverse_handle_reference = empty_t;
    static const bool enable_delegate_handle = false;
};

class delegate_handle
{
    friend class delegate_handle_ref;

protected:
    class delegate_handle_ref* delegate_object;

public:
    static constexpr bool container_reference = false;

    using inverse_handle_type = delegate_handle_ref;

    delegate_handle() : delegate_object() {}

    delegate_handle(delegate_handle_ref* obj);

    delegate_handle(const delegate_handle&) = delete;

    delegate_handle(delegate_handle&& other) noexcept;

    ~delegate_handle();

    void release();
};

class delegate_handle_ref
{
    friend class delegate_handle;

protected:
    delegate_handle* handle;
public:

    delegate_handle_ref() : handle() {}

    delegate_handle_ref(const delegate_handle_ref&) = delete;

    delegate_handle_ref(delegate_handle_ref&& other) noexcept: handle(other.handle)
    {
        if (handle)
        {
            handle->delegate_object = this;
            other.handle = nullptr;
        }
    }

    delegate_handle_ref& operator=(delegate_handle_ref&& other) noexcept
    {
        if (this == &other) return *this;
        this->~delegate_handle_ref();
        std::construct_at(this, std::move(other));
        return *this;
    }

    ~delegate_handle_ref() { if (handle) handle->delegate_object = nullptr; }

    static delegate_handle_ref* get(delegate_handle* h)
    {
        return h->delegate_object;
    }
};

delegate_handle::delegate_handle(delegate_handle_ref* obj) : delegate_object(obj) { obj->handle = this; }

delegate_handle::delegate_handle(delegate_handle&& other) noexcept: delegate_object(other.delegate_object)
{
    if (delegate_object)
    {
        delegate_object->handle = this;
        other.delegate_object = nullptr;
    }
}

delegate_handle::~delegate_handle()
{
    if (delegate_object) delegate_object->handle = nullptr;
}

void delegate_handle::release()
{
    assert(delegate_object);
    delegate_object->handle = nullptr;
}

template<typename Container>
class unique_delegate_handle_ref;

template<typename Container>
class unique_delegate_handle : public delegate_handle
{
    friend class unique_delegate_handle_ref<Container>;

    Container* container;
public:
    using inverse_handle_type = unique_delegate_handle_ref<Container>;

    static constexpr bool container_reference = true;

    unique_delegate_handle() : container() {}

    unique_delegate_handle(Container* container, inverse_handle_type* ref) : container(container),
                                                                             delegate_handle(ref) {}
    unique_delegate_handle( const unique_delegate_handle& other) = delete;
    unique_delegate_handle(unique_delegate_handle&& other) noexcept:
            delegate_handle(std::move(other)),
            container(other.container)
    {
        other.container = nullptr;
    }

    ~unique_delegate_handle() { if (container) container->unbind(*this); }

};

template<typename Container>
class unique_delegate_handle_ref : public delegate_handle_ref
{
    unique_delegate_handle<Container>* get_handle() { return static_cast<unique_delegate_handle<Container>*>(handle); }

public:
    unique_delegate_handle_ref() = default;
    unique_delegate_handle_ref(const unique_delegate_handle_ref&) = delete;
    unique_delegate_handle_ref(unique_delegate_handle_ref&& other) noexcept = default;
    void notify_container_moved(Container* new_container)
    {
        if (auto h = get_handle())
            h->container = new_container->container;
    }

    unique_delegate_handle_ref& operator=(unique_delegate_handle_ref&& other) noexcept
    {
        delegate_handle_ref::operator=(std::move(other));
        return *this;
    }

    ~unique_delegate_handle_ref() { if (auto h = get_handle()) h->container = nullptr; }

    static unique_delegate_handle_ref* get(unique_delegate_handle<Container>* h)
    {
        return static_cast<unique_delegate_handle_ref*>(h->delegate_object);
    }

};

#pragma endregion

template<typename = void>
class default_delegate_container
{
public:
//    using delegate_handle_t = delegate_handle_traits<DelegateHandle>::delegate_handle_type;
    using delegate_handle_t = unique_delegate_handle<default_delegate_container>;
    using delegate_handle_t_ref = delegate_handle_traits<delegate_handle_t>::delegate_handle_reference;
    using inverse_handle_t = delegate_handle_traits<delegate_handle_t>::inverse_handle_type;
    using inverse_handle_t_ref = delegate_handle_traits<delegate_handle_t>::inverse_handle_reference;
    static constexpr bool enable_delegate_handle = delegate_handle_traits<delegate_handle_t>::enable_delegate_handle;

private:
    struct delegate_object
    {
        void* ptr;
        void* mem_fn;
        [[no_unique_address]] inverse_handle_t inv_handle;
    };

    std::vector<delegate_object> objects;
public:
    auto size() { return objects.size(); }

    bool empty() { return objects.empty(); }

    void clear() { objects.clear(); }

    delegate_handle_t bind(void* obj, void* invoker)
    {
        auto& [ptr, fn, handle_ref] = objects.emplace_back(obj, invoker, inverse_handle_t{} );
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
        assert(inv_handle);
        intptr_t handle_ref_element_offset = offsetof(delegate_object, inv_handle);
        auto* o = (delegate_object*) (intptr_t(inv_handle) - handle_ref_element_offset);
        int64_t index = o - objects.data();
        objects[index] = std::move(objects.back());
        objects.pop_back();
    }

    void unbind(void* obj)requires (not enable_delegate_handle)
    {
        for (int i = 0; i < objects.size(); ++i)
        {
            if (objects[i].ptr == obj)
            {
                objects[i] = std::move(objects.back());
                objects.pop_back();
                return;
            }
        }
        assert(false);
    }

    using iterator = std::vector<delegate_object>::iterator;

    auto begin() { return objects.begin(); }

    auto end() { return objects.end(); }
};


template<typename Func, typename DelegateContainer = default_delegate_container<>>
class multicast_delegate;

template<typename DelegateContainer, typename Ret, typename... Args> requires (!std::is_rvalue_reference_v<Args> && ...)

class multicast_delegate<Ret(Args...), DelegateContainer>
{
    template<typename T, auto MemFunc>
    static Ret Invoker(void* obj, Args... args)
    {
        return (reinterpret_cast<T*>(obj)->*MemFunc)(std::forward<Args>(args)...);
    }

    template<typename T, auto Lambda>
    static Ret LambdaInvoker(void* obj, Args... args)
    {
        return Lambda(*(T*) obj, std::forward<Args>(args)...);
    }

    template<auto Callable>
    static Ret StaticInvoker(void*, Args... args)
    {
        return Callable(std::forward<Args>(args)...);
    }

    template<auto Lambda>
    static Ret StaticLambdaInvoker(void*, Args... args)
    {
        return Lambda(std::forward<Args>(args)...);
    }

    using invoker_t = Ret (*)(void*, Args...);

    template<class T>
    using mem_func_t = Ret(T::*)(Args...);

    using object_container_t = DelegateContainer;

    object_container_t objects;

    template<typename T_ptr>
    using value_of = typename std::pointer_traits<T_ptr>::element_type;
public:
    multicast_delegate() = default;

    multicast_delegate(const multicast_delegate&) = delete;

    multicast_delegate(multicast_delegate&& other) noexcept = default;

    auto size() { return objects.size(); }

    bool empty() { return objects.empty(); }

    void clear() { objects.clear(); }

public:

    using function_type = Ret(Args...);
    using function_pointer = Ret(*)(Args...);

    using delegate_handle_t = typename object_container_t::delegate_handle_t;
    using delegate_handle_t_ref = typename object_container_t::delegate_handle_t_ref;
    using inverse_handle_t = typename object_container_t::inverse_handle_t;
    using inverse_handle_t_ref = typename object_container_t::inverse_handle_t_ref;
    static const bool enable_delegate_handle = object_container_t::enable_delegate_handle;

    //bind methods
    template<auto MemFunc, typename T_ptr>
    delegate_handle_t bind(const T_ptr& obj)
    {
        return objects.bind(obj, (void*) Invoker<value_of<T_ptr>, MemFunc>);
    }

    //bind methods with signature inference
    template<typename T, mem_func_t<T> MemFunc, typename T_ptr>
    delegate_handle_t bind(const T_ptr& obj)
    {
        return objects.bind(obj, (void*) Invoker<value_of<T_ptr>, MemFunc>);
    }

    //bind object with lambda
    template<typename T_ptr, typename Callable>
    requires std::is_empty_v<Callable> && requires { LambdaInvoker<value_of<T_ptr>, std::decay_t<Callable>{}>; }
    delegate_handle_t bind(const T_ptr& obj, Callable&& func)
    {
        return objects.bind(obj, (void*) LambdaInvoker<value_of<T_ptr>, std::decay_t<Callable>{}>);
    }

    //bind callable object
    template<typename T_ptr>
    delegate_handle_t bind(const T_ptr& callable)
    requires
    std::same_as<std::invoke_result_t<std::decay_t<decltype(*callable)>, Args...>, Ret>
    {
        return objects.bind(callable, (void*)
                Invoker<value_of<T_ptr>, &std::decay_t<decltype(*callable)>::operator()>);
    }

    //bind a stateless callable object
    template<typename Callable>
    delegate_handle_t bind(Callable&& callable)
    requires enable_delegate_handle and
             std::same_as<std::invoke_result_t<std::decay_t<Callable>, Args...>, Ret> and
             std::is_empty_v<std::decay_t<Callable>>
    {
        return objects.bind(nullptr, (void*) StaticLambdaInvoker<Callable{}>);
    }

    //bind static function
    template<function_pointer StaticFunc>
    delegate_handle_t bind() requires enable_delegate_handle
    {
        return objects.bind(nullptr, (void*) StaticInvoker<StaticFunc>);
    }


    template<typename T_ptr>
    void
    unbind(const T_ptr& obj)
    requires requires{ typename std::pointer_traits<T_ptr>; }
             and (not enable_delegate_handle)
    {
        objects.unbind(obj);
    }

    void unbind(delegate_handle_t_ref handle) requires enable_delegate_handle { objects.unbind(handle); }

private:
    static Ret invoke_single(const auto& ptr, void* invoker, Args... args)
    {
        void* c;
        if constexpr (requires { static_cast<void*>(ptr); })
            c = static_cast<void*>(ptr);
        else if constexpr (requires { ptr.get(); })
            c = reinterpret_cast<void*>(ptr.get());
        else if constexpr (requires { ptr.lock().get(); })
        {
            return reinterpret_cast<invoker_t>(invoker)(
                    reinterpret_cast<void*>(ptr.lock().get()),
                    std::forward<Args>(args)...);
        } else
            static_assert(!std::same_as<int, int>);//ptr get method is not implemented
        return reinterpret_cast<invoker_t>(invoker)(c, std::forward<Args>(args)...);
    }

public:
    //no need to forward as the parameters types are already defined
    void invoke(Args... args) requires std::same_as<Ret, void>
    {
        for (auto&& [obj, mem_fn, _]: objects)
        {
            invoke_single(obj, mem_fn, std::forward<Args>(args)...);
        }
    }

    void operator()(Args... args) requires std::same_as<Ret, void>
    {
        invoke(std::forward<Args>(args)...);
    }

private:
    template<typename Invoker>
    class iterable
    {
        using object_iterator_t = object_container_t::iterator;
        using sentinel_t = decltype(std::declval<object_container_t&>().end());
        const object_iterator_t begin_it;
        const sentinel_t end_it;
        const Invoker& invoker;
    public:
        iterable(object_container_t& objects, const Invoker& invoker) :
                begin_it(objects.begin()), end_it(objects.end()),
                invoker(invoker) {}

        class iterator
        {
        public:
            using iterator_category = std::input_iterator_tag;
            using difference_type = std::ptrdiff_t;
            using value_type = Ret;
            using pointer = Ret*;
            using reference = Ret&;
        private:
            iterable* info;
            object_iterator_t it;
        public:
            iterator(iterable& info) : info(&info), it(info.begin_it) {}

            Ret operator*()
            {
                return info->invoker(*it);
            }

            void operator++() { ++it; }

            void operator++(int) { operator++(); }

            bool operator==(std::default_sentinel_t)
            {
                return it == info->end_it;
            }
        };

        iterator begin() { return iterator(*this); }

        std::default_sentinel_t end() { return {}; }
    };

    template<typename Invoker>
    iterable(object_container_t& objects, const Invoker& invoker) -> iterable<std::decay_t<Invoker>>;

public:

    template<typename Callable>
    requires (!std::same_as<Ret, void>)
    void invoke(Args... args, Callable&& result_proc)
    {
        auto lambda = [&](decltype(*std::declval<typename object_container_t::iterator>()) invoke_info) -> Ret
        {
            auto&& [obj, mem_fn, _] = invoke_info;
            return invoke_single(obj, mem_fn, std::forward<Args>(args)...);
        };
        result_proc(iterable(objects, lambda));
    }
};

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
        [[no_unique_address]] inverse_handle_t inv_handle;
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