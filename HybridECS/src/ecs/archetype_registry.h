#pragma once
#include "archetype_registry_node.h"
#include "../meta/assertion.h"

namespace hyecs
{



	class archetype_registry
	{

		vaildref_map<uint64_t, archetype> m_archetypes;
		vaildref_map<uint64_t, archetype_node> m_archetype_nodes;
		vaildref_map<uint64_t, tag_archetype_node> m_tag_archetype_nodes;

		vaildref_map<archetype_query_index, archetype_query_node> m_archetype_query_nodes;
		vaildref_map<query_index, query_node> m_query_nodes;

		std::function<void(archetype_index)> m_untag_archetype_addition_callback;
		std::function<void(archetype_index, archetype_index)> m_tag_archetype_addition_callback;
	public:
		struct archetype_query_addition_info
		{
			archetype_query_index query_index;
			archetype_index base_archetype_index;
			const query_condition& tag_condition;
			bool is_full_set;
			bool is_direct_set;
			archetype_query_node::notify_adding_tag_archetype_t& notify_adding_tag;
			archetype_query_node::notify_partial_convert_t& notify_partial_convert;
		};
	private:
		using archetype_query_addition_callback_t = std::function<void(archetype_query_addition_info)>;
		archetype_query_addition_callback_t m_archetype_query_addition_callback;
	public:
		struct query_addition_info
		{
			query_index index;
			const query_condition& condition;
			query_node::callback_t& archetype_query_addition_callback;
		};
	private:
		using query_addition_callback_t = std::function<void(query_addition_info)>;
		query_addition_callback_t m_query_addition_callback;

	public:
		void bind_untag_archetype_addition_callback(std::function<void(archetype_index)>&& callback)
		{
			m_untag_archetype_addition_callback = callback;
		}

		void bind_tag_archetype_addition_callback(std::function<void(archetype_index, archetype_index)>&& callback)
		{
			m_tag_archetype_addition_callback = callback;
		}

		void bind_archetype_query_addition_callback(archetype_query_addition_callback_t&& callback)
		{
			m_archetype_query_addition_callback = callback;
		}

		void bind_query_addition_callback(query_addition_callback_t&& callback)
		{
			m_query_addition_callback = callback;
		}
	private:

		void invoke_archetype_query_addition(archetype_query_node* arch_query_node, query_index hash) const
		{
			archetype_query_node::notify_adding_tag_archetype_t notify_adding_tag;
			archetype_query_node::notify_partial_convert_t notify_partial_convert;
			m_archetype_query_addition_callback({
				hash,
				arch_query_node->archetype_node()->archetype(),
				arch_query_node->tag_condition(),
				arch_query_node->is_full_set(),
				arch_query_node->is_direct_set(),
				notify_adding_tag, notify_partial_convert });
			arch_query_node->add_notify_adding_tag_archetype_callback(std::move(notify_adding_tag));
			arch_query_node->add_notify_partial_convert_callback(std::move(notify_partial_convert));
		}

		void invoke_query_addition(query_node* query_node) const
		{
			query_node::callback_t archetype_query_addition_callback;
			m_query_addition_callback({
				query_node->condition().hash(),
				query_node->condition(),
				archetype_query_addition_callback
				});
			query_node->add_callback(std::move(archetype_query_addition_callback));
		}

		void invoke_untag_archetype_addition(archetype_index arch) const
		{
			m_untag_archetype_addition_callback(arch);
		}

		void invoke_tag_archetype_addition(archetype_index arch, archetype_index base_arch) const
		{
			m_tag_archetype_addition_callback(arch, base_arch);
		}

	private:



		archetype_query_node* add_archetype_node(archetype_index arch)
		{
			assert(!m_archetype_nodes.contains(arch.hash()));
			assert(!m_archetype_query_nodes.contains(arch.hash()));
			auto arch_node = &m_archetype_nodes.emplace(arch.hash(), archetype_node(arch));
			auto arch_query_node = &m_archetype_query_nodes.emplace(arch.hash(), archetype_query_node(arch_node));
			//callbacks
			invoke_untag_archetype_addition(arch);
			invoke_archetype_query_addition(arch_query_node, arch.hash());
			return arch_query_node;
		}

		archetype_query_node* get_archetype_node_for_pure_tag_q(
			archetype_node* base_archetype_node,
			const query_condition& tag_condition)
		{
			assert(!tag_condition.empty());
			auto q_hash = archetype_query_hash(base_archetype_node->archetype(), tag_condition);
			if (auto iter = m_archetype_query_nodes.find(q_hash); iter != m_archetype_query_nodes.end())
			{
				auto instance_node = &(*iter).second;
				//the pure tag arch-query is impossible to reuse across pure tag query
				assert(!instance_node->archetype_node()->archetype().empty());
				if (instance_node->isolated())
				{
					auto super_set = instance_node->archetype_node()->direct_query_node();
					instance_node->convert_to_connected(super_set);
				}
				return instance_node;
			}
			auto arch_query_node = &m_archetype_query_nodes.emplace(
				q_hash,
				archetype_query_node(base_archetype_node, tag_condition));

			invoke_archetype_query_addition(arch_query_node, q_hash);

			return arch_query_node;
		}


		const archetype_query_node* get_archetype_query_node(
			const archetype_query_node* superset,
			const query_node* target_query,
			const query_condition tag_condition
		)
		{
			ASSERTION_CODE(
			if(tag_condition.all().empty() && tag_condition.anys().empty() && !tag_condition.none().empty())
				assert(false)//havent support for untaged part query yet
			);

			if (target_query->type() == query_node::untag)
			{
				assert(superset->is_direct_set());
				return superset;
			}
			auto q_hash = archetype_query_hash(superset->archetype_node()->archetype(), tag_condition);
			if (auto iter = m_archetype_query_nodes.find(q_hash); iter != m_archetype_query_nodes.end())
			{
				auto instance_node = &(*iter).second;
				assert(instance_node->archetype_node() == superset->archetype_node());
				assert(instance_node->tag_condition() == tag_condition);
				assert(instance_node->is_partial_set());
				if (instance_node->isolated()) instance_node->convert_to_connected(superset);
				return instance_node;
			}
			auto arch_query_node = &m_archetype_query_nodes.emplace(
				q_hash,
				archetype_query_node(superset, tag_condition));//filter inside constructor

			invoke_archetype_query_addition(arch_query_node, q_hash);

			return arch_query_node;
		}

		void add_tag_to_pure_tag_query(query_node* query, tag_archetype_node* tag_arch_node)
		{
			auto& arch_q_nodes = query->archetype_query_nodes();
			auto base_arch_node = tag_arch_node->base_archetype_node();
			//add arch to arch query if it's exist and isolated
			if (auto it = arch_q_nodes.find(base_arch_node); it != arch_q_nodes.end())
			{
				auto instance_node = it->second;
				auto mut_instance_node = (archetype_query_node*)(instance_node);
				if (instance_node->isolated())
					mut_instance_node->add_matched(tag_arch_node);
			}
			else//if not exist create a new arch query
			{
				auto instance_node = get_archetype_node_for_pure_tag_q(base_arch_node, query->tag_condition());
				instance_node->add_matched(tag_arch_node);
				query->add_matched(instance_node);
			}
		};

		void try_add_arch_query_to_subquery(
			query_node* subquery,
			const archetype_query_node* arch_query,
			archetype_index arch,
			const query_condition* base_condition_opt = nullptr,
			const query_condition* tag_condition_opt = nullptr
		)
		{
			decltype(auto) condition = subquery->condition();
			std::optional<query_condition> base_condition_stack = std::nullopt;
			if (!base_condition_opt) base_condition_stack = condition.base_incomplete_condition();
			auto& base_condition = base_condition_opt ? *base_condition_opt : base_condition_stack.value();

			// base filter
			if (!base_condition.match_all_none(arch)) return;

			query_condition tag_condition = tag_condition_opt ? *tag_condition_opt : condition.tag_incomplete_condition();

			const auto& comp_mask = arch.component_mask();
			small_vector<uint32_t> maintain_any_index;

			auto& anys = condition.anys();
			for (uint32_t i = 0; i < anys.size(); i++)
			{
				auto& any = anys[i];
				if (!query_condition::match_any(comp_mask, any))//if any is empty it will be remove
                {
                    if (!tag_condition.anys()[i].empty())
                        maintain_any_index.push_back(i);
                    else
                        return;
                }
			}

			if (maintain_any_index.empty())
				if (tag_condition.all().empty() && tag_condition.none().empty())
				{
					assert(arch_query->is_direct_set());
					subquery->add_matched(arch_query);
					return;
				}
			// tag filter
			tag_condition.filter_anys_by_index(maintain_any_index);
			tag_condition.completion();
			//find or create new tag_archetype_node
			subquery->add_matched(get_archetype_query_node(arch_query, subquery, tag_condition));
		};



		archetype_query_node* get_base_archetype_node(
			archetype_index origin_arch,
			append_component adding,
			remove_component removings,
			archetype_index arch = archetype_index::empty_archetype
		)
		{
			ASSERTION_CODE(
				for (auto comp : adding) assert(!comp.is_tag());
			for (auto comp : removings) assert(!comp.is_tag());
				);

			if (arch .empty())
			{
				uint64_t arch_hash = origin_arch.hash();
				arch_hash = archetype::addition_hash(arch_hash, adding);
				arch_hash = archetype::subtraction_hash(arch_hash, removings);
				if (auto iter = m_archetype_query_nodes.find(arch_hash); iter != m_archetype_query_nodes.end())
					return &(*iter).second;
				arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));
			}
			archetype_query_node* full_node = add_archetype_node(arch);



			uint64_t iterate_version = seqence_allocator<query_node>::allocate();

			//update for original arch related queries
			if (!origin_arch.empty())
			{
				auto& origin_arch_node = m_archetype_nodes.at(origin_arch.hash());
				if (removings.size() != 0)
					for (auto query : origin_arch_node.related_queries())
					{
						query->iterate_version = iterate_version;
						try_add_arch_query_to_subquery(query, full_node, arch);
					}
				else
					for (auto query : origin_arch_node.related_queries())
					{
						query->iterate_version = iterate_version;
						if (query->type() == query_node::untag)//optimize for untag case
						{
							if (query->condition().match_none(arch))
								query->add_matched(full_node);
						}
						else //mixed
						{
							try_add_arch_query_to_subquery(query, full_node, arch);
						}
					}

			}

			//single layer iteration : subquery try add
			for (decltype(auto) component : adding)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				assert(m_query_nodes.contains(com_node_hash));
				query_node* com_node_index = &m_query_nodes.at(com_node_hash);

				com_node_index->add_matched(full_node);
				for (auto query_node : com_node_index->subquery_nodes())
				{
					if (query_node->iterate_version == iterate_version) continue;
					query_node->iterate_version = iterate_version;

					try_add_arch_query_to_subquery(query_node, full_node, arch);

				}
			}

			return full_node;
		}


		archetype_node_variant get_ingroup_archetype_node(
			archetype_index origin_arch, append_component adding, remove_component removings)
		{
			uint64_t arch_hash = origin_arch.hash();
			arch_hash = archetype::addition_hash(arch_hash, adding);
			arch_hash = archetype::subtraction_hash(arch_hash, removings);

			//todo optimize
			if (auto iter = m_tag_archetype_nodes.find(arch_hash); iter != m_tag_archetype_nodes.end())
				return &(*iter).second;
			if (auto iter = m_archetype_nodes.find(arch_hash); iter != m_archetype_nodes.end())
				return &(*iter).second;

			vector<component_type_index> untag_removings_vec;
			for (decltype(auto) comp : removings)
			{
				if (!comp.is_tag()) untag_removings_vec.push_back(comp);
			}
			vector<component_type_index> tag_adding_vec;
			vector<component_type_index> untag_adding_vec;
			for (decltype(auto) comp : adding)
			{
				if (comp.is_tag()) tag_adding_vec.push_back(comp);
				else untag_adding_vec.push_back(comp);
			}


			remove_component untag_removings(untag_removings_vec);
			append_component untag_adding(untag_adding_vec);


			const auto& arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));

			archetype_index origin_base_arch;
			if (origin_arch.is_tag())
				origin_base_arch = m_tag_archetype_nodes.at(origin_arch.hash()).base_archetype_node()->archetype();
			else
				origin_base_arch = origin_arch;

			if (!arch.is_tag())
				return get_base_archetype_node(origin_base_arch, untag_adding, untag_removings, arch)->archetype_node();


			archetype_query_node* base_arch_full_q_node = get_base_archetype_node(origin_base_arch, untag_adding, untag_removings);
			assert(base_arch_full_q_node->type() == archetype_query_node::full_set);
			archetype_node* base_arch_node = base_arch_full_q_node->archetype_node();

			//adding a tag archetype
			auto tag_arch_node = &m_tag_archetype_nodes.emplace(arch_hash, tag_archetype_node(arch, base_arch_node));
			invoke_tag_archetype_addition(arch, base_arch_node->archetype());
			//add to direct set this will dispatch to all sub-archetype_query_node
			base_arch_full_q_node->add_matched(tag_arch_node);
			//for pure tag queries
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			for (decltype(auto) component : tag_adding_vec)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				assert(m_query_nodes.contains(com_node_hash));
				query_node* com_node_index = &m_query_nodes.at(com_node_hash);

				add_tag_to_pure_tag_query(com_node_index, tag_arch_node);

				for (auto query_node : com_node_index->subquery_nodes())
				{
					if (query_node->iterate_version == iterate_version) continue;
					query_node->iterate_version = iterate_version;

					if (query_node->condition().match(tag_arch_node->archetype()))
						add_tag_to_pure_tag_query(query_node, tag_arch_node);
				}
			}


			return tag_arch_node;
		}

		query_node* get_ingroup_query(const query_condition& condition)
		{
			uint64_t hash = condition.hash();
			if (auto iter = m_query_nodes.find(hash); iter != m_query_nodes.end())
				return &(*iter).second;

			//todo optimize this
			query_condition base_condition = condition.base_incomplete_condition();
			query_condition tag_condition = condition.tag_incomplete_condition();

			assert(!condition.all().empty());	//todo: support no all condition 

			query_node::query_type q_type = query_node::mixed;
			if (tag_condition.empty()) q_type = query_node::untag;
			else if (base_condition.all().empty()) q_type = query_node::pure_tag;

			query_node* node = &m_query_nodes.emplace(hash, query_node(condition, q_type));
			invoke_query_addition(node);
			// select for initial set
			const query_node::archetype_nodes_t* initial_set = nullptr;
			size_t min_initial_set_size = std::numeric_limits<size_t>::max();

			auto& search_condition_all = q_type == query_node::pure_tag ? tag_condition.all() : base_condition.all();
			for (auto c : search_condition_all)
			{
				assert(m_query_nodes.contains(c.hash()));
				query_node* component_node = &m_query_nodes.at(c.hash());
				component_node->add_subquery(node);
				if (component_node->archetype_query_nodes().size() < min_initial_set_size)
				{
					initial_set = &component_node->archetype_query_nodes();
					min_initial_set_size = initial_set->size();
				}
			}

			if (q_type != query_node::pure_tag)
			{
				for (auto& [arch_node, arch_q_node] : *initial_set)
				{
					try_add_arch_query_to_subquery(node, arch_q_node, arch_node->archetype(), &base_condition, &tag_condition);
					//// base filter
					//if (!base_condition.match_all_none(arch_node->archetype()))
					//	continue;
					//const auto& comp_mask = arch_node->archetype().component_mask();
					//small_vector<uint32_t> maintain_any_index;
					//bool discard = false;
					//auto& anys = condition.anys();
					//for (uint32_t i = 0; i < anys.size(); i++)
					//{
					//	auto& any = anys[i];
					//	if (!query_condition::match_any(comp_mask, any))
					//		if (!tag_condition.anys()[i].empty())
					//		{
					//			maintain_any_index.push_back(i);
					//		}
					//		else
					//		{
					//			discard = true;
					//			break;
					//		}
					//}
					//if (discard) continue;

					//if (maintain_any_index.empty())
					//	if (tag_condition.all().empty() && tag_condition.none().empty())
					//	{
					//		assert(arch_q_node->is_direct_set());
					//		node->add_matched(arch_q_node);
					//		continue;
					//	}
					//// tag filter
					//tag_condition.filter_anys_by_index(maintain_any_index);
					////find or create new tag_archetype_node
					//node->add_matched(get_archetype_query_node(arch_q_node, node, tag_condition));

				}
			}
			else // pure_tag
			{
				for (auto& [arch_node, q_node] : *initial_set)
				{
					for (auto tag_node : q_node->tag_archetype_nodes())
					{
						if (node->condition().match(tag_node->archetype()))
							add_tag_to_pure_tag_query(node, tag_node);
					}
				}
			}

			return node;
		}




	public:
		void register_component(component_type_index component)
		{
			query_node* node;
			if (component.is_tag())
				node = &m_query_nodes.emplace(component.hash(), query_node({ component }, query_node::pure_tag));
			else
				node = &m_query_nodes.emplace(component.hash(), query_node({ component }, query_node::untag));
			invoke_query_addition(node);
		}


		archetype_index get_archetype(archetype_index origin_arch, append_component adding, remove_component removings)
		{
			auto arch_node = get_ingroup_archetype_node(origin_arch, adding, removings);
			return std::visit([&](auto&& arg) {return arg->archetype(); }, arch_node);
		}

		archetype_index get_archetype(archetype_index origin_arch, append_component adding)
		{
			return get_archetype(origin_arch, adding, {});
		}

		archetype_index get_archetype(archetype_index origin_arch, remove_component removings)
		{
			return get_archetype(origin_arch, {}, removings);
		}

		archetype_index get_archetype(append_component components)
		{
			return get_archetype(archetype_index::empty_archetype, components, {});
		}

		query_index get_query(const query_condition& condition)
		{
			const auto query_node = get_ingroup_query(condition);
			return query_node->condition().hash();
		}
	};


}

