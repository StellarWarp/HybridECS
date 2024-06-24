#pragma once
#pragma region container

#include <iostream>
#include <string>
#include <vector>
#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <deque>

#include <queue>
#include <stack>


#pragma endregion


#include <stdint.h>
#include <assert.h>

#include<algorithm>

#include <functional>

#include <tuple>
#include <variant>
#include <optional>


//meta
#include <type_traits>
#include <typeinfo>
#include <typeindex>

#include <any>

#if defined _MSC_VER
#define no_unique_address msvc::no_unique_address
#endif


#if _HAS_CXX20
#include <format>
#include <bit>
#else
#include "cxx20_extend_for_cxx17.h"
#endif