#include "ecs/storage/component_storage.h"
#include "ut.hpp"

using namespace hyecs;

namespace ut = boost::ut;


static ut::suite _ = [] {
    using namespace ut;

    "component storage"_test = [] {

        struct A
        {
            int a, b, c, d;
        };

        using namespace hyecs;

        component_group_index g = component_group_info{
                .id = component_group_id{}
        };
        component_type_index c1 = component_type_info(generic::type_info::of<A>(), g, false);

        component_storage storage(c1);


    };
};
