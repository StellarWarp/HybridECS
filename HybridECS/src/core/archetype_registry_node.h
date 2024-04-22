#pragma once
#include "archetype.h"
#include "query_condition.h"

namespace hyecs
{

	class query_node;
	class archetype_query_node;

	class archetype_node
	{
		archetype_index m_archetype;
		vector<query_node*> m_related_queries;//todo as component node is seperated from query node, need addition code for component node
	public:
		archetype_node(archetype_index arch) :m_archetype(arch) {}
		archetype_index archetype() const { return m_archetype; }

		const vector<query_node*>& related_queries() const { return m_related_queries; }

		void add_related_query(query_node* query) { m_related_queries.push_back(query); }
	};

	class taged_archetype_node
	{
		archetype_index m_archetype;
		archetype_node* m_base_archetype;
		vector<query_node*> m_related_queries;

	public:
		taged_archetype_node(archetype_index arch, archetype_node* base_archetype_node)
			:m_archetype(arch), m_base_archetype(base_archetype_node) {}

		archetype_index archetype() const { return m_archetype; }
		archetype_node* base_archetype_node() const { return m_base_archetype; }
		const vector<query_node*>& related_queries() const { return m_related_queries; }

		void add_related_query(query_node* query) { m_related_queries.push_back(query); }
	};

	using archetype_node_variant = std::variant<archetype_node*, taged_archetype_node*>;


	using archetype_query_index = uint64_t;

	archetype_query_index archetype_query_hash(archetype_index arch, const query_condition& taged_condition)
	{
		return arch.hash() + taged_condition.hash();
	}

	class archetype_query_node
	{
	public:
		using callback_t = std::function<void(archetype_index)>;
		using notify_adding_taged_t = std::function<void(taged_archetype_node*)>;
		using taged_archetype_nodes_t = vector<taged_archetype_node*>;
		enum set_type
		{
			direct_set,
			partial_set,
		};
	private:
		archetype_node* m_base_archetype_node;
		query_condition m_taged_condition;
		taged_archetype_nodes_t m_taged_archetype_nodes;
		set_type m_type;//todo

		vector<callback_t> m_archetype_addition_callbacks;
		vector<notify_adding_taged_t> m_notify_adding_taged_callbacks;
	public:
		archetype_query_node(archetype_node* base_archetype_node, const query_condition& taged_condition) :
			m_base_archetype_node(base_archetype_node), m_taged_condition(taged_condition), m_type(direct_set) {}
		archetype_query_node(
			archetype_node* base_archetype_node,
			const query_condition& taged_condition,
			const archetype_query_node* superset)
			:archetype_query_node(base_archetype_node, taged_condition)
		{
			m_type = superset->m_type;
			m_taged_archetype_nodes.reserve(superset->m_taged_archetype_nodes.size());
			for (auto& node : superset->m_taged_archetype_nodes)
			{
				if(/*match condition*/0)
					m_taged_archetype_nodes.push_back(node);
				else
					m_type = partial_set;
			}
			auto mut_superset = const_cast<archetype_query_node*>(superset);
			mut_superset->m_notify_adding_taged_callbacks.emplace_back(
				[this](taged_archetype_node* node) {
				try_match(node);
				});
		}

		query_index query_index() const { return m_taged_condition.hash(); }
		archetype_node* archetype_node() const { return m_base_archetype_node; }
		const taged_archetype_nodes_t& taged_archetype_nodes() const { return m_taged_archetype_nodes; }
		set_type type() const { return m_type; }

		void add_callback(callback_t&& callback)
		{
			for (auto& node : m_taged_archetype_nodes)
				callback(node->archetype());
			m_archetype_addition_callbacks.emplace_back(std::forward<callback_t>(callback));
		}

		void add_matched(taged_archetype_node* node)
		{
			m_taged_archetype_nodes.push_back(node);
			for (auto& callback : m_notify_adding_taged_callbacks)
				callback(node);
		}

		void try_match(taged_archetype_node* node)
		{
			if (m_taged_condition.match(node->archetype()))
				add_matched(node);
		}

		archetype_query_index hash() const { return archetype_query_hash(m_base_archetype_node->archetype(), m_taged_condition); }
	};


	class query_node
	{
	public:
		using callback_t = std::function<void(query_index)>;
		using archetype_nodes_t = set<const archetype_query_node*>;
	private:
		query_condition m_condition;
		archetype_nodes_t m_archetype_query_nodes;
		vector<query_node*> m_subquery_nodes;

		vector<callback_t> m_archetype_addition_callbacks;

		void publish_archetype_addition(query_index arch)
		{
			for (auto& callback : m_archetype_addition_callbacks)
				callback(arch);
		}

	public:
		query_node(sequence_ref<const component_type_index> component_types) :m_condition(component_types) {}
		query_node(const query_condition& condition) :m_condition(condition) {}
		const archetype_nodes_t& archetype_query_nodes() const { return m_archetype_query_nodes; }
		const vector<query_node*>& subquery_nodes() const { return m_subquery_nodes; }

		void add_callback(callback_t&& callback)
		{
			for (auto& node : m_archetype_query_nodes)
				callback(node->query_index());
			m_archetype_addition_callbacks.emplace_back(std::forward<callback_t>(callback));
		}

		void add_subquery(query_node* subquery) { m_subquery_nodes.push_back(subquery); }
		void add_matched(const archetype_query_node* node)
		{
			m_archetype_query_nodes.insert(node);
			publish_archetype_addition(node->query_index());
		}
		void try_match(const archetype_query_node* node)
		{
			assert(!m_archetype_query_nodes.contains(node));
			if (m_condition.match(node->archetype_node()->archetype()))
				add_matched(node);
		}


	public:
		uint64_t iterate_version = std::numeric_limits<uint64_t>::max();
	};


}