#pragma once

#include <common.h>

struct Chunk;

class Asset 
{
public:
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

class AssetSlot
{
public:
	Asset* pAsset;
	uint refcount;

	AssetSlot() : refcount(0) {};

	void _incRef() {++refcount;}
	
	void _decRef() 
	{	
		assert(refcount > 0);
		// TODO: evict should only be called by Asset Manager. 
		// It should check for assets that can be evicted on every tick or so.
		if(--refcount <= 0) evict();
	}

	RESULT evict()
	{
		RESULT r;
		IFC(pAsset->Unload());
		delete pAsset;
cleanup:
		return r;
	}

	RESULT stuff(Asset* asset, Chunk* chnk)
	{
		pAsset = asset;
		return pAsset->Load(chnk);
	}
};