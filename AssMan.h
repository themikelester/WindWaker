#pragma once

#include <common.h>
#include <hash_map>
#include "Package.h"
#include "Asset.h"

#ifdef DEBUG
	#include <map>
#endif

class AssetManager
{
public:
	unsigned int numAssets;

public:
	AssetManager() : numAssets(0){};

	RESULT OpenPkg(char* filename, Package** pkg);
	RESULT ClosePkg(Package* pkg);

	// Load a specific asset from a package into the Asset Manager
	// pkg		[in]	- package in which the asset risides. Get this pointer by calling OpenPkg();
	// nodepath	[in]	- path to the asset inside the package. E.g. "models/myModel.obj"
	// hashkey	[out]	- hashkey of the loaded asset that can be used as an index into the asset table
	RESULT Load(Package* pkg, char* nodepath, Asset** ppAsset);
	RESULT Load(Package* pkg, char* nodepath, Asset* ppAsset);
	RESULT Load(Package* pkg, int index, Asset** ppAsset);
	
	// Load all assets in a package
	// pkg		[in]	- package from which to load assets. Get this pointer by calling OpenPkg();
	// numLoaded[out]	- number of assets successfully loaded from the package. If NULL, returns nothing.
	// hashkeys [out]	- an array of hashkeys of the newly loaded objects. 
	//						If NULL, or if numLoaded is NULL, no hashkeys are returned. 
	RESULT LoadAll(Package* pkg, Asset** &ppAssets, int* numLoaded = NULL);

	RESULT Get(int hashkey, Asset** asset);
	RESULT Get(char* fileNodePath, Asset** asset);

protected:
	std::hash_map <std::string, Asset*> m_AssetMap;

protected:
	RESULT LoadChunk(Chunk* chnk, const char* nodepath, Asset** asset);

#ifdef DEBUG
protected:
	std::map <std::string, Asset*> m_AssetNameMap;
#endif
};
