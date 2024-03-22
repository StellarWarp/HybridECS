#pragma once

#include "cxx20_set_extend.h"
#include "cxx20_unordered_map_extend.h"
#include "cxx20_xmemort_extend.h"
#include "cxx20_bit_extend.h"

namespace std {

#if !_HAS_CXX20

	_EXPORT_STD template <class _Ty>
		using remove_cvref_t = _Remove_cvref_t<_Ty>;

	_EXPORT_STD template <class _Ty>
		struct remove_cvref {
		using type = remove_cvref_t<_Ty>;
	};
#endif
}

