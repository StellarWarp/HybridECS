#pragma once

#include "ecs/dense_map.h"

#include "ut.hpp"

using namespace hyecs;

namespace ut = boost::ut;

static ut::suite _ = [] {
    using namespace ut;
    "dense_map"_test = [] {
        raw_entity_dense_map table(sizeof(int), alignof(int));

        unordered_map<entity, int> map;

        const int test_scale = 1<<12;

        for (uint32_t i = 0; i < test_scale; i++)
        {
        	new(table.allocate_value(entity{ i,0 })) int(i);
        	map[entity{ i,0 }] = i;
        }

        //random remove

        for (uint32_t i = 0; i < test_scale; i++)
        {
        	if (rand() % 2 == 0)
        	{
        		table.deallocate_value(entity{ i,0 });
        		map.erase(entity{ i,0 });
        	}
        }

        for (uint32_t i = 1024; i < 1024 + test_scale; i++)
        {
        	new(table.allocate_value(entity{ i,0 })) int(i);
        	map[entity{ i,0 }] = i;
        }

        for (uint32_t i = 0; i < test_scale; i++)
        {
        	if (rand() % 2 == 0)
        	{
        		if(table.contains(entity{ i,0 }))
        			table.deallocate_value(entity{ i,0 });
        		map.erase(entity{ i,0 });
        	}
        }

        for (uint32_t i = 0; i < test_scale; i++)
        {
        	if (rand() % 2 == 0)
        	{
        		if (table.contains(entity{ i,0 }))
        			table.deallocate_value(entity{ i,0 });
        		map.erase(entity{ i,0 });
        	}
        }


        for(raw_entity_dense_map::entity_value ev:table)
        {
        	std::cout << ev.e << " " << *(int*)ev.value << std::endl;
        }

        for (auto& [e, v] : map)
        {
        	expect(*(int*)table.at(e) == v);
        }

        entity_sparse_set set;

        set.insert(entity{ 0,0 });
    };
};