#pragma once

#include "generic_type.h"

namespace hyecs
{
	namespace generic
	{
		template<typename T, typename HashValueType = uint64_t>
		constexpr HashValueType hash_function(const void* key)
		{
			return *reinterpret_cast<const HashValueType*>(key);
		}
	}

	//template<typename Key>
	class generic_dense_map
	{
		using Key = uint64_t;
	public:

		using hash_function_type = uint64_t(*)(const void*);

	private:
		unordered_map<Key, void*> m_map;
		raw_segmented_vector m_values;


	public:

		generic_dense_map(generic::type_index value_type)
			: m_values(value_type)
		{

		}

		using iterator = generic_vector::iterator;

		iterator begin() { return m_values.begin(); }

		iterator end() { return m_values.end(); }

		const iterator begin() const { return m_values.begin(); }

		const iterator end() const { return m_values.end(); }



		generic::type_index value_type() const
		{
			return m_values.type();
		}

		void* allocate_value(Key key)
		{
			auto it = m_map.find(key);
			if (it != m_map.end())
			{
				return it->second;
			}
			void* value_ptr = m_values.allocate_element();
			m_map.emplace(key, value_ptr);
			return value_ptr;
		}

		void deallocate_value(Key key)
		{
			auto it = m_map.find(key);
			if (it != m_map.end())
			{
				m_values.deallocate_element(it->second);
				m_map.erase(it);
			}
		}




		template <typename T, typename... Args, typename Key>
		std::pair<iterator, bool> emplace(Key&& key, Args&&... args)
		{
			auto it = m_map.find(key);
			if (it != m_map.end())
			{
				return { m_values.get_iterator(it->second), false };
			}
			void* value_ptr = m_values.emplace_back<T>(std::forward<Args>(args)...);
			m_map.emplace(key, value_ptr);
			return { m_values.get_iterator(value_ptr), true };
		}

		std::pair<iterator, bool> emplace(const Key& key, generic::constructor constructor)
		{
			auto it = m_map.find(key);
			if (it != m_map.end())
			{
				return { m_values.get_iterator(it->second), false };
			}
			void* value_ptr = m_values.emplace_back(constructor);
			m_map.emplace(key, value_ptr);
			return { m_values.get_iterator(value_ptr), true };
		}

		template<typename Key, typename Value>
		std::pair<iterator, bool> insert(std::pair<const Key, Value> pair)
		{
			return emplace<Value>(pair.first, pair.second);
		}


		void erase(const Key& key)
		{
			auto it = m_map.find(key);
			if (it != m_map.end())
			{
				m_values.swap_back_erase(it->second);
				m_map.erase(it);
			}
		}

		void* find(const Key& key)
		{
			auto it = m_map.find(key);
			if (it != m_map.end())
			{
				return it->second;
			}
			return nullptr;
		}

		void* at(const Key& key)
		{
			auto it = m_map.find(key);
			if (it != m_map.end())
			{
				return it->second;
			}
			throw std::out_of_range("Key not found");
		}

		bool contains(const Key& key)
		{
			return m_map.contains(key);
		}

		void clear()
		{
			m_map.clear();
			m_values.clear();
		}





	};
}