#pragma once
#include <assert.h>

// debugging macros so we can pin down message origin at a glance
#define WHERESTR  " [file %s, line %d]: "
#define WHEREARG  __FILE__, __LINE__

#define DEBUGPRINT(type, ...) {fprintf(stderr, type##WHERESTR##, WHEREARG); fprintf(stderr, __VA_ARGS__);}

//#if ENABLE_ASSERTIONS
//	#define ASSERT(exp) assert(exp)
//#else
//	#define ASSERT(exp) // Do nothing
//#endif

#if ENABLE_WARNINGS
	#define WARN(...) DEBUGPRINT("WARN", __VA_ARGS__);
#else
	#define WARN(...)
#endif

#if ENABLE_LOGGING
	#define LOG(...) DEBUGPRINT("LOG", __VA_ARGS__);
#else
	#define LOG(...)
#endif
