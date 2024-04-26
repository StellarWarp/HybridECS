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
		archetype_query_node* m_direct_query_node;
		vector<query_node*> m_related_queries;
	
		friend class archetype_query_node;
	public:
		archetype_node(archetype_index arch) :m_archetype(arch) {}
		archetype_index archetype() const { return m_archetype; }
		archetype_query_node* direct_query_node() const { return m_direct_query_node; }
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
		using notify_adding_taged_t = std::function<void(taged_archetype_node*)>;
		using notify_partial_convert_t = std::function<void()>;
		using taged_archetype_nodes_t = vector<taged_archetype_node*>;
		enum set_type
		{
			full_set,
			partial_set,
		};
	private:
		archetype_node* m_base_archetype_node;
		query_condition m_taged_condition;
		taged_archetype_nodes_t m_taged_archetype_nodes;
		set_type m_type;
		bool m_is_isolated;

		vector<notify_adding_taged_t> m_notify_adding_taged_callbacks;
		vector<notify_partial_convert_t> m_notify_partial_convert_callbacks;
	public:
		archetype_query_node(archetype_node* base_archetype_node) :
			m_base_archetype_node(base_archetype_node),
			m_taged_condition(),
			m_type(full_set),
			m_is_isolated(true)
		{
			m_base_archetype_node->m_direct_query_node = this;
		}
		archetype_query_node(archetype_node* base_archetype_node, const query_condition& taged_condition) :
			m_base_archetype_node(base_archetype_node),
			m_taged_condition(taged_condition),
			m_type(partial_set),
			m_is_isolated(true)
		{}

		void convert_to_connected(const archetype_query_node* superset)
		{
			m_is_isolated = false;
			auto mut_superset = const_cast<archetype_query_node*>(superset);
			mut_superset->m_notify_adding_taged_callbacks.emplace_back(
				[this](taged_archetype_node* node) {try_match(node);});
		}

		archetype_query_node(
			const archetype_query_node* superset,
			const query_condition& taged_condition)
			:m_base_archetype_node(superset->archetype_node()),
			m_taged_condition(taged_condition), 
			m_type(full_set),
			m_is_isolated(false)
		{
			assert(!taged_condition.empty());
			m_taged_archetype_nodes.reserve(superset->m_taged_archetype_nodes.size());
			if ( !taged_condition.match(m_base_archetype_node->archetype()) )
				m_type = partial_set;

			for (auto& node : superset->m_taged_archetype_nodes)
			{
				if(taged_condition.match(node->archetype()))
					m_taged_archetype_nodes.push_back(node);
				else
					m_type = partial_set;
			}
			auto mut_superset = const_cast<archetype_query_node*>(superset);
			mut_superset->m_notify_adding_taged_callbacks.emplace_back(
				[this](taged_archetype_node* node) {
					if (!try_match(node) && m_type == full_set)
					{
						m_type = partial_set;
						for (auto& callback : m_notify_partial_convert_callbacks)
							callback();
					}
				});
		}

		query_index query_index() const { return m_taged_condition.hash(); }
		const query_condition& taged_condition() const { return m_taged_condition; }
		archetype_node* archetype_node() const { return m_base_archetype_node; }
		const taged_archetype_nodes_t& taged_archetype_nodes() const { return m_taged_archetype_nodes; }
		set_type type() const { return m_type; }
		bool is_full_set() const { return m_type == full_set; }
		bool is_partial_set() const { return m_type == partial_set; }
		bool isolated() const { return m_is_isolated; }

		//void add_callback(callback_t&& callback)
		//{
		//	for (auto& node : m_taged_archetype_nodes)
		//		callback(node->archetype());
		//	m_archetype_addition_callbacks.emplace_back(std::forward<callback_t>(callback));
		//}

		void add_matched(taged_archetype_node* node)
		{
			m_taged_archetype_nodes.push_back(node);
			for (auto& callback : m_notify_adding_taged_callbacks)
				callback(node);
		}

		bool try_match(taged_archetype_node* node)
		{
			if (m_taged_condition.match(node->archetype()))
			{
				add_matched(node);
				return true;
			}
			return false;
		}

		archetype_query_index hash() const { return archetype_query_hash(m_base_archetype_node->archetype(), m_taged_condition); }
	};

	
	class query_node
	{
	public:
		using callback_t = std::function<void(query_index)>;
		using archetype_nodes_t = map<archetype_node* ,const archetype_query_node*>;
		enum query_type
		{
			untaged,
			mixed,
			pure_tag,
		};
	private:
		query_condition m_condition;
		archetype_nodes_t m_archetype_query_nodes;
		vector<query_node*> m_subquery_nodes;

		vector<callback_t> m_archetype_addition_callbacks;
		
		query_type m_type;

		void publish_archetype_addition(query_index arch)
		{
			for (auto& callback : m_archetype_addition_callbacks)
				callback(arch);
		}

	public:
		query_node(sequence_ref<const component_type_index> component_types,query_type type)
		:m_condition(component_types),m_type(type) {}
		query_node(const query_condition& condition,query_type type)
		:m_condition(condition),m_type(type) {}
		query_type type() const { return m_type; }
		const archetype_nodes_t& archetype_query_nodes() const { return m_archetype_query_nodes; }
		const vector<query_node*>& subquery_nodes() const { return m_subquery_nodes; }
		const query_condition& condition() const { return m_condition; }
		query_condition tag_condition() const
		{
			if(m_type == untaged) return query_condition();
			query_condition tag_condition;
			tag_condition.all.reserve(m_condition.all.size());
			tag_condition.any.reserve(m_condition.any.size());
			tag_condition.none.reserve(m_condition.none.size());			
			for (auto& comp : m_condition.all)
				if (comp.is_tag()) tag_condition.all.push_back(comp);
			for (auto& comp : m_condition.any)
				if (comp.is_tag()) tag_condition.any.push_back(comp);
			for (auto& comp : m_condition.none)
				if (comp.is_tag()) tag_condition.none.push_back(comp);
			return tag_condition;
		}
		query_condition base_condition() const
		{
			if (m_type == untaged) return m_condition;
			query_condition base_condition;
			base_condition.all.reserve(m_condition.all.size());
			base_condition.any.reserve(m_condition.any.size());
			base_condition.none.reserve(m_condition.none.size());
			for (auto& comp : m_condition.all)
				if (!comp.is_tag()) base_condition.all.push_back(comp);
			for (auto& comp : m_condition.any)
				if (!comp.is_tag()) base_condition.any.push_back(comp);
			for (auto& comp : m_condition.none)
				if (!comp.is_tag()) base_condition.none.push_back(comp);
			return base_condition;
		}

		void add_callback(callback_t&& callback)
		{
			for (auto& [_,node] : m_archetype_query_nodes)
				callback(node->query_index());
			m_archetype_addition_callbacks.emplace_back(std::forward<callback_t>(callback));
		}

		void add_subquery(query_node* subquery) { m_subquery_nodes.push_back(subquery); }
		void add_matched(const archetype_query_node* node)
		{
			assert(
				m_type == untaged && node->type() == archetype_query_node::full_set ||
				m_type == mixed || 
				m_type == pure_tag && node->type() == archetype_query_node::partial_set
			);
			m_archetype_query_nodes.insert({ node->archetype_node(),node});
			if(m_type != pure_tag)
				node->archetype_node()->add_related_query(this);//bidirectional
			publish_archetype_addition(node->query_index());
		}


	public:
		uint64_t iterate_version = std::numeric_limits<uint64_t>::max();
	};


	//class pure_tag_archetype_query_node
	//{
	//	query_condition m_condition;
	//	vector<taged_archetype_node*> m_taged_archetype_nodes;
	//};

	//class pure_tag_query_node
	//{
	//	query_condition m_condition;
	//	map<archetype_node*, pure_tag_archetype_query_node*> m_archetype_query_nodes;
	//public:
	//	pure_tag_query_node(const query_condition& condition) :m_condition(condition) {}
	//	const query_condition& condition() const { return m_condition; }
	//	const map<archetype_node*, pure_tag_archetype_query_node*>& archetype_query_nodes() const { return m_archetype_query_nodes; }
	//
	//
	//
	//};
		

	


}