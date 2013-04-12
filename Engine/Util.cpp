#include "Util.h"

namespace util
{
	uint bitcount (uint n)
	{
		uint count = 0 ;
		while (n)  {
			++count ;
			n &= (n - 1) ;
		}
		return count ;
	}
}