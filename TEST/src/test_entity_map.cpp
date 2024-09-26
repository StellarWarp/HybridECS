#include "ecs/storage/entity_map.h"
#include "include.h"

using namespace hyecs;

namespace ut = boost::ut;



static ut::suite _ = []
{
    using namespace ut;

    using O = managed_object_tester<[]{}, true>;

    "sparse_map"_test = []
    {
        MemoryLeakDetector detector;

        entity_sparse_map<uint32_t> map;
        unordered_map<entity, uint32_t> std_map;

        const int test_scale = 1 << 12;

        for (uint32_t i = 0; i < test_scale; ++i)
        {
            entity e = {i,0};
            map.insert({e, i});
            std_map.insert({e, i});
        }

        for (auto& [e, o] : std_map)
        {
            expect(map.at(e) == o);
        }

        //random remove
        vector<entity> remove_list;
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                entity e = { i, 0 };
                map.erase(e);
                std_map.erase(e);
                remove_list.push_back(e);
            }
        }

        for (auto& [e, o] : std_map)
        {
            expect(map.at(e) == o);
        }

        for (auto& e : remove_list)
        {
            expect(map.contains(e) == false);
        }

        for (uint32_t i = 0; i < test_scale; ++i)
        {
            entity e = {i,0};
            expect(map.contains(e) == std_map.contains(e));
        }

        //random add
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                entity e = {i,0};

                if(!map.contains(e))
                {
                    map.insert({e, i});
                    std_map.insert({e, i});
                }
            }
        }

        for (uint32_t i = 0; i < test_scale; ++i)
        {
            entity e = {i,0};
            expect(map.contains(e) == std_map.contains(e));
        }

        //random remove
        remove_list.clear();
        for (uint32_t i = 0; i < test_scale; i++)
        {
            if (rand() % 2 == 0)
            {
                entity e = { i, 0 };
                map.erase(e);
                std_map.erase(e);
                remove_list.push_back(e);
            }
        }

        for(auto& e:remove_list)
        {
            e = entity{ e.id(), 1};
            //add back
            map.insert({e, e.id()});
            std_map.insert({e, e.id()});
        }

        for(auto& e:remove_list)
        {
            expect(map.contains(e) == std_map.contains(e));
        }

        struct test{
            std::pair<uint32_t,void*> p;
            uint32_t a;
        };

        const auto tt_size = sizeof(test);
        const auto ttt_size = sizeof(std::tuple<uint32_t,uint32_t,void*>);



    };

};