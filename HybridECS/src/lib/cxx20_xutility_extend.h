#pragma once
#include <xutility>

#if !_HAS_CXX20
namespace std
{
#ifdef __EDG__ // TRANSITION, DevCom-1691516
	// per LWG-3888
	_EXPORT_STD template <class _Ty, class... _Types,
		class = void_t<decltype(::new(static_cast<void*>(_STD declval<_Ty*>())) _Ty(_STD declval<_Types>()...))>>
#else // ^^^ no workaround / workaround vvv
	// per LWG-3888
	_EXPORT_STD template <class _Ty, class... _Types,
		void_t<decltype(::new(static_cast<void*>(_STD declval<_Ty*>())) _Ty(_STD declval<_Types>()...))>* = nullptr>
#endif // TRANSITION, DevCom-1691516
		constexpr _Ty* construct_at(_Ty* const _Location, _Types&&... _Args) noexcept(
			noexcept(::new(static_cast<void*>(_Location)) _Ty(_STD forward<_Types>(_Args)...))) /* strengthened */ {
		_MSVC_CONSTEXPR return ::new (static_cast<void*>(_Location)) _Ty(_STD forward<_Types>(_Args)...);
	}
}
#endif // _HAS_CXX20
