#pragma once

#include "ecs/storage/entity_map.h"

#include "ut.hpp"

using namespace hyecs;

namespace ut = boost::ut;

namespace
{
    struct B
    {
        static inline size_t object_counter = 0;
        static inline size_t construct_call = 0;
        static inline size_t move_call = 0;
        static inline size_t copy_call = 0;
        static inline size_t destroy_call = 0;

        static void check_log()
        {
            ut::log << "object_counter: " << object_counter << "\n"
            << "construct_call: " << construct_call << "\n"
            << "move_call: " << move_call << "\n"
            << "copy_call: " << copy_call << "\n"
            << "destroy_call: " << destroy_call << "\n";
        }

        int a, b, c, d;

        B(int a, int b, int c, int d) : a(a), b(b), c(c), d(d)
        {
            object_counter++;
            construct_call++;
        }

        B(const B& other) : a(other.a), b(other.b), c(other.c), d(other.d)
        {
            object_counter++;
            copy_call++;
        }

        B(B&& other) : a(other.a), b(other.b), c(other.c), d(other.d)
        {
            object_counter++;
            move_call++;
        }

        B& operator=(const B& other)
        {
            a = other.a;
            b = other.b;
            c = other.c;
            d = other.d;
            return *this;
        }


        ~B()
        {
            a = b = c = d = 0;
            object_counter--;
            destroy_call++;
        }

        bool operator==(const B& o) const
        {
            return a == o.a && b == o.b && c == o.c && d == o.d;
        }

    };
}

static ut::suite _ = []
{
    using namespace ut;

    "dense_map class"_test = []
    {
        raw_entity_dense_map table(sizeof(B), alignof(B));

        unordered_map<entity, B> map;

        const int test_scale = 1 << 12;

        for (uint32_t i = 0; i < test_scale; i++)
        {
            new(table.allocate_value(entity{i, 0})) B(i, i + 1, i + 2, i + 3);
            map.try_emplace(entity{i, 0}, B(i, i + 1, i + 2, i + 3));
        }


        for (auto& [e, v]: map)
        {
            B& b = *(B*) table.at(e);
            expect(b == v);
            assert(b == v);
        }

        auto remove_entity_from_table = [&](entity e)
        {
            table.deallocate_value(
                    e,
                    [](void* dest, void* src) { new(dest) B(std::move(*(B*) src)); },
                    [](void* ptr) { ((B*) ptr)->~B(); }
            );
        };

        //random remove
        int remove_count = 0;
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                remove_count++;
                auto e = entity{i, 0};
                if (table.contains(e))
                    remove_entity_from_table(e);
                map.erase(e);
            }
        }

        //adding new
        for (uint32_t i = test_scale; i < test_scale + test_scale; i++)
        {
            new(table.allocate_value(entity{i, 0})) B(i, i + 1, i + 2, i + 3);
            map.try_emplace(entity{i, 0}, B(i, i + 1, i + 2, i + 3));
        }

        //random remove
        for (uint32_t i = 0; i < test_scale*2; i++)
        {
            if (rand() % 2 == 0)
            {
                if (table.contains(entity{i, 0}))
                    remove_entity_from_table(entity{i, 0});
                map.erase(entity{i, 0});
            }
        }


        //random remove
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                if (table.contains(entity{i, 0}))
                    remove_entity_from_table(entity{i, 0});
                map.erase(entity{i, 0});
            }
        }

        for (auto& [e, v]: map)
        {
            B& b = *(B*) table.at(e);
            expect(b == v);
            assert(b == v);
        }
        for (auto [e, ptr]: table)
        {
            expect(map.at(e) == *(B*) ptr);
        }

        for (uint32_t i = 0; i < test_scale * 3; i++)
        {
            auto e = entity{i, 0};
            bool b1 = map.contains(e);
            auto b2 = table.contains(e);
            expect(b1 == b2);
        }

        expect(table.size() == map.size());

        table.clear([](void* ptr) { ((B*) ptr)->~B(); });

    };

    "leak test"_test = []{
        expect(B::object_counter == 0) << "B::object_counter = " << B::object_counter << "\n"
        << "B::construct_call = " << B::construct_call << "\n"
        << "B::move_call = " << B::move_call << "\n"
        << "B::copy_call = " << B::copy_call << "\n"
        << "B::destroy_call = " << B::destroy_call << "\n";
    };

    "dense_map"_test = []
    {
        raw_entity_dense_map table(sizeof(int), alignof(int));

        unordered_map<entity, int> map;

        const int test_scale = 1 << 12;

        for (uint32_t i = 0; i < test_scale; i++)
        {
            new(table.allocate_value(entity{i, 0})) int(i);
            map[entity{i, 0}] = i;
        }


        //random remove
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                table.deallocate_value(entity{i, 0});
                map.erase(entity{i, 0});
            }
        }

        for (uint32_t i = test_scale; i < test_scale + test_scale; i++)
        {
            new(table.allocate_value(entity{i, 0})) int(i);
            map[entity{i, 0}] = i;
        }

        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                if (table.contains(entity{i, 0}))
                    table.deallocate_value(entity{i, 0});
                map.erase(entity{i, 0});
            }
        }

        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                if (table.contains(entity{i, 0}))
                    table.deallocate_value(entity{i, 0});
                map.erase(entity{i, 0});
            }
        }


        for (raw_entity_dense_map::entity_value ev: table)
        {
            std::cout << ev.e << " " << *(int*) ev.value << std::endl;
        }

        for (auto& [e, v]: map)
        {
            expect(*(int*) table.at(e) == v);
        }
        for (auto [e, ptr]: table)
        {
            expect(map.at(e) == *(int*) ptr);
        }

        for (uint32_t i = 0; i < test_scale * 3; i++)
        {
            auto e = entity{i, 0};
            bool b1 = map.contains(e);
            auto b2 = table.contains(e);
            expect(b1 == b2);
        }

    };


};