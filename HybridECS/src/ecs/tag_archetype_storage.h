#pragma once
#include "archetype_storage.h"
#include "component_storage.h"

namespace hyecs
{
	class tag_archetype_storage : non_movable
	{
		archetype_index m_index;
		dense_map<entity, storage_key> m_entities;
		archetype_storage* m_untag_storage;
		vector<component_storage*> m_tag_storages;

		//event
		vector<std::function<void(entity, storage_key)>> m_on_entity_add;
		vector<std::function<void(entity, storage_key)>> m_on_entity_remove;

	public:
		tag_archetype_storage(
			archetype_index index,
			archetype_storage* untag_storage,
			sorted_sequence_cref<component_storage*> tag_storages
		) :
			m_index(index),
			m_untag_storage(untag_storage),
			m_tag_storages(tag_storages.begin(), tag_storages.end())
		{
			m_untag_storage->add_callback_on_sparse_to_chunk([this]()
			{
				notify_storage_chunk_convert();
			});
		}

		dense_map<entity, storage_key>& entities() { return m_entities; }

		void add_callback_on_entity_add(std::function<void(entity, storage_key)> callback)
		{
			for (auto [e, key] : m_entities)
			{
				callback(e, key);
			}
			m_on_entity_add.push_back(callback);
		}

		void add_callback_on_entity_remove(std::function<void(entity, storage_key)> callback)
		{
			m_on_entity_remove.push_back(callback);
		}


		void notify_storage_chunk_convert()
		{
			auto key_registry = m_untag_storage->get_key_registry();
			for (auto& [e, key] : m_entities)
			{
				key = key_registry.at(e);
			}
		}

		void notify_storage_sparse_convert()
		{
			//todo if this needed?
			// for (auto& [e, key] : m_entities)
			// {
			// 	key = storage_key{};
			// }
		}

		//todo emplace entity etc..


		void entity_change_archetype(
			sequence_cref<entity> entities,
			tag_archetype_storage* dest_archetype,
			remove_component tag_remove_components,
			sorted_sequence_cref<generic::constructor> untag_adding_constructors,
			sorted_sequence_cref<generic::constructor> tag_adding_constructors)
		{
			m_untag_storage->entity_change_archetype(entities, dest_archetype->m_untag_storage, untag_adding_constructors);

			auto src_tag_storages_iter = m_tag_storages.begin();
			auto dest_tag_storages_iter = dest_archetype->m_tag_storages.begin();
			auto src_end = m_tag_storages.end();
			auto constructors_iter = tag_adding_constructors.begin();
			for (; src_tag_storages_iter != src_end; ++src_tag_storages_iter, ++dest_tag_storages_iter)
			{
				auto& src_tag_storages = **src_tag_storages_iter;
				auto& dest_tag_storages = **dest_tag_storages_iter;
				if (src_tag_storages.component_type() < dest_tag_storages.component_type())
				{
					//remove
					src_tag_storages.erase(entities);
				}
				else if (src_tag_storages.component_type() > dest_tag_storages.component_type())
				{
					//add
					dest_tag_storages.emplace(entities, *constructors_iter);
					constructors_iter++;
				}
			}
		}

		template <typename SeqParam>
		class allocate_accessor
		{
			//using SeqParam = const entity*;
			using end_iterator = nullptr_t;
			using tag_component_iterator = vector<component_storage*>::iterator;
			using entity_seq = sequence_cref<entity, SeqParam>;

			tag_archetype_storage& m_archetype;
			archetype_storage::allocate_accessor<SeqParam> m_untag_accessor;
			entity_seq m_entities;
			ASSERTION_CODE(bool m_is_construct_finished = false);

		public:
			allocate_accessor(tag_archetype_storage& archetype, entity_seq entities) :
				m_archetype(archetype),
				m_untag_accessor(m_archetype.m_untag_storage->get_allocate_accessor(entities)),
				m_entities(entities)
			{
				if (const table::allocate_accessor<SeqParam>* chunk_accessor = m_untag_accessor.get_chunk_allocate_accessor())
				{
					chunk_accessor->for_each_entity_key([&](entity e, storage_key key)
					{
						m_archetype.m_entities.insert({e, key});
					});
				}
				else
					for (auto e : m_entities)
					{
						m_archetype.m_entities.insert({e, storage_key{}});
					}
			}

			allocate_accessor(allocate_accessor&& other) noexcept :
				m_archetype(other.m_archetype),
				m_untag_accessor(std::move(other.m_untag_accessor)),
				m_entities(std::move(other.m_entities))
			{
				ASSERTION_CODE(other.m_is_construct_finished = true);
			}

			allocate_accessor(const allocate_accessor& other) = delete;


			void notify_construct_finish()
			{
				m_untag_accessor.notify_construct_finish();
				for (auto& callback : m_archetype.m_on_entity_add)
				{
					for (auto [e, key] : m_archetype.m_entities)
					{
						callback(e, key);
					}
				}
				ASSERTION_CODE(m_is_construct_finished = true);
			}

			~allocate_accessor() { assert(m_is_construct_finished); }


			class component_array_accessor
			{
				using untag_accessor = typename archetype_storage::allocate_accessor<SeqParam>::component_array_accessor;
				using tag_accessor = component_storage::component_allocate_accessor;
				using accessor_variant = std::variant<untag_accessor, tag_accessor>;
				accessor_variant m_accessor;
				tag_component_iterator m_tag_storages_iter;
				tag_component_iterator m_tag_storages_end;
				entity_seq m_entities;

			public:
				component_array_accessor(
					untag_accessor&& untag_accessor,
					tag_component_iterator tag_storages_iter,
					tag_component_iterator tag_storages_end,
					entity_seq entities
				) :
					m_accessor(std::move(untag_accessor)),
					m_tag_storages_iter(tag_storages_iter),
					m_tag_storages_end(tag_storages_end),
					m_entities(entities)
				{
				}

				component_type_index component_type() const
				{
					if (m_accessor.index() == 0)
					{
						return std::get<untag_accessor>(m_accessor).component_type();
					}
					else
					{
						return (*m_tag_storages_iter)->component_type();
					}
				}


				component_array_accessor& operator++()
				{
					std::visit([this](auto& accessor)
					{
						using accessor_type = std::decay_t<decltype(accessor)>;
						if constexpr (std::is_same_v<accessor_type, untag_accessor>)
						{
							++accessor;
							if (accessor == end_iterator{} && m_tag_storages_iter != m_tag_storages_end)
							{
								m_accessor = (*m_tag_storages_iter)->allocate(m_entities); //change accessor
								++m_tag_storages_iter;
							}
						}
						else if constexpr (std::is_same_v<accessor_type, tag_accessor>)
						{
							accessor = (*m_tag_storages_iter)->allocate(m_entities);
							++m_tag_storages_iter;
						}
						else static_assert(!std::is_same_v<accessor_type, accessor_type>, "not support type");
					}, m_accessor);
					return *this;
				}

				//component_array_accessor operator++(int) { auto temp = *this; ++* this; return temp; }

				class iterator
				{
					using iterator_variant = std::variant<typename untag_accessor::iterator, tag_accessor::iterator>;
					iterator_variant m_iterator;

				public:
					iterator(typename untag_accessor::iterator iter) : m_iterator(iter)
					{
					}

					iterator(typename tag_accessor::iterator iter) : m_iterator(iter)
					{
					}

					iterator& operator++()
					{
						std::visit([](auto& iter) { ++iter; }, m_iterator);
						return *this;
					}

					bool operator==(const iterator& other) const { return m_iterator == other.m_iterator; }
					bool operator==(const end_iterator&) const { return std::visit([](auto& iter) { return iter == end_iterator{}; }, m_iterator); }
					void* operator*() { return std::visit([](auto& iter) { return *iter; }, m_iterator); }
				};

				iterator begin() { return std::visit([](auto& accessor) { return iterator{accessor.begin()}; }, m_accessor); }
				end_iterator end() { return {}; }

				bool operator==(const component_array_accessor& other) const { return m_accessor == other.m_accessor; }

				bool operator==(const end_iterator&) const
				{
					return std::visit([this](auto& accessor)
					{
						using accessor_type = std::decay_t<decltype(accessor)>;
						if constexpr (std::is_same_v<accessor_type, untag_accessor>)
							return accessor == end_iterator{};
						else if constexpr (std::is_same_v<accessor_type, tag_accessor>)
							return m_tag_storages_iter == m_tag_storages_end;
						else static_assert(!std::is_same_v<accessor_type, accessor_type>, "not support type");
					}, m_accessor);
				}

			};

			component_array_accessor begin()
			{
				return component_array_accessor(
					m_untag_accessor.begin(),
					m_archetype.m_tag_storages.begin(),
					m_archetype.m_tag_storages.end(),
					m_entities
				);
			}

			end_iterator end() { return {}; }
		};


		auto allocate(sequence_cref< entity> entities)
		{
			allocate_accessor accessor(*this, entities);
			return accessor;
		}
	};
}
