#pragma once

#include <cstdio>
#include <cstdlib>
#include <string>
#include <direct.h>
#include <hash_map>
#include <iostream>
#include <assert.h>
#include <common.h>
#include <gcio.h>

// TODO: fix this weird header file dependency
#include "../engine/Package.h"

namespace RARC {
	struct RarcHeader
	{
	  char type[4]; //'RARC'
	  u32 size; //size of the file
	  u32 unknown;
	  u32 dataStartOffset; //where does the actual data start?
	  u32 unknown2[4];
  
	  u32 numNodes;
	  u32 unknown3[2];
	  u32 fileEntriesOffset;
	  u32 unknown4;
	  u32 stringTableOffset; //where is the string table stored?
	  u32 unknown5[2];
	};

	struct Node
	{
	  char type[4];
	  u32 filenameOffset; //directory name, offset into string table
	  u16 unknown;
	  u16 numFileEntries; //how many files belong to this node?
	  u32 firstFileEntryOffset;
	};

	struct FileEntry
	{
	  u16 id; //file id. If this is 0xFFFF, then this entry is a subdirectory link
	  u16 unknown;
	  u16 unknown2;
	  u16 filenameOffset; //file/subdir name, offset into string table
	  u32 dataOffset; //offset to file data (for subdirs: index of Node representing the subdir)
	  u32 dataSize; //size of data
	  u32 zero; //seems to be always '0'
	};
}

class RARCReader : public FileReader
{
public:
	RESULT Init (void* data, FILE* f);
	RESULT Read (const char* nodeName, Chunk** chnk);
	RESULT Read (int nodeIndex, Chunk** ppChnk, const char** nodepath);

protected:
	RESULT Index ();
	RESULT Index (int nodeIndex, std::string parentName);
	RESULT GetFileEntry (int index, RARC::FileEntry* file);
	RESULT GetNode (int index, RARC::Node* n);
	RESULT GetString (int index, std::string* str);
	RESULT GetData (int offset, int size, void* buf);

private:
	RARC::RarcHeader m_Hdr;

	FILE* m_File;
	char* m_Data;

	// Maps between node names and file entry indices
	std::hash_map <std::string, int> m_TOC;		   // Table of Contents
	std::vector < std::pair<int, std::string> > m_RTOC; // Reverse Table of Contents
};