#pragma once
#include "archetype_storage.h"

namespace hyecs
{
	class query;
	class query_node;
	class taged_query_node;
	class taged_archetype_node;
	class cross_query_node;
	class untaged_component_node;
	class taged_component_node;
	class archetype_node;


	class archetype_node
	{
		archetype_index m_archetype;
		vector<query_node*> m_related_queries;//todo as component node is seperated from query node, need addition code for component node
		vector<untaged_component_node*> m_component_nodes;
	public:
		archetype_node(archetype_index arch) :m_archetype(arch) {}
		archetype_index archetype() const { return m_archetype; }

		const vector<query_node*>& related_queries() const { return m_related_queries; }
		const vector<untaged_component_node*>& component_nodes() const { return m_component_nodes; }

		void add_related_query(query_node* query) { m_related_queries.push_back(query); }
		void add_component_node(untaged_component_node* component) { m_component_nodes.push_back(component); }
	};

	class taged_archetype_node
	{
		archetype_index m_archetype;
		archetype_node* m_base_archetype;
		vector<taged_query_node*> m_related_queries;
		vector<taged_component_node*> m_component_nodes;

	public:
		taged_archetype_node(archetype_index arch, archetype_node* base_archetype_node)
			:m_archetype(arch), m_base_archetype(base_archetype_node) {}

		archetype_index archetype() const { return m_archetype; }
		archetype_node* base_archetype_node() const { return m_base_archetype; }
		const vector<taged_query_node*>& related_queries() const { return m_related_queries; }
		const vector<taged_component_node*>& component_nodes() const { return m_component_nodes; }

		void add_related_query(taged_query_node* query) { m_related_queries.push_back(query); }
		void add_component_node(taged_component_node* component) { m_component_nodes.push_back(component); }
	};

	class untaged_component_node
	{
		component_type_index m_component;
		set<archetype_node*> m_related_archetypes;
		vector<query_node*> m_subquery_nodes;
		vector<taged_query_node*> m_taged_subquery_nodes;
		vector<cross_query_node*> m_cross_subquery_nodes;
		vector<query*> m_single_component_queries;///todo

	public:
		untaged_component_node(component_type_index component) :m_component(component) {}


		const vector<query_node*>& children_node() const { return m_subquery_nodes; }
		const vector<taged_query_node*>& taged_children_node() const { return m_taged_subquery_nodes; }
		const vector<cross_query_node*>& cross_children_node() const { return m_cross_subquery_nodes; }

		const set<archetype_node*>& related_archetypes() const { return m_related_archetypes; }
		const component_type_index& component() const { return m_component; }

		void add_child(query_node* child)
		{
			m_subquery_nodes.push_back(child);
		}

		void add_child(taged_query_node* child)
		{
			m_taged_subquery_nodes.push_back(child);
		}

		void add_child(cross_query_node* child)
		{
			m_cross_subquery_nodes.push_back(child);
		}

		void add_child_query(query* child)
		{
			m_single_component_queries.push_back(child);
		}

		void publish_archetype_addition(archetype_index arch)
		{
			//todo
		}

		void add_related(archetype_node* arch_node)
		{
			assert(m_related_archetypes.contains(arch_node) == false);
			m_related_archetypes.insert(arch_node);
			publish_archetype_addition(arch_node->archetype());
		}

		void try_relate(archetype_node* arch_node)
		{
			if (arch_node->archetype().contains(m_component))
				add_related(arch_node);
		}
	};


	class taged_component_node
	{
		component_type_index m_component;
		set<taged_archetype_node*> m_related_archetypes;
		vector<taged_query_node*> m_taged_subquery_nodes;
		vector<cross_query_node*> m_cross_subquery_nodes;
		vector<query*> m_single_component_queries;///todo
	public:
		taged_component_node(component_type_index component) :m_component(component) {}


		const vector<taged_query_node*>& taged_children_node() const { return m_taged_subquery_nodes; }
		const vector<cross_query_node*>& cross_children_node() const { return m_cross_subquery_nodes; }

		const set<taged_archetype_node*>& related_archetypes() const { return m_related_archetypes; }
		const component_type_index& component() const { return m_component; }

		void add_child(taged_query_node* child)
		{
			m_taged_subquery_nodes.push_back(child);
		}

		void add_child(cross_query_node* child)
		{
			m_cross_subquery_nodes.push_back(child);
		}

		void add_child_query(query* child)
		{
			m_single_component_queries.push_back(child);
		}

		void publish_archetype_addition(archetype_index arch)
		{
			//todo
		}

		void add_related(taged_archetype_node* arch_node)
		{
			assert(m_related_archetypes.contains(arch_node) == false);
			m_related_archetypes.insert(arch_node);
			arch_node->add_component_node(this);
			publish_archetype_addition(arch_node->archetype());
		}

		void try_relate(taged_archetype_node* arch_node)
		{
			if (arch_node->archetype().contains(m_component))
				add_related(arch_node);
		}
	};


	class query_node
	{
		uint64_t m_iterate_version;
		archetype m_all_component_condition;//seperate from world's archetype pool
		set<archetype_node*> m_fit_archetypes;
		vector<query*> m_child_queries;///todo 
	public:

		query_node(append_component all_components) :m_all_component_condition(all_components) {}
		constexpr uint64_t& iterate_version() { return m_iterate_version; }

		size_t archetype_count() const { return m_fit_archetypes.size(); }
		const set<archetype_node*>& archetype_nodes() const { return m_fit_archetypes; }
		const archetype& all_component_set() const { return m_all_component_condition; }


		void add_child_query(query* child)
		{
			m_child_queries.push_back(child);
		}

	private:
		void publish_archetype_addition(archetype_index arch)
		{
			//todo
		}

		bool is_match(archetype_index arch) const
		{
			return arch.get_info().contains(m_all_component_condition);
		}
	public:

		void add_matched(archetype_node* arch_node)
		{
			assert(is_match(arch_node->archetype()) == true);
			assert(m_fit_archetypes.contains(arch_node) == false);
			m_fit_archetypes.insert(arch_node);
			arch_node->add_related_query(this);
			publish_archetype_addition(arch_node->archetype());
		}

		bool try_match(archetype_node* arch_node)
		{
			auto arch = arch_node->archetype();
			if (is_match(arch))
			{
				m_fit_archetypes.insert(arch_node);
				arch_node->add_related_query(this);
				publish_archetype_addition(arch);
				return true;
			}
			return false;
		}

	};



	class taged_query_node
	{
		uint64_t m_iterate_version;
		archetype m_all_component_condition;//seperate from world's archetype pool
		set<taged_archetype_node*> m_fit_archetypes;
		vector<query*> m_child_queries;///todo

	public:

		taged_query_node(append_component all_components) :m_all_component_condition(all_components) {}


		constexpr uint64_t& iterate_version() { return m_iterate_version; }

		size_t archetype_count() const { return m_fit_archetypes.size(); }
		const set<taged_archetype_node*>& archetype_nodes() const { return m_fit_archetypes; }
		const archetype& all_component_set() const { return m_all_component_condition; }


		void add_child_query(query* child)
		{
			m_child_queries.push_back(child);
		}

	private:
		void publish_archetype_addition(archetype_index arch)
		{
			//todo
		}

		bool is_match(archetype_index arch) const
		{
			return arch.get_info().contains(m_all_component_condition);
		}
	public:

		void add_matched(taged_archetype_node* arch_node)
		{
			assert(is_match(arch_node->archetype()) == true);
			assert(m_fit_archetypes.contains(arch_node) == false);
			m_fit_archetypes.insert(arch_node);
			arch_node->add_related_query(this);
			publish_archetype_addition(arch_node->archetype());
		}

		bool try_match(taged_archetype_node* arch_node)
		{
			auto arch = arch_node->archetype();
			if (is_match(arch))
			{
				m_fit_archetypes.insert(arch_node);
				arch_node->add_related_query(this);
				publish_archetype_addition(arch);
				return true;
			}
			return false;
		}

	};

	class cross_query_node
	{
		archetype m_all_component_condition;//seperate from world's archetype pool
		vector<cross_query_node*> m_children;
		uint64_t m_iterate_version;

		vector<query*> m_child_queries;///todo
	};





	class archetype_registry
	{
		vaildref_map<uint64_t, archetype> m_archetypes;
		vaildref_map<uint64_t, archetype_node> m_archetype_nodes;
		vaildref_map<uint64_t, taged_archetype_node> m_taged_archetype_nodes;
		vaildref_map<uint64_t, untaged_component_node> m_untaged_component_nodes;
		vaildref_map<uint64_t, taged_component_node> m_taged_component_nodes;
		vaildref_map<uint64_t, query_node> m_query_nodes;
		vaildref_map<uint64_t, taged_query_node> m_taged_query_nodes;
		vaildref_map<uint64_t, cross_query_node> m_cross_query_nodes;


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
				return &m_untaged_component_nodes.emplace(component.hash(), untaged_component_node(component));
		}

		archetype_node* add_archetype(archetype_index origin_arch, append_component adding, remove_component removings, uint64_t arch_hash, archetype_index arch = archetype_index::empty_archetype)
		{
			if (arch == archetype_index::empty_archetype)
				arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));
			auto& arch_node = m_archetype_nodes.emplace(arch_hash, archetype_node(arch));
			auto& origin_arch_node = m_archetype_nodes.at(origin_arch.hash());
			if (removings.size() != 0)
			{
				for (auto query : origin_arch_node.related_queries())
					query->try_match(&arch_node);
				for (auto component : origin_arch_node.component_nodes())
					component->try_relate(&arch_node);
			}
			else
			{
				for (auto query : origin_arch_node.related_queries())
					query->add_matched(&arch_node);
				for (auto component : origin_arch_node.component_nodes())
					component->add_related(&arch_node);
			}


			//single layer iteraton
			uint64_t iterate_version = seqence_allocator<query_node>::allocate();
			for (auto component : adding)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				untaged_component_node* com_node_index;
				if (auto iter = m_untaged_component_nodes.find(com_node_hash); iter == m_untaged_component_nodes.end())
					com_node_index = add_single_component_query_node<untaged_component_node>(component);
				else
					com_node_index = &(*iter).second;

				com_node_index->add_related(&arch_node);
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
			untaged_component_node* inital_match_set = nullptr;
			vector<untaged_component_node*> filter_component_nodes(all_components.size() - 1, nullptr);
			for (auto component : all_components)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				untaged_component_node* com_node_index;
				if (auto iter = m_untaged_component_nodes.find(com_node_hash); iter == m_untaged_component_nodes.end())
					com_node_index = add_single_component_query_node<untaged_component_node>(component);/// todo
				else
					com_node_index = &(*iter).second;

				com_node_index->add_child(&new_query_node);
				//get the min set of archetype
				int related_archetype_count = com_node_index->related_archetypes().size();
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
			for (auto archetype_node : inital_match_set->related_archetypes())
			{
				bool is_match = true;
				for (auto filter_node : filter_component_nodes)
				{
					if (!filter_node->related_archetypes().contains(archetype_node))
					{
						is_match = false;
						break;
					}
				}
				if (is_match)
					new_query_node.add_matched(archetype_node);
			}

			//alternative method for union operation
			//for (auto archetype_node : inital_match_set->related_archetypes())
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

			const auto& arch = m_archetypes.emplace(arch_hash, archetype(origin_arch.get_info(), adding, removings));
			if (!arch.is_tagged())/*new arch is not taged archetype*/
			{
				if (!origin_arch.is_tagged())
				{
					return add_archetype(origin_arch, adding, removings, arch_hash, arch);
				}
				else
				{
					auto untaged_arch_node = origin_arch_node.base_archetype_node();
					return add_archetype(untaged_arch_node->archetype(), adding, untaged_removings, arch_hash, arch);//the adding do not include tag
				}
			}
			else//new arch is taged
			{
				//find or create base arch
				auto base_arch_node = get_archetype_node(origin_arch_node.base_archetype_node()->archetype(), untaged_adding, untaged_removings);
				auto& arch_node = m_taged_archetype_nodes.emplace(arch_hash, taged_archetype_node(arch, base_arch_node));
				if (origin_arch.is_tagged())
				{
					auto& origin_arch_node = m_taged_archetype_nodes.at(origin_arch.hash());

					//try match origin archetype related query
					if (removings.size() != 0)
					{
						for (auto query : origin_arch_node.related_queries())
							query->try_match(&arch_node);
						for (auto component : origin_arch_node.component_nodes())
							component->try_relate(&arch_node);
					}
					else
					{
						for (auto query : origin_arch_node.related_queries())
							query->add_matched(&arch_node);
						for (auto component : origin_arch_node.component_nodes())
							component->add_related(&arch_node);
					}
				}

				//try match all taged component related query
				auto iterate_version = seqence_allocator<taged_query_node>::allocate();
				for (auto component : taged_adding_vec)
				{
					auto com_node_hash = archetype::addition_hash(0, { component });
					taged_component_node* taged_node;
					if (auto iter = m_taged_component_nodes.find(com_node_hash); iter == m_taged_component_nodes.end())
						taged_node = add_single_component_query_node<taged_component_node>(component);
					else
						taged_node = &(*iter).second;

					taged_node->add_related(&arch_node);
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
				for (auto component : untaged_adding_vec)
				{
					auto com_node_hash = archetype::addition_hash(0, { component });
					untaged_component_node* untaged_node;
					if (auto iter = m_untaged_component_nodes.find(com_node_hash); iter == m_untaged_component_nodes.end())
						untaged_node = add_single_component_query_node<untaged_component_node>(component);
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

				
			}
		}

		taged_query_node* add_ingroup_query(append_component all_components)
		{
			uint64_t query_hash = archetype::addition_hash(0, all_components);
			//check if query node already exist
			if (auto iter = m_taged_query_nodes.find(query_hash); iter != m_taged_query_nodes.end())
				return &(*iter).second;
			//create new query node
			auto& new_query_node = m_taged_query_nodes.emplace(
				query_hash,
				taged_query_node(all_components));

			vector<component_type_index> untaged_components;
			vector<component_type_index> taged_components;
			for (auto comp : all_components)
			{
				if (comp.is_tag()) taged_components.push_back(comp);
				else untaged_components.push_back(comp);
			}

			//init iteration
			uint64_t iterate_version = seqence_allocator<taged_query_node>::allocate();
			int min_archetype_count = std::numeric_limits<int>::max();
			set<taged_archetype_node*> const* inital_match_set = nullptr;
			vector<set<taged_archetype_node*> const*> filter_set(all_components.size() - 1, nullptr);

			for (auto component : untaged_components)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				untaged_component_node* com_node_index;
				if (auto iter = m_untaged_component_nodes.find(com_node_hash); iter == m_untaged_component_nodes.end())
					com_node_index = add_single_component_query_node<untaged_component_node>(component);
				else
					com_node_index = &(*iter).second;

				com_node_index->add_child(&new_query_node);
				///untaged component do not store the related taged archetype
				//get the min set of archetype
				//int related_archetype_count = com_node_index->related_archetypes().size();
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

			for (auto component : taged_components)
			{
				auto com_node_hash = archetype::addition_hash(0, { component });
				taged_component_node* com_node_index;
				if (auto iter = m_taged_component_nodes.find(com_node_hash); iter == m_taged_component_nodes.end())
					com_node_index = add_single_component_query_node<taged_component_node>(component);/// todo
				else
					com_node_index = &(*iter).second;

				com_node_index->add_child(&new_query_node);
				//get the min set of archetype
				int related_archetype_count = com_node_index->related_archetypes().size();
				if (related_archetype_count < min_archetype_count)
				{
					min_archetype_count = related_archetype_count;
					if (inital_match_set != nullptr)
						filter_set.push_back(&com_node_index->related_archetypes());
					inital_match_set = &com_node_index->related_archetypes();
				}
				else
				{
					filter_set.push_back(&com_node_index->related_archetypes());
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

			//alternative method for union operation
			for (auto archetype_node : *inital_match_set) new_query_node.try_match(archetype_node);
			

		
			return &new_query_node;
		}

	public:

		void register_component(component_type_index component)
		{
			//todo
		}










	};
}