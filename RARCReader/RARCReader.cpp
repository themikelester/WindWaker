//version 1.0 (20050213)
//by thakis

#include "RARCReader.h"

using namespace std;
using namespace RARC;

RESULT RARCReader::GetString(int index, string* str)
{
	RESULT r = S_OK;
	int t = ftell(m_File);

	fseek(m_File, m_Hdr.stringTableOffset + 0x20 + index, SEEK_SET);

	char c;
	while((c = fgetc(m_File)) != '\0')
	str->append(1, c);

	fseek(m_File, t, SEEK_SET);

	return r;
}

RESULT RARCReader::GetData(int offset, int size, void* buf)
{
	RESULT r = S_OK;
	fseek(m_File, offset + m_Hdr.dataStartOffset + 0x20, SEEK_SET);

	int numRead = fread(buf, 1, size, m_File);
	if (numRead != size)
		r = E_FAIL;

	return r;
}

RESULT RARCReader::GetNode(int index, Node* n)
{
	RESULT r = S_OK;

	fseek(m_File, sizeof(RarcHeader) + index*sizeof(Node), SEEK_SET);
	fread(n, 1, sizeof(Node), m_File);

	toDWORD(n->filenameOffset);
	toWORD(n->unknown);
	toWORD(n->numFileEntries);
	toDWORD(n->firstFileEntryOffset);

	return r;
}

RESULT RARCReader::GetFileEntry(int index, FileEntry* file)
{
	RESULT r = S_OK;

	fseek(m_File, m_Hdr.fileEntriesOffset + index*sizeof(FileEntry) + 0x20, SEEK_SET);
	fread(file, 1, sizeof(FileEntry), m_File);

	toWORD(file->id);
	toWORD(file->unknown);
	toWORD(file->unknown2);
	toWORD(file->filenameOffset);
	toDWORD(file->dataOffset);
	toDWORD(file->dataSize);
	toDWORD(file->zero);

	return r;
}

RESULT RARCReader::Index (int nodeIndex, string parentName)
{
	RESULT r = S_OK;
	Node node;
	string nodeName;
	
	GetNode(nodeIndex, &node);
	GetString(node.filenameOffset, &nodeName);

	for(int i = 0; i < node.numFileEntries; ++i)
	{
		FileEntry curr;
		int fileEntryIndex = node.firstFileEntryOffset + i;
		GetFileEntry(fileEntryIndex, &curr);

		if(curr.id == 0xFFFF) //subdirectory
		{
			if(curr.filenameOffset != 0 && curr.filenameOffset != 2) //don't go to "." and ".."
				Index(curr.dataOffset, parentName + nodeName);
		}
		else //file
		{
			string fullName;
			string currName;
			GetString(curr.filenameOffset, &currName);

			fullName = parentName + "/" + nodeName + "/" + currName;

			m_TOC.insert( pair<const char*, int>  ( fullName.c_str(), fileEntryIndex) );
			m_RTOC.push_back( pair<int, const char*> ( fileEntryIndex, fullName.c_str()) );
		}
	}

	return r;
}

RESULT RARCReader::Index ()
{
	RESULT r = S_OK;
	Node node;
	string nodeName;

	// RARCs always have one root node named "archive"
	GetNode(0, &node);
	GetString(node.filenameOffset, &nodeName);
	assert(nodeName.compare("archive") == 0);

	for(int i = 0; i < node.numFileEntries; ++i)
	{
		FileEntry curr;
		int fileEntryIndex = node.firstFileEntryOffset + i;
		GetFileEntry(fileEntryIndex, &curr);

		if(curr.id == 0xFFFF) //subdirectory
		{
			if(curr.filenameOffset != 0 && curr.filenameOffset != 2) //don't go to "." and ".."
				Index(curr.dataOffset, "");
		}
		else //file
		{
			ErrorMsg("Invalid RARC format");
		}
	}

	return r;
}

RESULT RARCReader::Read(const char* nodeName, Chunk** ppChnk)
{
	RESULT r = S_OK;
	FileEntry fileEntry;
	int fileIndex;
	
	fileIndex = m_TOC[nodeName];
	GetFileEntry(fileIndex, &fileEntry);
	
	*ppChnk = new Chunk(fileEntry.dataSize);
	GetData(fileEntry.dataOffset, fileEntry.dataSize, (*ppChnk)->base);
		
	return r;
}

RESULT RARCReader::Read(int nodeIndex, Chunk** ppChnk, const char** nodepath)
{
	RESULT r = S_OK;
	FileEntry fileEntry;

	if ((unsigned int)nodeIndex >= m_TOC.size())
		return E_INVALIDARG;
		
	GetFileEntry(m_RTOC[nodeIndex].first, &fileEntry);
	
	*ppChnk = new Chunk(fileEntry.dataSize);
	GetData(fileEntry.dataOffset, fileEntry.dataSize, (*ppChnk)->base);

	*nodepath = m_RTOC[nodeIndex].second.c_str();
		
	return r;
}

/*
RESULT RARCReader::ReadAll(Chunk** &ppChnk, char** &nodepaths, int* numFiles)
{
	RESULT r = S_OK;
	FileEntry fileEntry;
	int fileIndex;
	
	int filecount = m_TOC.size();
	ppChnk	  = (Chunk**)malloc(sizeof(Chunk*) * filecount);
	nodepaths = (char**) malloc(sizeof(char*)  * filecount);
	for( int i = 0; i < filecount; i++ )
	{
		GetFileEntry(i, &fileEntry);
		ppChnk[i] = new Chunk(fileEntry.dataSize);
		GetData(fileEntry.dataOffset, fileEntry.dataSize, (*ppChnk)->base);
		GetString(fileEntry.filenameOffset, &(nodepaths[i]));
	}

	*numFiles = filecount;
	
	return r;
}
*/

RARCReader::RARCReader( void* data, FILE* f, const char* filename )
{
	m_Filename = filename;
	
	if (data == NULL)
		m_File = f;
	else
		m_Data = (char*)data;

	// read header
	fread(&m_Hdr, 1, sizeof(m_Hdr), f);
	toDWORD(m_Hdr.size);
	toDWORD(m_Hdr.unknown);
	toDWORD(m_Hdr.dataStartOffset);
	for(int i = 0; i < 4; ++i)
	toDWORD(m_Hdr.unknown2[i]);

	toDWORD(m_Hdr.numNodes);
	toDWORD(m_Hdr.unknown3[0]);
	toDWORD(m_Hdr.unknown3[1]);
	toDWORD(m_Hdr.fileEntriesOffset);
	toDWORD(m_Hdr.unknown4);
	toDWORD(m_Hdr.stringTableOffset);
	toDWORD(m_Hdr.unknown5[0]);
	toDWORD(m_Hdr.unknown5[1]);

	// Build the Table of Contents
	Index();
}

RARCReader::~RARCReader()
{
	fclose(m_File);
}
