#pragma once

#include <Types.h>

class Allocator
{
protected:
	const char* m_Name;

public:
	// TODO: Add alignment parameter to alloc
	virtual void*  Alloc (u32 size)	 = 0;
	virtual void   Free	 (void* ptr) = 0;
	virtual u32	   Size  (void* ptr) = 0;

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
            deallocate(p);
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
	// Constructs a stack allocator with the given size
	explicit StackAllocator(const char* name, void* base, u32 stackSize_bytes);

	// Clears the entire stack (rolls back to zero)
	void Clear();

protected:
	u8* m_pBase;
	u8* m_pTop;
	u8* m_pEnd;
};

class PoolAllocator : public Allocator
{
public: // Allocator methods
	// Allocates a new block from the pool
	void* Alloc(u32 unnused);

	// Rolls the stack back to a previous marker
	void Free(void* ptr);

	u32 Size(void* ptr);

public:
	// Constructs a pool allocator with the given size
	explicit PoolAllocator(const char* name, void* base, u32 sizeOfElement, u32 numElements);
	
protected:
	typedef void* Ptr;

	void* m_pBase;		// pointer to base of allocated memory
	void* m_pLLHead;	// pointer to the first free block (Linked List Head)
};
