#pragma once
#include "../lib/std_lib.h"
#include "ankerl/unordered_dense.h"

namespace hyecs
{
#if !_HAS_CXX20
	namespace cxx20_extend_for_cxx17
	{
		template <
			class T,
			class Compare,
			class Alloc
		>
		class cxx20_set;

		template<
			typename Key,
			typename Value,
			typename Hash,
			typename Equal,
			typename Alloc
		>
		class cxx20_unordered_map;
	}
#endif


	template<typename T, typename Alloc = std::allocator<T>>
	using vector = std::vector<T, Alloc>;

	template<typename T, typename Alloc = std::allocator<T>>
	using list = std::list<T, Alloc>;

	template<typename T, typename Alloc = std::allocator<T>>
	using deque = std::deque<T, Alloc>;

	template <
		class T,
		class Compare = std::less<T>,
		class Alloc = std::allocator<T>
	>
#if _HAS_CXX20
	using set = std::set<T, Compare, Alloc>;
#else
	using set = cxx20_extend_for_cxx17::cxx20_set<T, Compare, Alloc>;
#endif

	template <
		class T,
		class Compare = std::less<T>,
		class Alloc = std::allocator<T>
	>
	using multiset = std::multiset<T, Compare, Alloc>;

	template <
		class T,
		class Hash = std::hash<T>,
		class Equal = std::equal_to<T>,
		class Alloc = std::allocator<T>
	>
	using unordered_set = ankerl::unordered_dense::set<T, Hash, Equal, Alloc>;

	template <
		class T,
		class Hash = std::hash<T>,
		class Equal = std::equal_to<T>,
		class Alloc = std::allocator<T>
	>
	using unordered_multiset = std::unordered_multiset<T, Hash, Equal, Alloc>;

	template<
		typename Key,
		typename Value,
		typename Compare = std::less<Key>,
		typename Alloc = std::allocator<std::pair<const Key, Value>>
	>
	using map = std::map<Key, Value, Compare, Alloc>;

	template<
		typename Key,
		typename Value,
		typename Compare = std::less<Key>,
		typename Alloc = std::allocator<std::pair<const Key, Value>>
	>
	using multimap = std::multimap<Key, Value, Compare, Alloc>;

	template<
		typename Key,
		typename Value,
		typename Hash = std::hash<Key>,
		typename Equal = std::equal_to<Key>,
		typename Alloc = std::allocator<std::pair<Key, Value>>
	>
#if _HAS_CXX20
	using unordered_map = std::unordered_map<Key, Value, Hash, Equal, Alloc>;
#else
	using unordered_map = ankerl::unordered_dense::map<Key, Value, Hash, Equal, Alloc>;
#endif

	template<
		typename Key,
		typename Value,
		typename Hash = std::hash<Key>,
		typename Equal = std::equal_to<Key>,
		typename Alloc = std::allocator<std::pair<const Key, Value>>
	>
	using unordered_multimap = std::unordered_multimap<Key, Value, Hash, Equal, Alloc>;

	template<typename T, typename Container = deque<T>>
	using stack = std::stack<T, Container>;

	template<typename T, typename Container = deque<T>>
	using queue = std::queue<T, Container>;

	template<
		typename T,
		typename Container = deque<T>,
		typename Compare = std::less<typename Container::value_type>
	>
	using priority_queue = std::priority_queue<T, Container, Compare>;

	//tuple/array
	template<typename ...T>
	using tuple = std::tuple<T...>;

	template<typename ...T>
	using array = std::array<T...>;
}










