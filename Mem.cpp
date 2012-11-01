#include "Mem.h"
#include <new>

// Buffer from which all dynamic allocations are made
static u8 __buffer[BUFFER_SIZE];

static StackAllocator* _rootAllocator = 0;

// Global allocators
namespace Mem {
	Allocator* tempAllocator = 0;
	Allocator* smallBlockAllocator = 0;
	Allocator* singleFrameAllocator = 0;
	Allocator* doubleFrameAllocator = 0;
	Allocator* permanentAllocator = 0;

	Allocator* defaultAllocator = 0;
}

void MemManager::init()
{
	_rootAllocator = new (__buffer) StackAllocator("root_allocator", __buffer + sizeof(StackAllocator), BUFFER_SIZE - sizeof(StackAllocator));
            
    Mem::tempAllocator = _rootAllocator->make_new<StackAllocator>("temp_allocator", MB(16), _rootAllocator);

	Mem::defaultAllocator = Mem::tempAllocator;
}

void MemManager::shutdown()
{
	_rootAllocator->make_delete(Mem::tempAllocator);

	_rootAllocator->~StackAllocator();
}