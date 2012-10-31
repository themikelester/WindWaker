#include "Mem.h"
#include <common.h>

// Constructs a stack allocator with the given size
StackAllocator::StackAllocator(const char* name, void* base, u32 stackSize_bytes)
{
	m_Name = name;
	m_pBase = (u8*)base;
	m_pTop = m_pBase;
	m_pEnd = m_pBase + stackSize_bytes;
}

// Allocates a new block of memory from the stack top
void* StackAllocator::Alloc(u32 size_bytes)
{
	if (m_pTop == m_pEnd)
		return NULL;	// Out of memory

	void* ptr = (void*)m_pTop;
	m_pTop += size_bytes;

	return ptr;
}

// Rolls the stack back to a previous marker
void StackAllocator::Free(void* marker)
{
	m_pTop = (u8*)marker;
}

// Clears the entire stack (rolls back to zero)
void StackAllocator::Clear()
{
	m_pTop = m_pBase;
}

// Return the allocated size of the given memory pointer
u32 StackAllocator::Size(void* marker)
{
	return 0; // Unimplemented
}

PoolAllocator::PoolAllocator(const char* name, void* base, u32 sizeOfElement, u32 numElements)
{
	ASSERT(sizeOfElement >= sizeof(Ptr));
	
	m_Name = name;
	m_pBase = base;
	m_pLLHead = m_pBase;
	
	// Construct a linked-list that points to each new block
	void** ptr = (void**)m_pBase;
	for (u32 i = 0; i < numElements; i++)
	{
		*ptr = ptr + sizeOfElement;
		ptr += sizeOfElement;
	}
}

// Allocates a new block from the pool
void* PoolAllocator::Alloc(u32 unnused)
{
	Ptr firstFree = m_pLLHead;
	m_pLLHead = *(Ptr*)m_pLLHead;

	return firstFree;
}

// Rolls the stack back to a previous marker
void PoolAllocator::Free(void* ptr)
{
	*(Ptr*)ptr =  m_pLLHead;
	m_pLLHead = ptr;
}

u32 PoolAllocator::Size(void* ptr)
{
	return 0; // Unimplemented
}