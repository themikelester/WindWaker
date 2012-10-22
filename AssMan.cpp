#include "AssMan.h"
#include "RARCReader\RARCReader.h"
#include "GCModel.h"

static RESULT GetExtension(const char* nodepath, char* &ext)
{
	RESULT r = S_OK;
	ext = PathFindExtension(nodepath) + 1;
	
	if (ext[0] == '\0')
		return E_FAIL;

	return r;
}

RESULT AssetManager::LoadChunk(Chunk* chnk, const char* nodepath, Asset** asset)
{
	RESULT r = S_OK;

	char* ext = NULL;
	if( FAILED( GetExtension(nodepath, ext) ) )
	{ 
		WarningMsg("Invalid extension");
		goto cleanup;
	}

	if (strcmp(ext, "bdl") == 0)
	{
		*asset = new GCModel();
	} else
	{
		return S_FALSE;
	}
	
	m_AssetMap.insert( std::pair<const char*, Asset*> (nodepath, *asset) ); 
	numAssets += 1;

	IFC((*asset)->Init(nodepath));
	IFC((*asset)->Load(chnk));

cleanup:
	free(chnk);
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
// hashkey	[out]	- hashkey of the loaded asset that can be used as an index into the asset table
RESULT AssetManager::Load(Package* pkg, char* nodepath, Asset* pAsset)
{
	RESULT r;

	Chunk* chnk;
	IFC(pkg->Read(nodepath, &chnk));
	IFC(pAsset->Init(nodepath));
	IFC(pAsset->Load(chnk));
	
	m_AssetMap.insert( std::pair<const char*, Asset*> (nodepath, pAsset) ); 
	numAssets += 1;
#ifdef DEBUG
	m_AssetNameMap.insert( std::pair<const char*, Asset*> (nodepath, pAsset) );
#endif

	return S_OK;

cleanup:
	WARN("Failed to load asset %s from package %s", pkg->GetFilename(), nodepath);
	return r;
}

// Load a specific asset from a package into the Asset Manager
// pkg		[in]	- package in which the asset risides. Get this pointer by calling OpenPkg();
// nodepath	[in]	- path to the asset inside the package. E.g. "models/myModel.obj"
// hashkey	[out]	- hashkey of the loaded asset that can be used as an index into the asset table
RESULT AssetManager::Load(Package* pkg, char* nodepath, Asset** ppAsset)
{
	RESULT r = S_OK;
	Chunk* chnk;

	IFC(pkg->Read(nodepath, &chnk));
	IFC(LoadChunk(chnk, nodepath, ppAsset));

cleanup:
	return r;
}

RESULT AssetManager::Load(Package* pkg, int index, Asset** ppAsset)
{
	RESULT r = S_OK;
	const char* nodepath;
	Chunk* chnk;

	IFC(pkg->Read(index, &chnk, &nodepath));
	IFC(LoadChunk(chnk, nodepath, ppAsset));

cleanup:
	return r;
}
	
// Load all assets in a package
// pkg		[in]	- package from which to load assets. Get this pointer by calling OpenPkg();
// numLoaded[out]	- number of assets successfully loaded from the package. If NULL, returns nothing.
// hashkeys [out]	- an array of hashkeys of the newly loaded objects. 
//						If NULL, or if numLoaded is NULL, no hashkeys are returned. 
RESULT AssetManager::LoadAll(Package* pkg, Asset** &ppAssets, int* numLoaded)
{
	RESULT r = S_OK;
	int i;

	for (i = 0; r == S_OK; i++) 
	{
		r = Load(pkg, i, ppAssets);
	}

	*numLoaded = i;

	return S_OK;
}

RESULT Get(int hashkey, Asset** asset);
RESULT Get(char* fileNodePath, Asset** asset);
RESULT Get(char* nodepath, Asset** asset);