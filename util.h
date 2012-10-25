#pragma once

#define STL_FOR_EACH(ELEM, CONTAINER) for (auto ELEM = CONTAINER##.begin(); ELEM != CONTAINER##.end(); ELEM++) 
	