#pragma once
#include "archetype_storage.h"
#include "component_storage.h"

namespace hyecs
{
	class taged_archetype_storage
	{
		archetype_index m_index;
		dense_set<entity> m_entities;
		dense_set<storage_key> m_sort_keys;
		archetype_storage* m_untaged_storage;
		vector<component_storage*> m_taged_storages;

		//event
		std::function<void(entity, storage_key)> m_on_entity_add;
		std::function<void(entity, storage_key)> m_on_entity_remove;


	public:
		taged_archetype_storage(
			archetype_index index,
			archetype_storage* untaged_storage,
			vector<component_storage*> taged_storages
		) :
			m_index(index),
			m_untaged_storage(untaged_storage),
			m_taged_storages(taged_storages)
		{
		}


		void entity_change_archetype(
			sequence_ref<const entity> entities,
			taged_archetype_storage* dest_archetype,
			remove_component taged_remove_components,
			sorted_sequence_ref<const generic::constructor> untaged_adding_constructors,
			sorted_sequence_ref<const generic::constructor> taged_adding_constructors)
		{
			m_untaged_storage->entity_change_archetype(entities, dest_archetype->m_untaged_storage, untaged_adding_constructors);

			auto src_taged_storages_iter = m_taged_storages.begin();
			auto dest_taged_storages_iter = dest_archetype->m_taged_storages.begin();
			auto src_end = m_taged_storages.end();
			auto constructors_iter = taged_adding_constructors.begin();
			for (; src_taged_storages_iter != src_end; ++src_taged_storages_iter, ++dest_taged_storages_iter)
			{
				auto& src_taged_storages = **src_taged_storages_iter;
				auto& dest_taged_storages = **dest_taged_storages_iter;
				if (src_taged_storages.component_type() < dest_taged_storages.component_type())
				{
					//remove
					src_taged_storages.erase(entities);
				}
				else if (src_taged_storages.component_type() > dest_taged_storages.component_type())
				{
					//add
					dest_taged_storages.emplace(entities, *constructors_iter);
					constructors_iter++;
				}

			}
		}

		template<typename SeqParam>
		class allocate_accessor
		{
			//using SeqParam = const entity*;
			using end_iterator = nullptr_t;
			using taged_component_iterator = vector<component_storage*>::iterator;
			using entity_seq = sequence_ref<const entity, SeqParam>;
			taged_archetype_storage& m_archetype;
			entity_seq m_entities;
		public:
			allocate_accessor(taged_archetype_storage& archetype, entity_seq entities) :
				m_archetype(archetype),
				m_entities(entities)
			{

			}


			class component_array_accessor
			{
				using untaged_accessor = typename archetype_storage::allocate_accessor<SeqParam>::component_array_accessor;
				using taged_accessor = component_storage::component_allocate_accessor;
				using accessor_variant = std::variant<untaged_accessor, taged_accessor>;
				accessor_variant m_accessor;
				taged_component_iterator m_taged_storages_iter;
				taged_component_iterator m_taged_storages_end;
				entity_seq m_entities;
			public:
				component_array_accessor(
					untaged_accessor untaged_accessor,
					taged_component_iterator taged_storages_iter,
					taged_component_iterator taged_storages_end,
					entity_seq entities
				) :
					m_accessor(untaged_accessor),
					m_taged_storages_iter(taged_storages_iter),
					m_taged_storages_end(taged_storages_end),
					m_entities(entities)
				{}

				component_type_index component_type() const
				{
					if (m_accessor.index() == 0)
					{
						return std::get<untaged_accessor>(m_accessor).component_type();
					}
					else
					{
						(*m_taged_storages_iter)->component_type();
					}
				}


				component_array_accessor& operator++() {
					std::visit([this](auto& accessor) {
						using accessor_type = std::decay_t<decltype(accessor)>;
						if constexpr (std::is_same_v<accessor_type, untaged_accessor>)
						{
							++accessor;
							if (accessor == end_iterator{})
							{
								m_accessor = (*m_taged_storages_iter)->allocate(m_entities);
								++m_taged_storages_iter;
							}
						}
						else if constexpr (std::is_same_v<accessor_type, taged_accessor>)
						{
							accessor = (*m_taged_storages_iter)->allocate(m_entities);
							++m_taged_storages_iter;
						}
						else static_assert(!std::is_same_v<accessor_type, accessor_type>, "not support type");
						}, m_accessor); 
					return *this;
				}

				//component_array_accessor operator++(int) { auto temp = *this; ++* this; return temp; }

				class iterator
				{
					using iterator_variant = std::variant<typename untaged_accessor::iterator, taged_accessor::iterator>;
					iterator_variant m_iterator;
				public:
					iterator(typename untaged_accessor::iterator iter) : m_iterator(iter) {}
					iterator(typename taged_accessor::iterator iter) : m_iterator(iter) {}
					iterator& operator++() { std::visit([](auto& iter) {++iter; }, m_iterator); return *this; }
					bool operator==(const iterator& other) const { return m_iterator == other.m_iterator; }
					bool operator!=(const iterator& other) const { return m_iterator != other.m_iterator; }
					void* operator*() { return std::visit([](auto& iter) {return *iter; }, m_iterator); }
				};

				iterator begin() { return std::visit([](auto& accessor) {return iterator{ accessor.begin() }; }, m_accessor); }
				iterator end() { return std::visit([](auto& accessor) {return iterator{ accessor.end() }; }, m_accessor); }

				bool operator== (const component_array_accessor& other) const { return m_accessor == other.m_accessor; }
				bool operator!= (const component_array_accessor& other) const { return m_accessor != other.m_accessor; }
				bool operator== (const end_iterator&) const { return m_taged_storages_iter == m_taged_storages_end; }
				bool operator!= (const end_iterator&) const { return m_taged_storages_iter != m_taged_storages_end; }
			};

			component_array_accessor begin()
			{
				auto untaged_accessor = m_archetype.m_untaged_storage->get_allocate_accessor(m_entities);
				return component_array_accessor(
					untaged_accessor.begin(),
					m_archetype.m_taged_storages.begin(),
					m_archetype.m_taged_storages.end(),
					m_entities
				);

			}
			end_iterator end() { return{}; }

		};


		auto allocate(sequence_ref<const entity> entities)
		{
			allocate_accessor accessor(*this, entities);
			return accessor;
		}

	};
}