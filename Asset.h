#pragma once

#include <common.h>

struct Chunk;

class Asset 
{
public:
	// A virtual destructor so that we can call ~Asset() during unload without needing the real type
	virtual ~Asset(){};

	virtual RESULT Init(const char* nodePath) {
		nodepath = nodePath;
		return S_OK;
	}
	virtual RESULT Load(Chunk* data) = 0;
	virtual RESULT Reload() = 0;
	virtual RESULT Unload() = 0;

	const char* nodepath;

protected:
	int ID;
	int hashkey;

	char* filename;

private:
};

