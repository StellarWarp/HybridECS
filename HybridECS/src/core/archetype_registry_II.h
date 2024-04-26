#pragma once
#include "archetype_registry_node.h"
#include <meta/assertion.h>

namespace hyecs
{



	class archetype_registry
	{

		vaildref_map<uint64_t, archetype> m_archetypes;
		vaildref_map<uint64_t, archetype_node> m_archetype_nodes;
		vaildref_map<uint64_t, taged_archetype_node> m_taged_archetype_nodes;

		vaildref_map<archetype_query_index, archetype_query_node> m_archetype_query_nodes;
		vaildref_map<query_index, query_node> m_query_nodes;

		std::function<void(archetype_index)> m_untaged_archetype_addition_callback;
		std::function<void(archetype_index, archetype_index)> m_taged_archetype_addition_callback;
		using archetype_query_addition_callback_t =
			std::function<void(
				archetype_query_index,									// in  query_index
				archetype_index,										// in  base_archetype_index
				archetype_query_node::notify_adding_taged_t&,			// out notify_adding_taged_t
				archetype_query_node::notify_partial_convert_t&  		// out notify_partial_convert_t
				)>;
		archetype_query_addition_callback_t m_archetype_query_addition_callback;

	public:
		void bind_untaged_archetype_addition_callback(std::function<void(archetype_index)>&& callback)
		{
			m_untaged_archetype_addition_callback = callback;
		}

		void bind_taged_archetype_addition_callback(std::function<void(archetype_index, archetype_index)>&& callback)
		{
			m_taged_archetype_addition_callback = callback;
		}

		void bind_archetype_query_addition_callback(archetype_query_addition_callback_t&& callback)
		{
			m_archetype_query_addition_callback = callback;
		}

	private:



		archetype_query_node* add_archetype_node(archetype_index arch)
		{
			assert(!m_archetype_query_nodes.contains(arch.hash()));
			auto arch_node = &m_archetype_nodes.emplace(arch.hash(), archetype_node(arch));
			return &m_archetype_query_nodes.emplace(arch.hash(), archetype_query_node(arch_node));
			
		}

		archetype_query_node* get_pure_tag_archetype_node(
			archetype_node* base_archetype_node,
			const query_condition& tag_condition)
		{
			assert(!tag_condition.empty());
			auto q_hash = archetype_query_hash(base_archetype_node->archetype(), tag_condition);
			if (auto iter = m_archetype_query_nodes.find(q_hash); iter != m_archetype_query_nodes.end())
			{
				auto instance_node = &(*iter).second;
				if (instance_node->isolated())
				{
					auto super_set = instance_node->archetype_node()->direct_query_node();
					instance_node->convert_to_connected(super_set);
				}
				return instance_node;
			}
			return &m_archetype_query_nodes.emplace(
				q_hash,
				archetype_query_node(base_archetype_node, tag_condition));
		}


		const archetype_query_node* get_archetype_query_node(
			const archetype_query_node* superset,
			const query_node* target_query)
		{
			if (target_query->type() == query_node::untaged)
			{
				assert(superset->type() == archetype_query_node::full_set);
				return superset;
			}
			decltype(auto) tag_condition = target_query->tag_condition();
			auto q_hash = archetype_query_hash(superset->archetype_node()->archetype(), tag_condition);
			if (auto iter = m_archetype_query_nodes.find(q_hash); iter != m_archetype_query_nodes.end())
			{
				auto instance_node = &(*iter).second;
				assert(instance_node->is_partial_set());
				if(instance_node->isolated()) instance_node->convert_to_connected(superset);
				return instance_node;
			}
			&m_archetype_query_nodes.emplace(
				q_hash,
				archetype_query_node(superset, tag_condition));//filter inside constructor
		}

		void add_tag_to_pure_tag_query(query_node* query, taged_archetype_node* tag_arch_node)
			{
				auto& arch_q_nodes = query->archetype_query_nodes();
				auto base_arch_node = tag_arch_node->base_archetype_node();
				if (auto it = arch_q_nodes.find(base_arch_node); it != arch_q_nodes.end())
				{
					auto instance_node = it->second;
					auto mut_instance_node = (archetype_query_node*)(instance_node);
					if (instance_node->isolated())
						mut_instance_node->add_matched(tag_arch_node);
				}
				else
				{
					auto instance_node = get_pure_tag_archetype_node(base_arch_node, query->tag_condition());
					instance_node->add_matched(tag_arch_node);
					query->add_matched(instance_node);
				}
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

			if (arch == archetype_index::empty_archetype)
			{
				uint64_t arch_hash = origin_arch.hash();
				arch_hash = archetype::addition_hash(arch_hash, adding);
				arch_hash = archetype::subtraction_hash(arch_hash, removings);
				if (auto iter = m_archetype_query_nodes.find(arch_hash); iter != m_archetype_query_nodes.end())
					return &(*iter).second;
				arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));
			}
			archetype_query_node* full_node = add_archetype_node(arch);

			auto try_add_arch_query_to_subquery = 
				[this](query_node* subquery, archetype_query_node* full_node, archetype_index arch)
				{
					if (subquery->condition().match_all_none(arch))
					{
						decltype(auto) tag_condition = subquery->tag_condition();
						if (tag_condition.match_any(arch))
						{
							assert(full_node->is_full_set());
							if (tag_condition.all.empty() && tag_condition.none.empty())
								subquery->add_matched(full_node);
							else
								subquery->add_matched(get_archetype_query_node(full_node, subquery));
						}
						else if (!tag_condition.any.empty())
						{
							subquery->add_matched(get_archetype_query_node(full_node, subquery));
						}
					}
				};

			uint64_t iterate_version = seqence_allocator<query_node>::allocate();

			if (origin_arch != archetype_index::empty_archetype)
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
						if (query->type() == query_node::untaged)
						{
							if(query->condition().match_none(arch))
								query->add_matched(full_node);
						}
						else //mixed
						{
							try_add_arch_query_to_subquery(query, full_node, arch);
						}
					}

			}
			m_untaged_archetype_addition_callback(arch);




			//single layer iteraton
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
			if (auto iter = m_taged_archetype_nodes.find(arch_hash); iter != m_taged_archetype_nodes.end())
				return &(*iter).second;
			if (auto iter = m_archetype_nodes.find(arch_hash); iter != m_archetype_nodes.end())
				return &(*iter).second;

			vector<component_type_index> untaged_removings_vec;
			for (decltype(auto) comp : removings)
			{
				if (!comp.is_tag()) untaged_removings_vec.push_back(comp);
			}
			vector<component_type_index> taged_adding_vec;
			vector<component_type_index> untaged_adding_vec;
			for (decltype(auto) comp : adding)
			{
				if (comp.is_tag()) taged_adding_vec.push_back(comp);
				else untaged_adding_vec.push_back(comp);
			}


			remove_component untaged_removings(untaged_removings_vec);
			append_component untaged_adding(untaged_adding_vec);


			const auto& arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));

			archetype_index origin_base_arch;
			if (origin_arch.is_tagged())
				origin_base_arch = m_taged_archetype_nodes.at(origin_arch.hash()).base_archetype_node()->archetype();
			else
				origin_base_arch = origin_arch;

			if (!arch.is_tagged())
				return get_base_archetype_node(origin_base_arch, untaged_adding, untaged_removings, arch)->archetype_node();


			archetype_query_node* base_arch_full_q_node = get_base_archetype_node(origin_base_arch, untaged_adding, untaged_removings);
			assert(base_arch_full_q_node->type() == archetype_query_node::full_set);
			archetype_node* base_arch_node = base_arch_full_q_node->archetype_node();

			//adding a tagged archetype
			auto taged_arch_node = &m_taged_archetype_nodes.emplace(arch_hash, taged_archetype_node(arch, base_arch_node));
			//add to direct set this will dispatch to all sub-archetype_query_node
			base_arch_full_q_node->add_matched(taged_arch_node);
			//for pure tag queries
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			for (decltype(auto) component : taged_adding_vec)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				assert(m_query_nodes.contains(com_node_hash));
				query_node* com_node_index = &m_query_nodes.at(com_node_hash);

				add_tag_to_pure_tag_query(com_node_index, taged_arch_node);

				for (auto query_node : com_node_index->subquery_nodes())
				{
					if (query_node->iterate_version == iterate_version) continue;
					query_node->iterate_version = iterate_version;

					if (query_node->condition().match(taged_arch_node->archetype()))
						add_tag_to_pure_tag_query(query_node, taged_arch_node);
				}
			}

			m_taged_archetype_addition_callback(arch, taged_arch_node->archetype());

			return taged_arch_node;
		}

		query_node* get_ingroup_query(query_condition condition)
		{
			uint64_t hash = condition.hash();
			if (auto iter = m_query_nodes.find(hash); iter != m_query_nodes.end())
				return &(*iter).second;

			query_condition base_condition;
			query_condition tag_condition;
			base_condition.all.reserve(condition.all.size());
			base_condition.any.reserve(condition.any.size());
			base_condition.none.reserve(condition.none.size());
			tag_condition.all.reserve(condition.all.size());
			tag_condition.any.reserve(condition.any.size());
			tag_condition.none.reserve(condition.none.size());
			for (auto c : condition.all)
				if (!c.is_tag()) base_condition.all.push_back(c);
				else              tag_condition.all.push_back(c);
			for (auto c : condition.any)
				if (!c.is_tag()) base_condition.any.push_back(c);
				else              tag_condition.any.push_back(c);
			for (auto c : condition.none)
				if (!c.is_tag()) base_condition.none.push_back(c);
				else              tag_condition.none.push_back(c);

			assert(!condition.all.empty());	//todo: support no all condition 

			query_node::query_type q_type = query_node::mixed;
			if (tag_condition.empty()) q_type = query_node::untaged;
			else if (base_condition.all.empty()) q_type = query_node::pure_tag;

			query_node* node = &m_query_nodes.emplace(hash, query_node(condition, q_type));

			// select for initial set
			const query_node::archetype_nodes_t* initial_set = nullptr;
			size_t min_initial_set_size = std::numeric_limits<size_t>::max();

			auto& search_condition_all = q_type == query_node::pure_tag ? tag_condition.all : base_condition.all;
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
			if(q_type != query_node::pure_tag)
			{
				// base filter phase
				vector<const archetype_query_node*> filter_set;
				filter_set.reserve(initial_set->size());
				for (auto& [arch_node, q_node] : *initial_set)
				{
					if (base_condition.match_all_none(arch_node->archetype()))
						if (base_condition.match_any(arch_node->archetype()))
						{
							if (tag_condition.all.empty() && tag_condition.none.empty() && q_node->is_full_set())
								node->add_matched(q_node);
							else
								filter_set.push_back(q_node);
						}
						else if (!tag_condition.any.empty())
						{
							filter_set.push_back(q_node);
						}
				}
				// tag filter phase
				for (auto arch_qnode : filter_set)
				{
					//find or create new taged_archetype_node
					const archetype_query_node* qnode = get_archetype_query_node(arch_qnode, node);
					node->add_matched(qnode);
				}
			}
			else // pure_tag
			{
				for (auto& [arch_node, q_node] : *initial_set)
				{
					for (auto tag_node : q_node->taged_archetype_nodes())
					{
						if(node->condition().match(tag_node->archetype()))
							add_tag_to_pure_tag_query(node, tag_node);
					}
				}
			}

			return node;
		}




	public:
		void register_component(component_type_index component)
		{
			sequence_ref<const component_type_index> seq = initializer_list<component_type_index>{ component };
			if (component.is_tag())
				m_query_nodes.emplace(component.hash(), query_node(seq, query_node::untaged));
			else
				m_query_nodes.emplace(component.hash(), query_node(seq, query_node::pure_tag));
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
	};


}

