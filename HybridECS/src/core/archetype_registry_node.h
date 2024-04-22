#pragma once
#include "archetype.h"
#include "query_condition.h"
namespace hyecs
{
	class query_node;
	class taged_query_node;
	class taged_archetype_node;
	class cross_query_node;
	class mixed_query_node;
	class component_node;
	class taged_component_node;
	class archetype_node;

	class archetype_base_node
	{

	};

	class archetype_node
	{
		archetype_index m_archetype;
		using query_node_variant = std::variant<component_node*, query_node*, mixed_query_node*>;
		vector<query_node_variant> m_related_queries;
	public:
		archetype_node(archetype_index arch) :m_archetype(arch) {}
		archetype_index archetype() const { return m_archetype; }

		const vector<query_node_variant>& related_queries() const { return m_related_queries; }

		void add_related_query(query_node* query) { m_related_queries.push_back(query); }
		void add_related_query(mixed_query_node* query) { m_related_queries.push_back(query); }
		void add_component_node(component_node* component) { m_related_queries.push_back(component); }
	};

	class taged_archetype_node
	{
		archetype_index m_archetype;
		archetype_node* m_base_archetype;
		using query_node_variant = std::variant<taged_component_node*, taged_query_node*, mixed_query_node*>;
		vector<query_node_variant> m_related_queries;

	public:
		taged_archetype_node(archetype_index arch, archetype_node* base_archetype_node)
			:m_archetype(arch), m_base_archetype(base_archetype_node) {}

		archetype_index archetype() const { return m_archetype; }
		archetype_index base_archetype() const { return m_base_archetype->archetype(); }
		archetype_node* base_archetype_node() const { return m_base_archetype; }
		const vector<query_node_variant>& related_queries() const { return m_related_queries; }

		void add_related_query(taged_query_node* query) { m_related_queries.push_back(query); }
		void add_related_query(mixed_query_node* query) { m_related_queries.push_back(query); }
		void add_component_node(taged_component_node* component) { m_related_queries.push_back(component); }
	};

	using archetype_node_variant = std::variant<archetype_node*, taged_archetype_node*>;

	class query_notify
	{
	public:
		using Callback = std::function<void(archetype_index)>;
	protected:
		vector<Callback> m_archetype_addition_callbacks;

		void publish_archetype_addition(archetype_index arch)
		{
			for (auto& callback : m_archetype_addition_callbacks)
				callback(arch);
		}

		template<typename ArchetypeNodeContainer>
		void add_callback(Callback&& callback, const ArchetypeNodeContainer& container)
		{
			for (auto& node : container)
				callback(node->archetype());
			m_archetype_addition_callbacks.emplace_back(std::move(callback));
		}
	};

	class untaged_query_parent_node
	{
	protected:
		vector<query_node*> m_subquery_nodes;
		vector<taged_query_node*> m_taged_subquery_nodes;
		vector<cross_query_node*> m_cross_subquery_nodes;
	public:

		void add_child(query_node* child) { m_subquery_nodes.push_back(child); }
		void add_child(taged_query_node* child) { m_taged_subquery_nodes.push_back(child); }
		void add_child(cross_query_node* child) { m_cross_subquery_nodes.push_back(child); }

		const vector<query_node*>& children_node() const { return m_subquery_nodes; }
		const vector<taged_query_node*>& taged_children_node() const { return m_taged_subquery_nodes; }
		const vector<cross_query_node*>& cross_children_node() const { return m_cross_subquery_nodes; }
	};

	class taged_query_parent_node
	{
	protected:
		vector<taged_query_node*> m_taged_subquery_nodes;
		vector<cross_query_node*> m_cross_subquery_nodes;
	public:

		void add_child(taged_query_node* child) { m_taged_subquery_nodes.push_back(child); }
		void add_child(cross_query_node* child) { m_cross_subquery_nodes.push_back(child); }

		const vector<taged_query_node*>& taged_children_node() const { return m_taged_subquery_nodes; }
		const vector<cross_query_node*>& cross_children_node() const { return m_cross_subquery_nodes; }
	};


	using untaged_query_node_variant = std::variant<component_node*, query_node*>;
	using taged_query_node_variant = std::variant<taged_component_node*, taged_query_node*>;

	using query_node_variant = std::variant<
		component_node*,
		taged_component_node*,
		query_node*,
		taged_query_node*,
		mixed_query_node*
	>;

	class component_node :
		public query_notify,
		public untaged_query_parent_node
	{
		component_type_index m_component;
		set<archetype_node*> m_related_archetypes;

	public:
		component_node(component_type_index component) :m_component(component) {}
		void add_callback(Callback&& callback)
		{
			query_notify::add_callback(std::move(callback), m_related_archetypes);
		}

		const set<archetype_node*>& archetype_nodes() const { return m_related_archetypes; }
		const component_type_index& component() const { return m_component; }

		void add_matched(archetype_node* arch_node)
		{
			assert(m_related_archetypes.contains(arch_node) == false);
			m_related_archetypes.insert(arch_node);
			publish_archetype_addition(arch_node->archetype());
		}

		void try_match(archetype_node* arch_node)
		{
			if (arch_node->archetype().contains(m_component))
				add_related(arch_node);
		}
	};



	class taged_component_node :
		public query_notify,
		public taged_query_parent_node
	{
		component_type_index m_component;
		set<taged_archetype_node*> m_related_archetypes;

	public:
		taged_component_node(component_type_index component) :m_component(component) {}
		void add_callback(Callback&& callback)
		{
			query_notify::add_callback(std::move(callback), m_related_archetypes);
		}

		const set<taged_archetype_node*>& archetype_nodes() const { return m_related_archetypes; }
		const component_type_index& component() const { return m_component; }

		void add_matched(taged_archetype_node* arch_node)
		{
			assert(m_related_archetypes.contains(arch_node) == false);
			m_related_archetypes.insert(arch_node);
			arch_node->add_component_node(this);
			publish_archetype_addition(arch_node->archetype());
		}

		void try_match(taged_archetype_node* arch_node)
		{
			if (arch_node->archetype().contains(m_component))
				add_related(arch_node);
		}
	};


	class query_node :
		public query_notify,
		public untaged_query_parent_node
	{
		uint64_t m_iterate_version = std::numeric_limits<uint64_t>::max();
		query_condition m_query_condition;
		set<archetype_node*> m_fit_archetypes;
	public:

		query_node(sequence_ref<component_type_index> all_components) :m_query_condition(all_components) {}
		query_node(const query_condition& condition) :m_query_condition(condition) {}
		void add_callback(Callback&& callback)
		{
			query_notify::add_callback(std::move(callback), m_fit_archetypes);
		}


		constexpr uint64_t& iterate_version() { return m_iterate_version; }

		size_t archetype_count() const { return m_fit_archetypes.size(); }
		const set<archetype_node*>& archetype_nodes() const { return m_fit_archetypes; }
		const query_condition& condition() const { return m_query_condition; }



	private:

		bool is_match(archetype_index arch) const
		{
			return m_query_condition.match(arch);
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



	class taged_query_node :
		public query_notify,
		public taged_query_parent_node
	{
		uint64_t m_iterate_version = std::numeric_limits<uint64_t>::max();
		query_condition m_query_condition;//seperate from world's archetype pool
		set<taged_archetype_node*> m_fit_archetypes;
	public:

		taged_query_node(sequence_ref<component_type_index> all_components) :m_query_condition(all_components) {}
		taged_query_node(const query_condition& condition) :m_query_condition(condition) {}
		void add_callback(Callback&& callback)
		{
			query_notify::add_callback(std::move(callback), m_fit_archetypes);
		}

		constexpr uint64_t& iterate_version() { return m_iterate_version; }

		size_t archetype_count() const { return m_fit_archetypes.size(); }
		const set<taged_archetype_node*>& archetype_nodes() const { return m_fit_archetypes; }
		const query_condition& condition() const { return m_query_condition; }


	private:

		bool is_match(archetype_index arch) const
		{
			return m_query_condition.match(arch);
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

	class mixed_query_node : public query_notify
	{
		uint64_t m_iterate_version = std::numeric_limits<uint64_t>::max();
		query_condition m_query_condition;//seperate from world's archetype pool
		set<archetype_node*> m_fit_archetypes;
		set<taged_archetype_node*> m_fit_taged_archetypes;

	public:

		mixed_query_node(append_component all_components) :m_query_condition(all_components) {}
		mixed_query_node(const query_condition& condition) :m_query_condition(condition) {}
		void add_callback(Callback&& callback)
		{
			query_notify::add_callback(std::move(callback), m_fit_taged_archetypes);
		}

		constexpr uint64_t& iterate_version() { return m_iterate_version; }

		size_t archetype_count() const { return m_fit_taged_archetypes.size(); }
		const set<taged_archetype_node*>& archetype_nodes() const { return m_fit_taged_archetypes; }
		const query_condition& condition() const { return m_query_condition; }




	private:

		bool is_match(archetype_index arch) const
		{
			return m_query_condition.match(arch);
		}

		void add_matched(taged_archetype_node* arch_node, archetype_index index)
		{
			m_fit_taged_archetypes.insert(arch_node);
			arch_node->add_related_query(this);
			publish_archetype_addition(index);
		}
		void add_matched(archetype_node* arch_node, archetype_index index)
		{
			m_fit_archetypes.insert(arch_node);
			arch_node->add_related_query(this);
			publish_archetype_addition(index);
		}
	public:

		void add_matched(taged_archetype_node* arch_node)
		{
			assert(is_match(arch_node->archetype()) == true);
			assert(m_fit_taged_archetypes.contains(arch_node) == false);
			add_matched(arch_node, arch_node->archetype());
		}

		void add_matched(archetype_node* arch_node)
		{
			assert(is_match(arch_node->archetype()) == true);
			assert(m_fit_archetypes.contains(arch_node) == false);
			add_matched(arch_node, arch_node->archetype());
		}

		bool try_match(taged_archetype_node* arch_node)
		{
			auto arch = arch_node->archetype();
			auto base_arch = arch_node->base_archetype();
			//if the base arch is added to, the taged arch should be filtered out
			if (!m_query_condition.match(base_arch) && is_match(arch))//todo optimize
			{
				add_matched(arch_node, arch);
				return true;
			}
			return false;
		}

		bool try_match(archetype_node* arch_node)
		{
			auto arch = arch_node->archetype();
			if (is_match(arch))
			{
				add_matched(arch_node, arch);
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

}