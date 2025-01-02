#pragma once

#include "query.h"

namespace hyecs
{
    //todo thread safe
    class cross_query
    {
        entity_sparse_map<uint32_t> m_potential_entities;
        dense_set<entity> m_entities;
        vector<query*> m_in_group_queries;

        ASSERTION_CODE(query_condition m_condition);


        uint32_t group_count() const
        {
            return m_in_group_queries.size();
        }

    public:

        cross_query(sequence_ref<query*> queries)
                : m_in_group_queries(queries)
        {
            for (auto q: queries)
            {
                q->bind_entity_event(
                        [this](entity e, storage_key)
                        {
                            notify_super_query_add(e);
                        },
                        [this](entity e)
                        {
                            notify_super_query_remove(e);
                        }
                );
            }
        }


        void notify_super_query_add(entity e)
        {
            auto& counter = m_potential_entities[e];
            counter += 1;
            if (counter == group_count())
                m_entities.insert(e);
        }

        void notify_super_query_remove(entity e)
        {
            auto& counter = m_potential_entities[e];
            if (counter == group_count())
                m_entities.erase(e);
            counter -= 1;
        }

#ifdef HYECS_DEBUG

        const query_condition& condition_debug() const { return m_condition; }

#endif

        struct access_info
        {
            small_vector<uint32_t> access_i_to_component_i;
            small_vector<component_type_index> components;
            struct table_indices_cache
            {
                vector<uint32_t> table_index;
                vector<sequence_ref<uint32_t>> component_indices;
                vector<uint32_t> component_indices_storage;
            };
            table_indices_cache table_indices_cache;
        };


//        class iteration_distributor
//        {
//            cross_query& m_q;
//
//
//            class iteration_set
//            {
//                sequence_cref<entity> m_entities;
//                sorted_sequence_cref<component_type_index> components;
//                small_vector<void*> m_addresses;
//                sequence_cref<uint32_t> access_i_to_component_i;
//                function<void(entity,
//                              sorted_sequence_cref<component_type_index>,
//                              sequence_ref<void*>)> m_random_access;
//
//                template<typename Callable>
//                void for_each(Callable&& func)
//                {
//                    for(auto e: m_entities)
//                    {
//                        m_random_access(e, components, m_addresses);
//                        system_callable_invoker<Callable&&> invoker(func);
//                        invoker.invoke(
//                                [&]{ return e; },
//                                [&]{ return storage_key{}; },
//                                [&]<typename type>(type_wrapper<type>, size_t access_i){
//                                    return m_addresses[access_i_to_component_i[access_i]];
//                                }
//                        );
//                    }
//                }
//            };
//
//        }


    };
}