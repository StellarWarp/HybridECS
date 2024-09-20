#pragma once

#include "ecs/table.h"
#include "ecs/sparse_table.h"
#include "ut.hpp"

using namespace hyecs;

namespace ut = boost::ut;


static ut::suite _ = [] {
    using namespace ut;

    "table"_test = [] {

        struct A
        {
            int a, b, c, d;
        };

        struct B
        {
            int a, b, c, d;
        };

        component_group_index g = component_group_info{
                .id = component_group_id{}
        };
        component_type_index c1 = component_type_info(generic::type_info::of<A>(), g, false);
        component_type_index c2 = component_type_info(generic::type_info::of<B>(), g, false);
        vector<component_type_index> components = { c1,c2 };
        std::ranges::sort(components);
        auto seq = sorted_sequence_cref(sequence_cref(components));
        table table(seq);

        vector<entity> entities;
        for (uint32_t i = 0; i < 10240; i++)
        {
            entities.push_back(entity{ i,0 });
        }

        vector< storage_key::table_offset_t> keys;
        unordered_map<entity, storage_key> key_map;
        unordered_map<storage_key::table_offset_t, entity> entity_map;

        auto accessor = table.get_allocate_accessor(sequence_ref(entities).as_const(), [&](entity e, storage_key key)
                                                    {
                                                        keys.push_back(key.get_table_offset());
                                                        key_map.insert({ e,key });
                                                        entity_map.insert({ key.get_table_offset(),e });
                                                        std::cout << (uint32_t)key.get_table_offset() << std::endl;
                                                    }
        );

        int j = 0;
        for (auto& component_accessor : accessor)
        {
            int i = 0;
            for (void* addr : component_accessor)
            {
                new(addr) A{ j * 10000 + i,j * 10000 + i + 1,j * 10000 + i + 2,j * 10000 + i + 3 };
                i++;
            }
            j++;
        }

        accessor.notify_construct_finish();

        auto raw_accessor = table.get_raw_accessor();
        for (auto& component_accessor : raw_accessor)
        {
            auto entity_accessor = table.get_entities();
            auto entity_iter = entity_accessor.begin();
            for (void* addr : component_accessor)
            {
                A* a = (A*)addr;
                std::cout << entity_iter->id() << " : "
                          << a->a << " " << a->b << " " << a->c << " " << a->d << std::endl;
                entity_iter++;
            }
        }

        vector< storage_key::table_offset_t> deallocate_keys;
        ////radom deallocate
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

        //for (uint32_t i = 0; i < 1; i++)
        //{
        //	deallocate_keys.push_back(keys[i]);
        //	auto e = entity_map[keys[i]];
        //	key_map.erase(e);
        //	entity_map.erase(keys[i]);
        //}

        auto deallocate_accessor = table.get_deallocate_accessor(deallocate_keys);

        deallocate_accessor.destruct();

        table.phase_swap_back();

        raw_accessor = table.get_raw_accessor();
        for (auto& component_accessor : raw_accessor)
        {
            auto entity_accessor = table.get_entities();
            auto entity_iter = entity_accessor.begin();
            for (void* addr : component_accessor)
            {
                A* a = (A*)addr;
                std::cout << entity_iter->id() << " : "
                          << a->a << " " << a->b << " " << a->c << " " << a->d << std::endl;
                entity_iter++;
            }
        }

        auto entity_accessor = table.get_entities();
        for (auto e : entity_accessor)
        {
            expect(key_map.contains(e));
        }
    };
};
