
#include <Foundation\memory.h>
#include "Package.h"
#include "RARCReader\RARCReader.h"

using namespace foundation;

RESULT Package::CopyFileToMem(const char *filename, void** contents)
{
	long size;
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL) return E_INVALIDARG;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	*contents = m_Allocator->allocate(size);
	rewind(fp);
	fread(*contents, size, 1, fp);
	fclose(fp);

	return S_OK;
}

RESULT Package::Open( const char* filename, PkgAccessMode mode )
{
	RESULT r = S_OK;
		
	m_Filename = filename;
	m_AccessMode = mode;
	m_Data = NULL;

	switch (mode)
	{
		case PKG_READ_MEM:	IFC( CopyFileToMem(filename, (void**)&m_Data) );
		case PKG_READ_DISK:	IFC( fopen_s(&m_File, filename, "rb") );
	}
	
	// Use the extention to instanciate the correct package object
	char* ext = PathFindExtension(filename) + 1;
	if( strcmp(ext, "rarc") == 0)
	{
		m_Reader = MAKE_NEW(*m_Allocator, RARCReader);
	} else {
		GTC( E_INVALIDARG );
	}

	m_Reader->Init(m_Data, m_File);

	return r;

cleanup:
	fclose(m_File);
	return r;
}

RESULT Package::Close()
{
	MAKE_DELETE(*m_Allocator, FileReader, m_Reader);

	switch(m_AccessMode)
	{
		case PKG_READ_MEM:  m_Allocator->deallocate(m_Data);
		case PKG_READ_DISK: return (fclose(m_File) == EOF ? E_FAIL : S_OK);
	}

	return S_OK;
}

RESULT Package::Read(const char* nodepath, Chunk** chnk)
{
	return m_Reader->Read(nodepath, chnk);
}

RESULT Package::Read(int nodeIndex, Chunk** ppChnk, const char** nodePath)
{
	return m_Reader->Read(nodeIndex, ppChnk, nodePath);
}