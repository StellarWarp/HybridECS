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
        union non_construct_value
        {
            Value value;
            char dummy[sizeof(Value)];
            non_construct_value(){}
            ~non_construct_value()
            {
                value.~Value();
            }
        };
		//todo add allocator for ValueContainer?
		using ValueContainer = deque<non_construct_value>;
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

        template<typename Constructor> requires std::invocable<Constructor, Value*>
        mapped_type& emplace_at(const key_type& key, Constructor&& constructor)
        {
            assert(!m_key_to_index.contains(key));
            if (m_free_value_iter.empty())
            {
                uint32_t index = (uint32_t)m_values.size();
                auto& ref = m_values.emplace_back();
                auto ptr = &ref.value;
                m_key_to_index.emplace(key, ptr);
                constructor(ptr);
                return ref.value;
            }
            else
            {
                auto iter = m_free_value_iter.top();
                m_free_value_iter.pop();
                auto& ref = *iter;
                auto ptr = &ref;
                m_key_to_index.emplace(key, ptr);
                constructor(ptr);
                return ref;
            }
        }

		template<typename... Args>
		mapped_type& emplace_value(const key_type& key, Args&&... args)
		{
            return emplace_at(key, [&](Value* value) { new (value) Value(std::forward<Args>(args)...); });
		}

        mapped_type& emplace(const key_type& key, auto&& value)
        {
            return emplace_value(key,  std::forward<decltype(value)>(value));
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

