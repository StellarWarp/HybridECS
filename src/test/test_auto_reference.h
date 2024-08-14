#pragma once

#include <iostream>
#include "../delegate.h"

namespace test_auto_reference
{


    struct A
    {
        array_ref_charger<generic_ref_protocol> refs;

        A() = default;

        A(A&& other) noexcept = default;

        template<class T>
        void bind(T* obj)
        {
            refs.bind(obj);
        }

        void print_ref()
        {
            for (auto [ref]: refs)
            {
                std::cout << this << " A ref *: " << ref << std::endl;
            }
        }
    };

    struct B : generic_ref_reflector
    {
        void print_ref()
        {
            for (auto [ref]: *this)
            {
                std::cout << this << " B ref *: " << ref << std::endl;
            }
        }
    };


//    struct XY_ref_protocol : auto_ref_protocol<
//            class X*, class Y*,
//            array_ref_charger<XY_ref_protocol>, array_ref_charger<XY_ref_protocol>,
//            true, true
//    >{};
//
//    struct X
//    {
//
//    };
//
//    struct Y
//    {
//
//    };


    void test()
    {
		std::cout << "test_auto_reference" << std::endl;

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
        b->print_ref();
        delete b;

    }
}