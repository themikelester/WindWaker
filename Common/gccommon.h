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


typedef unsigned char ubyte;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
#ifdef _MSC_VER
typedef unsigned __int64 u64;
#else
typedef unsigned long long u64;
#endif

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
#ifdef _MSC_VER
typedef signed __int64 s64;
#else
typedef signed long long s64;
#endif

typedef float f32;

