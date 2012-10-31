#include "Mem.h"
#include <new>

// Global buffer from which all dynamic allocations are made
u8 __buffer[BUFFER_SIZE];

static StackAllocator* _bufAllocator = 0;

// Global allocators
StackAllocator* _tempAllocator = 0;
PoolAllocator*	_smallBlockAllocator = 0;
StackAllocator* _singleFrameAllocator = 0;
StackAllocator* _doubleFrameAllocator = 0;
StackAllocator* _permanentAllocator = 0;

void MemManager::init()
{
	_bufAllocator = new (__buffer) StackAllocator("root_allocator", __buffer + sizeof(StackAllocator), BUFFER_SIZE - sizeof(StackAllocator));
            
	tempAllocation = _bufAllocator->Alloc( MB(16) );
    _tempAllocator = _bufAllocator->make_new<StackAllocator>("temp_allocator", tempAllocation, MB(16));
}

void MemManager::shutdown()
{
	_bufAllocator->Free(tempAllocation);
	_tempAllocator->~StackAllocator();
	_bufAllocator->~StackAllocator();
}