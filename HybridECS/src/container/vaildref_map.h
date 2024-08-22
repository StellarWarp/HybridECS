#pragma once
#include "stl_container.h"

namespace hyecs
{
	//using Key = int;
	//using Value = float;
	//using Hash = std::hash<Key>;
	//using Equal = std::equal_to<Key>;
	//using Alloc = std::allocator<std::pair<const Key, Value*>>;
	//template<typename _Key, typename _Value, typename _Hash, typename _Equal, typename _Alloc>
	//using MapContainer = unordered_map<_Key, _Value, _Hash, _Equal, _Alloc>;

	template<
		typename Key,
		typename Value,
		typename Hash = std::hash<Key>,
		typename Equal = std::equal_to<Key>,
		typename Alloc = std::allocator<std::pair<Key, Value*>>,
		template<typename, typename, typename, typename, typename> typename MapContainer = unordered_map
	>
	class vaildref_map
	{

		//todo add allocator for this?
		using ValueContainer = deque<Value>;
		using Map = MapContainer<Key, Value*, Hash, Equal, Alloc>;

		Map m_key_to_index;
		ValueContainer m_values;
		stack<Value*> m_free_value_iter;


	public:
		using hasher = Hash;
		using key_type = Key;
		using mapped_type = Value;
		using key_equal = Equal;

		class iterator
		{
			friend class vaildref_map;
			typename Map::iterator m_iter;

			iterator(typename Map::iterator iter) : m_iter(iter) {}

		public:

			//iterator() = default;

			iterator& operator++()
			{
				++m_iter;
				return *this;
			}

			iterator operator++(int)
			{
				iterator temp = *this;
				++m_iter;
				return temp;
			}

			iterator& operator--()
			{
				--m_iter;
				return *this;
			}

			iterator operator--(int)
			{
				iterator temp = *this;
				--m_iter;
				return temp;
			}

			bool operator==(const iterator& other) const
			{
				return m_iter == other.m_iter;
			}

			std::pair<const key_type&, mapped_type&> operator*() const
			{
				return { m_iter->first, *m_iter->second };
			}

			//this works but it's a UB, not advised to use, use operator*() instead
			std::pair<const key_type&, mapped_type&>* operator->() const
			{
				std::pair<const key_type&, mapped_type&> temp = { m_iter->first, *m_iter->second };
				return &temp;
			}

		};

		iterator begin() { return iterator(m_key_to_index.begin()); }
		iterator end() { return iterator(m_key_to_index.end()); }

		template<typename... Args>
		mapped_type& emplace_value(const key_type& key, Args&&... args)
		{
			if (m_free_value_iter.empty())
			{
				auto& ref = m_values.emplace_back(std::forward<Args>(args)...);
				m_key_to_index.emplace(key, &*(m_values.end() - 1));
				return ref;
			}
			else
			{
				auto iter = m_free_value_iter.top();
				m_free_value_iter.pop();
				new(iter) mapped_type(std::forward<Args>(args)...);
				m_key_to_index.emplace(key, &*iter);
				return *iter;
			}
		}

		mapped_type& emplace(const key_type& key, mapped_type&& value)
		{
			assert(!m_key_to_index.contains(key));
			return emplace_value(key, std::forward<mapped_type>(value));
		}



		mapped_type& insert(std::pair<const key_type, mapped_type>&& value)
		{
			assert(!m_key_to_index.contains(value.first));
			return emplace_value(std::move(value.first), std::move(value.second));
		}

		mapped_type& at(const key_type& key)
		{
			return *m_key_to_index.at(key);
		}

		const mapped_type& at(const key_type& key) const
		{
			return *m_key_to_index.at(key);
		}

		iterator find(const key_type& key)
		{
			return iterator(m_key_to_index.find(key));
		}

		bool contains(const key_type& key) const
		{
			return m_key_to_index.contains(key);
		}



	};
}

