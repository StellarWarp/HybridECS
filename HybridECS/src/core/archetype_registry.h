#pragma once
#include "archetype_storage.h"

namespace hyecs
{
	class query;
	class query_node;
	class taged_query_node;
	class taged_archetype_node;
	class cross_query_node;
	class cross_archetype_node;


	class archetype_node
	{
		archetype_index m_archetype;
		vector<query_node*> m_related_queries;

	public:
		archetype_node(archetype_index arch) :m_archetype(arch) {}
		archetype_index archetype() const { return m_archetype; }

		vector<query_node*>& related_queries() { return m_related_queries; }
		void add_related_query(query_node* query) { m_related_queries.push_back(query); }
	};
	class query_node
	{
		uint64_t m_iterate_version;
		archetype m_all_component_condition;//seperate from world's archetype pool
		vector<query_node*> m_children;
		set<archetype_node*> m_fit_archetypes;
		//external link
		vector<taged_query_node*> m_taged_children;
		vector<cross_query_node*> m_cross_children;

		vector<query*> m_child_queries;///todo 
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

		constexpr vector<query_node*>& children_node() { return m_children; }

		constexpr uint64_t& iterate_version() { return m_iterate_version; }

		void publish_archetype_addition(archetype_index arch)
		{

		}

		void add_matched(archetype_node* arch_node)
		{
			assert(m_fit_archetypes.contains(arch_node) == false);
			m_fit_archetypes.insert(arch_node);
			arch_node->add_related_query(this);
		}

		bool is_match(archetype_index arch)
		{
			return arch.get_info().contains(m_all_component_condition);
		}

		bool try_match(archetype_node* arch_node)
		{
			auto arch = arch_node->archetype();
			if (arch.get_info().contains(m_all_component_condition))
			{
				m_fit_archetypes.insert(arch_node);
				arch_node->add_related_query(this);
				publish_archetype_addition(arch);
				return true;
			}
			return false;
		}

		size_t archetype_count() const { return m_fit_archetypes.size(); }

		const set<archetype_node*>& archetype_nodes() const { return m_fit_archetypes; }



		const archetype& all_component_set() const { return m_all_component_condition; }

	};

	class taged_archetype_node
	{
		archetype_index m_archetype;
		archetype_node* m_base_archetype;
		vector<taged_query_node*> m_related_queries;

	public:

		taged_archetype_node(archetype_index arch, archetype_node* base_archetype_node)
			:m_archetype(arch), m_base_archetype(base_archetype_node) {}

		archetype_index archetype() const { return m_archetype; }
		void add_related_query(taged_query_node* query) { m_related_queries.push_back(query); }

		vector<taged_query_node*>& related_queries() { return m_related_queries; }

		archetype_node* base_archetype_node() const { return m_base_archetype; }
	};

	class taged_query_node
	{
		uint64_t m_iterate_version;
		archetype m_all_component_condition;//seperate from world's archetype pool
		vector<taged_query_node*> m_children;
		set<taged_archetype_node*> m_fit_archetypes;
		//external link
		vector<cross_query_node*> m_cross_children;

		vector<query*> m_child_queries;///todo

	public:

		taged_query_node(append_component all_components) :m_all_component_condition(all_components) {}


		void add_matched(taged_archetype_node* arch_node)
		{
			assert(m_fit_archetypes.contains(arch_node) == false);
			m_fit_archetypes.insert(arch_node);
			arch_node->add_related_query(this);
		}

		bool try_match(taged_archetype_node* arch_node)
		{
			auto arch = arch_node->archetype();
			if (arch.get_info().contains(m_all_component_condition))
			{
				m_fit_archetypes.insert(arch_node);
				arch_node->add_related_query(this);
				return true;
			}
			return false;
		}

		size_t archetype_count() const { return m_fit_archetypes.size(); }

		const set<taged_archetype_node*>& archetype_nodes() const { return m_fit_archetypes; }

		const archetype& all_component_set() const { return m_all_component_condition; }

		uint64_t& iterate_version() { return m_iterate_version; }

		vector<taged_query_node*>& children_node() { return m_children; }

	};

	class cross_archetype_node
	{
		archetype_index m_archetype;
		vector<archetype_node*> m_base_archetypes;
		vector<cross_query_node*> m_related_queries;
	};

	class cross_query_node
	{
		archetype m_all_component_condition;//seperate from world's archetype pool
		vector<cross_query_node*> m_children;
		set<cross_archetype_node*> m_fit_archetypes;
		uint64_t m_iterate_version;

		vector<query*> m_child_queries;///todo
	};






	class archetype_registry
	{
		vaildref_map<uint64_t, archetype> m_archetypes;
		vaildref_map<uint64_t, archetype_node> m_archetype_nodes;
		vaildref_map<uint64_t, taged_archetype_node> m_taged_archetype_nodes;
		vaildref_map<uint64_t, cross_archetype_node> m_cross_archetype_nodes;
		vaildref_map<uint64_t, query_node> m_query_nodes;
		vaildref_map<uint64_t, taged_query_node> m_taged_query_nodes;
		vaildref_map<uint64_t, cross_query_node> m_cross_query_nodes;

		vector<std::unique_ptr<archetype>> m_query_conditions;


	private:

		/// <summary>
		/// untaged archetype/query func
		/// </summary>

		query_node* add_single_component_query_node(component_type_index component)
		{
			..
		}



		archetype_node* add_archetype(archetype_index origin_arch, append_component adding, remove_component removings, uint64_t arch_hash)
		{
			const auto& arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));
			auto& arch_node = m_archetype_nodes.emplace(arch_hash, archetype_node(arch));
			auto& origin_arch_node = m_archetype_nodes.at(origin_arch.hash());
			if (removings.size() != 0)
				for (auto query : origin_arch_node.related_queries())
					query->try_match(&arch_node);
			else
				for (auto query : origin_arch_node.related_queries())
					query->add_matched(&arch_node);
			//single layer iteraton
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			for (auto component : adding)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				query_node* com_node_index;
				if (auto iter = m_query_nodes.find(com_node_hash); iter == m_query_nodes.end())
					com_node_index = add_single_component_query_node(component);
				else
					com_node_index = &(*iter).second;

				com_node_index->iterate_version() = iterate_version;
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

		query_node* add_query(append_component all_components)
		{
			uint64_t query_hash = archetype::addition_hash(0, all_components);
			//check if query node already exist
			if (auto iter = m_query_nodes.find(query_hash); iter != m_query_nodes.end())
				return &(*iter).second;
			//create new query node
			auto& new_query_node = m_query_nodes.emplace(
				query_hash,
				query_node(all_components));
			//init iteration
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			int min_archetype_count = std::numeric_limits<int>::max();
			query_node* inital_match_set = nullptr;
			vector<query_node*> filter_component_nodes(all_components.size() - 1, nullptr);
			for (auto component : all_components)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				query_node* com_node_index;
				if (auto iter = m_query_nodes.find(com_node_hash); iter == m_query_nodes.end())
					com_node_index = add_single_component_query_node(component);/// todo
				else
					com_node_index = &(*iter).second;
				com_node_index->iterate_version() = iterate_version;
				com_node_index->add_child(&new_query_node);
				//get the min set of archetype
				int related_archetype_count = com_node_index->archetype_count();
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
			hash = archetype::addition_hash(
				hash,
				std::initializer_list<component_type_index>(
					addings.begin(), addings.end()));
			hash = archetype::subtraction_hash(
				hash,
				std::initializer_list<component_type_index>(
					removings.begin(), removings.end()));

			if (auto iter = m_archetype_nodes.find(hash); iter != m_archetype_nodes.end())
				return &(*iter).second;
			return add_archetype(origin_arch, addings, removings, hash);
		}

		/// <summary>
		/// in-group archetype/query func
		/// </summary>


		taged_query_node* add_single_component_taged_query_node(component_type_index component)
		{
			..
		}

		archetype_node* add_ingroup_archetype(archetype_index origin_arch, append_component adding, remove_component removings, uint64_t arch_hash)
		{
			auto& origin_arch_node = m_taged_archetype_nodes.at(origin_arch.hash());

			vector<component_type_index> untaged_removings_vec;
			for (auto comp : removings)
			{
				if (!comp.is_tag()) untaged_removings_vec.push_back(comp);
			}
			vector<component_type_index> taged_adding_vec;
			vector<component_type_index> untaged_adding_vec;
			for (auto comp : adding)
			{
				if (comp.is_tag()) taged_adding_vec.push_back(comp);
				else untaged_adding_vec.push_back(comp);
			}

			remove_component untaged_removings(&*untaged_removings_vec.begin(), &*untaged_removings_vec.end());
			append_component untaged_adding(&*untaged_adding_vec.begin(), &*untaged_adding_vec.end());


			if (/*new arch is not taged archetype*/)
			{
				if (/*base arch is not taged archetype*/)
				{
					return add_archetype(origin_arch, adding, removings, arch_hash);
				}
				else
				{

					auto untaged_arch_node = origin_arch_node.base_archetype_node();
					return add_archetype(untaged_arch_node->archetype(), adding, untaged_removings, arch_hash);//the adding do not include tag
				}
			}
			else//new arch is taged
			{

				const auto& arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));
				//find or create base arch
				auto base_arch_node = get_archetype_node(origin_arch_node.base_archetype_node()->archetype(), untaged_adding, untaged_removings);
				auto& arch_node = m_taged_archetype_nodes.emplace(arch_hash, taged_archetype_node(arch, base_arch_node));
				if (/*base arch is not taged archetype*/)
				{

				}
				else
				{
					auto& origin_arch_node = m_taged_archetype_nodes.at(origin_arch.hash());

					//try match origin archetype related query
					if (removings.size() != 0)
						for (auto query : origin_arch_node.related_queries())
							query->try_match(&arch_node);
					else
						for (auto query : origin_arch_node.related_queries())
							query->add_matched(&arch_node);

				}

				//try match all taged component related query
				auto iterate_version = seqence_allocator<taged_query_node>::allocate();
				for (auto component : taged_adding_vec)
				{
					auto com_node_hash = archetype::addition_hash(0, { component });
					taged_query_node* taged_node;
					if (auto iter = m_taged_query_nodes.find(com_node_hash); iter == m_taged_query_nodes.end())
						taged_node = add_single_component_taged_query_node(component);
					else
						taged_node = &(*iter).second;

					taged_node->iterate_version() = iterate_version;
					taged_node->add_matched(&arch_node);
					for (auto query_node : taged_node->children_node())
					{
						if (query_node->iterate_version() != iterate_version)
						{
							query_node->iterate_version() = iterate_version;
							query_node->try_match(&arch_node);
						}
					}
				}
			}
		}

		archetype_index add_cross_archetype(archetype_index origin_arch, append_component adding, remove_component removings, uint64_t arch_hash)
		{
			if (/*new arch is in group*/)
			{
				if(/*origin_arch is in group*/)
					add_ingroup_archetype(origin_arch, adding, removings, arch_hash);
				else
				{
					/*
					spilt the addition subtruction to each group
					ger related query of each base arch
					get intersed cross combination sub-query
					try match for those sub-query (no need graph iteration)
					*/
				}
			}
			else
			{
				if (/*origin_arch is in group*/)
			}
		}


	public:










	};
}