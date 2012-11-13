#pragma once

#include <common.h>

class Allocator
{
protected:
	const char* m_Name; // Name of this allocator (for memory tracking)
	
	Allocator* m_ParentAllocator;

	/* For error checking */
	u32 m_Size;			// Total size (in bytes) of all allocations made by this allocator
	u32 m_Allocations;	// Number of unfreed allocations made by this allocator

public:
	// TODO: Add alignment parameter to alloc
	virtual void*  Alloc (u32 size)	 = 0;
	virtual void   Free	 (void* ptr) = 0;
	virtual u32	   Size  (void* ptr) = 0;

	virtual ~Allocator() { 
		ASSERT(m_Size == 0 && m_Allocations == 0);
	};

	template <class T, class P1> T *make_new(const char* name, const P1 &p1) 
	{
		// TODO: Add alignment parameter to alloc
		//return new (allocate(sizeof(T), alignof(T))) T(p1);
		return new (Alloc(sizeof(T))) T(name, p1);
	}

	template <class T, class P1, class P2> T *make_new(const char* name, const P1 &p1, const P2 &p2) 
	{
		// TODO: Add alignment parameter to alloc
		//return new (allocate(sizeof(T), alignof(T))) T(p1);
		return new (Alloc(sizeof(T))) T(name, p1, p2);
	}
 
    template <class T> void make_delete(T *p) 
	{
        if (p) {
            p->~T();
            Free(p);
        }
    }
};

class StackAllocator : public Allocator
{
public: // Allocator methods

	// Allocates a new block of memory from the stack top
	void* Alloc(u32 size_bytes);
	
	// Rolls the stack back to a previous marker
	void Free(void* marker);

	// Return the allocated size of this marker
	u32 Size(void* marker);

public: 
	// Constructs a stack allocator whose memory starts at base and is stackSize_bytes long
	explicit StackAllocator(const char* name, void* base, u32 stackSize_bytes);

	// Constructs a stack allocator and uses allocator to allocate stackSize_bytes for its use
	explicit StackAllocator(const char* name, u32 stackSize_bytes, StackAllocator* allocator);

	~StackAllocator();

	// Return a marker that can be used as an argument to Free()
	void* GetMarker();

	// Clears the entire stack (rolls back to zero)
	void Clear();

protected:
	u8* m_pBase;
	u8* m_pTop;
	u8* m_pEnd;

	void* m_ParentMarker;
	void* m_PrevMarker;
};

class PoolAllocator : public Allocator
{
public: // Allocator methods
	// Allocates a new block from the pool
	void* Alloc(u32 unnused);

	// Free a particular block of memory
	void Free(void* ptr);

	// Return the allocated size of this block (always equal to m_ElementSize);
	u32 Size(void* ptr);

public:
	// Constructs a pool allocator with the given size
	explicit PoolAllocator(const char* name, void* base, u32 sizeOfElement, u32 numElements);
	
protected:
	typedef void* Ptr;

	u32 m_ElementSize;	// Size of each block in bytes

	void* m_pBase;		// pointer to base of allocated memory
	void* m_pLLHead;	// pointer to the first free block (Linked List Head)
};

class HeapAllocator : public Allocator
{
public: // Allocator methods
	// Allocates a new block from the heap
	void* Alloc(u32 size_bytes);

	// Free the given block of memory
	void Free(void* ptr);
	
	// Return the allocated size of this block
	u32 Size(void* ptr);

public:
	// Constructs a pool allocator with the given size
	explicit HeapAllocator(const char* name, void* base, u32 size_bytes);
	
	// Constructs a stack allocator and uses allocator to allocate stackSize_bytes for its use
	explicit HeapAllocator(const char* name, u32 size_bytes, Allocator* allocator);

protected:
	void* m_pBase;		// pointer to base of allocated memory
	void* m_pLLHead;	// pointer to the first free block (Linked List Head)
};
