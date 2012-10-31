#pragma once

#include "Configuration.h" // Must be included first
#include "Debug.h"
#include "Types.h"
#include <Framework3\Platform.h>

// RESULT Enum and related functions
#define IFC(x) if( FAILED((r = x)) ) {goto cleanup;}	// If Failed Cleanup
#define IFE(x) if( FAILED((r = x)) ) {goto error;}		// If Failed Error
#define ISC(x) if( !FAILED((r = x))) {goto cleanup;}	// If Success Cleanup
#define GTC(x) { r = x; goto cleanup; }					// Go To Cleanup

// Math
#define DEGTORAD(x) (x/360.f*2*PI)

typedef int RESULT;