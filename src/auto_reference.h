#pragma once

#include <type_traits>
#include <assert.h>
#include <vector>
#include "offset_ptr.h"

struct referencer_interface
{
    constexpr virtual void notify_reference_removed(void* reference_handel_address) = 0;
};


template<typename To,typename From>
To interconvertible_pointer_cast(From obj)
{
    auto ptr = static_cast<To>(obj);
    if constexpr (requires {reinterpret_cast<uintptr_t>(ptr) == reinterpret_cast<uintptr_t>(obj); })
        assert(reinterpret_cast<uintptr_t>(ptr) == reinterpret_cast<uintptr_t>(obj));
    return ptr;
}

template<typename AutoRefProtocol, bool IsFirstObject>
struct ref_charger_convert_trait
{
    using object_t = std::conditional_t<IsFirstObject,
            typename AutoRefProtocol::first_object_t,
            typename AutoRefProtocol::second_object_t>;
    using object_ptr_t = std::conditional_t<IsFirstObject,
            typename AutoRefProtocol::first_object_ptr_t,
            typename AutoRefProtocol::second_object_ptr_t>;
    using charger_t = std::conditional_t<IsFirstObject,
            typename AutoRefProtocol::first_ref_charger_t,
            typename AutoRefProtocol::second_ref_charger_t>;

    static constexpr bool has_static_caster =
            requires(object_ptr_t o)
            {
                { object_t::ref_charger_static_caster::cast_ref_charger(o) } -> std::convertible_to<charger_t*>;
            } &&
            requires(charger_t* o)
            {
                { object_t::ref_charger_static_caster::cast_pointer(o) } -> std::convertible_to<object_ptr_t>;
            };

    static charger_t* cast_ref_charger(object_ptr_t obj)
    {
        if constexpr (has_static_caster)
            return object_t::ref_charger_static_caster::cast_ref_charger(obj);
        else if (std::convertible_to<object_ptr_t, charger_t*>)
            return static_cast<charger_t*>(obj);
        else
            return interconvertible_pointer_cast<charger_t*>(obj);
    }

    static object_ptr_t cast_pointer(charger_t* obj)
    {
        if constexpr (has_static_caster)
            return object_t::ref_charger_static_caster::cast_pointer(obj);
        else if (std::convertible_to<charger_t*, object_ptr_t>)
            return static_cast<object_ptr_t>(obj);
        else
            return interconvertible_pointer_cast<object_ptr_t>(obj);
    }
};


template<typename FirstObjectPtr,
        typename SecondObjectPtr,
        typename FirstRefCharger,
        typename SecondRefCharger,
        bool FirstObjectPointerInterconvertible,
        bool SecondObjectPointerInterconvertible
>
struct auto_ref_protocol
{
    using protocol_t = auto_ref_protocol;
    using first_object_ptr_t = FirstObjectPtr;
    using first_object_t = std::pointer_traits<first_object_ptr_t>::element_type;
    using second_object_ptr_t = SecondObjectPtr;
    using second_object_t = std::pointer_traits<second_object_ptr_t>::element_type;
    using first_ref_charger_t = FirstRefCharger;
    using second_ref_charger_t = SecondRefCharger;
    static constexpr bool first_object_pointer_interconvertible = FirstObjectPointerInterconvertible;
    static constexpr bool second_object_pointer_interconvertible = SecondObjectPointerInterconvertible;

    template<class T>
    using modify_first_object_ptr = auto_ref_protocol<
            T, second_object_ptr_t,
            first_ref_charger_t, second_ref_charger_t,
            first_object_pointer_interconvertible, second_object_pointer_interconvertible>;
    template<class T>
    using modify_second_object_ptr = auto_ref_protocol<
            first_object_ptr_t, T,
            first_ref_charger_t, second_ref_charger_t,
            first_object_pointer_interconvertible, second_object_pointer_interconvertible>;
    template<class T>
    using modify_first_ref_charger = auto_ref_protocol<
            first_object_ptr_t, second_object_ptr_t,
            T, second_ref_charger_t,
            first_object_pointer_interconvertible, second_object_pointer_interconvertible>;
    template<class T>
    using modify_second_ref_charger = auto_ref_protocol<
            first_object_ptr_t, second_object_ptr_t,
            first_ref_charger_t, T,
            first_object_pointer_interconvertible, second_object_pointer_interconvertible>;

};

template<typename AutoRefProtocol, bool IsFirstReferencer>
class ref_handle
{
    template<typename, bool>
    friend
    class ref_handle;

    using self_object_t = std::conditional_t<IsFirstReferencer,
            typename AutoRefProtocol::first_object_t,
            typename AutoRefProtocol::second_object_t>;
    using other_object_t = std::conditional_t<IsFirstReferencer,
            typename AutoRefProtocol::second_object_t,
            typename AutoRefProtocol::first_object_t>;
    using self_object_ptr_t = std::conditional_t<IsFirstReferencer,
            typename AutoRefProtocol::first_object_ptr_t,
            typename AutoRefProtocol::second_object_ptr_t>;
    using other_object_ptr_t = std::conditional_t<IsFirstReferencer,
            typename AutoRefProtocol::second_object_ptr_t,
            typename AutoRefProtocol::first_object_ptr_t>;

    using self_charger_t = std::conditional_t<IsFirstReferencer,
            typename AutoRefProtocol::first_ref_charger_t,
            typename AutoRefProtocol::second_ref_charger_t>;
    using other_charger_t = std::conditional_t<IsFirstReferencer,
            typename AutoRefProtocol::second_ref_charger_t,
            typename AutoRefProtocol::first_ref_charger_t>;


    struct ref_object_info_1
    {
        other_object_ptr_t obj;

        struct charger_t_ref_wrapper
        {
            other_object_ptr_t& obj;

            charger_t_ref_wrapper(other_object_ptr_t& o) : obj(o) {}

            auto& operator=(other_charger_t* p)
            {
                obj = ref_charger_convert_trait<AutoRefProtocol, !IsFirstReferencer>::cast_pointer(p);
                return *this;
            }

            operator other_charger_t*()
            {
                return ref_charger_convert_trait<AutoRefProtocol, !IsFirstReferencer>::cast_ref_charger(obj);
            }

            other_charger_t* operator->() { return operator other_charger_t*(); }
        };

        charger_t_ref_wrapper ref_charger() { return charger_t_ref_wrapper{obj}; }

        other_object_ptr_t& pointer() { return obj; }

        other_object_ptr_t pointer() const { return obj; }

        void clear()
        {
            obj = nullptr;
        }
    };

    struct ref_object_info_2
    {
        other_object_ptr_t obj;
        other_charger_t* charger;

        other_charger_t*& ref_charger() { return charger; }

        other_object_ptr_t& pointer() { return obj; }

        other_object_ptr_t pointer() const { return obj; }

        void clear()
        {
            obj = nullptr;
            charger = nullptr;
        }
    };

    static constexpr bool is_other_object_pointer_interconvertible =
            IsFirstReferencer ?
            AutoRefProtocol::second_object_pointer_interconvertible :
            AutoRefProtocol::first_object_pointer_interconvertible;
    static constexpr bool is_self_object_pointer_interconvertible =
            IsFirstReferencer ?
            AutoRefProtocol::first_object_pointer_interconvertible :
            AutoRefProtocol::second_object_pointer_interconvertible;
    using info_t = std::conditional_t<is_other_object_pointer_interconvertible, ref_object_info_1, ref_object_info_2>;
    using other_handle_t = ref_handle<AutoRefProtocol, !IsFirstReferencer>;
    info_t info{};
    other_handle_t* inv_ref{};

    void clear()
    {
        inv_ref = nullptr;
        info.clear();
    }

public:
    ref_handle() = default;

    ref_handle(const ref_handle& other) = delete;

    ref_handle(ref_handle&& other) noexcept
            : inv_ref(other.inv_ref), info(other.info)
    {
        if (inv_ref) inv_ref->inv_ref = this;
        other.clear();
    }

    ref_handle& operator=(ref_handle&& other) noexcept
    {
        if (this != &other)
        {
            inv_ref = other.inv_ref;
            info = other.info;
            if (inv_ref) inv_ref->inv_ref = this;
            other.clear();
        }
        return *this;
    }

    void notify_charger_move(self_charger_t* new_charger)
    {
        inv_ref->info.ref_charger() = new_charger;
    }

    void notify_object_charger_move(self_object_ptr_t new_obj,
                                    self_charger_t* new_charger)requires (!is_self_object_pointer_interconvertible)
    {
        if (inv_ref)
        {
            inv_ref->info.ref_charger() = new_charger;
            inv_ref->info.pointer() = new_obj;
        }
    }

    ~ref_handle()
    {
        if (inv_ref)
        {
            inv_ref->clear();
            info.ref_charger()->notify_reference_removed(inv_ref);
        }
    }

    void bind(self_charger_t* my_charger,
              other_object_ptr_t bind_object,
              other_charger_t* bind_charger,
              other_handle_t* bind_handle) requires IsFirstReferencer && (!is_other_object_pointer_interconvertible)
    {
        assert(bind_handle);
        assert(bind_charger != info.ref_charger());//should deal outside
        if (info.ref_charger()) info.ref_charger()->notify_reference_removed(this);
        info.ref_charger() = bind_charger;
        info.pointer() = bind_object;
        inv_ref = bind_handle;
        bind_handle->info.ref_charger() = my_charger;
    }

    void bind(self_charger_t* my_charger,
              other_charger_t* bind_charger,
              other_handle_t* bind_handle) requires IsFirstReferencer && is_other_object_pointer_interconvertible
    {
        assert(bind_handle);
        assert(bind_charger != info.ref_charger());//should deal outside
        if (info.ref_charger()) info.ref_charger()->notify_reference_removed(this);
        info.ref_charger() = bind_charger;
        inv_ref = bind_handle;
        bind_handle->inv_ref = this;
        bind_handle->info.ref_charger() = my_charger;
    }

    ref_handle(self_charger_t* my_charger,
               other_object_ptr_t bind_object,
               other_charger_t* bind_charger,
               other_handle_t* bind_handle) requires IsFirstReferencer && (!is_other_object_pointer_interconvertible)
    {
        bind(my_charger, bind_object, bind_charger, bind_handle);
    }

    ref_handle(self_charger_t* my_charger,
               other_charger_t* bind_charger,
               other_handle_t* bind_handle) requires IsFirstReferencer && is_other_object_pointer_interconvertible
    {
        bind(my_charger, bind_charger, bind_handle);
    }

    void unbind()
    {
        if (info.ref_charger())
        {
            info.ref_charger()->notify_reference_removed(this);
            clear();
        }
    }

    void destory()
    {
        assert(inv_ref);
        auto self_charger = inv_ref->info.ref_charger();
        self_charger->notify_reference_removed(this);//destroy this, other is auto destroy when destructor call
    }

    auto& operator=(nullptr_t)
    {
        unbind();
        return *this;
    }

    auto operator<=>(other_object_ptr_t p) const { return info.pointer() <=> p; }
    bool operator ==(nullptr_t) const { return info.obj == nullptr; }

    operator bool() const { return info.obj != nullptr; }

    [[nodiscard]] other_object_ptr_t get() const { return info.pointer(); }

private:
    template<typename T>
    struct object_t_reference
    {
        using type = T&;
    };
    template<>
    struct object_t_reference<void>
    {
        using type = void;
    };
public:

    object_t_reference<other_object_t> operator*() requires (!std::is_same_v<other_object_t, void>)
    {
        if constexpr (!std::is_same_v<other_object_t, void>) return *get();
    }

    other_object_ptr_t operator->() const { return get(); }

    template<typename T>
    T* cast() const { return static_cast<T*>(get()); }
};


using generic_ref_protocol = auto_ref_protocol<
        referencer_interface*,
        void*,
        referencer_interface,
        class generic_ref_reflector,
        true, true>;

class generic_ref_reflector
{
    template<typename, bool>
    friend
    class ref_handle;

    using RefHandle = ref_handle<generic_ref_protocol, false>;

    struct element_void
    {
        RefHandle handle;

        template<typename... Args>
        explicit element_void(Args&& ... args) : handle(std::forward<Args>(args)...) {}
    };

    using element_t = element_void;

    std::vector<element_t> refs;

    void notify_reference_removed(void* reference_handel_address)
    {
        auto addr = (element_t*) reference_handel_address;
        size_t index = addr - refs.data();
        //swap back
        refs[index] = std::move(refs.back());
        refs.pop_back();
    }

public:
    generic_ref_reflector() = default;

    generic_ref_reflector(generic_ref_reflector&& other) noexcept
            : refs(std::move(other.refs))
    {
        for (auto& ref: refs)
        {
            ref.handle.notify_charger_move(this);
        }
    }

    template<typename ToProtocol = generic_ref_protocol>
    auto new_bind_handle()
    {
        return (ref_handle<ToProtocol, false>*) &refs.emplace_back().handle;
    }

    void unbind(void* obj)
    {
        for (int i = 0; i < refs.size(); ++i)
        {
            if (refs[i].handle.get() == obj)
            {
                refs[i] = std::move(refs.back());
                refs.pop_back();
                break;
            }
        }
        assert(false);
    }

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::tuple<void*>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
    private:
        std::vector<element_t>::iterator it;
    public:
        iterator(std::vector<element_t>::iterator&& it) : it(std::move(it)) {}

        bool operator==(const iterator& other) const { return it == other.it; }

        void operator++() { ++it; }

        void operator++(int) { ++it; }

        value_type operator*() { return std::make_tuple(it->handle.get()); }
    };

    iterator begin() { return iterator(refs.begin()); }

    iterator end() { return iterator(refs.end()); }
};


template<typename Derived, bool IsFirstReferencer>
struct optional_referencer_interface;
template<typename Derived>
struct optional_referencer_interface<Derived, false>
{
};

template<typename Derived>
class optional_referencer_interface<Derived, true> : public referencer_interface
{
    void notify_reference_removed(void* reference_handel_address) override
    {
        static_cast<Derived*>(this)->notify_reference_removed(reference_handel_address);
    }
};


template<typename AutoRefProtocol, typename AppendingData = void>
class array_ref_charger : public referencer_interface
{
    template<typename, bool>
    friend
    class ref_handle;

    using RefHandle = ref_handle<AutoRefProtocol, true>;

    struct element_void
    {
        RefHandle handle;

        template<typename... Args>
        explicit element_void(Args&& ... args) : handle(std::forward<Args>(args)...) {}
    };

    struct element_non_void
    {
        RefHandle handle;
        AppendingData data;

        template<typename... Args>
        explicit element_non_void(Args&& ... args) : handle(std::forward<Args>(args)...) {}
    };

    using element_t = std::conditional_t<std::is_same_v<AppendingData, void>, element_void, element_non_void>;

    std::vector<element_t> refs;

    void notify_reference_removed(void* reference_handel_address) override
    {
        auto addr = (element_t*) reference_handel_address;
        size_t index = addr - refs.data();
        //swap back
        refs[index] = std::move(refs.back());
        refs.pop_back();
    }

public:
    array_ref_charger() = default;

    array_ref_charger(array_ref_charger&& other) noexcept
            : refs(std::move(other.refs))
    {
        for (auto& ref: refs)
        {
            ref.handle.notify_charger_move(this);
        }
    }

//    template<class T>
    decltype(auto) bind(AutoRefProtocol::second_object_ptr_t obj)
    {
//        auto generic_object = reinterpret_cast<typename AutoRefProtocol::second_object_ptr_t>(obj);
        auto charger = ref_charger_convert_trait<AutoRefProtocol, false>::cast_ref_charger(obj);
        auto handle = charger->new_bind_handle<AutoRefProtocol>();
        if constexpr (std::is_same_v<AppendingData, void>)
        {
            refs.emplace_back(this, charger, handle);
            assert(handle->get() == this);
        } else
        {
            auto& storage = refs.emplace_back(this, charger, handle);
            assert(handle->get() == this);
            return (AppendingData&) storage.data;
        }
    }
//    decltype(auto) bind(nullptr_t)
//    {
//        if constexpr (std::is_same_v<AppendingData, void>)
//        {
//            objects.emplace_back();
//        } else
//        {
//            auto& storage = objects.emplace_back();
//            return (AppendingData&) storage.data;
//        }
//    }

    void unbind(AutoRefProtocol::second_object_ptr_t obj)
    {
        for (int i = 0; i < refs.size(); ++i)
        {
            if (refs[i].handle.get() == obj)
            {
                refs[i] = std::move(refs.back());
                refs.pop_back();
                return;
            }
        }
        assert(false);
    }

    class iterator
    {
    public:
        using iterator_category = std::input_iterator_tag;
        using value_type = std::tuple<typename AutoRefProtocol::second_object_ptr_t, AppendingData>;
        using difference_type = std::ptrdiff_t;
        using pointer = value_type*;
        using reference = value_type&;
    private:
        std::vector<element_t>::iterator it;
    public:
        iterator(std::vector<element_t>::iterator&& it) : it(std::move(it)) {}

        bool operator==(const iterator& other) const { return it == other.it; }

        void operator++() { ++it; }

        void operator++(int) { ++it; }

        value_type operator*() { return std::make_tuple(it->handle.get(), it->data); }
    };

    iterator begin() { return iterator(refs.begin()); }

    iterator end() { return iterator(refs.end()); }
};

#ifdef _MSC_VER
#define MSC_EBO __declspec(empty_bases)
#else
#define MSC_EBO
#endif

template<typename T>
struct MSC_EBO reference_reflector : generic_ref_reflector, T
{
    using reflector_reference_type = T;

    template<typename... Args>
    reference_reflector(Args&& ... args) : generic_ref_reflector(), T(std::forward<Args>(args)...) {}

};

struct extern_reflector_generic_object_t
{
	int pad;
};
using extern_reflector_generic_object_ptr_t = offset_ptr<
        extern_reflector_generic_object_t,
        reference_reflector<extern_reflector_generic_object_t>>;

using reference_reflector_ref_protocol = generic_ref_protocol
::modify_second_object_ptr<extern_reflector_generic_object_ptr_t>
::modify_second_ref_charger<reference_reflector<extern_reflector_generic_object_t>>;

struct no_ref_charger : referencer_interface
{
    void notify_reference_removed(void*) override {};

    static no_ref_charger* ptr()
    {
        static no_ref_charger no_charger;
        return &no_charger;
    }

private:
    no_ref_charger() = default;
};

template<typename T, bool IsFirstReferencer = true>
using typed_ref_handle = ref_handle<generic_ref_protocol::modify_second_object_ptr<T*>, IsFirstReferencer>;

template<typename T>
struct auto_reference_traits;
template<typename T> requires (!requires{ sizeof(T); })//incomplete type
struct auto_reference_traits<T>
{
    using reflectible_t = reference_reflector<T>;
};
template<typename T> requires requires{ sizeof(T); }
struct auto_reference_traits<T>
{
    static const bool reflectible = std::is_base_of_v<generic_ref_reflector, T>;
    using reflectible_t = std::conditional_t<reflectible, T, reference_reflector<T>>;
};

template<typename T>
struct embedded_ref_reflector;

template<typename T>
struct auto_reference_traits<embedded_ref_reflector<T>>
{
    using reflectible_t = T;
};

template<typename T, typename Reflector = auto_reference_traits<T>::reflectible_t>
class weak_reference;

template<typename T, typename Reflector>
class weak_reference_base : protected typed_ref_handle<Reflector>
{
    using super = typed_ref_handle<Reflector>;
public:
    weak_reference_base() = default;

    weak_reference_base(Reflector* obj) { bind(obj); }

    void bind(Reflector* obj)
    {
        if (obj == super::template cast<Reflector>()) return;
        auto charger = interconvertible_pointer_cast<generic_ref_reflector*>(obj);
        auto h = charger->new_bind_handle();
        super::bind(no_ref_charger::ptr(), charger, (typed_ref_handle<Reflector, false>*) h);
    }

    using super::unbind;

    auto& operator=(Reflector* obj)
    {
        bind(obj);
        return *this;
    }

    using super::operator bool;

    operator void*()
    {
        return super::get();
    }

};

template<typename T, typename Reflector> requires std::same_as<T, Reflector>
class weak_reference<T, Reflector> : public weak_reference_base<T, Reflector>
{
    using super = weak_reference_base<T, Reflector>;
public:
    using super::super;

    using super::get;
    using super::operator*;
    using super::operator->;
    using super::operator<=>;
    using super::operator==;
};



template<typename T>
class weak_reference<T, reference_reflector<T>> : public weak_reference_base<T, reference_reflector<T>>
{
    using super = weak_reference_base<T, reference_reflector<T>>;
public:
    using super::super;

    T* get() { return static_cast<T*>(super::get()); }

    add_reference_possible_void_t<T> operator*() { if constexpr (!std::same_as<T, void>) return *get(); }

    T* operator->() { return get(); }

    auto operator<=>(T* p) { return get() <=> p; }
};

//using generic_weak_reference = weak_reference<generic_ref_reflector,embedded_ref_reflector<generic_ref_reflector>>;

//class generic_weak_reference : protected typed_ref_handle<void>
//{
//    using super = typed_ref_handle<void>;
//public:
//    generic_weak_reference() = default;
//
//    template<class T>
//    requires std::convertible_to<T*, generic_ref_reflector*>
//    generic_weak_reference(T* obj)
//            : super(
//            no_ref_charger::ptr(),
//            interconvertible_pointer_cast<generic_ref_reflector*>(obj),
//            interconvertible_pointer_cast<generic_ref_reflector*>(obj)->new_bind_handle()
//    ) {}
//
//    template<class T>
//    requires std::convertible_to<T*, generic_ref_reflector*>
//    void bind(T* obj)
//    {
//        if (obj == super::template cast<T>()) return;
//        auto charger = interconvertible_pointer_cast<generic_ref_reflector*>(obj);
//        auto h = charger->new_bind_handle();
//        super::bind(no_ref_charger::ptr(), charger, h);
//    }
//
//    template<class T>
//    requires std::convertible_to<T*, generic_ref_reflector*>
//    auto& operator=(T* obj)
//    {
//        bind(obj);
//        return *this;
//    }
//
//    operator void*()
//    {
//        return super::get();
//    }
//
//    using super::unbind;
//    using super::operator bool;
//    using super::operator<=>;
//    using super::operator==;
//    using super::operator=;
//    using super::get;
//    using super::cast;
//};




template<typename T, typename Reflector = auto_reference_traits<T>::reflectible_t>
class unique_reference : public offset_ptr<T, Reflector>
{
    using super = offset_ptr<T, Reflector>;
public:
    using super::super;
    unique_reference(const unique_reference&) = delete;

    super offset_ptr(){ return super(*this); }

    operator extern_reflector_generic_object_ptr_t() const requires std::same_as<Reflector, reference_reflector<T>>
    {
        using ptr_t = extern_reflector_generic_object_ptr_t;
        return super::template reinterpret<ptr_t>();
    }

    ~unique_reference()
    {
        delete super::get_final_ptr();
    }
};

template<typename Reflector>
unique_reference(Reflector*) -> unique_reference<typename Reflector::reflector_reference_type, Reflector>;