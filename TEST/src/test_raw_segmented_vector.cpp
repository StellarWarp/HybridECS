#include "ut.hpp"
#include "memleak_detect.h"
#include "ecs/storage/entity_map.h"
#include "container/raw_segmented_vector.h"

using namespace hyecs;

namespace ut = boost::ut;

static ut::suite _ = [] {
    using namespace ut;
    "raw_segmented_vector"_test = [] {

        MemoryLeakDetector detector;

        struct A
        {
            uint32_t a, b, c, d;
        };

        raw_segmented_vector vec(sizeof(A), alignof(A));

        const size_t test_scale = 1 << 12;

        vector<std::pair<void*,uint32_t>> index_vec;

        for (uint32_t i = 0; i < test_scale; i++)
        {
            auto& index = index_vec.emplace_back(vec.allocate_value());
            new(index.first) A{i, i + 1, i + 2, i + 3};
        }

        //random remove
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                vec.deallocate_value(index_vec[i].first, index_vec[i].second,[]{});
            }
        }

    };

    "entity_sparse_map"_test = [] {

        MemoryLeakDetector detector;

        struct A
        {
            uint32_t a, b, c, d;
        };

        entity_sparse_map<A> map;

        const size_t test_scale = 1 << 12;

        for (uint32_t i = 0; i < test_scale; i++)
        {
            map.emplace(entity{i, 0}, A{i, i + 1, i + 2, i + 3});
        }

        //random remove
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                map.erase(entity{i, 0});
            }
        }



    };
};