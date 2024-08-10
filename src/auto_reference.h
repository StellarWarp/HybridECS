#pragma once

#include <type_traits>
#include <assert.h>
#include <vector>

struct referencer_interface
{
     constexpr virtual void notify_reference_removed(void* reference_handel_address) = 0;
};

//struct DefaultAutoRefProtocol
//{
//    //va_ref_array = false means that the ref_array address can be obtained using reference object
//    //two case that va_ref_array = false:
//    //1. the ref object type is known
//    //2, the ref object type is unknown but the ref_array_t has the same address with object
//    static constexpr bool variable_ref_charger = false;
//    using first_object_t = void;
//    using second_object_t = void;
//    using first_ref_charger_t = referencer_interface;//this count for the remove notify
//    using second_ref_charger_t = referencer_interface;//this count for the remove notify
//};

template<typename FirstObject,
        typename SecondObject,
        typename FirstRefCharger,
        typename SecondRefCharger,
        bool FirstObjectPointerInterconvertible,
        bool SecondObjectPointerInterconvertible
>
struct auto_ref_protocol
{
    using first_object_t = FirstObject;
    using second_object_t = SecondObject;
    using first_ref_charger_t = FirstRefCharger;
    using second_ref_charger_t = SecondRefCharger;
    static constexpr bool first_object_pointer_interconvertible = FirstObjectPointerInterconvertible;
    static constexpr bool second_object_pointer_interconvertible = SecondObjectPointerInterconvertible;
};

//using DefaultAutoRefProtocol = auto_ref_protocol<
//        referencer_interface,
//        void,
//        referencer_interface,
//        referencer_interface,
//        true,true>;

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

    using self_charger_t = std::conditional_t<IsFirstReferencer,
            typename AutoRefProtocol::first_ref_charger_t,
            typename AutoRefProtocol::second_ref_charger_t>;
    using other_charger_t = std::conditional_t<IsFirstReferencer,
            typename AutoRefProtocol::second_ref_charger_t,
            typename AutoRefProtocol::first_ref_charger_t>;

    struct ref_object_info_1
    {
        other_object_t* obj;

        other_charger_t*& ref_charger() { return reinterpret_cast<other_charger_t*&>(obj); }

        other_object_t*& pointer() { return obj; }

        void clear()
        {
            obj = nullptr;
        }
    };

    struct ref_object_info_2
    {
        other_object_t* obj;
        other_charger_t* charger;

        other_charger_t*& ref_charger() { return charger; }

        other_object_t*& pointer() { return obj; }

        void clear()
        {
            obj = nullptr;
            charger = nullptr;
        }
    };

    static constexpr bool is_other_object_pointer_interconvertible = IsFirstReferencer ?
                                                                     AutoRefProtocol::second_object_pointer_interconvertible
                                                                                       :
                                                                     AutoRefProtocol::first_object_pointer_interconvertible;
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
        if (inv_ref) inv_ref->info.ref_charger() = new_charger;
    }

    void notify_object_charger_move(self_object_t* new_obj, self_charger_t* new_charger)
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
              other_object_t* bind_object,
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
              other_charger_t* bind_object,
              other_handle_t* bind_handle) requires IsFirstReferencer && is_other_object_pointer_interconvertible
    {
        assert(bind_handle);
        assert(bind_object != info.ref_charger());//should deal outside
        if (info.ref_charger()) info.ref_charger()->notify_reference_removed(this);
        info.pointer() = bind_object;
        inv_ref = bind_handle;
        bind_handle->inv_ref = this;
        bind_handle->info.ref_charger() = my_charger;
    }

    ref_handle(self_charger_t* my_charger,
               other_object_t* bind_object,
               other_charger_t* bind_charger,
               other_handle_t* bind_handle) requires IsFirstReferencer && (!is_other_object_pointer_interconvertible)
    {
        bind(my_charger, bind_object, bind_charger, bind_handle);
    }

    ref_handle(self_charger_t* my_charger,
               other_charger_t* bind_object,
               other_handle_t* bind_handle) requires IsFirstReferencer && is_other_object_pointer_interconvertible
    {
        bind(my_charger, bind_object, bind_handle);
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

    bool operator==(nullptr_t) const { return info.pointer() == nullptr; }

    bool operator==(auto* p) const { return info.pointer() == p; }

    operator bool() const { return info.obj != nullptr; }

    other_object_t* get() { return info.pointer(); }

    template<typename T>
    T* cast() { return static_cast<T*>(get()); }
};

template<bool IsFirstReferencer, typename AppendingData = void>
class array_ref_charger;

using array_ref_protocol = auto_ref_protocol<
        referencer_interface,
        void,
        referencer_interface,
        array_ref_charger<false>,
        true, true>;

template<bool IsFirstReferencer, typename AppendingData>
class array_ref_charger : public referencer_interface
{
    template<typename, bool>
    friend
    class ref_handle;

    using RefHandle = ref_handle<array_ref_protocol, IsFirstReferencer>;

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

    auto new_bind_handle() requires (!IsFirstReferencer)
    {
        return &refs.emplace_back().handle;
    }

    template<class T>
    decltype(auto) bind(T* obj) requires IsFirstReferencer
    {
        auto charger = static_cast<array_ref_charger<false>*>(obj);
        assert(charger == obj);
        auto handle = charger->new_bind_handle();

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
        using value_type = std::tuple<void*, AppendingData>;
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


template<typename T>
class bi_ref
{
    template<class U>
    friend
    class bi_ref;

    T* ptr = nullptr;
    bi_ref<void>* referencer = nullptr;

    void on_referencer_set(void* obj, bi_ref<void>* new_ref)
    {
        referencer = new_ref;
        ptr = obj;
    }

    void on_referencer_remove()
    {
        referencer = nullptr;
        ptr = nullptr;
    }

public:
    bi_ref() = default;


    bi_ref(const bi_ref&) = delete;

    bi_ref(bi_ref&& other) noexcept
            : ptr(other.ptr), referencer(other.referencer)
    {
        if (referencer) referencer->referencer = this;
        other.on_referencer_remove();
    }

    bi_ref(void* owner, bi_ref&& other) noexcept
            : ptr(other.ptr), referencer(other.referencer)
    {
        if (referencer) referencer->on_referencer_set(owner, this);
        other.on_referencer_remove();
    }

    bi_ref& operator=(bi_ref&& other) noexcept
    {
        if (this != &other)
        {
            if (referencer) referencer->on_referencer_remove();
            ptr = other.ptr;
            referencer = other.referencer;
            if (referencer) referencer->referencer = this;
            other.on_referencer_remove();
        }
        return *this;
    }

    ~bi_ref()
    {
        if (referencer) referencer->on_referencer_remove();
    }

    bi_ref(void* owner, T* obj, bi_ref<void>* ref) noexcept { bind(owner, obj, ref); }

    void bind(void* owner, void* obj, bi_ref<void>* ref)
    {
        on_referencer_set(obj, ref);
        ref->on_referencer_set(owner, this);
    }

    void notify_owner_change(void* owner) noexcept
    {
        if (referencer) referencer->ptr = owner;
    }


    void unbind()
    {
        referencer->on_referencer_remove();
        on_referencer_remove();
    }

    [[nodiscard]] T* get() const { return ptr; }

    operator bool() const
    {
        return ptr != nullptr;
    }

    bool operator==(auto* p) const { return ptr == p; }

};