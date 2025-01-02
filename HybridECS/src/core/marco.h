#pragma once

#if defined(_MSC_VER)
#define MSC_EBO __declspec(empty_bases)
#else
#define MSC_EBO
#endif


#ifdef NDEBUG
#define ASSERTION_CODE(...)
#else
#define ASSERTION_CODE(...) __VA_ARGS__
#define HYECS_DEBUG
#endif

//#if defined(_MSC_VER)
//#define no_unique_address msvc::no_unique_address
//#endif

#ifdef __clang__
#define _hyecs_assume(expr) __builtin_assume(expr)
#elifdef _MSC_VER
#define _hyecs_assume(expr) __assume(expr)
#elifdef __GNUC__
#define _hyecs_assume(expr) __attribute__((expr))
#endif
#define hyecs_assume(expr) _hyecs_assume(expr); assert(expr);
