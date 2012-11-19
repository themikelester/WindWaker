#pragma once

#include <stdio.h>
#include <common.h>
#include <Foundation\memory_types.h>

struct Chunk;

enum PkgAccessMode
{
	PKG_READ_MEM,	// Read the entire file into memory and perform individual node reads from memory
	PKG_READ_DISK,	// Read nothing initially, perform individual node reads from disk
	PKG_WRITE,		// Unnused for now
};

class FileReader
{
public:
	virtual ~FileReader(){}; // Virtual destructor so that we call the child destructor
	virtual RESULT Init(void* data, FILE* f) = 0;
	virtual RESULT Read(const char* nodepath, Chunk** chnk) = 0;
	virtual RESULT Read(int nodeIndex, Chunk** ppChnk, const char** nodePath = NULL) = 0;
};

class Package
{
public:	
	Package (foundation::Allocator* allocator) : m_Allocator(allocator) {};

	// Opens a package file and maintains a file pointer until close is called
	//	
	//	filename[IN]	- Directory + filename of the package file (relative to running dir)
	//	mode	[IN]	- (Optional) File mode (read, write) to open the file in. See the PkgAccessMode enum
	RESULT Open(const char* filename, PkgAccessMode mode = PKG_READ_MEM);

	// Close the pkg for reading/writing and cleanup
	//
	// [None]
	RESULT Close();
	
	// Load the data from a specific package node into memory as a "Chunk"
	// When the chunk is destroyed, the memory will be freed.
	//	
	//	nodepath	[IN]  - Path of the file inside the archive. E.g. "Models/MyModel.obj"
	//	chnk		[OUT] - A new chunk will be allocated in this position. Must be freed by caller
	RESULT Read(const char* nodepath, Chunk** chnk);

	// Access and load a package node by its index. 
	//	
	//	nodeIndex	[IN]  - Index of the node to access. Indexing scheme depends on package implementation.
	//	chnk		[OUT] - A new chunk will be allocated in this position. Must be freed by caller
	//	nodepath	[OUT] - (Optional) Path of the file inside the archive. E.g. "Models/MyModel.obj"
	RESULT Read(int nodeIndex, Chunk** ppChnk, const char** nodePath = NULL);

	const char* GetFilename() { return m_Filename; }

protected:
	RESULT CopyFileToMem(const char *filename, void** contents);
	
protected:
	const char* m_Filename;
	char* m_Data;
	FILE* m_File;

	foundation::Allocator* m_Allocator;
	FileReader* m_Reader;

	PkgAccessMode m_AccessMode;

};
