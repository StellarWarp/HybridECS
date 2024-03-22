#pragma once

#include "generic_type.h"
#include "generic_vector.h"

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
	class generic_dense_map
	{
	public:

		using hash_function_type = uint64_t(*)(const void*);

	private:
		hash_function_type m_hash_function;
		unordered_map<uint64_t, void*> m_map;
		generic_vector m_values;


	public:

		generic_dense_map(hash_function_type hash_function, generic::type_index value_type)
			: m_hash_function(hash_function),
			m_values(value_type)
		{

		}

		generic::type_index value_type() const
		{
			return m_values.type();
		}

		template <typename T,typename... Args, typename Key>
		std::pair<void*, bool> emplace(Key&& key, Args&&... args)
		{
			uint64_t hash = m_hash_function(&key);
			auto it = m_map.find(hash);
			if (it != m_map.end())
			{
				return { it->second, false };
			}
			void* value_ptr = m_values.emplace_back<T>(std::forward<Args>(args)...);
			m_map.emplace(hash, value_ptr);
			return { value_ptr, true };
		}

		std::pair<void*, bool> emplace(const void* key, generic::constructor constructor)
		{
			uint64_t hash = m_hash_function(key);
			auto it = m_map.find(hash);
			if (it != m_map.end())
			{
				return { it->second, false };
			}
			void* value_ptr = m_values.emplace_back(constructor);
			m_map.emplace(hash, value_ptr);
			return { value_ptr, true };
		}

		template<typename Key, typename Value>
		std::pair<void*, bool> insert(std::pair<const Key, Value> pair)
		{
			return emplace<Value>(pair.first, pair.second);
		}
 

		void erase(const void* key)
		{
			uint64_t hash = m_hash_function(key);
			auto it = m_map.find(hash);
			if (it != m_map.end())
			{
				m_values.swap_back_erase(it->second);
				m_map.erase(it);
			}
		}

		void* find(const void* key)
		{
			uint64_t hash = m_hash_function(key);
			auto it = m_map.find(hash);
			if (it != m_map.end())
			{
				return it->second;
			}
			return nullptr;
		}

		void* at(const void* key)
		{
			uint64_t hash = m_hash_function(key);
			auto it = m_map.find(hash);
			if (it != m_map.end())
			{
				return it->second;
			}
			throw std::out_of_range("Key not found");
		}

		[[nodiscard]] bool contains(const void* key)
		{
			uint64_t hash = m_hash_function(key);
			return m_map.contains(hash);
		}

		void clear()
		{
			m_map.clear();
			m_values.clear();
		}

		using iterator = generic_vector::iterator;

		iterator begin()
		{
			return m_values.begin();
		}

		iterator end()
		{
			return m_values.end();
		}

		const iterator begin() const
		{
			return m_values.begin();
		}

		const iterator end() const
		{
			return m_values.end();
		}



	};
}