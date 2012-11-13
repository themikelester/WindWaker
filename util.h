#pragma once

#include "Types.h"

#define STL_FOR_EACH(ELEM, CONTAINER) for (auto ELEM = CONTAINER##.begin(); ELEM != CONTAINER##.end(); ELEM++) 
	
uint bitcount (uint n)  {
   uint count = 0 ;
   while (n)  {
      ++count ;
      n &= (n - 1) ;
   }
   return count ;
}