#pragma once

#include "Common\common.h"
#include "Framework3\Renderer.h"
#include "GC3D.h"

// TODO: Remove this
#include <d3d10.h>

struct TextureDesc;
struct Header;
namespace Json {
	class Value;
}

namespace GDModel
{
	struct TextureResource
	{
		char name[32];
		SamplerStateID samplerStateIndex;
		TextureID textureIndex;
	};

	struct DrwElement
	{
		u16 index;
		bool isWeighted;
	};

	struct JointElement
	{
		mat4 matrix;
		char name[16];
		u16 parent;
	};
	
	struct WeightedIndex
	{
		float weight;
		uint  index;
	};

	enum SgNodeType {
		SG_END		= 0x00,
		SG_DOWN		= 0x01,
		SG_UP		= 0x02,
		SG_JOINT	= 0x10,
		SG_MATERIAL	= 0x11,
		SG_PRIM		= 0x12
	};
	
	enum VertexAttributeFlags
	{
		HAS_MATRIX_INDICES	= 1 << 0,
		HAS_POSITIONS		= 1 << 1,
		HAS_NORMALS			= 1 << 2,
		HAS_COLORS0			= 1 << 3,
		HAS_COLORS1			= 1 << 4,
		HAS_TEXCOORDS0		= 1 << 5,
		HAS_TEXCOORDS1		= 1 << 6,
		HAS_TEXCOORDS2		= 1 << 7,
		HAS_TEXCOORDS3		= 1 << 8,
		HAS_TEXCOORDS4		= 1 << 9,
		HAS_TEXCOORDS5		= 1 << 10,
		HAS_TEXCOORDS6		= 1 << 11,
		HAS_TEXCOORDS7		= 1 << 12,
	};

	struct Scenegraph
	{
		u16 index;
		u16 type; //One of SgNodeType
	};
	
	// These are only needed at load time, we should find a way to remove them
	struct TemporaryGFXData
	{
		uint nVertexIndexBuffers;
		ubyte* vertexIndexBuffers;

		uint nSamplerStates;
		GC3D::SamplerState* samplerStates;

		uint nTextures;
		TextureDesc* textures;

		ubyte* textureData;
	};

	typedef ubyte ModelAsset;

	struct GDModel
	{
		ModelAsset* _asset;

		Scenegraph* scenegraph;
		uint* batchOffsetTable; // Batch* batch3 = _asset + batchOffsetTable[3];
		
		u16 numJoints;
		JointElement* jointTable;

		DrwElement* drwTable;
		mat4*  evpMatrixTable;
		u8*	   evpWeightedIndexSizesTable;
		u16*   evpWeightedIndexOffsetTable;
		WeightedIndex* evpWeightedIndexTable;

		// TODO: This is only temporary until we start loading animations
		// Store a copy of our original joints 
		JointElement* emptyAnim;
		
		// TODO: Temprorary for testing
		ShaderID shaderID;
		VertexFormatID vertFormat;

		// This is set on load/reload, and tells the next draw call 
		//		to load/reload all the GPU assets that we own
		bool loadGPU; 
		TemporaryGFXData gfxData;
	};
	
	
	RESULT Update(GDModel* model);

	RESULT Draw(Renderer* renderer, ID3D10Device* device, GDModel* model);
	
	//Given a JSON object, compile a binary blob that can be loaded with Load() and Reload()
	RESULT Compile(const Json::Value& root, Header& hdr, char** data);

	//Save our asset reference and initialize the model in the renderer
	RESULT Load(GDModel* model, ModelAsset* blob);
	
	//Unregister our old asset with the renderer. Save our new reference.
	//Initialize our new asset with the renderer. The asset manager will then delete the old asset.
	RESULT Reload(GDModel* model, ModelAsset* blob);
}