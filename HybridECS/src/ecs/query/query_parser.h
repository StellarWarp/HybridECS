#pragma once

#include "query_api.h"

namespace hyecs
{


    struct query_descriptor
    {


        enum class relation_category
        {
            unsolved,
            single_reference,
            single_embedded,
            multi_embedded,
        };

        struct filer_info
        {

        };

        struct access_info
        {
            enum read_write_tag
            {
                ro, wo, rw
            };

            type_hash hash;
            uint32_t param_index;
            read_write_tag read_write;
        };

        struct anys_access_info
        {
            uint32_t param_index;
            small_vector<access_info> access_components;
        };

        struct relation_reference_info
        {
            std::string name;
            type_hash tag_hash;
            uint32_t param_index;
            relation_category category;
            uint32_t scope_index = -1;
        };

        struct entity_access_info
        {
            //for reference
            std::string scope_name{};
            type_hash tag_hash{};
            //filter
            small_vector<type_hash> cond_all{};
            small_vector<type_hash> cond_none{};
            vector<small_vector<type_hash>> cond_anys{};
            //relation
            vector<relation_reference_info> relation_references{};
            //access component
            small_vector<access_info> access_components{};
            small_vector<access_info> optional_access_components{};
            small_vector<anys_access_info> variant_access_components{};
            //info
            int entity_access = -1;
            int storage_key_access = -1;
        };

        vector<entity_access_info> multi_access_info{};
        std::stack<uint32_t> scope_stack{};

        void init()
        {
            scope_stack.push(0);
            auto& scope = multi_access_info.emplace_back();
            scope.scope_name = "root";
        }

        void push_reference_scope(uint32_t param_index, type_hash tag_hash, const std::string& name)
        {
            scope_stack.push(multi_access_info.size());
            auto& new_scope = multi_access_info.emplace_back();
            new_scope.scope_name = name;
            new_scope.tag_hash = tag_hash;
        }


        void push_embedded_scope(uint32_t param_index,
                                 type_hash tag_hash,
                                 relation_category category)
        {
            assert(category == relation_category::multi_embedded || category == relation_category::single_embedded);
            auto& scope = get_current_scope();
            //embedded
            scope.relation_references.emplace_back(relation_reference_info{
                    .name = "embedded",
                    .tag_hash = tag_hash,
                    .param_index = param_index,
                    .category = category,
                    .scope_index = scope_stack.top()
            });
            scope_stack.push(multi_access_info.size());
            auto& new_scope = multi_access_info.emplace_back();
        }

        void add_scope_reference(uint32_t param_index, const std::string& name)
        {
            get_current_scope().relation_references.emplace_back(relation_reference_info{
                    .name = name,
                    .tag_hash = type_hash{},
                    .param_index = param_index,
                    .category = relation_category::unsolved
            });
        }

        void solved_scope_reference()
        {
            std::map<std::string, uint32_t> name_to_index;
            for (uint32_t i = 0; i < multi_access_info.size(); i++)
            {
                auto& scope = multi_access_info[i];
                if(scope.tag_hash != type_hash{})
                {
                    if(auto it = name_to_index.find(scope.scope_name); it != name_to_index.end())
                    {
                        throw std::runtime_error("duplicated scope name");
                    }
                    name_to_index.emplace(scope.scope_name, i);
                }
            }
            for(auto& scope : multi_access_info)
            {
                for(auto& scope_reference : scope.relation_references)
                {
                    if(scope_reference.category == relation_category::unsolved)
                    {
                        scope_reference.category = relation_category::single_reference;
                        if(auto it = name_to_index.find(scope_reference.name); it != name_to_index.end())
                        {
                            scope_reference.scope_index = it->second;
                        }
                        else
                        {
                            throw std::runtime_error("cannot find scope reference");
                        }
                    }
                }
            }
        }

        void pop_scope()
        {
            scope_stack.pop();
        }

        entity_access_info& get_current_scope()
        {
            return multi_access_info[scope_stack.top()];
        }

        void add_cond_all(type_hash hash)
        {
            get_current_scope().cond_all.push_back(hash);
        }

        void add_cond_none(type_hash hash)
        {
            get_current_scope().cond_none.push_back(hash);
        }

        small_vector<type_hash>& add_cond_anys()
        {
            return get_current_scope().cond_anys.emplace_back();
        }

        void add_access_component(const access_info& info)
        {
            auto& scope = get_current_scope();
            scope.access_components.push_back(info);
            scope.cond_all.push_back(info.hash);
        }

        void add_optional_access_component(const access_info& info)
        {
            get_current_scope().optional_access_components.push_back(info);
        }

        anys_access_info& add_variant_access_component()
        {
            return get_current_scope().variant_access_components.emplace_back();
        }

        void set_entity_access(size_t param_index)
        {
            get_current_scope().entity_access = (int)param_index;
        }

        void set_storage_key_access(size_t param_index)
        {
            get_current_scope().storage_key_access = (int)param_index;
        }

        //template<template <typename...> typename T, typename... U>
        //static auto cast_to_type_list_helper(T<U...>){
        //	return type_list<U...>{};
        //};
        //template<typename T>
        //using cast_to_type_list = decltype(cast_to_type_list_helper(std::declval<T>()));

        //using comp_list = cast_to_type_list<query_parameter::any_of<int, float>>;

        template<typename T, size_t I>
        auto access_info_from()
        {
            using decayed = std::decay_t<T>;
            static constexpr bool is_const = std::is_const_v<T>;
            static constexpr bool is_ref = std::is_reference_v<T>;
            static constexpr bool is_wo = std::is_base_of_v<query_parameter::wo_component_param, T>;

            std::cout << type_name<T> << std::endl;


            if constexpr (is_wo)
            {
                static_assert(!std::is_empty_v<typename T::type>, "empty component is not allowed to access");
                return access_info{type_hash::of<typename T::type>(), I, access_info::wo};
            }
            else
            {
                static_assert(!std::is_empty_v<decayed>, "empty component is not allowed to access");

                if constexpr (is_const)
                    return access_info{type_hash::of<decayed>(), I, access_info::ro};
                else if constexpr (is_ref)
                    return access_info{type_hash::of<decayed>(), I, access_info::rw};
                else
                    return access_info{type_hash::of<decayed>(), I, access_info::wo};
            }
        };

        template<typename T, size_t I>
        void analysis_descriptor_param()
        {
            if constexpr (std::is_base_of_v<query_parameter::all_param, T>)
            {
                for_each_type([&]<typename U>(type_wrapper<U>)
                              {
                                  add_cond_all(type_hash::of<U>());
                              }, typename T::types{});
            }
            else if constexpr (std::is_base_of_v<query_parameter::any_param, T>)
            {
                auto& cond_any = add_cond_anys();
                for_each_type([&]<typename U>(type_wrapper<U>)
                              {
                                  cond_any.push_back({type_hash::of<U>()});
                              }, typename T::types{});
            }
            else if constexpr (std::is_base_of_v<query_parameter::none_param, T>)
            {
                for_each_type([&]<typename U>(type_wrapper<U>)
                              {
                                  add_cond_none(type_hash::of<U>());
                              }, typename T::types{});
            }
            else if constexpr (std::is_base_of_v<query_parameter::variant_param, T>)
            {
                auto& cond_any = add_cond_anys();
                auto& access_any = add_variant_access_component();
                access_any.param_index = I;
                for_each_type([&]<typename U>(type_wrapper<U>)
                              {
                                  auto info = access_info_from<U>();
                                  cond_any.push_back(info.hash);
                                  access_any.access_components.push_back(info);
                              }, typename T::types{});
            }
            else if constexpr (std::is_base_of_v<query_parameter::optional_param, T>)
            {
                auto info = access_info_from<typename T::type, I>();
                add_optional_access_component(info);
            }
            else if constexpr (std::is_base_of_v<query_parameter::relation_param, T>)
            {

            }
            else if constexpr (std::is_base_of_v<query_parameter::entity_relation_param, T>)
            {
                using relation_tag = typename T::relation_tag;
                using query_param = typename T::query_param;
                push_embedded_scope(I,type_hash::of<relation_tag>(),relation_category::single_embedded);
                parse_args(query_param{});
                pop_scope();
            }
            else if constexpr (std::is_base_of_v<query_parameter::entity_multi_relation_param, T>)
            {
                using relation_tag = typename T::relation_tag;
                using query_param = typename T::query_param;
                push_embedded_scope(I, type_hash::of<relation_tag>(), relation_category::multi_embedded);
                parse_args(query_param{});
                pop_scope();
            }
            else
            {
                static_assert(false, "unknown type");
            }
        }

        template<typename...Args>
        void parse_args(type_list<Args...> list)
        {
            for_each_type_indexed(
                    [&]<typename T, size_t I>(type_wrapper<T>, std::integral_constant<size_t, I>)
                    {
                        if constexpr (std::is_base_of_v<query_parameter::descriptor_param, T>)
                        {
                            analysis_descriptor_param<T, I>();
                        }
                        else if constexpr (std::is_base_of_v<query_parameter::begin_rel_scope_param, T>)
                        {
                            push_reference_scope(I, type_hash::of<typename T::relation_tag>(), T::name.c_str());
                        }
                        else if constexpr (std::same_as<query_parameter::end_rel_scope, T>)
                        {
                            pop_scope();
                        }
                        else if constexpr (std::is_base_of_v<query_parameter::relation_ref_param, T>)
                        {
                            add_scope_reference(I, T::name.c_str());
                        }
                        else if constexpr (is_static_component<T>::value || std::is_base_of_v<query_parameter::wo_component_param, T>)
                        {
                            access_info info = access_info_from<T, I>();
                            add_access_component(info);
                        }
                        else if constexpr (std::is_same_v<std::decay_t<T>, entity>)
                        {
                            static_assert(std::is_same_v<T, entity>, "entity must be value parameter");
                            std::cout << type_name<T> << std::endl;
                            set_entity_access(I);
                        }
                        else if constexpr (std::is_same_v<std::decay_t<T>, storage_key>)
                        {
                            static_assert(std::is_same_v<T, storage_key>,
                                          "storage_key must be value parameter");
                            std::cout << type_name<T> << std::endl;
                            set_storage_key_access(I);
                        }
                        else
                        {
                            //std::cout << "unknown type " << type_name<T> << std::endl;
                            static_assert(false, "unknown type");
                        }

                    }, list);
        }

        template<typename...Args>
        query_descriptor(type_list<Args...> list)
        {
            init();

            parse_args(list);

            scope_stack.pop();
            assert(scope_stack.empty());

            solved_scope_reference();
        }
    };
}