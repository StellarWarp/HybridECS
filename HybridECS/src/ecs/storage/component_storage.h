#pragma once
#include "ecs/type/entity.h"
#include "ecs/type/component.h"
#include "entity_map.h"

namespace hyecs
{
	class component_storage : non_movable
	{
		component_type_index m_component_type;
		raw_entity_dense_map m_storage;


		//todo notify addition/removal of components
	public:
		component_storage(component_type_index index) :
			m_component_type(index),
			m_storage(index.size(),index.alignment())
		{
			assert(!index.is_empty());
		}
        ~component_storage()
        {
            if(!m_component_type.is_trivially_destructible())
            {
                for (auto [e, ptr] : m_storage)
                {
                    m_component_type.destructor_ptr()(ptr);
                }
            }
        }

		component_type_index component_type() const
		{
			return m_component_type;
		}

		void* at(entity e)
		{
			assert(m_storage.contains(e));
			return m_storage.at(e);
		}

		template <typename T>
		T& at(entity e)
		{
			return *(T*)at(e);
		}

//		void erase(entity e)
//		{
//            if(!m_component_type.is_trivially_destructible())
//
//            m_storage.erase(e);
//
//		}
//
//		void erase(sequence_cref<entity> entities)
//		{
//			for (auto e : entities)
//			{
//				m_storage.erase(e);
//			}
//		}

		//template<typename T, typename... Args>
		//void emplace(entity e, Args&&... args)
		//{
		//	m_storage.emplace(e, std::forward<Args>(args)...);
		//}

        template<std::invocable<void*> Constructor>
		void emplace(sequence_cref<entity> entities, Constructor&& constructor)
		{
			for (auto e : entities)
			{
				auto ptr = m_storage.allocate_value(e);
				constructor(ptr);
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

        void deallocate_components(sequence_cref<entity> entities)
        {
            for (auto e : entities)
            {
                deallocate_component(e);
            }
        }

		void erase_component(entity e)
		{
			m_storage.deallocate_value(
                    e,
                    [this](void* dest,void* src){ m_component_type.move_constructor(dest,src); },
                    [this](void* o){ m_component_type.destructor(o); });
		}

        void erase_components(sequence_cref<entity> entities)
        {
            for (auto e : entities)
            {
                erase_component(e);
            }
        }

        //todo untested
        void move_out_component(void* dest, entity e)
        {
            m_storage.deallocate_value(
                    e,
                    [this](void* dest_,void* src_){ m_component_type.move_constructor(dest_,src_); },
                    [&](void* src){ m_component_type.move_constructor(dest,src); },
                    [this](void* o){ m_component_type.destructor(o); });

        }

		template <typename SeqParam = const entity*>
		void allocate_components(sequence_cref<entity, SeqParam> entities, sequence_ref<void*> addrs)
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

		template <typename T>
		class accessor
		{
			raw_entity_dense_map* m_storage;

		public:
			accessor(raw_entity_dense_map& storage) : m_storage(&storage)
			{
			}

			static constexpr bool is_raw_accessor = std::is_same_v<std::decay_t<T>, void*>;
			static_assert(!is_raw_accessor || !std::is_reference_v<T>, "Raw accessors cannot be references");

			class iterator
			{
				raw_entity_dense_map::iterator m_iter;

			public:
				iterator(raw_entity_dense_map::iterator iter) : m_iter(iter)
				{
				}

				auto operator*() -> std::conditional_t<is_raw_accessor, T, T&>
				{
					auto [e, ptr] = *m_iter;
					if constexpr (is_raw_accessor)
						return ptr;
					else
						return *(T*)ptr;
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

		template <typename T>
		accessor<T> get_accessor()
		{
			assert(m_component_type == generic::type_info::of<T>());
			return accessor<T>(m_storage);
		}

		using end_iterator = nullptr_t;

		class component_allocate_accessor
		{
			raw_entity_dense_map* m_storage;
			sequence_cref<entity> m_entities;
			vector<void*> m_components;

		public:
			component_allocate_accessor(raw_entity_dense_map& storage, sequence_cref<entity> entities) :
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

		component_allocate_accessor allocate(sequence_cref<entity> entities)
		{
			return component_allocate_accessor(m_storage, entities);
		}
	};
}
