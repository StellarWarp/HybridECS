#pragma once

#if defined(_MSC_VER)
#define MSC_EBO __declspec(empty_bases)
#else
#define MSC_EBO
#endif


#if defined(_DEBUG) || (!defined(NDEBUG) && !defined(_NDEBUG))
#define ASSERTION_CODE(...) __VA_ARGS__
#define HYECS_DEBUG
#else
#define ASSERTION_CODE(...)
#endif

//#if defined(_MSC_VER)
//#define no_unique_address msvc::no_unique_address
//#endif
