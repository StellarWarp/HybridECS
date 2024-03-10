#pragma once

#include "generic_type.h"
#include "generic_vector.h"

namespace hyecs
{
	class generic_dense_map
	{
	public:

		using hash_function_type = uint64_t(*)(const void*);

	private:
		generic::type_index m_key_type;
		hash_function_type m_hash_function;
		unordered_map<uint64_t, void*> m_map;
		generic_vector m_values;


	public:

		generic_dense_map(generic::type_index key_type, generic::type_index value_type, hash_function_type hash_function)
			: m_hash_function(hash_function),
			m_key_type(key_type), 
			m_values(value_type)
		{

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



	};
}