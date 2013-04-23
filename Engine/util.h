#pragma once

#include "Types.h"

#define STL_FOR_EACH(ELEM, CONTAINER) for (auto ELEM = CONTAINER##.begin(); ELEM != CONTAINER##.end(); ELEM++) 

namespace util
{
	uint bitcount (uint n);

	uint64_t hash64(const void * key, uint32_t len, uint64_t seed);
}