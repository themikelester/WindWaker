#pragma once

#include "Types.h"

#define STL_FOR_EACH(ELEM, CONTAINER) for (auto ELEM = CONTAINER##.begin(); ELEM != CONTAINER##.end(); ELEM++) 

namespace util
{
	uint bitcount (uint n);
}