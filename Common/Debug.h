#pragma once
#include <assert.h>
#include "Configuration.h"

#ifdef _DEBUG
#	define DEBUG_ONLY(x) x
#else
#	define DEBUG_ONLY(x)
#endif 

extern char _DEBUG_BUFFER[256];

// debugging macros so we can pin down message origin at a glance
#define WHERESTR  " [file %s, line %d]: "
#define WHEREARG  __FILE__, __LINE__

#define DEBUGPRINT(type, ...) { snprintf(_DEBUG_BUFFER, 256, __VA_ARGS__); OutputDebugStringA(_DEBUG_BUFFER);}
//fprintf(stderr, type##WHERESTR##, WHEREARG); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n");

//#if ENABLE_ASSERTIONS
//	#define ASSERT(exp) assert(exp)
//#else
//	#define ASSERT(exp) // Do nothing
//#endif

#if ENABLE_WARNINGS
	#define WARN(...) DEBUGPRINT("WARN", __VA_ARGS__)
#else
#define WARN(...) {}
#endif

#if ENABLE_LOGGING
	#define LOG(...) DEBUGPRINT("LOG", __VA_ARGS__)
#else
	#define LOG(...) 
#endif
