#pragma once

#include <common.h>
#include <stdio.h>

struct Chunk;

enum PkgAccessMode
{
	PKG_READ_MEM,	// Read the entire file into memory and perform individual node reads from memory
	PKG_READ_DISK,	// Read nothing initially, perform individual node reads from disk
	PKG_WRITE,		// Unnused for now
};

class Package
{
protected:
	const char* m_Filename;
	char* m_Data;
	FILE* m_File;

	PkgAccessMode m_AccessMode;

public:	

	// Opens a package file and maintains a file pointer until close is called
	//	
	//	filename[IN]	- Directory + filename of the package file (relative to running dir)
	//	pkg		[OUT]	- A new package reader object that is ready to access internal files
	//	mode	[IN]	- (Optional) File mode (read, write) to open the file in. See the PkgAccessMode enum
	static RESULT Open(const char* filename, Package** pkg, PkgAccessMode mode = PKG_READ_MEM);

	// Close the pkg for reading/writing and cleanup
	//
	// [None]
	virtual RESULT Close ();
	
	// Load the data from a specific package node into memory as a "Chunk"
	// When the chunk is destroyed, the memory will be freed.
	//	
	//	nodepath	[IN]  - Path of the file inside the archive. E.g. "Models/MyModel.obj"
	//	chnk		[OUT] - A new chunk will be allocated in this position. Must be freed by caller
	virtual RESULT Read(const char* nodepath, Chunk** chnk) = 0;

	// Access and load a package node by its index. 
	//	
	//	nodeIndex	[IN]  - Index of the node to access. Indexing scheme depends on package implementation.
	//	chnk		[OUT] - A new chunk will be allocated in this position. Must be freed by caller
	//	nodepath	[OUT] - (Optional) Path of the file inside the archive. E.g. "Models/MyModel.obj"
	virtual RESULT Read(int nodeIndex, Chunk** ppChnk, const char** nodePath = NULL) = 0;

	const char* GetFilename() { return m_Filename; }
};
