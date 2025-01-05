#pragma once

#include <ecs/data_registry.h>

#include "query.h"

namespace hyecs
{
    //todo thread safe
    class cross_query
    {
        entity_sparse_map<uint32_t> m_potential_entities;
        dense_set<entity> m_entities;
        vector<query*> m_in_group_queries;
        class data_registry* m_data_registry;

        ASSERTION_CODE(query_condition m_condition);


        uint32_t group_count() const
        {
            return m_in_group_queries.size();
        }

    public:
        cross_query(sequence_ref<query*> queries, data_registry* data_registry)
            : m_in_group_queries(queries), m_data_registry(data_registry)
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
            small_vector<uint32_t> group_division; // | base begin | tag begin  | base begin | tag begin... | end
            small_vector<component_type_index> sorted_components;
            small_vector<component_type_index> access_list; //access order

            //todo redesign this
            struct table_indices_cache
            {
                vector<uint32_t> table_index;
                vector<sequence_ref<uint32_t> > component_indices;
                vector<uint32_t> component_indices_storage;
            };

            table_indices_cache table_indices_cache;

            access_info(sequence_cref<component_type_index> access_list)
                : access_list(access_list), sorted_components(access_list)
            {
                std::ranges::sort(sorted_components);
                //find the order mapping
                access_i_to_component_i.resize(access_list.size());
                for (uint32_t i = 0; i < access_list.size(); i++)
                {
                    auto iter = std::ranges::find(sorted_components, access_list[i]);
                    assert(iter != sorted_components.end());
                    access_i_to_component_i[i] = iter - sorted_components.begin();
                }

                //compute the group_division
                size_t group_count = 0;
                component_group_id prev_group = component_group_id{};
                for (const auto& comp: access_list)
                    if (comp.group().id() != prev_group)
                    {
                        prev_group = comp.group().id();
                        group_count++;
                    }
                group_division.reserve(group_count * 2 + 1);
                uint32_t index = 0;
                bool tag_section = true;
                prev_group = component_group_id{};
                for (const auto& comp: sorted_components)
                {
                    if (comp.group().id() != prev_group)
                    {
                        prev_group = comp.group().id();
                        if (!tag_section)
                            group_division.push_back(index);
                        else
                            tag_section = false;
                        group_division.push_back(index);
                    }
                    else if (!tag_section && comp.is_tag())
                    {
                        tag_section = true;
                        group_division.push_back(index);
                    }
                    index ++;
                }
                if (!tag_section)
                    group_division.push_back(access_list.size());
                group_division.push_back(access_list.size());
            }
        };

    private:
        using access_hash = uint64_t; //arch hash of access component list
        map<access_hash, access_info> m_access_infos;

    public:
        const access_info& get_access_info(sequence_cref<component_type_index> access_list)
        {
            access_hash hash = archetype::addition_hash(0, append_component(access_list));
            if (auto iter = m_access_infos.find(hash); iter != m_access_infos.end())
                return iter->second;
            auto [iter, _] = m_access_infos.insert(
                {
                    hash, access_info{access_list}
                });
            auto& info = iter->second;
            return info;
        }


        void dynamic_for_each(
            const access_info& acc_info,
            function<void(entity, sequence_ref<void*>)> func);


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
