#pragma once
#if defined(_DEBUG) || (!defined(NDEBUG) && !defined(_NDEBUG))
#define ASSERTION_CODE(...) __VA_ARGS__
#define HYECS_DEBUG
#else
#define ASSERTION_CODE(...)
#endif 
#define ASSERTION_CODE_BEGIN() ASSERTION_CODE(
#define ASSERTION_CODE_END() )