#pragma once
#include "entity.h"
#include "component_storage.h"
namespace hyecs
{
	class sparse_table
	{
		dense_set<entity> m_entities;
		vector<component_type_index> m_types;
		vector<component_storage*> m_component_storages;


	public:

		sparse_table(vector<component_storage*> component_storages)
			: m_component_storages(component_storages), m_types(component_storages.size())
		{
			for (auto storage : m_component_storages)
			{
				m_types.push_back(storage->component_type());
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


		class end_iterator {};
		//todo template SeqIter
		class raw_accessor
		{
		protected:
			sparse_table& m_table;
			sequence_ref<entity> m_entities;
		public:

			raw_accessor(sparse_table& in_table)
				: m_table(in_table)
			{
			}

			raw_accessor(sparse_table& in_table, sequence_ref<entity> entities)
				: m_table(in_table), m_entities(entities)
			{
			}


			class component_array_accessor
			{
				raw_accessor* m_accessor;
				component_type_index m_type;
				uint32_t m_component_index;

			public:

				component_array_accessor(uint32_t index, raw_accessor* accessor)
					: m_type(accessor->m_table.m_types[index]), m_component_index(index), m_accessor(accessor)
				{
				}

				component_type_index component_type() const { return m_type; }

				void operator++()
				{
					m_component_index++;
					m_type = m_accessor->m_table.m_types[m_component_index];
				}

				component_array_accessor& operator++(int)
				{
					component_array_accessor copy = *this;
					++(*this);
					return copy;
				}

				auto comparable() const
				{
					return m_type;
				}

				bool operator==(const component_array_accessor& other) const
				{
					return m_type == other.m_type;
				}

				bool operator!=(const component_array_accessor& other) const
				{
					return !(*this == other);
				}

				bool operator==(const end_iterator& other) const
				{
					return m_component_index == m_accessor->m_table.m_types.size();
				}

				bool operator!=(const end_iterator& other) const
				{
					return !(*this == other);
				}


				class iterator
				{
					component_storage* m_storage;
					raw_accessor* m_accessor;
					typename sequence_ref<entity>::iterator m_entity_iter;

					sparse_table& sparse_table() const { return m_accessor->m_table; }
					const sequence_ref<entity>& sequence() const { return m_accessor->m_entities; }
				public:
					iterator(uint32_t component_index,
						raw_accessor* accessor,
						typename sequence_ref<entity>::iterator entity_iter)
						:
						m_storage(accessor->m_table.m_component_storages[component_index]),
						m_accessor(accessor),
						m_entity_iter(entity_iter)
					{
					}

					void operator++()
					{
						m_entity_iter++;
					}

					iterator& operator++(int)
					{
						iterator copy = *this;
						++(*this);
						return copy;
					}

					bool operator==(const iterator& other) const
					{
						return m_entity_iter == other.m_entity_iter;
					}

					bool operator!=(const iterator& other) const
					{
						return !(*this == other);
					}

					bool operator==(const end_iterator& other) const
					{
						return m_entity_iter == m_accessor->m_entities.end();
					}

					bool operator!=(const end_iterator& other) const
					{
						return !(*this == other);
					}

					void* operator*()
					{
						return m_storage->at(*m_entity_iter);
					}
				};

				iterator begin() const
				{
					return iterator(m_component_index, m_accessor, m_accessor->m_entities.begin());
				}

				end_iterator end() const
				{
					return end_iterator{};
				}
			};

			component_array_accessor begin()
			{
				return component_array_accessor(0u, this);
			}

			end_iterator end()
			{
				return end_iterator{};
			}

		};

		class allocate_accessor
		{
			sparse_table& m_table;
			sequence_ref<entity> m_entities;
			std::function<void(entity,storage_key)> m_storage_key_builder;
		public:
			allocate_accessor(sparse_table& in_table, sequence_ref<entity> entities,
				std::function<void(entity, storage_key)>&& builder)
				: m_table(in_table), m_entities(entities), m_storage_key_builder(std::move(builder))
			{
			}

			class component_array_accessor
			{
				allocate_accessor* m_accessor;
				vector<void*> m_components;
				component_type_index m_type;
				uint32_t m_component_index;


				void allocate_for_index(uint32_t index)
				{
					auto storage = m_accessor->m_table.m_component_storages[index];
					auto entity_iter = m_accessor->m_entities.begin();
					for (int i = 0; i < m_components.size(); i++, entity_iter++)
					{
						m_components[i] = storage->allocate_component(*entity_iter);
					}
				}

			public:

				component_array_accessor(allocate_accessor* accessor, uint32_t index)
					: m_accessor(accessor),
					m_type(accessor->m_table.m_types[index]),
					m_component_index(index),
					m_components(accessor->m_entities.size())
				{
					allocate_for_index(index);
				}

				component_type_index component_type() const { return m_type; }

				void operator++()
				{
					m_component_index++;
					m_type = m_accessor->m_table.m_types[m_component_index];
					if (operator!=(end_iterator{}))
						allocate_for_index(m_component_index);
				}

				component_array_accessor& operator++(int)
				{
					component_array_accessor copy = *this;
					++(*this);
					return copy;
				}

				auto comparable() const
				{
					return m_type;
				}

				bool operator==(const component_array_accessor& other) const
				{
					return m_type == other.m_type;
				}

				bool operator!=(const component_array_accessor& other) const
				{
					return !(*this == other);
				}

				bool operator==(const end_iterator& other) const
				{
					return m_component_index == m_accessor->m_table.m_types.size();
				}

				bool operator!=(const end_iterator& other) const
				{
					return !(*this == other);
				}


				using iterator = typename vector<void*>::iterator;

				iterator begin()
				{
					return m_components.begin();
				}

				iterator end()
				{
					return m_components.end();
				}
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

		raw_accessor get_raw_accessor(sequence_ref<entity> entities)
		{
			return raw_accessor(*this, entities);
		}

		template<typename StorageKeyBuilder>
		allocate_accessor get_allocate_accessor(sequence_ref<entity> entities, StorageKeyBuilder&& builder)
		{
			return allocate_accessor(*this, entities, std::move(builder));
		}



	};
}