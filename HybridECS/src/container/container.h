#pragma once

#include "stl_container.h"
#include "generic_container.h"
//#include "entt/dense_map.h"
//#include "entt/dense_set.h"
#include "vaildref_map.h"
#include "fixed_vector.h"
#include "bit_set.h"
#include "sequence_ref.h"
#include "raw_segmented_vector.h"
namespace hyecs
{

	template<
		typename Key,
		typename Value,
		typename Hash = std::hash<Key>,
		typename Equal = std::equal_to<Key>,
		typename Alloc = std::allocator<std::pair<Key, Value>>
	>
	using dense_map = unordered_map<Key, Value, Hash, Equal, Alloc>;

	template <
		class T,
		typename Hash = std::hash<T>,
		typename Equal = std::equal_to<T>,
		typename Alloc = std::allocator<T>
	>
	using dense_set = unordered_set<T, Hash, Equal, Alloc>;



}



