#pragma once
#include "entity.h"
#include "component_storage.h"
#include "storage_key_registry.h"
namespace hyecs
{
	class sparse_table
	{
		dense_set<entity> m_entities;
		vector<component_type_index> m_notnull_components;
		vector<component_storage*> m_component_storages;


	public:

		sparse_table(sorted_sequence_ref<component_storage*> component_storages)
			: m_component_storages(component_storages.begin(), component_storages.end())
		{
			m_notnull_components.reserve(m_component_storages.size());
			for (uint64_t i = 0; i < m_component_storages.size(); ++i)
			{
				m_notnull_components.push_back(m_component_storages[i]->component_type());
			}
		}

	private:
		void allocate_entity(entity e, sequence_ref<void*> components)
		{
			m_entities.insert(e);
			for (auto [i, comp_iter] = std::make_tuple(0, components.begin()); comp_iter != components.end(); ++i, ++comp_iter)
			{
				*comp_iter = m_component_storages[i]->allocate_component(e);
			}
		}

	public:

		const dense_set<entity>& get_entities() { return m_entities; }


		using end_iterator = nullptr_t;
		//todo template SeqIter
		class raw_accessor
		{
		protected:
			using entity_seq = sequence_ref<const entity>;
			sparse_table& m_table;
			entity_seq m_entities;
		public:

			raw_accessor(sparse_table& in_table)
				: m_table(in_table)
			{
			}

			raw_accessor(sparse_table& in_table, entity_seq entities)
				: m_table(in_table), m_entities(entities)
			{
			}

			raw_accessor(const raw_accessor&) = default;
			raw_accessor& operator=(const raw_accessor&) = default;
			raw_accessor(raw_accessor&& other) : m_table(other.m_table), m_entities(std::move(other.m_entities))
			{
			}


			class component_array_accessor
			{
			protected:
				raw_accessor* m_accessor;
				component_type_index m_type;
				uint32_t m_component_index;

			public:

				component_array_accessor(uint32_t index, raw_accessor* accessor)
					: m_type(accessor->m_table.m_notnull_components[index]), m_component_index(index), m_accessor(accessor)
				{
				}

				component_type_index component_type() const { return m_type; }

				void operator++()
				{
					m_component_index++;
					m_type = m_accessor->m_table.m_notnull_components[m_component_index];
				}

				component_array_accessor& operator++(int)
				{
					component_array_accessor copy = *this;
					++(*this);
					return copy;
				}

				auto comparable() const { return m_type; }
				bool operator==(const component_array_accessor& other) const { return m_type == other.m_type; }
				bool operator!=(const component_array_accessor& other) const { return !(*this == other); }
				bool operator==(const end_iterator& other) const { return m_component_index == m_accessor->m_table.m_notnull_components.size(); }
				bool operator!=(const end_iterator& other) const { return !(*this == other); }


				class iterator
				{
				protected:
					component_storage* m_storage;
					raw_accessor* m_accessor;
					typename entity_seq::iterator m_entity_iter;

					sparse_table& sparse_table() const { return m_accessor->m_table; }
					const entity_seq& sequence() const { return m_accessor->m_entities; }
				public:
					iterator(uint32_t component_index,
						raw_accessor* accessor,
						typename entity_seq::iterator entity_iter)
						:
						m_storage(accessor->m_table.m_component_storages[component_index]),
						m_accessor(accessor),
						m_entity_iter(entity_iter)
					{
					}

					void operator++() { m_entity_iter++; }
					iterator& operator++(int) { iterator copy = *this; ++(*this); return copy; }
					bool operator==(const iterator& other) const { return m_entity_iter == other.m_entity_iter; }
					bool operator!=(const iterator& other) const { return !(*this == other); }
					bool operator==(const end_iterator& other) const { return m_entity_iter == m_accessor->m_entities.end(); }
					bool operator!=(const end_iterator& other) const { return !(*this == other); }
					void* operator*() { return m_storage->at(*m_entity_iter); }
				};

				iterator begin() const { return iterator(m_component_index, m_accessor, m_accessor->m_entities.begin()); }
				end_iterator end() const { return end_iterator{}; }
			};

			component_array_accessor begin() { return component_array_accessor(0u, this); }
			end_iterator end() { return end_iterator{}; }

		};

		template<typename SeqParam = const entity*>
		class allocate_accessor
		{
			using entity_seq = sequence_ref<const entity, SeqParam>;
			sparse_table& m_table;
			entity_seq m_entities;
			std::function<void(entity, storage_key)> m_storage_key_builder;
		public:
			allocate_accessor(sparse_table& in_table, entity_seq entities,
				std::function<void(entity, storage_key)>&& builder)
				: m_table(in_table),
				m_entities(entities),
				m_storage_key_builder(std::move(builder))
			{
			}
			allocate_accessor(const allocate_accessor&) = default;
			allocate_accessor& operator=(const allocate_accessor&) = default;
			allocate_accessor(allocate_accessor&& other)
				: m_table(other.m_table),
				m_entities(std::move(other.m_entities)),
				m_storage_key_builder(std::move(other.m_storage_key_builder))
			{
			}

			class component_array_accessor
			{
				const allocate_accessor* const m_accessor;
				vector<void*> m_components;
				component_type_index m_type;
				uint32_t m_component_index;


				void allocate_for_index(uint32_t index)
				{
					auto storage = m_accessor->m_table.m_component_storages[index];
					storage->allocate_components(m_accessor->m_entities, m_components);
				}

			public:

				component_array_accessor(allocate_accessor* accessor, uint32_t index)
					: m_accessor(accessor),
					m_type(accessor->m_table.m_notnull_components[index]),
					m_component_index(index),
					m_components(accessor->m_entities.size())
				{
					allocate_for_index(index);
				}

				component_type_index component_type() const { return m_type; }

				void operator++()
				{
					m_component_index++;
					if (operator!=(end_iterator{}))
					{
						m_type = m_accessor->m_table.m_notnull_components[m_component_index];
						allocate_for_index(m_component_index);
					}
				}
				component_array_accessor& operator++(int) { component_array_accessor copy = *this; ++(*this); return copy; }
				auto comparable() const { return m_type; }
				bool operator==(const component_array_accessor& other) const { return m_type == other.m_type; }
				bool operator!=(const component_array_accessor& other) const { return !(*this == other); }
				bool operator==(const end_iterator& other) const { return m_component_index == m_accessor->m_table.m_notnull_components.size(); }
				bool operator!=(const end_iterator& other) const { return !(*this == other); }
				using iterator = typename vector<void*>::iterator;
				iterator begin() { return m_components.begin(); }
				iterator end() { return m_components.end(); }
			};

			component_array_accessor begin()
			{
				return component_array_accessor(this, 0u);
			}

			end_iterator end()
			{
				return end_iterator{};
			}
		};

		raw_accessor get_raw_accessor(sequence_ref<const entity> entities)
		{
			return raw_accessor(*this, entities);
		}

		raw_accessor get_raw_accessor()
		{
			auto entity_seq = make_sequence_ref(m_entities.begin(), m_entities.end());
			return raw_accessor(*this, entity_seq);
		}


		template<typename SeqParam, typename StorageKeyBuilder>
		auto get_allocate_accessor(sequence_ref<const entity, SeqParam> entities, StorageKeyBuilder&& builder)
		{
			return allocate_accessor<SeqParam>(*this, entities, std::forward<StorageKeyBuilder>(builder));
		}

		class deallocate_accessor : public raw_accessor
		{
		public:
			using raw_accessor::raw_accessor;
			~deallocate_accessor()
			{
				for (auto entity : m_entities)
					for (auto storage : m_table.m_component_storages)
						storage->deallocate_component(entity);
			}

		};

		deallocate_accessor get_deallocate_accessor(sequence_ref<const entity> entities)
		{
			return deallocate_accessor(*this, entities);
		}

		void dynamic_for_each(sequence_ref<const uint32_t> component_indices, std::function<void(entity, sequence_ref<void*>)> func)
		{
			vector<void*> addrs(component_indices.size());//todo this allocation can be optimized
			for (const auto& entity : m_entities)
			{
				for (auto i = 0; i < component_indices.size(); i++)
				{
					addrs[i] = m_component_storages[component_indices[i]]->at(entity);
				}
				func(entity, addrs);
			}
		}

	};



}
