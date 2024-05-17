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
		archetype_node(archetype_index arch) : m_archetype(arch)
		{
		}
		archetype_node(archetype_node&) = delete;
		archetype_node(archetype_node&&) = default;

		archetype_index archetype() const { return m_archetype; }
		archetype_query_node* direct_query_node() const { return m_direct_query_node; }
		const vector<query_node*>& related_queries() const { return m_related_queries; }


		void add_related_query(query_node* query) { m_related_queries.push_back(query); }
	};

	class tag_archetype_node
	{
		archetype_index m_archetype;
		archetype_node* m_base_archetype;
		vector<query_node*> m_related_queries;

	public:
		tag_archetype_node(archetype_index arch, archetype_node* base_archetype_node)
			: m_archetype(arch), m_base_archetype(base_archetype_node)
		{
		}
		tag_archetype_node(tag_archetype_node&) = delete;
		tag_archetype_node(tag_archetype_node&&) = default;

		archetype_index archetype() const { return m_archetype; }
		archetype_node* base_archetype_node() const { return m_base_archetype; }
		const vector<query_node*>& related_queries() const { return m_related_queries; }

		void add_related_query(query_node* query) { m_related_queries.push_back(query); }
	};

	using archetype_node_variant = std::variant<archetype_node*, tag_archetype_node*>;


	using archetype_query_index = uint64_t;

	archetype_query_index archetype_query_hash(archetype_index arch, const query_condition& tag_condition)
	{
		return arch.hash() + tag_condition.hash();
	}

	class archetype_query_node
	{
	public:
		using notify_adding_tag_t = std::function<void(tag_archetype_node*)>;
		using notify_adding_tag_archetype_t = std::function<void(archetype_index)>;
		using notify_partial_convert_t = std::function<void()>;
		using tag_archetype_nodes_t = vector<tag_archetype_node*>;

		enum set_type
		{
			full_set,
			partial_set,
		};

	private:
		archetype_node* m_base_archetype_node;
		query_condition m_tag_condition;
		tag_archetype_nodes_t m_tag_archetype_nodes;
		set_type m_type;
		bool m_is_isolated;

		vector<notify_adding_tag_t> m_notify_adding_tag_callbacks;
		vector<notify_adding_tag_archetype_t> m_archetype_addition_callbacks;
		vector<notify_partial_convert_t> m_notify_partial_convert_callbacks;

	public:
		archetype_query_node(archetype_node* base_archetype_node) :
			m_base_archetype_node(base_archetype_node),
			m_tag_condition(),
			m_type(full_set),
			m_is_isolated(true)
		{
			m_base_archetype_node->m_direct_query_node = this;
		}

		archetype_query_node(archetype_query_node&) = delete;

		archetype_query_node(archetype_query_node&& other) noexcept :
			m_base_archetype_node(other.m_base_archetype_node),
			m_tag_condition(std::move(other.m_tag_condition)),
			m_tag_archetype_nodes(std::move(other.m_tag_archetype_nodes)),
			m_type(other.m_type),
			m_is_isolated(other.m_is_isolated),
			m_notify_adding_tag_callbacks(std::move(other.m_notify_adding_tag_callbacks)),
			m_archetype_addition_callbacks(std::move(other.m_archetype_addition_callbacks)),
			m_notify_partial_convert_callbacks(std::move(other.m_notify_partial_convert_callbacks))
		{
			if (m_base_archetype_node->m_direct_query_node == &other)
				m_base_archetype_node->m_direct_query_node = this;
		}

		archetype_query_node(archetype_node* base_archetype_node, const query_condition& tag_condition) :
			m_base_archetype_node(base_archetype_node),
			m_tag_condition(tag_condition),
			m_type(partial_set),
			m_is_isolated(true)
		{
		}

		void convert_to_connected(const archetype_query_node* superset)
		{
			m_is_isolated = false;
			auto mut_superset = const_cast<archetype_query_node*>(superset);
			mut_superset->m_notify_adding_tag_callbacks.emplace_back(
				[this](tag_archetype_node* node) { try_match(node); });
		}

		archetype_query_node(
			const archetype_query_node* superset,
			const query_condition& tag_condition)
			: m_base_archetype_node(superset->archetype_node()),
			m_tag_condition(tag_condition),
			m_type(full_set),
			m_is_isolated(false)
		{
			assert(!tag_condition.empty());
			m_tag_archetype_nodes.reserve(superset->m_tag_archetype_nodes.size());
			if (!tag_condition.match(m_base_archetype_node->archetype()))
				m_type = partial_set;

			for (auto& node : superset->m_tag_archetype_nodes)
			{
				if (tag_condition.match(node->archetype()))
					m_tag_archetype_nodes.push_back(node);
				else
					m_type = partial_set;
			}
			auto mut_superset = const_cast<archetype_query_node*>(superset);
			mut_superset->m_notify_adding_tag_callbacks.emplace_back(
				[this](tag_archetype_node* node)
				{
					if (!try_match(node) && m_type == full_set)
					{
						m_type = partial_set;
						for (auto& callback : m_notify_partial_convert_callbacks)
							callback();
					}
				});
		}

		query_index query_index() const { return archetype_query_hash(m_base_archetype_node->archetype(), m_tag_condition); }
		const query_condition& tag_condition() const { return m_tag_condition; }
		archetype_node* archetype_node() const { return m_base_archetype_node; }
		const tag_archetype_nodes_t& tag_archetype_nodes() const { return m_tag_archetype_nodes; }
		set_type type() const { return m_type; }
		bool is_full_set() const { return m_type == full_set; }
		bool is_partial_set() const { return m_type == partial_set; }
		bool is_direct_set() const { return m_base_archetype_node->direct_query_node() == this; }
		bool isolated() const { return m_is_isolated; }

		void add_matched(tag_archetype_node* node)
		{
			m_tag_archetype_nodes.push_back(node);
			for (auto& callback : m_notify_adding_tag_callbacks)
				callback(node);
			for (auto& callback : m_archetype_addition_callbacks)
				callback(node->archetype());
		}

		void add_notify_adding_tag_callback(notify_adding_tag_t&& callback)
		{
			for (tag_archetype_node* node : m_tag_archetype_nodes)
				callback(node);
			m_notify_adding_tag_callbacks.emplace_back(std::move(callback));
		}

		void add_notify_adding_tag_archetype_callback(notify_adding_tag_archetype_t&& callback)
		{
			for (tag_archetype_node* node : m_tag_archetype_nodes)
				callback(node->archetype());
			m_archetype_addition_callbacks.emplace_back(std::move(callback));
		}

		void add_notify_partial_convert_callback(notify_partial_convert_t&& callback)
		{
			m_notify_partial_convert_callbacks.emplace_back(std::move(callback));
		}

		bool try_match(tag_archetype_node* node)
		{
			if (m_tag_condition.match(node->archetype()))
			{
				add_matched(node);
				return true;
			}
			return false;
		}

		archetype_query_index hash() const { return archetype_query_hash(m_base_archetype_node->archetype(), m_tag_condition); }
	};


	class query_node
	{
	public:
		using callback_t = std::function<void(query_index)>;
		using archetype_nodes_t = map<archetype_node*, const archetype_query_node*>;

		enum query_type
		{
			untag,
			mixed,
			pure_tag,
		};

	private:
		query_condition m_condition;
		archetype_nodes_t m_archetype_query_nodes;
		vector<query_node*> m_subquery_nodes;

		vector<callback_t> m_archetype_query_addition_callbacks;

		query_type m_type;

		void publish_archetype_addition(query_index arch_query_index)
		{
			for (auto& callback : m_archetype_query_addition_callbacks)
				callback(arch_query_index);
		}

	public:
		query_node(sequence_cref<component_type_index> component_types, query_type type)
			: m_condition(component_types), m_type(type)
		{
		}

		query_node(const query_condition& condition, query_type type)
			: m_condition(condition), m_type(type)
		{
		}
		query_node(query_node&) = delete;
		query_node(query_node&&) = default;

		query_type type() const { return m_type; }
		const archetype_nodes_t& archetype_query_nodes() const { return m_archetype_query_nodes; }
		const vector<query_node*>& subquery_nodes() const { return m_subquery_nodes; }
		const query_condition& condition() const { return m_condition; }

		query_condition tag_condition() const
		{
			if (m_type == untag) return query_condition();
			return condition().tag_condition();
		}

		query_condition base_condition() const
		{
			if (m_type == untag) return m_condition;
			return condition().base_condition();
		}

		void add_callback(callback_t&& callback)
		{
			for (auto& [_, node] : m_archetype_query_nodes)
				callback(node->query_index());
			m_archetype_query_addition_callbacks.emplace_back(std::move(callback));
		}

		void add_subquery(query_node* subquery) { m_subquery_nodes.push_back(subquery); }

		void add_matched(const archetype_query_node* node)
		{
			assert(
				m_type == untag && node->type() == archetype_query_node::full_set ||
				m_type == mixed ||
				m_type == pure_tag && node->type() == archetype_query_node::partial_set
			);
			m_archetype_query_nodes.insert({ node->archetype_node(), node });
			if (m_type != pure_tag)
				node->archetype_node()->add_related_query(this); //bidirectional
			publish_archetype_addition(node->query_index());
		}

	public:
		uint64_t iterate_version = std::numeric_limits<uint64_t>::max();
	};


	//class pure_tag_archetype_query_node
	//{
	//	query_condition m_condition;
	//	vector<tag_archetype_node*> m_tag_archetype_nodes;
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
