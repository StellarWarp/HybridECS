#pragma once
#include "entity.h"
#include "component.h"
#include "../container/container.h"

namespace hyecs
{
	class component_storage
	{
		component_type_index m_component_type;
		generic_dense_map m_storage;
	public:

		component_storage(component_type_index index) :
			m_component_type(index),
			m_storage(index.type_index()) {}

		component_type_index component_type() const
		{
			return m_component_type;
		}

		void* at(entity e)
		{
			assert(m_storage.contains(e));
			return m_storage.at(e);
		}

		template<typename T>
		T& at(entity e)
		{
			return *(T*)at(e);
		}

		void erase(entity e)
		{
			m_storage.erase(e);
		}

		void erase(sequence_ref<const entity> entities)
		{
			for (auto e : entities)
			{
				m_storage.erase(e);
			}
		}

		template<typename T, typename... Args>
		void emplace(entity e, Args&&... args)
		{
			m_storage.emplace(e, std::forward<Args>(args)...);
		}

		void emplace(sequence_ref<const entity> entities, generic::constructor constructor)
		{
			for (auto e : entities)
			{
				auto ptr = m_storage.emplace(e, constructor);
			}
		}

		void* allocate_component(entity e)
		{
			return m_storage.allocate_value(e);
		}

		void deallocate_component(entity e)
		{
			m_storage.deallocate_value(e);
		}

		template<typename SeqParam = const entity*>
		void allocate_components(sequence_ref<const entity, SeqParam> entities, sequence_ref<void*> addrs)
		{
			auto e_iter = entities.begin();
			auto a_iter = addrs.begin();
			while (e_iter != entities.end())
			{
				*a_iter = m_storage.allocate_value(*e_iter);
				++e_iter;
				++a_iter;
			}
		}







		//void move_into(entity e, void* data)
		//{
		//	auto ptr = m_storage.at(&e);
		//	m_component_type.move_constructor(ptr, data);
		//}

		//void copy_into(entity e, const void* data)
		//{
		//	auto ptr = m_storage.at(&e);
		//	m_component_type.copy_constructor(ptr, data);
		//}

		



		template<typename T>
		class accessor
		{
			const generic_dense_map * m_storage;

		public:
			accessor(const generic_dense_map& storage) : m_storage(&storage) {
				assert(storage.value_type().hash() == type_hash::of<T>);
			}

			class iterator
			{
				generic_dense_map::iterator m_iter;

			public:
				iterator(generic_dense_map::iterator iter) : m_iter(iter) {}

				T& operator*()
				{
					return *(T*)(void*)m_iter;
				}

				iterator& operator++()
				{
					++m_iter;
					return *this;
				}

				bool operator==(const iterator& other) const
				{
					return m_iter == other.m_iter;
				}

				bool operator!=(const iterator& other) const
				{
					return m_iter != other.m_iter;
				}

			};

			iterator begin()
			{
				return m_storage->begin();
			}
			iterator end()
			{
				return m_storage->end();
			}
		};

		template<typename T>
		accessor<T> get_accessor()
		{
			return accessor<T>(m_storage);
		}

		class component_allocate_accessor
		{
			generic_dense_map* m_storage;
			sequence_ref<const entity> m_entities;
			vector<void*> m_components;

		public:
			component_allocate_accessor(generic_dense_map& storage, sequence_ref<const entity> entities) :
				m_storage(&storage),
				m_entities(entities),
				m_components(entities.size())
			{
				auto e_iter = entities.begin();
				auto a_iter = m_components.begin();
				while (e_iter != entities.end())
				{
					*a_iter = m_storage->allocate_value(*e_iter);
					++e_iter;
					++a_iter;
				}
			}

			using iterator = typename vector<void*>::iterator;
			iterator begin() { return m_components.begin(); }
			iterator end() { return m_components.end(); }
		};

		component_allocate_accessor allocate(sequence_ref<const entity> entities)
		{
			return component_allocate_accessor(m_storage, entities);
		}





	};
}