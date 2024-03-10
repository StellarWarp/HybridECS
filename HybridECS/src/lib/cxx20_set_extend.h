#pragma once

#if !_HAS_CXX20

#include <set>

namespace hyecs
{
	namespace cxx20_extend_for_cxx17
	{

		using T = int;
		using Compare = std::less<T>;
		using Alloc = std::allocator<T>;
		template <
			class T,
			class Compare = std::less<T>,
			class Alloc = std::allocator<T>
		>
		class cxx20_set : public std::set<T, Compare, Alloc>
		{
			using _base = std::set<T, Compare, Alloc>;
#ifdef _MSC_VER
		public:
			_NODISCARD bool contains(const _base::key_type& _Keyval) const {
				return _Lower_bound_duplicate(_Find_lower_bound(_Keyval)._Bound, _Keyval);
			}

#else
			_NODISCARD bool contains(const key_type& _Keyval) const {
				return this->find(_Keyval) != this->end();
			}
#endif
		};

	}
}

#endif

