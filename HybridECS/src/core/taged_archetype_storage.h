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
		table* m_table;
		archetype_storage* m_untaged_storage;
		vector<component_storage*> m_taged_storages;

		//event
		std::function<void(entity, storage_key)> m_on_entity_add;
		std::function<void(entity, storage_key)> m_on_entity_remove;


	private:
		void entity_change_archetype(
			sequence_ref<entity> entities,
			taged_archetype_storage* dest_archetype,
			remove_component taged_remove_components,
			sorted_sequence_ref<generic::constructor> untaged_adding_constructors,
			sorted_sequence_ref<generic::constructor> taged_adding_constructors)
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

		class allocate_accessor
		{
			using end_iterator = nullptr_t;
			using taged_component_iterator = vector<component_storage*>::iterator;
			using entity_seq = sequence_ref<const entity>;
			taged_archetype_storage& m_archetype;
			entity_seq m_entities;
		public:
			allocate_accessor(taged_archetype_storage& archetype, entity_seq entities) :
				m_archetype(archetype),
				m_entities(entities)
			{

			}
			

			archetype_storage::allocate_accessor m_untaged_accessor;

			class component_array_accessor
			{
				using untaged_accessor = archetype_storage::allocate_accessor::component_array_accessor;
				using taged_accessor = component_storage::component_allocate_accessor;
				using accessor_variant = std::variant<untaged_accessor, taged_accessor>;
				accessor_variant m_accessor;
				taged_component_iterator m_taged_storages_iter;
				taged_component_iterator m_taged_storages_end;
			public:
				component_array_accessor(untaged_accessor untaged_accessor, taged_component_iterator taged_storages_iter, taged_component_iterator taged_storages_end) :
					m_accessor(untaged_accessor),
					m_taged_storages_iter(taged_storages_iter),
					m_taged_storages_end(taged_storages_end)
				{}


				component_array_accessor& operator++() {
					std::visit([this](auto& accessor) {
						using accessor_type = std::decay_t<decltype(accessor)>;
						if constexpr (std::is_same_v<accessor_type, untaged_accessor>)
						{
							++accessor;
							if (accessor == end_iterator{})
							{
								accessor = (*m_taged_storages_iter)->get_component_allocate_accessor();
								++m_taged_storages_iter;
							}
						}
						else if constexpr (std::is_same_v<accessor_type, taged_accessor>)
						{
							accessor = (*m_taged_storages_iter)->get_component_allocate_accessor();
							++m_taged_storages_iter;
						}
					}, m_accessor); return *this;
				}

				class iterator
				{
					using iterator_variant = std::variant<untaged_accessor::iterator, taged_accessor::iterator>;
					iterator_variant m_iterator;
				public:
					iterator(archetype_storage::allocate_accessor::component_array_accessor::iterator iter) : m_iterator(iter) {}
					iterator(component_storage::component_allocate_accessor::iterator iter) : m_iterator(iter) {}
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

			component_array_accessor begin() {}
			end_iterator end() { return{}; }

		};


		allocate_accessor allocate(sequence_ref<const entity> entities)
		{
			allocate_accessor accessor(*this, entities);
			return accessor;
		}

	};
}