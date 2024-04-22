#pragma once

#include "archetype_registry_node.h"

namespace hyecs
{

	class archetype_registry
	{
		vaildref_map<uint64_t, archetype> m_archetypes;
		vaildref_map<uint64_t, archetype_node> m_archetype_nodes;
		vaildref_map<uint64_t, taged_archetype_node> m_taged_archetype_nodes;
		vaildref_map<uint64_t, component_node> m_untaged_component_nodes;
		vaildref_map<uint64_t, taged_component_node> m_taged_component_nodes;
		vaildref_map<uint64_t, query_node> m_query_nodes;
		vaildref_map<uint64_t, taged_query_node> m_taged_query_nodes;
		vaildref_map<uint64_t, mixed_query_node> m_mixed_query_nodes;
		vaildref_map<uint64_t, cross_query_node> m_cross_query_nodes;
		//group base phantom tag node
		vaildref_map<component_group_index, taged_component_node> m_group_phantom_component_nodes;

		std::function<void(archetype_index)> m_untaged_archetype_addition_callback;
		std::function<void(archetype_index, archetype_index)> m_taged_archetype_addition_callback;

	public:


		void bind_untaged_archetype_addition_callback(std::function<void(archetype_index)>&& callback)
		{
			m_untaged_archetype_addition_callback = callback;
		}

		void bind_taged_archetype_addition_callback(std::function<void(archetype_index, archetype_index)>&& callback)
		{
			m_taged_archetype_addition_callback = callback;
		}

	private:

		/// <summary>
		/// untaged archetype/query func
		/// </summary>

		//todo this mybe unused
		template<typename NodeType>
		auto add_single_component_query_node(component_type_index component)
		{
			if constexpr (std::is_same_v<NodeType, taged_component_node>)
				return &m_taged_component_nodes.emplace(component.hash(), taged_component_node(component));
			else
				return &m_untaged_component_nodes.emplace(component.hash(), component_node(component));
		}

		archetype_node* add_archetype(archetype_index origin_arch, append_component adding, remove_component removings, uint64_t arch_hash, archetype_index arch = archetype_index::empty_archetype)
		{
			if (arch == archetype_index::empty_archetype)
				arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));
			auto& arch_node = m_archetype_nodes.emplace(arch_hash, archetype_node(arch));
			if (origin_arch != archetype_index::empty_archetype)
			{
				auto& origin_arch_node = m_archetype_nodes.at(origin_arch.hash());
				if (removings.size() != 0)
					for (auto query : origin_arch_node.related_queries())
						std::visit([&](auto&& arg) {arg->try_match(&arch_node); }, query);
				else
					for (auto query : origin_arch_node.related_queries())
						std::visit([&](auto&& arg) {arg->add_matched(&arch_node); }, query);
				
			}
			m_untaged_archetype_addition_callback(arch);


			//single layer iteraton
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			for (decltype(auto) component : adding)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				component_node* com_node_index;
				if (auto iter = m_untaged_component_nodes.find(com_node_hash); iter == m_untaged_component_nodes.end())
					com_node_index = add_single_component_query_node<component_node>(component);
				else
					com_node_index = &(*iter).second;

				com_node_index->add_matched(&arch_node);
				for (auto query_node : com_node_index->children_node())
				{
					if (query_node->iterate_version() != iterate_version)
					{
						query_node->iterate_version() = iterate_version;
						query_node->try_match(&arch_node);
					}
				}
			}

			return &arch_node;

		}

		query_node_variant get_query(sequence_ref<component_type_index> condition_all)
		{
			uint64_t query_hash = archetype::addition_hash(0, condition_all);
			//check if query node already exist
			if (auto iter = m_untaged_component_nodes.find(query_hash); iter != m_untaged_component_nodes.end())
				return &(*iter).second;
			if (auto iter = m_query_nodes.find(query_hash); iter != m_query_nodes.end())
				return &(*iter).second;
			//create new query node
			auto& new_query_node = m_query_nodes.emplace(
				query_hash,
				query_node(condition_all));
			//init iteration
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			size_t min_archetype_count = std::numeric_limits<int>::max();
			component_node* inital_match_set = nullptr;
			vector<component_node*> filter_component_nodes(condition_all.size() - 1, nullptr);
			for (decltype(auto) component : condition_all)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				component_node* com_node_index;
				if (auto iter = m_untaged_component_nodes.find(com_node_hash); iter == m_untaged_component_nodes.end())
					com_node_index = add_single_component_query_node<component_node>(component);/// todo
				else
					com_node_index = &(*iter).second;

				com_node_index->add_child(&new_query_node);
				//get the min set of archetype
				size_t related_archetype_count = com_node_index->archetype_nodes().size();
				if (related_archetype_count < min_archetype_count)
				{
					min_archetype_count = related_archetype_count;
					if (inital_match_set != nullptr)
						filter_component_nodes.push_back(com_node_index);
					inital_match_set = com_node_index;
				}
				else
				{
					filter_component_nodes.push_back(com_node_index);
				}
			}
			ASSERTION_CODE(
				assert(inital_match_set != nullptr);
			if (!inital_match_set) return (component_node*)nullptr;
			)
				//union operation
				//inverse iteration order might be better
				for (auto archetype_node : inital_match_set->archetype_nodes())
				{
					bool is_match = true;
					for (auto filter_node : filter_component_nodes)
					{
						if (!filter_node->archetype_nodes().contains(archetype_node))
						{
							is_match = false;
							break;
						}
					}
					if (is_match)
						new_query_node.add_matched(archetype_node);
				}

			//alternative method for union operation
			//for (auto archetype_node : inital_match_set->archetype_nodes())
			//	new_query_node.try_match(archetype_node);


			return &new_query_node;
		}


		archetype_node* get_archetype_node(archetype_index origin_arch, append_component components)
		{

			auto arch_hash = origin_arch.hash(components);
			if (auto iter = m_archetype_nodes.find(arch_hash); iter != m_archetype_nodes.end())
				return &(*iter).second;
			return add_archetype(origin_arch, components, {}, arch_hash);
		}

		archetype_node* get_archetype_node(archetype_index origin_arch, remove_component components)
		{
			auto arch_hash = origin_arch.hash(components);
			if (auto iter = m_archetype_nodes.find(arch_hash); iter != m_archetype_nodes.end())
				return &(*iter).second;
			return add_archetype(origin_arch, {}, components, arch_hash);
		}

		archetype_node* get_archetype_node(archetype_index origin_arch, append_component addings, remove_component removings)
		{

			uint64_t hash = origin_arch.hash();
			hash = archetype::addition_hash(hash, addings);
			hash = archetype::subtraction_hash(hash, removings);

			if (auto iter = m_archetype_nodes.find(hash); iter != m_archetype_nodes.end())
				return &(*iter).second;
			return add_archetype(origin_arch, addings, removings, hash);
		}

		/// <summary>
		/// in-group archetype/query func
		/// </summary>

		archetype_node_variant add_ingroup_archetype(
			archetype_index origin_arch, append_component adding, remove_component removings, uint64_t arch_hash)
		{
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
			if (!arch.is_tagged())/*new arch is not taged archetype*/
			{
				if (!origin_arch.is_tagged())
				{
					return add_archetype(origin_arch, adding, removings, arch_hash, arch);
				}
				else
				{
					auto& origin_arch_node = m_taged_archetype_nodes.at(origin_arch.hash());
					auto untaged_arch_node = origin_arch_node.base_archetype_node();
					return add_archetype(untaged_arch_node->archetype(), adding, untaged_removings, arch_hash, arch);//the adding do not include tag
				}
			}
			else//new arch is taged
			{
				//find or create base arch
				archetype_index origin_arch_base;
				if (origin_arch.is_tagged())
					origin_arch_base = m_taged_archetype_nodes.at(origin_arch.hash()).base_archetype_node()->archetype();
				else
					origin_arch_base = origin_arch;
				auto base_arch_node = get_archetype_node(origin_arch_base, untaged_adding, untaged_removings);
				auto& arch_node = m_taged_archetype_nodes.emplace(arch_hash, taged_archetype_node(arch, base_arch_node));
				m_group_phantom_component_nodes.at(arch.group()).add_matched(&arch_node);
				if (origin_arch.is_tagged())
				{
					auto& origin_arch_node = m_taged_archetype_nodes.at(origin_arch.hash());

					//try match origin archetype related query
					if (removings.size() != 0)
						for (auto query : origin_arch_node.related_queries())
							std::visit([&](auto&& arg) {arg->try_match(&arch_node); }, query);
					else
						for (auto query : origin_arch_node.related_queries())
							std::visit([&](auto&& arg) {arg->add_matched(&arch_node); }, query);
				}
				m_taged_archetype_addition_callback(arch, base_arch_node->archetype());

				//todo duplicate for taged part and untaged part
				//try match all taged component related query
				auto iterate_version = seqence_allocator<taged_query_node>::allocate();
				for (decltype(auto) component : taged_adding_vec)
				{
					auto com_node_hash = archetype::addition_hash(0, { component });
					taged_component_node* taged_node;
					if (auto iter = m_taged_component_nodes.find(com_node_hash); iter == m_taged_component_nodes.end())
						taged_node = add_single_component_query_node<taged_component_node>(component);
					else
						taged_node = &(*iter).second;

					taged_node->add_matched(&arch_node);
					for (auto query_node : taged_node->taged_children_node())
					{
						if (query_node->iterate_version() != iterate_version)
						{
							query_node->iterate_version() = iterate_version;
							query_node->try_match(&arch_node);
						}
					}
				}
				//try match all untaged component related query
				for (decltype(auto) component : untaged_adding_vec)
				{
					auto com_node_hash = archetype::addition_hash(0, { component });
					component_node* untaged_node;
					if (auto iter = m_untaged_component_nodes.find(com_node_hash); iter == m_untaged_component_nodes.end())
						untaged_node = add_single_component_query_node<component_node>(component);
					else
						untaged_node = &(*iter).second;

					//untaged_node->add_related(&arch_node);
					for (auto query_node : untaged_node->taged_children_node())
					{
						if (query_node->iterate_version() != iterate_version)
						{
							query_node->iterate_version() = iterate_version;
							query_node->try_match(&arch_node);
						}
					}
				}

				return &arch_node;


			}
		}

		archetype_node_variant get_ingroup_archetype_node(
			archetype_index origin_arch, append_component adding, remove_component removings)
		{
			uint64_t hash = origin_arch.hash();
			hash = archetype::addition_hash(hash, adding);
			hash = archetype::subtraction_hash(hash, removings);

			//todo optimize
			if (auto iter = m_taged_archetype_nodes.find(hash); iter != m_taged_archetype_nodes.end())
				return &(*iter).second;
			if (auto iter = m_archetype_nodes.find(hash); iter != m_archetype_nodes.end())
				return &(*iter).second;

			return add_ingroup_archetype(origin_arch, adding, removings, hash);
		}

		query_node_variant get_taged_query(
			sequence_ref<component_type_index> condition_all,
			sequence_ref<component_type_index> untaged_components,
			sequence_ref<component_type_index> taged_components)
		{
			uint64_t query_hash = archetype::addition_hash(0, condition_all);
			//check if query node already exist
			if (auto iter = m_taged_component_nodes.find(query_hash); iter != m_taged_component_nodes.end())
				return &(*iter).second;
			if (auto iter = m_taged_query_nodes.find(query_hash); iter != m_taged_query_nodes.end())
				return &(*iter).second;
			//create new query node
			auto& new_query_node = m_taged_query_nodes.emplace(
				query_hash,
				taged_query_node(condition_all));


			//init iteration
			uint64_t iterate_version = seqence_allocator<taged_query_node>::allocate();
			size_t min_archetype_count = std::numeric_limits<size_t>::max();
			const set<taged_archetype_node*>* inital_match_set = nullptr;
			vector<const set<taged_archetype_node*>*> filter_set(condition_all.size() - 1, nullptr);

			for (decltype(auto) component : untaged_components)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				component_node* com_node_index;
				if (auto iter = m_untaged_component_nodes.find(com_node_hash); iter == m_untaged_component_nodes.end())
					com_node_index = add_single_component_query_node<component_node>(component);
				else
					com_node_index = &(*iter).second;

				com_node_index->add_child(&new_query_node);
				///untaged component do not store the related taged archetype
				//get the min set of archetype
				//int related_archetype_count = com_node_index->archetype_nodes().size();
				//if (related_archetype_count < min_archetype_count)
				//{
				//	min_archetype_count = related_archetype_count;
				//	if (untaged_inital_match_set != nullptr)
				//		filter_untaged_component_nodes.push_back(com_node_index);
				//	untaged_inital_match_set = com_node_index;
				//}
				//else
				//{
				//	filter_untaged_component_nodes.push_back(com_node_index);
				//}
			}
			if (taged_components.size() == 0)//support for any/none condition
			{
				auto group = untaged_components[0].group();
				inital_match_set = &m_group_phantom_component_nodes.at(group).archetype_nodes();
			}
			for (decltype(auto) component : taged_components)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				taged_component_node* com_node_index;
				if (auto iter = m_taged_component_nodes.find(com_node_hash); iter == m_taged_component_nodes.end())
					com_node_index = add_single_component_query_node<taged_component_node>(component);/// todo
				else
					com_node_index = &(*iter).second;

				com_node_index->add_child(&new_query_node);
				//get the min set of archetype
				size_t related_archetype_count = com_node_index->archetype_nodes().size();
				if (related_archetype_count < min_archetype_count)
				{
					min_archetype_count = related_archetype_count;
					if (inital_match_set != nullptr)
						filter_set.push_back(&com_node_index->archetype_nodes());
					inital_match_set = &com_node_index->archetype_nodes();
				}
				else
				{
					filter_set.push_back(&com_node_index->archetype_nodes());
				}
			}
			//union operation
			//inverse iteration order might be better
			//for (auto archetype_node : *inital_match_set) //this is nearly O(n) operation
			//{
			//	bool is_match = true;
			//	for (auto filter_node : filter_set)
			//	{
			//		if (!filter_node->contains(archetype_node))
			//		{
			//			is_match = false;
			//			break;
			//		}
			//	}
			//	if (is_match)
			//		new_query_node.try_match(archetype_node);
			//	//try match is required as untaged component do not store for related taged archetype
			//	//need try match to filter out the unrelated archetype
			//}
			ASSERTION_CODE(
				assert(inital_match_set != nullptr);
			if (!inital_match_set) return (query_node*)nullptr;
			);
			//alternative method for union operation
			for (auto archetype_node : *inital_match_set) new_query_node.try_match(archetype_node);



			return &new_query_node;
		}

		query_node_variant get_ingroup_query(sequence_ref<component_type_index> all_condition)
		{
			vector<component_type_index> untaged_components;
			vector<component_type_index> taged_components;
			for (decltype(auto) comp : all_condition)
			{
				if (comp.is_tag()) taged_components.push_back(comp);
				else untaged_components.push_back(comp);
			}

			if (taged_components.size() == 0)
				return get_query(all_condition);
			else
				return get_taged_query(all_condition, untaged_components, taged_components);
		}


		query_node_variant get_untaged_subquery(untaged_query_node_variant base_query, const query_condition& condition)
		{
			//no need to check existance
			//filter archetype from base query as the subquery is a subset of the base query

			auto& new_query_node = m_query_nodes.emplace(
				condition.hash(),
				query_node(condition));
			std::visit([&](auto&& node) {
				const auto& arches = node->archetype_nodes();
				for (decltype(auto) arch : arches)
				{
					new_query_node.try_match(arch);
				}
				node->add_child(&new_query_node);
				}, base_query);
			return &new_query_node;
		}

		query_node_variant get_taged_subquery(taged_query_node_variant base_query, const query_condition& condition)
		{
			auto& new_query_node = m_taged_query_nodes.emplace(
				condition.hash(),
				taged_query_node(condition));
			std::visit([&](auto&& node) {
				const auto& arches = node->archetype_nodes();
				for (decltype(auto) arch : arches)
				{
					new_query_node.try_match(arch);
				}
				}, base_query);
			return &new_query_node;
		}

		query_node_variant get_mixed_subquery(
			untaged_query_node_variant untaged_base_query,
			taged_query_node_variant taged_base_query,
			const query_condition& condition)
		{
			auto& new_query_node = m_mixed_query_nodes.emplace(
				condition.hash(),
				mixed_query_node(condition));
			/*
			* filter algorithm:
			* 1. untaged arch filter as normal
			* 2. if taged arch's base arch match the condition, filter out, as it's included in untaged arch
			*/
			std::visit([&](auto&& node) {
				const auto& arches = node->archetype_nodes();
				for (decltype(auto) arch : arches)
				{
					new_query_node.try_match(arch);
				}
				}, untaged_base_query);
			std::visit([&](auto&& node) {
				const auto& arches = node->archetype_nodes();
				for (decltype(auto) arch : arches)
				{
					if (condition.match(arch->base_archetype_node()->archetype()))
						continue;
					new_query_node.try_match(arch);
				}
				}, taged_base_query);
			return &new_query_node;
		}


		query_node_variant get_ingroup_query(const query_condition& condition)
		{
			enum class query_type_t
			{
				untaged,
				taged,
				mixed
			}query_type = query_type_t::untaged;
			//todo support for no all condition query
			if (condition.all.size() != 0)
			{
				//compute query type
				auto basic_query_node = get_ingroup_query(condition.all);//todo need to add callback
				if (std::get<taged_query_node*>(basic_query_node) != nullptr)
					query_type = query_type_t::taged;
				/* end condition all*/
				/* condition any*/
				if (query_type == query_type_t::untaged)
				{
					bool has_tag;
					bool has_untag;
					for (auto comp : condition.any)
					{
						if (comp.is_tag()) has_tag = true;
						else has_untag = true;
					}
					if (has_tag && has_untag)
						query_type = query_type_t::mixed;
					else if (has_tag)
						query_type = query_type_t::taged;
					//else stay untaged
				}
				/* end condition any */
				/* condition none*/
				for (auto comp : condition.none)
				{
					if (comp.is_tag())
					{
						query_type = query_type_t::taged;
						break;
					}
				}
				/* end condition none */

				switch (query_type)
				{
				case query_type_t::untaged:
					return get_untaged_subquery(std::get<query_node*>(basic_query_node), condition);
					break;
				case query_type_t::taged:
					return get_taged_subquery(std::get<taged_query_node*>(basic_query_node), condition);
					break;
				case query_type_t::mixed:
					//depart mixed_query into untaged and taged part
					break;
				}
			}

		}




	public:

		void register_component(component_type_index component)
		{
			if (!m_group_phantom_component_nodes.contains(component.group()))
			{
				m_group_phantom_component_nodes.emplace(component.group(), taged_component_node({}));
			}
			if (component.is_tag())
				m_taged_component_nodes.emplace(component.hash(), taged_component_node(component));
			else
				m_untaged_component_nodes.emplace(component.hash(), component_node(component));
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


		void get_query(append_component condition_all, query_notify::Callback&& callback)
		{
			auto query_node = get_ingroup_query(condition_all);
			std::visit([&](auto&& arg) {arg->add_callback(std::move(callback)); }, query_node);
		}

		void get_query(const query_condition& condition, query_notify::Callback&& callback)
		{
			auto query_node = get_ingroup_query(condition);
			std::visit([&](auto&& arg) {arg->add_callback(std::move(callback)); }, query_node);
		}











	};
}