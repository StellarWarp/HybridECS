#pragma once


#if !_HAS_CXX20

#include <unordered_map>

namespace hyecs
{
	namespace cxx20_extend_for_cxx17
	{

		//using Key = int;
		//using Value = int;
		//using Hash = std::hash<Key>;
		//using Equal = std::equal_to<Key>;
		//using Alloc = std::allocator<std::pair<const Key, Value>>;
		template<
			typename Key,
			typename Value,
			typename Hash = std::hash<Key>,
			typename Equal = std::equal_to<Key>,
			typename Alloc = std::allocator<std::pair<const Key, Value>>
		>
		class cxx20_unordered_map : public std::unordered_map<Key, Value, Hash, Equal, Alloc>
		{
			using _base = std::unordered_map<Key, Value, Hash, Equal, Alloc>;
		public:
#ifdef _MSC_VER
			_NODISCARD bool contains(Key _Keyval) const {
				return static_cast<bool>(_base::_Find_last(_Keyval, _base::_Traitsobj(_Keyval))._Duplicate);
			}
#else

#error "Not implemented"

#endif
		};
	}
}

#endif