#pragma once

#include <common.h>
#include <Foundation\collection_types.h>
#include <Foundation\murmur_hash.h>
#include <Foundation\hash.h>
#include <Foundation\memory_types.h>
#include "Package.h"
#include "Asset.h"
#include "AssetPtr.h"

class AssetManager
{
public:
	AssetManager() {};

	RESULT Init();
	RESULT Shutdown();

	RESULT Init();
	RESULT Shutdown();

	RESULT OpenPkg(char* filename, Package** pkg);
	RESULT ClosePkg(Package* pkg);
	
	RESULT Unload(Asset* pAsset);

	// Load all assets in a package
	// pkg		[in]	- package from which to load assets. Get this pointer by calling OpenPkg();
	// numLoaded[out]	- number of assets successfully loaded from the package. If NULL, returns nothing.
	RESULT LoadAll(Package* pkg, int* numLoaded = NULL);

	// Load a specific asset from a package into the Asset Manager
	// pkg		[in]	- package in which the asset risides. Get this pointer by calling OpenPkg();
	// nodepath	[in]	- path to the asset inside the package. E.g. "models/myModel.obj"
	RESULT Load(Package* pkg, int startIndex, int numAssets = 1);
	RESULT Load(Package* pkg, char* nodepath);
	
	RESULT AssetManager::Unload(Package* pkg, char* nodepath);

	template <typename AssetType> RESULT Get(const char* nodepath, AssetPtr<AssetType>* assetPtr)
	{
		u64 key = foundation::murmur_hash_64(nodepath, strlen(nodepath), hashSeed);
		AssetSlot* slot = foundation::hash::get(*m_AssetMap, key, (AssetSlot*)NULL);
		if (slot == NULL)
		{
			WARN("Attempting to get asset %s which does not exit", nodepath);
			return E_FAIL;
		}

		new (assetPtr) AssetPtr<AssetType>(slot);
		return S_OK;
	}

protected:
	RESULT LoadChunk(Chunk* chnk, const char* nodepath);
	
	foundation::Allocator* m_AssetAllocator;

	static const u64 hashSeed = 0xAAFFBBCC;
	foundation::Hash<AssetSlot*> *m_AssetMap;

	// Temporary solution for now. Should be dyn allocated from a pool later.
	AssetSlot m_Assets[6];
};
