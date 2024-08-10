#pragma once
#include <iostream>
#include "../delegate.h"

namespace test_auto_reference
{

    template<typename Owner>
    struct array_bi_ref
    {
        template<class T> friend
        class array_bi_ref;

        std::vector<bi_ref<void>> refs;

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
            return refs.emplace_back();
        }

        template<class T>
        void bind(T* obj)
        {
            auto& ref = obj->new_bind_handle();
            refs.emplace_back(static_cast<Owner*>(this), (bi_ref<void>*) &ref);
        }

        template<class T>
        void bind(Owner* owner, T* obj)
        {
            auto& ref = obj->new_bind_handle();
            refs.emplace_back(owner, obj, (bi_ref<void>*) &ref);
        }
    };

    struct A
    {
        array_ref_charger<true,void*> refs;

        A() = default;

        A(A&& other) noexcept = default;

        template<class T>
        void bind(T* obj)
        {
            auto& data = refs.bind(obj);
            data = nullptr;
        }

        void print_ref()
        {
            for (auto& [ref,_]: refs.refs)
            {
                std::cout << this << " A ref *: " << ref.get() << std::endl;
            }
        }
    };

    struct B : array_ref_charger<false>
    {
        void print_ref()
        {
            for (auto& [ref]: refs)
            {
                std::cout << this << " B ref *: " << ref.get() << std::endl;
            }
        }
    };


    void test()
    {
        A* a = new A();
        B* b = new B();
        a->bind(b);
        a->print_ref();
        b->print_ref();
        A* temp = new A(std::move(*a));
        delete a;
        a = temp;
        std::cout << "after move" << std::endl;
        a->print_ref();
        b->print_ref();


        a->print_ref();
        b->print_ref();

        delete a;
        std::cout << "after delete" << std::endl;
        a->print_ref();
        b->print_ref();
        delete b;
        a->print_ref();
        b->print_ref();

    }
}