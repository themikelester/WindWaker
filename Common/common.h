#pragma once

#include <Framework3\Platform.h>

// Assorted definitions
#define NULL 0

// ASSERT definitions
#ifdef DEBUG
	// debugging macros so we can pin down message origin at a glance
	#define WHERESTR  "[file %s, line %d]: "
	#define WHEREARG  __FILE__, __LINE__
	#define DEBUGPRINT(...)       fprintf(stderr, __VA_ARGS__)
	#define _LOG DEBUGPRINT("LOG" WHERESTR, WHEREARG)
	#define _WARN DEBUGPRINT("WARN" WHERESTR, WHEREARG)

	#define LOG(...)  DEBUGPRINT("LOG: " __VA_ARGS__)
	#define WARN(...) DEBUGPRINT("WARN: " __VA_ARGS__)
#else //if RELEASE
	#define LOG(...)  
	#define WARN(...)
#endif

// RESULT Enum and related functions
#define IFC(x) if( FAILED((r = x)) ) {goto cleanup;}	// If Failed Cleanup
#define IFE(x) if( FAILED((r = x)) ) {goto error;}		// If Failed Error
#define ISC(x) if( !FAILED((r = x))) {goto cleanup;}	// If Success Cleanup
#define GTC(x) { r = x; goto cleanup; }					// Go To Cleanup

// Math
#define DEGTORAD(x) (x/360.f*2*PI)

typedef int RESULT;