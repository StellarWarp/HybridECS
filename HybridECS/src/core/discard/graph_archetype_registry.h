#pragma once
#include "archetype_storage.h"

namespace hyecs
{
	class query;
	class query_node;
	class archetype_node
	{
		archetype_index m_archetype;
		vector<query_node*> m_related_queries;

	public:
		archetype_node(archetype_index arch) :m_archetype(arch) {}
		archetype_index archetype() const { return m_archetype; }

		vector<query_node*>& related_queries() { return m_related_queries; }
	};
	class query_node
	{
		archetype m_all_component_condition;//seperate from world's archetype pool
		vector<query_node*> m_children;
		vector<query*> m_child_queries;
		vector<archetype_node*> m_fit_archetypes;
		uint64_t m_iterate_version;

	public:

		query_node(append_component all_components) :m_all_component_condition(all_components) {}

		void add_child(query_node* child)
		{
			m_children.push_back(child);
		}

		void add_child_query(query* child)
		{
			m_child_queries.push_back(child);
		}

		vector<query_node*>& children_node() { return m_children; }
		vector<archetype_node*>& archetype_nodes() { return m_fit_archetypes; }

		uint64_t& iterate_version() { return m_iterate_version; }

		void publish_archetype_addition(archetype_index arch)
		{
			...
		}

		bool is_match(archetype_index arch)
		{
			return m_all_component_condition.contains(arch.get_info());
		}


		bool try_match(archetype_node* arch_node)
		{
			auto arch = arch_node->archetype();
			if (arch.get_info().contains(m_all_component_condition))
			{
				m_fit_archetypes.push_back(arch_node);
				arch_node->related_queries().push_back(this);
				publish_archetype_addition(arch);
				return true;
			}
			return false;
		}

		const archetype& all_component_set() const { return m_all_component_condition; }

	};

	class cross_archetype_node;
	class cross_query_node
	{
		vector<cross_archetype_node*> m_fit_archetypes;

	};

	class cross_archetype_node
	{
		archetype_index m_archetype;
		vector<archetype_node*> m_base_archetypes;
		vector<cross_query_node*> m_related_queries;


	};

	class archetype_registry
	{
		vaildref_map<uint64_t, archetype> m_archetypes;
		vaildref_map<uint64_t, archetype_node> m_archetype_nodes;
		vaildref_map<uint64_t, query_node> m_query_nodes;
		vector<std::unique_ptr<archetype>> m_query_conditions;


	private:

		query_node* add_single_component_query_node(component_type_index component)
		{

		}

		archetype_index add_archetype(archetype_index base_arch, append_component components, uint64_t arch_hash)
		{
			auto arch = m_archetypes.emplace(arch_hash, archetype(base_arch.get_info(), components));
			auto arch_node = m_archetype_nodes.emplace(arch_hash, archetype_node(arch));
			if (auto iter = m_archetype_nodes.find(base_arch.hash()); iter != m_archetype_nodes.end())
			{
				auto base_arch_node = (*iter).second;
				for (auto query : base_arch_node.related_queries())
				{
					query->try_match(&arch_node);//match must be ture
				}
			}
			//init iteration
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			queue<query_node*> graph_iterate_queue;
			for (auto component : components)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				query_node* com_node_index;
				if (auto iter = m_query_nodes.find(com_node_hash); iter == m_query_nodes.end())
					com_node_index = add_single_component_query_node(component);
				else
					com_node_index = &(*iter).second;

				com_node_index->iterate_version() = iterate_version;
				graph_iterate_queue.push(com_node_index);
			}
			//iteration loop
			while (!graph_iterate_queue.empty())
			{
				//iteration control
				auto node = graph_iterate_queue.front();
				graph_iterate_queue.pop();

				//algorithm
				if (node->try_match(&arch_node))
					for (auto child : node->children_node())
						if (child->iterate_version() != iterate_version)
						{
							child->iterate_version() = iterate_version;
							graph_iterate_queue.push(child);
						}
			}
		}

		archetype_index add_archetype(archetype_index base_arch, remove_component components, uint64_t arch_hash)
		{
			auto arch = m_archetypes.emplace(arch_hash, archetype(base_arch.get_info(), components));
			auto arch_node = m_archetype_nodes.emplace(arch_hash, archetype_node(arch));
			auto origin_arch_node = m_archetype_nodes.at(base_arch.hash());
			for (auto query : origin_arch_node.related_queries())
			{
				query->try_match(&arch_node);
			}

		}

		archetype_index add_archetype(archetype_index base_arch, append_component adding, remove_component removings, uint64_t arch_hash)
		{
			auto arch = m_archetypes.emplace(arch_hash, archetype(base_arch.get_info(), adding, removings));
			auto arch_node = m_archetype_nodes.emplace(arch_hash, archetype_node(arch));
			auto origin_arch_node = m_archetype_nodes.at(base_arch.hash());
			for (auto query : origin_arch_node.related_queries())
			{
				query->try_match(&arch_node);
			}
			//init iteration
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			queue<query_node*> graph_iterate_queue;
			for (auto component : adding)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				query_node* com_node_index;
				if (auto iter = m_query_nodes.find(com_node_hash); iter == m_query_nodes.end())
					com_node_index = add_single_component_query_node(component);
				else
					com_node_index = &(*iter).second;

				com_node_index->iterate_version() = iterate_version;
				graph_iterate_queue.push(com_node_index);
			}
			//iteration loop
			while (!graph_iterate_queue.empty())
			{
				//iteration control
				auto node = graph_iterate_queue.front();
				graph_iterate_queue.pop();

				//algorithm
				if (node->try_match(&arch_node))
					for (auto child : node->children_node())
						if (child->iterate_version() != iterate_version)
						{
							child->iterate_version() = iterate_version;
							graph_iterate_queue.push(child);
						}
			}

		}

		query_node* add_query(append_component all_components)
		{
			uint64_t query_hash = archetype::addition_hash(0, all_components);
			if (auto iter = m_query_nodes.find(query_hash); iter != m_query_nodes.end())
				return &(*iter).second;
			auto& new_query_node = m_query_nodes.emplace(
				query_hash,
				query_node(all_components));
			//init iteration
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			queue<query_node*> graph_iterate_queue;
			std::vector<query_node*> final_queries;
			for (auto component : all_components)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				query_node* com_node_index;
				if (auto iter = m_query_nodes.find(com_node_hash); iter == m_query_nodes.end())
					com_node_index = add_single_component_query_node(component);
				else
					com_node_index = &(*iter).second;
				com_node_index->iterate_version() = iterate_version;
				graph_iterate_queue.push(com_node_index);
			}
			//iteration loop
			//get the final queries of current state to the all comp to query
			//the final queries are the queries that no other sub-queries can match
			while (!graph_iterate_queue.empty())
			{
				//iteration control
				auto node = graph_iterate_queue.front();
				graph_iterate_queue.pop();
				//algorithm
				bool is_final = true;
				for (auto child : node->children_node())
				{
					if (node->all_component_set().contains(child->all_component_set()))
					{
						if (child->iterate_version() != iterate_version)
						{
							child->iterate_version() = iterate_version;
							graph_iterate_queue.push(child);
						}
						is_final = false;
					}
				}
				if (is_final)
					final_queries.push_back(node);
			}
			//add final final_queries to new query and try match
			int min_matched_archetype = INT_MAX;
			query_node* filter_from = nullptr;
			for (auto child : final_queries)
			{
				child->add_child(&new_query_node);
				if (child->archetype_nodes().size() < min_matched_archetype)
				{
					min_matched_archetype = child->archetype_nodes().size();
					filter_from = child;
				}
			}
			if (!filter_from)_UNLIKELY
			{
				throw std::exception("no archetype matched");
				return nullptr;
			}
				for (auto arch_node : filter_from->archetype_nodes())
				{
					new_query_node.try_match(arch_node);
				}
			return &new_query_node;
		}

	public:

		archetype_index get_archetype(archetype_index base_arch, append_component components)
		{

			auto arch_hash = base_arch.hash(components);
			if (auto iter = m_archetypes.find(arch_hash); iter != m_archetypes.end())
				return (*iter).second;
			return add_archetype(base_arch, components, arch_hash);
		}

		archetype_index get_archetype(archetype_index base_arch, remove_component components)
		{
			auto arch_hash = base_arch.hash(components);
			if (auto iter = m_archetypes.find(arch_hash); iter != m_archetypes.end())
				return (*iter).second;
			return add_archetype(base_arch, components, arch_hash);
		}

		archetype_index get_archetype(archetype_index base_arch, append_component addings, remove_component removings)
		{

			uint64_t hash = base_arch.hash();
			hash = archetype::addition_hash(
				hash,
				std::initializer_list<component_type_index>(
					addings.begin(), addings.end()));
			hash = archetype::subtraction_hash(
				hash,
				std::initializer_list<component_type_index>(
					removings.begin(), removings.end()));

			if (auto iter = m_archetypes.find(hash); iter != m_archetypes.end())
				return (*iter).second;
			return add_archetype(base_arch, components, hash);
		}








	};
}