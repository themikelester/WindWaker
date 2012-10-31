#pragma once

#include "Allocators.h"
#include <Types.h>

#define B(x) x
#define KB(x) 1024 * B(x)
#define MB(x) 1024 * KB(x)
#define GB(x) 1024 * MB(x)

// TOTAL MEMORY SIZE!!
#define BUFFER_SIZE MB(256)

enum MemPools {
	MEM_TEMP,
	MEM_SMALLBLOCK,
	MEM_SINGLEFRAME,
	MEM_DOUBLEFRAME,
	MEM_PERMANENT,
};

class MemManager
{
public:
	void init();
	void shutdown();

private:
	void* tempAllocation;
};

