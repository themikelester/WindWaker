#pragma once

#include <common.h>
#include <Foundation\memory.h>

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

class AssetSlot
{
public:
	Asset* pAsset;
	uint refcount;

	AssetSlot(foundation::Allocator* allocator) : refcount(0), alctr(allocator) {};
	~AssetSlot() {assert(pAsset == NULL);} // Assert that our asset has been evicted before we clear this slot

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
		MAKE_DELETE(*alctr, Asset, pAsset);
		pAsset = NULL;
cleanup:
		return r;
	}

	RESULT stuff(Asset* asset, Chunk* chnk)
	{
		pAsset = asset;
		return pAsset->Load(chnk);
	}

private:
	AssetSlot();
	foundation::Allocator* alctr;
};