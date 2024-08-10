#include <iostream>
#include <print>
#include <memory>
//#include "multicast_delegate.h"
//#include "test/test_delegate.h"
#include "delegate.h"

template<typename Owner>
struct array_bi_ref
{
    template<class T> friend
    class array_bi_ref;

    std::vector<bi_ref<Owner, void>> refs;

    template<typename Callable>
    void for_each_ref(Callable&& func)
    {
        auto begin = refs.begin();
        auto end = refs.end();
        auto iter = begin;
        while (iter != end)
        {
            auto& ref = *iter;
            if (!ref)
            {
                end--;
                *iter = std::move(*end);
                continue;
            }
            func(ref);
            ++iter;
        }
        refs.erase(end, refs.end());
    }

    array_bi_ref() = default;

    array_bi_ref(array_bi_ref&& other) noexcept
            : refs(std::move(other.refs))
    {
        for_each_ref([&](auto&& ref)
                     {
                         ref.notify_owner_change(static_cast<Owner*>(this));
                     });
    }

    array_bi_ref(Owner* owner, array_bi_ref&& other) noexcept
            : refs(std::move(other.refs))
    {
        for_each_ref([&](auto&& ref)
                     {
                         ref.notify_owner_change(owner);
                     });
    }

    auto& new_bind_handle()
    {
        return refs.emplace_back(static_cast<Owner*>(this));
    }

    template<class T>
    void bind(T* obj)
    {
        auto& ref = obj->new_bind_handle();
        refs.emplace_back(static_cast<Owner*>(this), (bi_ref<void, Owner>*) &ref);
    }

    template<class T>
    void bind(Owner* owner, T* obj)
    {
        auto& ref = obj->new_bind_handle();
        refs.emplace_back(owner, (bi_ref<void, Owner>*) &ref);
    }
};

struct A
{
    array_bi_ref<A> refs;

    A() = default;
    A(A&& other) noexcept
            : refs(this, std::move(other.refs))
    {    }

    template<class T>
    void bind(T* obj) { refs.bind(this, obj); }

    void print_ref()
    {
        for (auto& ref: refs.refs)
        {
            std::cout << "A ref *: " << ref.get() << std::endl;
        }
    }
};

struct ref_h : array_bi_ref<ref_h>
{
};

struct B : array_bi_ref<ref_h>
{
    void print_ref()
    {
        for (auto& ref: refs)
        {
            std::cout << "B ref *: " << ref.get() << std::endl;
        }
    }
};


int main()
{
    A* a = new A();
    B* b = new B();
    a->bind(b);
    a->print_ref();
    b->print_ref();
    A* temp = new A(std::move(*a));
    a->print_ref();
    b->print_ref();
    delete a;
    a = temp;
    a->print_ref();
    b->print_ref();

    delete a;
    a->print_ref();
    b->print_ref();
    delete b;
    a->print_ref();
    b->print_ref();

}
