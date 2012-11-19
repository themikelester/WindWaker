#pragma once

#include <stdint.h>

#ifndef _alignof
#	define _alignof(x) __alignof(x)
#endif

// Disable warnings of deprecated functions for the MS C Compiler
#if defined(_MSC_VER)
#	pragma warning(disable: 4996)
#endif

// Fix C99 functions that are not implemented in MSVC
#if defined(_WIN32)
#	define snprintf _snprintf
#endif

#define NULL 0

typedef unsigned char ubyte;
typedef unsigned short ushort;
typedef unsigned int uint;

#ifdef _MSC_VER
	typedef unsigned __int8		u8;
	typedef unsigned __int16	u16;
	typedef unsigned __int32	u32;
	typedef unsigned __int64	u64;
	
	typedef signed __int8		s8;
	typedef signed __int16		s16;
	typedef signed __int32		s32;
	typedef signed __int64		s64;
#else
	typedef unsigned char		u8;
	typedef unsigned short		u16;
	typedef unsigned int		u32;
	typedef unsigned long long	u64;

	typedef signed char			s8;
	typedef signed short		s16;
	typedef signed int			s32;
	typedef signed long long	s64;
#endif

typedef float					f32;
typedef double					d64;