
#include <Foundation\memory.h>
#include "Package.h"
#include "RARCReader\RARCReader.h"

using namespace foundation;

static RESULT copyFileToMem(const char *filename, void** contents)
{
	Allocator* allocator = &memory_globals::default_allocator();

	long size;
	FILE *fp = fopen(filename, "rb");
	if(fp == NULL) return E_INVALIDARG;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	*contents = allocator->allocate(size);
	rewind(fp);
	fread(*contents, size, 1, fp);
	fclose(fp);

	return S_OK;
}

RESULT Package::Open( const char* filename, Package** pkg, PkgAccessMode mode )
{
	RESULT r = S_OK;
	FILE* f;
	void* data = NULL;
			
	switch (mode)
	{
		case PKG_READ_MEM:	IFC( copyFileToMem(filename, &data) );
		case PKG_READ_DISK:	IFC( fopen_s(&f, filename, "rb") );
	}
	
	// Use the extention to instanciate the correct package object
	char* ext = PathFindExtension(filename) + 1;
	if( strcmp(ext, "rarc") == 0)
	{
		*pkg = new RARCReader(data, f, filename);
	} else {
		GTC( E_INVALIDARG );
	}

	return r;

cleanup:
	fclose(f);
	return r;
}

RESULT Package::Close()
{
	Allocator* allocator = &memory_globals::default_allocator();

	switch(m_AccessMode)
	{
		case PKG_READ_MEM:  allocator->deallocate(m_Data);
		case PKG_READ_DISK: fclose(m_File);
	}

	return S_OK;
}