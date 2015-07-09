#pragma once

#include "Common\common.h"
#include "Framework3\Renderer.h"
#include "GC3D.h"
#include "GDAnim.h"

// TODO: Remove this
#include <d3d10.h>

struct TextureResource;
struct TextureDesc;
struct BlendMode;
struct DepthMode;
struct MaterialInfo;
struct DrwElement;
struct Scenegraph;
struct JointElement;
struct DrwElement;
struct WeightedIndex;

struct Header;
namespace Json {
	class Value;
}

namespace GDModel
{	
	// These are only needed at load time, we should find a way to remove them
	struct TemporaryGFXData
	{
		Renderer* renderer;

		uint nVertexIndexBuffers;
		ubyte* vertexIndexBuffers;

		uint nTextureResources;
		// TODO: This should be GDModel::SamplerState
		TextureResource* textureResources;

		uint nTextures;
		TextureDesc* textures;

		ubyte* textureData;

		uint nBlendModes;
		BlendMode* blendModes;

		uint nDepthModes;
		DepthMode* depthModes;
		
		uint nCullModes;
		u8* cullModes;

		uint nShaders;
		uint* vsOffsets;
		uint* psOffsets;
		char* vsShaders;
		char* psShaders;
	};

	typedef ubyte* ModelAsset;

	struct GDModel
	{
		ModelAsset _asset;

		Scenegraph* scenegraph;
		uint* batchOffsetTable; // Batch* batch3 = _asset + batchOffsetTable[3];
		
		u16 nMaterials;
		MaterialInfo* materials;
		
		u16 numJoints;
		JointElement* jointTable;

		DrwElement* drwTable;
		mat4*  evpMatrixTable;
		u8*	   evpWeightedIndexSizesTable;
		u16*   evpWeightedIndexOffsetTable;
		WeightedIndex* evpWeightedIndexTable;

		// This is set on load/reload, and tells the next draw call 
		//		to load/reload all the GPU assets that we own
		bool loadGPU; 
		TemporaryGFXData gfxData;
	};
	
	
	RESULT Update(GDModel* model, GDAnim::GDAnim* anim, float time);

	RESULT Draw(Renderer* renderer, GDModel* model);
	
	//Given a JSON object, compile a binary blob that can be loaded with Load() and Reload()
	RESULT Compile(const Json::Value& root, Header& hdr, char** data);

	//Save our asset reference and initialize the model in the renderer
	RESULT Load(GDModel* model, ModelAsset blob);
	
	//Unregister our old asset with the renderer. Delete our asset reference
	RESULT Unload(GDModel* model);

	//Unregister our old asset with the renderer. Save our new reference.
	//Initialize our new asset with the renderer. The asset manager will then delete the old asset.
	RESULT Reload(GDModel* model, ModelAsset* blob);
}