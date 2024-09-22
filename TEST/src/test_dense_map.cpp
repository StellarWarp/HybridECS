#include "ecs/storage/entity_map.h"

#include "ut.hpp"
#include "memleak_detect.h"
#include "managed_object_tester.h"

using namespace hyecs;

namespace ut = boost::ut;

namespace
{

    using B = managed_object_tester<[] {}>;
}

static ut::suite _ = []
{
    using namespace ut;

    "dense_map class"_test = []
    {
        MemoryLeakDetector detector;

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
                    [](void* dest, void* src)
                    {
                        generic::debug_scope_dynamic_move_signature _{};
                        new(dest) B(std::move(*(B*) src));
                    },
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
        for (uint32_t i = 0; i < test_scale * 2; i++)
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

    "object leak test"_test = []
    {
        expect(B::object_counter == 0) << "B::object_counter = " << B::object_counter << "\n"
                                       << "B::construct_call = " << B::construct_call << "\n"
                                       << "B::move_call = " << B::move_call << "\n"
                                       << "B::copy_call = " << B::copy_call << "\n"
                                       << "B::destroy_call = " << B::destroy_call << "\n";
    };

    "dense_map"_test = []
    {
        MemoryLeakDetector detector;

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


    "dense_map leak test"_test = []
    {
        MemoryLeakDetector detector;

        raw_entity_dense_map table(sizeof(int), alignof(int));


        const int test_scale = 1 << 12;

        for (uint32_t i = 0; i < test_scale; i++)
        {
            new(table.allocate_value(entity{i, 0})) int(i);
        }


        //random remove
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                table.deallocate_value(entity{i, 0});
            }
        }

        for (uint32_t i = test_scale; i < test_scale + test_scale; i++)
        {
            new(table.allocate_value(entity{i, 0})) int(i);
        }

        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                if (table.contains(entity{i, 0}))
                    table.deallocate_value(entity{i, 0});
            }
        }

        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                if (table.contains(entity{i, 0}))
                    table.deallocate_value(entity{i, 0});
            }
        }


    };

    "unordered map leak"_test = []
    {
        MemoryLeakDetector detector;

        unordered_map<entity, int> map;

        const int test_scale = 1 << 12;

        for (uint32_t i = 0; i < test_scale; i++)
        {
            map[entity{i, 0}] = i;
        }


        //random remove
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                map.erase(entity{i, 0});
            }
        }

        for (uint32_t i = test_scale; i < test_scale + test_scale; i++)
        {
            map[entity{i, 0}] = i;
        }

        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                map.erase(entity{i, 0});
            }
        }

        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                map.erase(entity{i, 0});
            }
        }

    };


};