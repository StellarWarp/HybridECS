#pragma once

#include "ecs/registry/archetype_registry.h"
#include "storage/archetype_storage.h"
#include "storage/tag_archetype_storage.h"
#include "ecs/query/query.h"
#include "ecs/query/cross_query.h"
#include "ecs/query/query_parser.h"
#include "debug_util.h"

namespace hyecs
{
    struct sort_key
    {
    };

#define DEBUG_PRINT


    class data_registry
    {
    public:
        // type infos - longest life-time
        vaildref_map<uint64_t, component_type_info> m_component_type_infos;
        vaildref_map<component_group_id, component_group_info> m_component_group_infos;
        // archetypes
        archetype_registry m_archetype_registry;
        // storages
        storage_key_registry m_storage_key_registry;
        vaildref_map<uint64_t, component_storage> m_component_storages;
        vaildref_map<uint64_t, archetype_storage> m_archetypes_storage;
        vaildref_map<uint64_t, tag_archetype_storage> m_tag_archetypes_storage;
        // query
        //        vaildref_map<uint64_t,
        vaildref_map<uint64_t, query> m_queries;
        vaildref_map<uint64_t, cross_query> m_cross_queries;
        vaildref_map<uint64_t, table_tag_query> m_table_queries;
        // entity
        dense_set<entity> m_entities;

        class entity_allocator
        {
            uint32_t m_next_id;
            unordered_map<uint32_t, uint32_t> m_reusable_entity_versions{};

        public:
            entity_allocator()
                : m_next_id(0)
            {
            }

            entity allocate()
            {
                entity e(m_next_id, 0);
                m_next_id++;
                return e;
            }

            void deallocate(entity e)
            {
                m_reusable_entity_versions[e.id()] = e.version();
            }
        };

        entity_allocator m_entity_allocator;

        entity allocate_entity()
        {
            return m_entity_allocator.allocate();
        }

        void deallocate_entity(entity e)
        {
            m_entity_allocator.deallocate(e);
        }

        void allocate_entity(sequence_ref<entity> entities)
        {
            for (uint32_t i = 0; i < entities.size(); i++)
            {
                entity e = m_entity_allocator.allocate();
                entities[i] = e;
                m_entities.insert(e);
            }
        }

        void deallocate_entity(sequence_ref<entity> entities)
        {
            for (uint32_t i = 0; i < entities.size(); i++)
            {
                deallocate_entity(entities[i]);
                m_entities.erase(entities[i]);
            }
        }


        void add_untag_archetype(archetype_index arch)
        {
#if defined(DEBUG_PRINT)
            scope_output_color c(scope_output_color::cyan);
#endif
            assert(!m_archetypes_storage.contains(arch.hash()));
            vector<component_storage*> storages;
            storages.reserve(arch.component_count());
            for (auto& component: arch)
            {
                if (!component.is_empty())
                    storages.push_back(&m_component_storages.at(component.hash()));
            }
            //todo process for single component arch
            m_archetypes_storage.emplace_value(arch.hash(), arch,
                                               sorted_sequence_cref(storages),
                                               m_storage_key_registry.get_group_key_accessor());
#if defined(DEBUG_PRINT)
            std::cout << "add untag archetype " << arch << std::endl;
#endif
        }

        void add_tag_archetype(archetype_index arch, archetype_index base_arch)
        {
#if defined(DEBUG_PRINT)
            scope_output_color c(scope_output_color::cyan);
#endif
            assert(!m_tag_archetypes_storage.contains(arch.hash()));
            assert(m_archetypes_storage.contains(base_arch.hash()));
            archetype_storage* base_storage = &m_archetypes_storage.at(base_arch.hash());
            vector<component_storage*> tag_storages;
            for (auto& component: arch)
            {
                if (component.is_tag() && !component.is_empty())
                    tag_storages.push_back(&m_component_storages.at(component.hash()));
            }
            m_tag_archetypes_storage.emplace_value(arch.hash(), arch, base_storage, sorted_sequence_cref(tag_storages));
#if defined(DEBUG_PRINT)
            std::cout << "add tag archetype " << arch << std::endl;
#endif
        }

        void add_table_query(const archetype_registry::archetype_query_addition_info& info)
        {
#if defined(DEBUG_PRINT)
            scope_output_color c(scope_output_color::green);
#endif
            auto& [
                query_index,
                base_archetype_index,
                tag_condition,
                is_full_set,
                is_direct_set,
                notify_adding_tag,
                notify_partial_convert] = info;
            assert(!m_table_queries.contains(query_index));
            assert(m_archetypes_storage.contains(base_archetype_index.hash()));

            archetype_storage* base_storage = &m_archetypes_storage.at(base_archetype_index.hash());
            vector<component_storage*> tag_storages;
            for (auto& component: tag_condition.all())
            {
                assert(component.is_tag());
                if (!component.is_empty())
                    tag_storages.push_back(&m_component_storages.at(component.hash()));
            }
            table_tag_query& table_query = m_table_queries.emplace_value(
                query_index, base_storage, tag_storages, is_full_set, is_direct_set ASSERTION_CODE(, tag_condition));
            notify_adding_tag = [this, &table_query](archetype_index arch)
            {
                tag_archetype_storage* tag_storage = &m_tag_archetypes_storage.at(arch.hash());
                table_query.notify_tag_archetype_add(tag_storage);
            };
            notify_partial_convert = [this, &table_query]()
            {
                table_query.notify_partial_convert();
            };
#if defined(DEBUG_PRINT)
            std::cout << "add table query [" << base_archetype_index << "] + " << tag_condition;
            if (is_full_set)
                if (is_direct_set) std::cout << " direct set";
                else std::cout << " full set";
            else std::cout << " partial set";

            std::cout << std::endl;
#endif
        }

        void add_query(const archetype_registry::query_addition_info& info)
        {
#if defined(DEBUG_PRINT)
            scope_output_color c(scope_output_color::yellow);
#endif
            assert(!m_queries.contains(info.index));
            query& q = m_queries.emplace_value(info.index, info.condition);
            info.archetype_query_addition_callback = [this, &q](query_index table_query_index)
            {
                table_tag_query* table_query = &m_table_queries.at(table_query_index);
                q.notify_table_query_add(table_query);
#if defined(DEBUG_PRINT)
                std::cout << "query::" << q.condition_debug()
                        << " add table query " << "[" << table_query->base_archetype() << "] "
                        << table_query->tag_condition()
                        << std::endl;
#endif
            };
#if defined(DEBUG_PRINT)
            std::cout << "add query " << info.condition << std::endl;
#endif
        }

        component_group_info& register_component_group(component_group_id id, std::string name)
        {
            return m_component_group_infos.emplace(id, component_group_info{id, name, {}});
        }

        component_type_index register_component_type(generic::type_index type, component_group_info& group, bool is_tag)
        {
            component_type_index component_index = m_component_type_infos.emplace(type.hash(), component_type_info(type, group, is_tag));

            group.component_types.push_back(component_index);
            m_archetype_registry.register_component(component_index);
            if (!type.is_empty())
                m_component_storages.emplace_value(type.hash(), component_index);
            return component_index;
        }

    public:
        void register_type(const ecs_rtti_context& context)
        {
            for (auto& [_, group]: context.groups())
            {
                component_group_info& group_info = register_component_group(group.id, group.name);

                for (auto& [_, component]: group.components)
                {
                    register_component_type(component.type, group_info, component.is_tag);
                }
            }
        }

        data_registry()
        {
            m_archetype_registry.bind_untag_archetype_addition_callback(
                [this](archetype_index arch)
                {
                    add_untag_archetype(arch);
                }
            );
            m_archetype_registry.bind_tag_archetype_addition_callback(
                [this](archetype_index arch, archetype_index base_arch)
                {
                    add_tag_archetype(arch, base_arch);
                }
            );

            m_archetype_registry.bind_archetype_query_addition_callback(
                [this](const archetype_registry::archetype_query_addition_info& info)
                {
                    add_table_query(info);
                }
            );

            m_archetype_registry.bind_query_addition_callback(
                [this](const archetype_registry::query_addition_info& info)
                {
                    return add_query(info);
                }
            );
        }

        data_registry(const ecs_rtti_context& context) : data_registry()
        {
            register_type(context);
        }

        ~data_registry() = default;

        void emplace(
            sorted_sequence_cref<component_type_index> components,
            sorted_sequence_cref<generic::constructor> constructors,
            sequence_ref<entity> entities)
        {
            assert(components.size() == constructors.size());
            auto group_begin = components.begin();
            auto group_end = components.begin();

            allocate_entity(entities);

            while (group_begin != components.end())
            {
                group_end = [&]()
                {
                    auto it = group_begin;
                    auto g = group_begin->group();
                    while (it != components.end() && it->group() == g)
                        ++it;
                    return it;
                }();

                archetype_index arch = m_archetype_registry.get_archetype(append_component(group_begin, group_end));

                auto constructors_begin = constructors.begin() + (group_begin - components.begin());
                auto constructors_end = constructors.begin() + (group_end - components.begin());

                emplace_in_group(arch, entities, sorted_sequence_cref(constructors_begin, constructors_end));

                group_begin = group_end;
            }
        }

        void emplace_in_group(archetype_index arch,
                              sequence_cref<entity> entities,
                              sorted_sequence_cref<generic::constructor> constructors)
        {
            if (arch.component_count() == 1)
            {
                auto& storage = m_component_storages.at(arch[0].hash());
                auto component_accessor = storage.allocate(entities);
                for (void* addr: component_accessor)
                {
                    constructors[0](addr);
                }
            }
            else
            {
                auto construct_process = [&](auto&& allocate_accessor)
                {
                    auto constructor_iter = constructors.begin();

                    auto component_accessor = allocate_accessor.begin();
                    while (component_accessor != allocate_accessor.end())
                    {
                        assert(component_accessor.component_type() == constructor_iter->type());
                        for (void* addr: component_accessor)
                        {
                            (*constructor_iter)(addr);
                        }
                        ++constructor_iter;
                        ++component_accessor;
                    }
                    allocate_accessor.notify_construct_finish();
                };
                if (arch.is_tag())
                {
                    auto& storage = m_tag_archetypes_storage.at(arch.hash());
                    construct_process(storage.allocate(entities));
                }
                else
                {
                    auto& storage = m_archetypes_storage.at(arch.hash());
                    construct_process(storage.get_allocate_accessor(entities));
                }
            }
        }

        template<typename... T>
        void emplace_(
            sequence_ref<entity> entities,
            T&&... components
        )
        {
            const auto component_types_info = get_sorted_component_types<T...>();
            const auto component_types = [&]
            {
                std::array<component_type_index, sizeof...(T)> res{
                    component_types_info[type_list<T...>::template index_of<T>].second...
                };
                return res;
            }();

            const auto order_mapping = [&]
            {
                std::array<size_t, sizeof...(T)> res;
                for (size_t sorted_loc = 0; sorted_loc < sizeof...(T); ++sorted_loc)
                {
                    size_t origin_loc = component_types_info[sorted_loc].first;
                    res[origin_loc] = sorted_loc;
                }
                return res;
            }();

            const auto constructors = [&]()
            {
                std::array<generic::constructor, sizeof...(T)> res{};
                for_each_arg_indexed([&]<typename type>(type&& component, size_t index)
                {
                    res[order_mapping[index]] = generic::constructor(std::forward<type>(component));
                }, std::forward<T>(components)...);
                return res;
            }();

            emplace(sorted_sequence_cref(component_types), sorted_sequence_cref(constructors), entities);
        }


        auto& get_query(const query_condition& condition)
        {
            const query_index index = m_archetype_registry.get_query(condition);
            return m_queries.at(index);
        }

        auto& get_cross_query(const query_condition& condition)
        {
            vector<component_type_index> cond_all(condition.all().begin(), condition.all().end());
            vector<component_type_index> cond_none(condition.none().begin(), condition.none().end());
            //assert if cond_all and cond_none is sorted
            assert(std::ranges::is_sorted(cond_all));
            assert(std::ranges::is_sorted(cond_none));
            std::ranges::sort(cond_all);
            std::ranges::sort(cond_none);
            vector<component_group_id> groups;
            vector<uint32_t> all_group_range = {0};
            vector<uint32_t> none_group_range = {0};
            //the all cond defines the groups
            //that is each group must have a all condition
            assert(cond_all.size() != 0);
            groups.push_back(cond_all[0].group().id());
            for (auto [idx, comp]: cond_all | std::ranges::views::enumerate)
            {
                if (comp.group().id() != groups.back())
                {
                    groups.push_back(comp.group().id());
                    all_group_range.push_back(idx);
                }
            }
            all_group_range.push_back(cond_all.size());
            //ensure all none cond exist in the group
            ASSERTION_CODE(
                for (auto comp: cond_none)
                {
                auto gid = comp.group().id();
                assert(std::ranges::find(groups, gid) != groups.end());
                }
            );
            none_group_range.resize(groups.size() + 1, 0);
            auto prev_group = component_group_id{};
            for (auto [idx, comp]: cond_none | std::ranges::views::enumerate)
            {
                if (prev_group != comp.group().id())
                {
                    for (uint32_t group_idx = 0; group_idx < groups.size(); group_idx++)
                    {
                        if (groups[group_idx] == prev_group)
                        {
                            none_group_range[group_idx + 1] = idx;
                            break;
                        }
                    }
                    prev_group = comp.group().id();
                }
            }

            //cross group any is not supported, each any must be in a group
            ASSERTION_CODE(
                for(auto& any : condition.anys())
                {
                auto gid = any[0].group().id();
                for(auto comp: any)
                {
                assert(comp.group().id() == gid);
                }
                assert(std::ranges::find(groups, gid) != groups.end());
                }
            );
            vector<vector<vector<component_type_index> > > multi_group_anys;
            multi_group_anys.resize(groups.size());
            for (auto& any: condition.anys())
            {
                auto gid = any[0].group().id();
                uint32_t g_idx = std::ranges::find(groups, gid) - groups.begin();
                auto& new_any = multi_group_anys[g_idx].emplace_back();
                new_any.append_range(any);
            }

            auto get_group_condition = [&](uint32_t g_idx)
            {
                return query_condition(
                    sequence_cref(cond_all.data() + all_group_range[g_idx], cond_all.data() + all_group_range[g_idx + 1]),
                    multi_group_anys[g_idx],
                    sequence_cref(cond_none.data() + none_group_range[g_idx], cond_none.data() + none_group_range[g_idx + 1])
                );
            };

            auto q_hash = condition.hash();
            vector<query*> in_group_queries;
            in_group_queries.reserve(groups.size());

            for (auto [g_idx,group_id]: groups | std::ranges::views::enumerate)
            {
                query_condition in_group_condition = get_group_condition(g_idx);
                query& q = get_query(in_group_condition);
                in_group_queries.push_back(&q);
            }

            cross_query& c_query = m_cross_queries.emplace_value(q_hash, in_group_queries, this);

            return c_query;
        }

        component_type_index get_component_index(type_hash hash)
        {
            return m_component_type_infos.at(hash);
        }

        template<typename... T>
        auto get_sorted_component_types() -> const std::array<std::pair<size_t, component_type_index>, sizeof...(T)>
        {
            using indexed_type = std::pair<size_t, component_type_index>;
            auto arr = std::array<indexed_type, sizeof...(T)>{
                indexed_type{
                    type_list<T...>::template index_of<T>,
                    m_component_type_infos.at(type_hash::of<T>())
                }...
            };
            std::ranges::sort(arr, [](const indexed_type& lhs, const indexed_type& rhs)
            {
                return lhs.second < rhs.second;
            });
            return arr;
        }

        template<typename... T>
        auto component_types(type_list<T...>  = {}) -> const std::array<component_type_index, sizeof...(T)>
        {
            const auto component_types_info = get_sorted_component_types<T...>();
            const auto component_types = [&]
            {
                std::array<component_type_index, sizeof...(T)> res{
                    component_types_info[type_list<T...>::template index_of<T>].second...
                };
                return res;
            }();
            return component_types;
        }

        template<typename... T>
        auto unsorted_component_types(type_list<T...>  = {}) -> const std::array<component_type_index, sizeof...(T)>
        {
            const std::array<component_type_index, sizeof...(T)> component_types = {
                m_component_type_infos.at(type_hash::of<T>())...
            };
            return component_types;
        }

        void component_ramdom_access(entity e,
                                     sorted_sequence_cref<component_type_index> components,
                                     sequence_ref<void*> addresses)
        {
            assert(components.size() == addresses.size());
            auto group_begin = components.begin();
            auto group_end = components.begin();

            while (group_begin != components.end())
            {
                group_end = [&]()
                {
                    auto it = group_begin;
                    auto g = group_begin->group();
                    while (it != components.end() && it->group() == g)
                        ++it;
                    return it;
                }();

                auto in_group_address = addresses.sub_sequence(
                    group_begin - components.begin(),
                    group_end - components.begin()
                );
                in_group_component_ramdom_access(e, {group_begin, group_end}, in_group_address);

                group_begin = group_end;
            }
        }

        void in_group_component_ramdom_access(entity e,
                                              sorted_sequence_cref<component_type_index> components,
                                              sequence_ref<void*> addresses)
        {
            sorted_sequence_cref<component_type_index> untagged_components;
            sorted_sequence_cref<component_type_index> tagged_components;
            uint32_t tagged_begin = components.size();
            for (auto iter = components.begin(); iter != components.end(); ++iter)
            {
                auto component_index = *iter;
                if (component_index.is_tag())
                {
                    tagged_begin = iter - components.begin();
                    untagged_components = {components.begin(), iter};
                    tagged_components = {iter, components.end()};
                    ASSERTION_CODE(
                        for (auto idx: untagged_components) assert(!idx.is_tag());
                        for (auto idx: tagged_components) assert(idx.is_tag());
                    );
                    break;
                }
            }

            if (auto iter = m_storage_key_registry.find(e); iter != m_storage_key_registry.end())
            {
                auto st_key = iter->second;
                //todo build the fast table
                auto* table = m_storage_key_registry.find_table(st_key.get_table_index());
                small_vector<uint32_t> indices(untagged_components.size(), uint32_t(-1));
                //todo check overload function call
                table->get_component_indices(untagged_components, indices);
                table->components_addresses(st_key, indices, addresses.sub_sequence(0, tagged_begin));
            }
            else
            {
                for (uint32_t i = 0; i < untagged_components.size(); ++i)
                {
                    auto component_index = untagged_components[i];
                    void* addr = m_component_storages.at(component_index.hash()).at(e);
                    addresses[i] = addr;
                }
            }

            for (uint32_t i = 0; i < tagged_components.size(); ++i)
            {
                auto component_index = tagged_components[i];
                void* addr = m_component_storages.at(component_index.hash()).at(e);
                addresses[tagged_begin + i] = addr;
            }
        }

    private:
#pragma region cross_query code

        friend class cross_query;

#pragma endregion
    };

#pragma region cross_query code

    inline void cross_query::dynamic_for_each(
        const access_info& acc_info,
        function<void(entity, sequence_ref<void*>)> func)
    {
        small_vector<void*> addresses_cache(acc_info.access_list.size());
        small_vector<void*> addresses(acc_info.access_list.size());
        auto& sorted_components = acc_info.sorted_components;
        auto& group_division = acc_info.group_division;

        for (auto e: m_entities)
        {
            for (auto [i, q]: m_in_group_queries | std::ranges::views::enumerate)
            {
                auto comp_begin_idx = group_division[i * 2];
                auto tag_begin_idx = group_division[i * 2 + 1];
                auto comp_end_idx = group_division[i * 2 + 2];
                auto comp_begin = sorted_components.begin() + comp_begin_idx;
                auto tag_begin = sorted_components.begin() + tag_begin_idx;
                auto comp_end = sorted_components.begin() + comp_end_idx;

                auto addresses = sequence_ref(addresses_cache).sub_sequence(comp_begin_idx, tag_begin_idx);

                if (auto iter = m_data_registry->m_storage_key_registry.find(e); iter != m_data_registry->m_storage_key_registry.end())
                {
                    auto st_key = iter->second;
                    //todo build the fast table
                    auto* table = m_data_registry->m_storage_key_registry.find_table(st_key.get_table_index());
                    //base part

                    //todo component index cache ?

                    // uint32_t table_cache_index = std::numeric_limits<uint32_t>::max();
                    // for (auto idx: acc_info.table_indices_cache.table_index)
                    // {
                    //     if (idx == table_index)
                    //     {
                    //         table_cache_index = idx;
                    //     }
                    // }
                    //
                    // if (table_cache_index == std::numeric_limits<uint32_t>::max())
                    // {
                    //
                    //     auto all_indices = table->get_all_component_indices();
                    //
                    //     uint32_t index = 0;
                    //     table->components_addresses(st_key,
                    //                                 tag_begin_idx - comp_begin_idx,
                    //                                 [&]()
                    //                                 {
                    //                                     const auto& elem = *comp_begin;
                    //                                     assert(elem >= all_indices[index]);
                    //                                     while (elem != all_indices[index])
                    //                                         index++;
                    //                                 },
                    //                                 [&](void* addr)
                    //                                 {
                    //                                     addresses_cache[comp_begin_idx] = addr;
                    //                                     ++comp_begin;
                    //                                     ++comp_begin_idx;
                    //                                 }
                    // }
                    // else
                    // {
                    //     auto& indices = acc_info.table_indices_cache.component_indices[table_cache_index];
                    //     table->components_addresses(st_key, indices, addresses);
                    // }

                    auto all_indices = table->get_all_component_indices();

                    uint32_t index = 0;
                    table->components_addresses(
                        st_key,
                        tag_begin_idx - comp_begin_idx,
                        [&]()
                        {
                            const auto& elem = *comp_begin;
                            assert(elem >= all_indices[index]);
                            while (elem != all_indices[index])
                                index++;
                            return index;
                        },
                        [&](void* addr)
                        {
                            addresses_cache[comp_begin_idx] = addr;
                            ++comp_begin;
                            ++comp_begin_idx;
                        });
                }
                else
                {
                    for (; comp_begin != tag_begin; ++comp_begin, ++comp_begin_idx)
                    {
                        assert(!comp_begin->is_tag());
                        addresses_cache[comp_begin_idx] = m_data_registry->m_component_storages.at(comp_begin->hash()).at(e);
                    }
                }

                //tag part
                for (; tag_begin != comp_end; ++tag_begin, ++tag_begin_idx)
                {
                    assert(tag_begin->is_tag());
                    addresses_cache[tag_begin_idx] = m_data_registry->m_component_storages.at(tag_begin->hash()).at(e);
                }
            }

            //invoke
            for (uint64_t access_i = 0; access_i < addresses.size(); ++access_i)
            {
                auto comp_i = acc_info.access_i_to_component_i[access_i];
                addresses[access_i] = addresses_cache[comp_i];
            }
            func(e, addresses);
        }
    }

#pragma endregion


    class executer_builder
    {
        template<typename T>
        struct is_query_descriptor
        {
            static constexpr bool value = std::is_base_of_v<query_parameter::descriptor_param, T>;
        };

        data_registry& m_registry;

    public:
        executer_builder(data_registry& registry) : m_registry(registry)
        {
        }

        template<typename Callable>
        auto register_executer(Callable&& callable)
        {
            using params = typename function_traits<std::decay_t<Callable> >::args;

            auto descriptor = query_descriptor(params{});

            //todo scoped description formation
            //			small_vector<component_type_index> cond_all;
            //			small_vector<component_type_index> cond_none;
            //			small_vector<small_vector<component_type_index>> cond_anys;
            //			cond_all.reserve(descriptor.cond_all.size());
            //			cond_none.reserve(descriptor.cond_none.size());
            //			cond_anys.reserve(descriptor.cond_anys.size());
            //
            //			std::ranges::sort(descriptor.cond_all);
            //			std::ranges::sort(descriptor.cond_none);
            //
            //			for (auto hash : descriptor.cond_all)
            //				cond_all.push_back(m_registry.get_component_index(hash));
            //			for (auto hash : descriptor.cond_none)
            //				cond_none.push_back(m_registry.get_component_index(hash));
            //			for (auto& vec : descriptor.cond_anys)
            //			{
            //				auto& cond_any = cond_anys.emplace_back();
            //				cond_any.reserve(vec.size());
            //				for (auto hash : vec)
            //					cond_any.push_back(m_registry.get_component_index(hash));
            //			}
            //
            //			//todo enhance the assertion
            //			ASSERTION_CODE(
            //
            //				auto is_unique = [](auto& sorted_range)->bool {
            //				if (sorted_range.size() <= 1) return true;
            //				for (int i = 1; i < sorted_range.size(); ++i)
            //					if (sorted_range[i] == sorted_range[i - 1])
            //						return false;
            //				return true;
            //				};
            //
            //				assert(is_unique(descriptor.cond_all));
            //				assert(is_unique(descriptor.cond_none));
            //				for (auto& vec : descriptor.cond_anys)
            //					assert(is_unique(vec));
            //
            //				);
            //
            //
            //
            //
            //
            //			query_condition cond(cond_all, cond_anys, cond_none);
            //
            //
            //			const auto& q = m_registry.get_query(cond);
            //
            //			q.condition_debug();


            //vector<component_type_index> access_components = std::ranges::views::transform(descriptor.access_components,
            //	[&](access_info info) { return m_registry.get_component_index(info.hash); });

            //vector<component_type_index> variant_access_components = std::ranges::views::transform(descriptor.variant_access_components,
            //	[&](auto&& vec) { return std::ranges::views::transform(vec,
            //		[&](access_info info) { return m_registry.get_component_index(info.hash); }); });
            //vector<component_type_index> optional_access_components = std::ranges::views::transform(descriptor.optional_access_components,
            //	[&](access_info info) { return m_registry.get_component_index(info.hash); });
        }
    };
}
