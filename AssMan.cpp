#include "AssMan.h"
#include "RARCReader\RARCReader.h"
#include "Foundation\hash.h"
#include "Foundation\murmur_hash.h"
#include <Foundation\memory.h>
#include "GCModel.h"

RESULT AssetManager::Init()
{
	m_AssetAllocator = &foundation::memory_globals::default_allocator();
	m_AssetMap = new foundation::Hash<AssetSlot*>(*m_AssetAllocator);
	return S_OK;
}

RESULT AssetManager::Shutdown()
{
	if (m_AssetMap->_data._size > 0)
	{
		// For now, slots handle the evicting of their assets when their refcount hits 0.
		// This should change so that we check slots every frame and evict if refcount is 0.
		// We also need to remove the corresponding entry in our map. For now we do it here.
		int SlotsThatShouldHaveBeenFreedButArent = 0;
		auto entry = foundation::hash::begin(*m_AssetMap);
		do
		{
			if (entry->value->refcount == 0)
				SlotsThatShouldHaveBeenFreedButArent++;
			else
				entry->value->evict();

			// Free all of our leftover slots
			MAKE_DELETE(*m_AssetAllocator, AssetSlot, entry->value);

			if (entry->next == ~0)
				break;
			else
				entry = &m_AssetMap->_data[entry->next];
		} while (true);

		// Anything left over in AssetMap is an asset we didn't unload()
		assert(m_AssetMap->_data._size == SlotsThatShouldHaveBeenFreedButArent);
	}

	delete m_AssetMap;
	return S_OK;
}

static RESULT GetExtension(const char* nodepath, char* &ext)
{
	RESULT r = S_OK;
	ext = PathFindExtension(nodepath) + 1;
	
	if (ext[0] == '\0')
		return E_FAIL;

	return r;
}

static RESULT createAsset(Chunk* chnk, foundation::Allocator* alctr, 
						  const char* nodepath, Asset** ppAsset)
{
	RESULT r = S_OK;
	char* ext = NULL;

	IFWC( GetExtension(nodepath, ext), "Failed attempting to create node %s", nodepath );

	if (strcmp(ext, "bdl") == 0)
	{
		*ppAsset = MAKE_NEW(*alctr, GCModel);
	} else
	{
		r = E_FAIL;
	}

cleanup:
	return r;
}

RESULT AssetManager::LoadChunk(Chunk* chnk, const char* nodepath)
{
	RESULT r = S_OK;
	Asset* asset;
	AssetSlot* slot;

	// Create an empty Asset of the correct type
	IFC(createAsset(chnk, m_AssetAllocator, nodepath, &asset)); 
	IFC(asset->Init(nodepath));
	
	// Create a new Slot for our new asset
	slot = MAKE_NEW(*m_AssetAllocator, AssetSlot, m_AssetAllocator);

	// Stuff the asset into our slot and feed it its data
	slot->stuff(asset, chnk);

	// Don't lose track of our Asset!
	u64 key = foundation::murmur_hash_64(nodepath, strlen(nodepath), hashSeed);
	foundation::hash::set(*m_AssetMap, key, slot);

cleanup:
	if (FAILED(r))
		WARN("Failed to load asset %s", nodepath);
	return r;
}

RESULT AssetManager::OpenPkg(char* filename, Package** pkg)
{
	RESULT r = S_OK;

	return Package::Open(filename, pkg, PKG_READ_DISK);
}


RESULT AssetManager::ClosePkg(Package* pkg)
{
	free(pkg);

	return S_OK;
}

// Load a specific asset from a package into the Asset Manager
// pkg		[in]	- package in which the asset risides. Get this pointer by calling OpenPkg();
// nodepath	[in]	- path to the asset inside the package. E.g. "models/myModel.obj"
RESULT AssetManager::Load(Package* pkg, char* nodepath)
{
	RESULT r;
	Chunk* chnk;

	// Read the raw data from memory into a Chunk
	IFC(pkg->Read(nodepath, &chnk));
	LoadChunk(chnk, nodepath);

cleanup:
	delete chnk;
	return r;
}

RESULT AssetManager::Load(Package* pkg, int startIndex, int numAssets)
{
	RESULT r = S_OK;
	const char* nodepath;

	for (int i = startIndex; i < numAssets; ++i)
	{
		Chunk* chnk;
		IFC(pkg->Read(i, &chnk, &nodepath));
		IFC(LoadChunk(chnk, nodepath));
		delete chnk;
	}

cleanup:
	return r;
}

RESULT AssetManager::Unload(Package* pkg, char* nodepath)
{
	RESULT r = S_OK;
	AssetSlot* slot;

	u64 key = foundation::murmur_hash_64(nodepath, strlen(nodepath), hashSeed);
	slot = foundation::hash::get(*m_AssetMap, key, (AssetSlot*)NULL);
	if (slot == NULL)
	{
		WARN("Attempted to unload %s which is not currently loaded", nodepath);
		return E_FAIL;
	}

	// Remove the asset (cleanly) from memory
	IFC(slot->evict());

	// Do not delete the slot, leave it open until its refcount is 0

	// Remove the asset from our map
	foundation::hash::remove(*m_AssetMap, key);

cleanup:
	return r;
}