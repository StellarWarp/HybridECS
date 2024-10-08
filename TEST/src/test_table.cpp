#include "ecs/storage/table.h"
#include "ecs/storage/sparse_table.h"
#include "ut.hpp"
#include "memleak_detect.h"
#include "managed_object_tester.h"

using namespace hyecs;

namespace ut = boost::ut;

namespace
{
    struct A
    {
        int a, b, c, d;
    };

    using B = managed_object_tester<[]{}>;
}

static ut::suite _ = []
{
    using namespace ut;

    component_group_index g = component_group_info{
            .id = component_group_id{}
    };
    component_type_index c1 = component_type_info(generic::type_info::of<A>(), g, false);
    component_type_index c2 = component_type_info(generic::type_info::of<B>(), g, false);
    vector<component_type_index> components = {c1, c2};
    std::ranges::sort(components);
    auto comp_seq = sorted_sequence_cref(sequence_cref(components));

    "small scale table"_test = [&]
    {
        MemoryLeakDetector detector;

        table table(comp_seq);

        vector<entity> entities;
        for (uint32_t i = 0; i < 20; i++)
        {
            entities.push_back(entity{i, 0});
        }

        vector<storage_key::table_offset_t> keys;
        unordered_map<entity, storage_key> key_map;
        unordered_map<storage_key::table_offset_t, entity> entity_map;

        auto accessor = table.get_allocate_accessor(sequence_ref(entities).as_const(), [&](entity e, storage_key key)
                                                    {
                                                        keys.push_back(key.get_table_offset());
                                                        key_map.insert({e, key});
                                                        entity_map.insert({key.get_table_offset(), e});
                                                    }
        );

        int j = 0;
        for (auto& component_accessor: accessor)
        {
            int i = 0;
            switch (j)
            {
                case 0:
                    for (void* addr: component_accessor)
                    {
                        new(addr) A{j * 10000 + i, j * 10000 + i + 1, j * 10000 + i + 2, j * 10000 + i + 3};
                        i++;
                    }
                    break;
                case 1:
                    for (void* addr: component_accessor)
                    {
                        new(addr) B{j * 10000 + i, j * 10000 + i + 1, j * 10000 + i + 2, j * 10000 + i + 3};
                        i++;
                    }
            }
            j++;
        }
        accessor.notify_construct_finish();

        vector<storage_key::table_offset_t> deallocate_keys;
        //// ramdom deallocate
        for (uint32_t i : {1,2,3,4})
        {
            deallocate_keys.push_back(keys[i]);
            auto e = entity_map[keys[i]];
            key_map.erase(e);
            entity_map.erase(keys[i]);
        }

        auto deallocate_accessor = table.get_deallocate_accessor(deallocate_keys);

        deallocate_accessor.destruct();

        table.phase_swap_back();

    };

    "leak test"_test = []
    {
        expect(B::object_counter == 0)
        << "B::object_counter = " << B::object_counter << "\n"
        << "B::construct_call = " << B::construct_call << "\n"
        << "B::move_call = " << B::move_call << "\n"
        << "B::copy_call = " << B::copy_call << "\n"
        << "B::destroy_call = " << B::destroy_call << "\n";
    };

    "table"_test = [&]
    {
        MemoryLeakDetector detector;

        table table(comp_seq);

        vector<entity> entities;
        for (uint32_t i = 0; i < 10240; i++)
        {
            entities.push_back(entity{i, 0});
        }

        vector<storage_key::table_offset_t> keys;
        unordered_map<entity, storage_key> key_map;
        unordered_map<storage_key::table_offset_t, entity> entity_map;

        auto accessor = table.get_allocate_accessor(sequence_ref(entities).as_const(), [&](entity e, storage_key key)
                                                    {
                                                        keys.push_back(key.get_table_offset());
                                                        key_map.insert({e, key});
                                                        entity_map.insert({key.get_table_offset(), e});
                                                    }
        );
        const int B_offset = 12345;

        for (auto& component_accessor: accessor)
        {
            int i = 0;
            switch (component_accessor.comparable().hash())
            {
                case type_hash::of<A>():
                    for (void* addr: component_accessor)
                    {
                        new(addr) A{i, i + 1, i + 2, i + 3};
                        i++;
                    }
                    break;
                case type_hash::of<B>():
                    for (void* addr: component_accessor)
                    {
                        new(addr) B{B_offset + i, B_offset + i + 1, B_offset + i + 2, B_offset + i + 3};
                        i++;
                    }
                    break;
                default:
                    assert(false);
            }
        }

        accessor.notify_construct_finish();

        auto raw_accessor = table.get_raw_accessor();
        for (auto& component_accessor: raw_accessor)
        {
            auto entity_accessor = table.get_entities();
            auto entity_iter = entity_accessor.begin();
            for (void* addr: component_accessor)
            {
                switch (component_accessor.comparable().hash())
                {
                    case type_hash::of<A>():
                    {
                        A* a = (A*) addr;
                        bool res = expect(entity_iter->id() == a->a
                                          && a->a + 1 == a->b
                                          && a->b + 1 == a->c
                                          && a->c + 1 == a->d)
                                << "entity : " << entity_iter->id()
                                << "a : " << a->a << " " << a->b << " " << a->c << " " << a->d;
                        assert(res);
                    }
                        break;
                    case type_hash::of<B>():
                    {
                        B* b = (B*) addr;
                        bool res = expect(entity_iter->id() + B_offset == b->a
                                          && b->a + 1 == b->b
                                          && b->b + 1 == b->c
                                          && b->c + 1 == b->d)
                                << "entity : " << entity_iter->id()
                                << "b : " << b->a << " " << b->b << " " << b->c << " " << b->d;
                        assert(res);
                    }
                        break;
                    default:
                        assert(false);
                }
                entity_iter++;
            }
        }

        vector<storage_key::table_offset_t> deallocate_keys;
        // ramdom deallocate
        for (uint32_t _ = 0; _ < 10240; _++)
        {
            if (uint32_t i = rand() % 10240; key_map.contains(entities[i]))
            {
                deallocate_keys.push_back(keys[i]);
                auto e = entity_map[keys[i]];
                key_map.erase(e);
                entity_map.erase(keys[i]);
            }
        }

        auto deallocate_accessor = table.get_deallocate_accessor(deallocate_keys);

        deallocate_accessor.destruct();

        table.phase_swap_back();

        raw_accessor = table.get_raw_accessor();
        for (auto& component_accessor: raw_accessor)
        {
            auto entity_accessor = table.get_entities();
            auto entity_iter = entity_accessor.begin();
            for (void* addr: component_accessor)
            {
                switch (component_accessor.comparable().hash())
                {
                    case type_hash::of<A>():
                    {
                        A* a = (A*) addr;
                        bool res = expect(entity_iter->id() == a->a
                                          && a->a + 1 == a->b
                                          && a->b + 1 == a->c
                                          && a->c + 1 == a->d)
                                << "entity : " << entity_iter->id()
                                << "a : " << a->a << " " << a->b << " " << a->c << " " << a->d;
                        assert(res);
                    }
                        break;
                    case type_hash::of<B>():
                    {
                        B* b = (B*) addr;
                        bool res = expect(entity_iter->id() + B_offset == b->a
                                          && b->a + 1 == b->b
                                          && b->b + 1 == b->c
                                          && b->c + 1 == b->d)
                                << "entity : " << entity_iter->id()
                                << "b : " << b->a << " " << b->b << " " << b->c << " " << b->d;
                        assert(res);
                    }
                        break;
                    default:
                        assert(false);
                }
                entity_iter++;
            }
        }

        auto entity_accessor = table.get_entities();
        for (auto e: entity_accessor)
        {
            expect(key_map.contains(e));
        }
    };
    "leak test"_test = []
    {
        expect(B::object_counter == 0)
                << "B::object_counter = " << B::object_counter << "\n"
                << "B::construct_call = " << B::construct_call << "\n"
                << "B::move_call = " << B::move_call << "\n"
                << "B::copy_call = " << B::copy_call << "\n"
                << "B::destroy_call = " << B::destroy_call << "\n";
    };

};
