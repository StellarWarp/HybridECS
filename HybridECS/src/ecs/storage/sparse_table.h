#pragma once
#include "ecs/type/entity.h"
#include "component_storage.h"
#include "storage_key_registry.h"

namespace hyecs
{
	class sparse_table : non_copyable
	{
		dense_set<entity> m_entities;
		vector<component_type_index> m_notnull_components;
		vector<component_storage*> m_component_storages;

		//event
		vector<std::function<void(entity, storage_key)>> m_on_entity_add;
		vector<std::function<void(entity, storage_key)>> m_on_entity_remove;

	public:
		sparse_table(sorted_sequence_cref<component_storage*> component_storages)
			: m_component_storages(component_storages.begin(), component_storages.end())
		{
			m_notnull_components.reserve(m_component_storages.size());
			for (uint64_t i = 0; i < m_component_storages.size(); ++i)
			{
				m_notnull_components.push_back(m_component_storages[i]->component_type());
			}
		}
		sparse_table(const sparse_table&) = delete;
		sparse_table& operator=(const sparse_table&) = delete;
		sparse_table(sparse_table&&) = default;
		sparse_table& operator=(sparse_table&&) = default;
        ~sparse_table()
        {
            for(auto storage : m_component_storages)
            {
                for(auto e: m_entities)
                {
                    storage->erase_component(e);
                }
            }
        }

        void deallocate_all()
        {
            for(auto storage : m_component_storages)
            {
                for(auto e: m_entities)
                {
                    storage->deallocate_component(e);
                }
            }
            m_entities.clear();
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
		size_t entity_count() const { return m_entities.size(); }
		const dense_set<entity>& get_entities() { return m_entities; }

		void add_callback_on_entity_add(std::function<void(entity, storage_key)> callback)
		{
			for (auto e : m_entities)
			{
				callback(e, {});
			}
			m_on_entity_add.push_back(callback);
		}

		void add_callback_on_entity_remove(std::function<void(entity, storage_key)> callback)
		{
			m_on_entity_remove.push_back(callback);
		}


		using end_iterator = nullptr_t;

		class raw_accessor
		{
		protected:
			using entity_seq = sequence_cref<entity>;
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

			raw_accessor(const raw_accessor&) = delete;

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
					if (!operator==(end_iterator{}))
						m_type = m_accessor->m_table.m_notnull_components[m_component_index];
					else
						m_type = component_type_index{};
				}

				component_array_accessor operator++(int)
				{
					component_array_accessor copy = *this;
					++(*this);
					return copy;
				}

				component_array_accessor& operator*() { return *this; }

				auto& comparable() const { return m_type; }
				bool operator==(const component_array_accessor& other) const { return m_type == other.m_type; }
				bool operator==(const end_iterator& other) const { return m_component_index == m_accessor->m_table.m_notnull_components.size(); }

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

					iterator operator++(int)
					{
						iterator copy = *this;
						++(*this);
						return copy;
					}

					bool operator==(const iterator& other) const { return m_entity_iter == other.m_entity_iter; }
					bool operator==(const end_iterator& other) const { return m_entity_iter == m_accessor->m_entities.end(); }
					void* operator*() { return m_storage->at(*m_entity_iter); }
				};

				iterator begin() const { return iterator(m_component_index, m_accessor, m_accessor->m_entities.begin()); }
				end_iterator end() const { return end_iterator{}; }
			};

			component_array_accessor begin() { return component_array_accessor(0u, this); }
			end_iterator end() { return end_iterator{}; }
		};

		template <typename SeqParam = const entity*>
		class allocate_accessor : non_copyable
		{
			using entity_seq = sequence_cref<entity, SeqParam>;
			sparse_table& m_table;
			entity_seq m_entities;
			ASSERTION_CODE(bool m_is_construct_finished = false);

		public:
			template <typename Callable>
			allocate_accessor(sparse_table& in_table, entity_seq entities, Callable&& builder)
				: m_table(in_table),
				  m_entities(entities)
			{
				for (auto e : m_entities)
				{
					m_table.m_entities.insert(e);
					//m_storage_key_builder(e, {});
				}
			}
			allocate_accessor(allocate_accessor&& other) noexcept
				: m_table(other.m_table),
				  m_entities(std::move(other.m_entities))
			{
				ASSERTION_CODE(other.m_is_construct_finished = true);
			}
			allocate_accessor& operator=(allocate_accessor&&) = delete;

			void notify_construct_finish()
			{
				for (auto& callback : m_table.m_on_entity_add)
				{
					for (auto e : m_entities)
					{
						callback(e, {});
					}
				}
				ASSERTION_CODE(m_is_construct_finished = true);
			}

			void construct_finish_external_notified()
			{
				ASSERTION_CODE(m_is_construct_finished = true);
			}

			~allocate_accessor() { assert(m_is_construct_finished); }


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
					  m_components(accessor->m_entities.size()),
					  m_component_index(index)
				{
					if (accessor->m_table.m_notnull_components.empty())
					{
						m_type = component_type_index{};
						return;
					}
					m_type = accessor->m_table.m_notnull_components[index];
					allocate_for_index(index);
				}

				component_type_index component_type() const { return m_type; }

				void operator++()
				{
					m_component_index++;
					if (!operator==(end_iterator{}))
					{
						m_type = m_accessor->m_table.m_notnull_components[m_component_index];
						allocate_for_index(m_component_index);
					}
				}
                void operator++(int)
                {
                    ++(*this);
                }

				component_array_accessor& operator*() { return *this; }



				auto& comparable() const { return m_type; }
				bool operator==(const component_array_accessor& other) const { return m_type == other.m_type; }
				bool operator==(const end_iterator& other) const { return m_component_index == m_accessor->m_table.m_notnull_components.size(); }

				class iterator
				{
					sequence_ref<void*>::iterator m_current;
					sequence_ref<void*>::iterator m_end;

				public:
					iterator(sequence_ref<void*> components)
						: m_current(components.begin()), m_end(components.end())
					{
					}

					iterator& operator++()
					{
						m_current++;
						return *this;
					}

					iterator operator++(int)
					{
						iterator copy = *this;
						++(*this);
						return copy;
					}

					void* operator*() const { return *m_current; }
					bool operator==(const iterator& other) const { return m_current == other.m_current; }
					bool operator==(const end_iterator& other) const { return m_current == m_end; }
				};

				iterator begin() { return iterator(m_components); }
				end_iterator end() { return {}; }
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

		raw_accessor get_raw_accessor(sequence_cref<entity> entities)
		{
			return raw_accessor(*this, entities);
		}

		raw_accessor get_raw_accessor()
		{
			auto entity_seq = sequence_cref(m_entities.begin(), m_entities.end());
			return raw_accessor(*this, entity_seq);
		}


		template <typename SeqParam, typename StorageKeyBuilder>
		auto get_allocate_accessor(sequence_cref<entity, SeqParam> entities, StorageKeyBuilder&& builder)
		{
			return allocate_accessor<SeqParam>(*this, entities, std::forward<StorageKeyBuilder>(builder));
		}

		class deallocate_accessor : public raw_accessor, non_copyable
		{
			ASSERTION_CODE(bool m_is_destruct_finished = false);

		public:
			using raw_accessor::raw_accessor;

			deallocate_accessor(deallocate_accessor&& other) : raw_accessor(std::move(other))
			{
				ASSERTION_CODE(other.m_is_destruct_finished = true);
			}
			deallocate_accessor& operator=(deallocate_accessor&&) = delete;

        private:
            template<bool Destruct>
            void notify_finish_()
            {
                assert(!m_is_destruct_finished);

                for (auto entity : m_entities)
                {
                    m_table.m_entities.erase(entity);
                    for (auto storage : m_table.m_component_storages)
                        if constexpr (Destruct)
                            storage->erase_component(entity);
                        else
                            storage->deallocate_component(entity);
                }
                for (auto& callback : m_table.m_on_entity_remove)
                {
                    for (const auto& e : m_entities)
                    {
                        callback(e, {});
                    }
                }
                ASSERTION_CODE(m_is_destruct_finished = true);
            }
        public:

			void destruct()
			{
                notify_finish_<true>();
			}

			void notify_destruct_finish()
			{
                notify_finish_<false>();
			}

			~deallocate_accessor() { assert(m_is_destruct_finished); }
		};

		deallocate_accessor get_deallocate_accessor(sequence_cref<entity> entities)
		{
			return deallocate_accessor(*this, entities);
		}

		void dynamic_for_each(sequence_cref<uint32_t> component_indices, std::function<void(entity, sequence_ref<void*>)> func)
		{
			vector<void*> addrs(component_indices.size()); //todo this allocation can be optimized
			for (const auto& entity : m_entities)
			{
				for (size_t i = 0; i < component_indices.size(); i++)
				{
					addrs[i] = m_component_storages[component_indices[i]]->at(entity);
				}
				func(entity, addrs);
			}
		}

		template <typename Callable>
		void for_each(Callable&& func, sequence_cref<uint32_t> component_indices)
		{
			auto invoker = system_callable_invoker(std::forward<Callable>(func));

			for (auto& e : m_entities)
			{
				invoker.invoke(
					[&] { return e; },
					[&] { return storage_key{}; },
					[&](auto type, size_t index) { return m_component_storages[component_indices[index]]->at(e); }
				);
			}
		}
	};

	template <typename SeqParam>
	sequence_cref(sequence_ref<entity, SeqParam>) -> sequence_cref<entity, SeqParam>;
}
