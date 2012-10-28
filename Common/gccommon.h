#pragma once

/*
 * On my Intel OS X, BIG_ENDIAN is defined for some reason when <sys/types.h>
 * is included (which it is by <vector> for example), so don't check for it.
 */

#if defined __BIG_ENDIAN__ || (defined __APPLE__ && defined __POWERPC__)
#define GC_BIG_ENDIAN
#endif

//sanity check
#if (defined __LITTLE_ENDIAN__ || defined LITTLE_ENDIAN) && defined GC_BIG_ENDIAN
#error Unable to determine endianness
#endif




