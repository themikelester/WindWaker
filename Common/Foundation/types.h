#pragma once

#include <stdint.h>

// Disable warnings of deprecated functions for the MS C Compiler
#if defined(_MSC_VER)
#	pragma warning(disable: 4996)
#endif

//#ifndef alignof
//	#define alignof(x) __alignof(x)
//#endif

// Fix C99 functions that are not implemented in MSVC
#if defined(_WIN32)
#	define snprintf _snprintf
#endif