#pragma once
#include "entity.h"
#include "component_storage.h"
namespace hyecs
{
	class sparse_table
	{
		dense_set<entity> m_entities;
		vector<component_storage*> m_component_storages;


	public:

		sparse_table(vector<component_storage*> component_storages)
			: m_component_storages(component_storages)
		{
		}

	private:
		void allocate_entity()
		{
			m_entities.insert
		}
	public:


		class end_iterator {};

		using SeqIter = entity*;
		class raw_accessor
		{
		protected:
			sparse_table& m_table;
			sequence_ref<entity, SeqIter> table_offsets;
		public:

			raw_accessor(sparse_table& in_table)
				: m_table(in_table)
			{
			}

			raw_accessor(sparse_table& in_table, sequence_ref<entity, SeqIter> offsets)
				: m_table(in_table), table_offsets(offsets)
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
					uint32_t m_comp_offset;
					uint32_t m_type_size;
					raw_accessor* m_accessor;
					typename sequence_ref<uint32_t, SeqIter>::iterator m_offset_iter;

					sparse_table& sparse_table() const { return m_accessor->m_table; }
					const sequence_ref<uint32_t>& sequence() const { return m_accessor->table_offsets; }
				public:
					iterator(uint32_t component_index,
						raw_accessor* accessor,
						typename sequence_ref<uint32_t, SeqIter>::iterator offset_iter)
						:
						m_comp_offset(accessor->m_table.m_offsets[component_index]),
						m_type_size(accessor->m_table.m_types[component_index].size()),
						m_accessor(accessor),
						m_offset_iter(offset_iter)
					{
					}

					void operator++()
					{
						m_offset_iter++;
					}

					iterator& operator++(int)
					{
						iterator copy = *this;
						++(*this);
						return copy;
					}

					bool operator==(const iterator& other) const
					{
						return m_offset_iter == other.m_offset_iter;
					}

					bool operator!=(const iterator& other) const
					{
						return !(*this == other);
					}

					bool operator==(const end_iterator& other) const
					{
						return m_offset_iter == m_accessor->table_offsets.end();
					}

					bool operator!=(const end_iterator& other) const
					{
						return !(*this == other);
					}

					void* operator*()
					{
						return sparse_table().component_address(
							sparse_table().chunk_index_offset(*m_offset_iter),
							m_comp_offset, m_type_size);
					}
				};

				iterator begin() const
				{
					return iterator(m_component_index, m_accessor, m_accessor->table_offsets.begin());
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

		class allocate_accessor : public raw_accessor
		{
			vector<entity> table_offsets;
		public:
			allocate_accessor(sparse_table& in_table, int count) : raw_accessor(in_table)
			{
				table_offsets.reserve(count);
				for (int i = 0; i < count; i++)
					table_offsets.push_back(m_table.allocate_entity());
			}
		};

		raw_accessor get_raw_accessor(sequence_ref<entity> entities)
		{
			return raw_accessor(*this, entities);
		}

		allocate_accessor get_allocate_accessor(sequence_ref<entity> entities)
		{
			return allocate_accessor(*this, count);
		}



	};
}