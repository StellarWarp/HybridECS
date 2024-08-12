#pragma once

#include <concepts>
template<typename T>
struct add_reference_possible_void
{
    using type = T&;
};
template<>
struct add_reference_possible_void<void>
{
    using type = void;
};
template<typename T>
using add_reference_possible_void_t = add_reference_possible_void<T>::type;


#include <iostream>

class offset_ptr_generic
{
    template<typename, typename>
    friend class offset_ptr;
protected:
    void* ptr;
    int64_t offset;
    offset_ptr_generic(void* ptr, int64_t offset): ptr(ptr),offset(offset){}
public:
    offset_ptr_generic() : ptr(nullptr) {}

    void* get() { return ptr; }

    void* get_final_ptr() { return reinterpret_cast<char*>(ptr) - offset; }

    auto operator<=>(void* p) { return get() <=> p; }
};

template<typename T, typename Final>
class offset_ptr
{
    template<typename , typename >
    friend class offset_ptr;
protected:
    T* ptr;

    Final* get_final_ptr() const { return static_cast<Final*>(ptr); }

public:
    using element_type = T;
    using final_type = Final;

    offset_ptr() : ptr(nullptr) {}

    offset_ptr(Final* obj) : ptr(static_cast<T*>(obj)) {}

    offset_ptr& operator=(Final* obj)
    {
        ptr = static_cast<T*>(obj);
        return *this;
    }
    offset_ptr& operator=(nullptr_t) { ptr = nullptr; return *this; }

    operator bool() const { return ptr != nullptr; }
    bool operator ==(nullptr_t){ return ptr == nullptr; }

    T* get() const { return ptr; }

    add_reference_possible_void_t<T> operator*() const { if constexpr (!std::same_as<T, void>) return *get(); }

    T* operator->() const { return get(); }

    auto operator<=>(T* p) const { return get() <=> p; }

    operator Final*() const { return get_final_ptr(); }
    operator T*() const { return get(); }

    explicit operator offset_ptr_generic() const
    {
        int64_t offset = reinterpret_cast<int64_t>(ptr) - reinterpret_cast<int64_t>(get_final_ptr());
        return generic_t(ptr, offset);
    };

    explicit operator void*() { return get(); }

    template<typename T_,typename Final_>
    auto reinterpret() const
    {
        offset_ptr<T_, Final_> ptr_;
        ptr_.ptr = reinterpret_cast<T_*>(ptr);
        assert(intptr_t(ptr_.get()) - intptr_t(ptr_.get_final_ptr()) ==  intptr_t(ptr) - intptr_t(get_final_ptr()));
        return  ptr_;
    }
    template<typename Other>
    auto reinterpret() const
    {
        return reinterpret<typename Other::element_type,typename Other::final_type>();
    }


};

//void test()
//{
//    struct A{};
//    struct B{};
//    struct C:A,B{};
//    offset_ptr<B,C> a;
//    offset_ptr<B,C> b;
//    C* c;
//    a = b;
//    a = nullptr;
//    a = c;
//    c = a;
//}