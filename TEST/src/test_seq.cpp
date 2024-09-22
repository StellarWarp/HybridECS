//#include "ecs/archetype_registry.h"
#include "container/container.h"
#include "ut.hpp"

//#include "ecs/entity.h"
////#include "meta/meta_utils.h"
// #include "ecs/sparse_table.h"
// #include "ecs/table.h"

// #include "ecs/archetype_storage.h"
//#include "ecs/component_storage.h"
// #include "ecs/tag_archetype_storage.h"
//#include "ecs/data_registry.h"

//#include "ecs/archetype_storage.h"
//#include "ecs/archetype_storage.h"
//#include "test_archetype_registry.h"

using namespace hyecs;

template<typename SeqParam>
void test_func(sequence_cref<int, SeqParam> seq)
{
    for (auto i: seq)
    {
        std::cout << i << std::endl;
    }
}

namespace ut = boost::ut;


static ut::suite _ = []
{
    using namespace ut;

    "array assign"_test = []
    {
        std::array a1{1, 2, 3, 4, 5};
        sequence_ref s = a1;
        sequence_cref seq = a1;
    };

    "vector assign"_test = []
    {
        vector<int> vec = {1, 2, 3, 4, 5};
        sequence_ref seq1 = vec;
        sequence_ref seq2(vec.begin(), vec.end());

        vector<int> vec2 = {};
        sequence_ref seq3(vec2.begin(), vec2.end());

        const vector<int> cvec = {1, 2, 3, 4, 5};
        sequence_ref seq5 = cvec;
    };

    "small vector"_test = []
    {
        sequence_cref seq = {1,2,3,4,5};
        small_vector<int> svec{seq};

    };

    "function overload"_test = []
    {
        struct fs
        {
            static void f(sequence_ref<int> seq){}
            static void f(sorted_sequence_ref<int> seq) {}
        };
        struct cfs
        {
            static void f(sequence_cref<int> seq) {}
            static void f(sorted_sequence_cref<int> seq) {}
        };
        vector<int> vec = {1,2,3,4};
        fs::f(vec);
        fs::f(sorted_sequence_ref(vec));

        cfs::f({1,2,3,4});
        cfs::f(sorted_sequence_cref(sequence_cref{1, 2, 3, 4}));

    };

    "other container"_test = []
    {
        set<int> set;
        for (int i = 0; i < 10; i++)
        {
            set.insert(i);
        }

        sequence_ref seq5 = set;
        sequence_cref seq6 = set;
        auto seq8 = sorted_sequence_ref(sequence_ref(set));
        auto seq7 = sorted_sequence_cref(sequence_cref(set));


        struct test_container
        {
            struct iterator
            {
                using difference_type = ptrdiff_t;
                using value_type = int;

                int& operator*()
                {
                    static int a;
                    return a;
                }

                iterator& operator++() { return *this; }

                iterator operator++(int) { return *this; }
            };


            iterator begin() { return {}; }

            nullptr_t end() { return {}; }
        };

        test_container testc;

        using t = std::ranges::iterator_t<test_container>;
        using t1 = std::iter_value_t<t>;


        sequence_ref seq11 = testc;
    };
};
