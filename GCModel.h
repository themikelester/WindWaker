#pragma once

#include "Common\common.h"
#include "Asset.h"
#include "BMDLoader\BMDLoader.h"
#include <Foundation\memory_types.h>
#include <Foundation\collection_types.h>
#include <Framework3\Direct3D10\Direct3D10Renderer.h>

class Renderer;

class ModelManager
{
public:
	void init();
	void shutdown();

	static foundation::Allocator* getAllocator() {return _allocator;}

protected:
	static foundation::Allocator* _allocator;
};

class GCModel;

struct GCBatch 
{
	struct GCIndexBuffer
	{
		IndexBufferID	id;		// ID of buffer in rendering engine
		u16*			data;	// pointer to actual data stored in buffer... 
								//		Debug only, a copy is made so this is not needed.
		uint			count;	// number of elements in buffer
	};

	struct GCVertexBuffer
	{
		VertexBufferID	id;		// ID of buffer in rendering engine
		ubyte*			data;	// pointer to actual data stored in buffer... 
								//		Debug only, a copy is made so this is not needed.
		uint			count;	// number of vertices in buffer
	};
	
	foundation::Hash<u16>* indexMap;
	
	// TODO: Remove BModel as a dependency. Convert frames to matrices at load time and never look back
	BModel*			bmd;

	u16				attribs;	// flags describing layout and size of each vertex. See BatchAttributeFlags
	u8				matrixType; // standard, billboard, y-billboard...
	std::vector<Packet> packets;
	
	GCIndexBuffer	indexBuffer;
	GCVertexBuffer	vertexBuffer;
	ShaderID		shader;
	VertexFormatID	vertexFormat;

	GCModel*		model;

	// Debug
	uint batchIndex; // Batch index in relation to the whole bmodel
	
	RESULT Init(uint index, BModel *bdl, Renderer *renderer, GCModel* model);
	RESULT Shutdown();
	RESULT Draw(Renderer *renderer, ID3D10Device *device, const mat4 &parentMatrix, int matIndex);

	void applyMaterial(Renderer* renderer, int matIndex);

};

class GCModel : public Asset
{
	friend struct GCBatch;

private:
	SceneGraph		m_Scenegraph;
	BModel*			m_BDL;

	std::vector<GCBatch> m_Batches;
	std::vector<TextureID> m_Textures;
	std::vector<SamplerStateID> m_Samplers;

protected:
	RESULT GCModel::Load(Chunk* data);
	RESULT GCModel::Reload();
	RESULT GCModel::Unload();

public:
	GCModel() : m_BDL(NULL) {}
	~GCModel() 
	{
		for (uint i = 0; i < m_Batches.size(); i++) 
		{	
			free(m_Batches[i].vertexBuffer.data); 
			free(m_Batches[i].indexBuffer.data); 
		}
	}

	RESULT Init(Renderer *renderer);
	RESULT Draw(Renderer *renderer, ID3D10Device *device);

	DEBUG_ONLY(static int _debugDrawBatch);

private:
	void GCModel::drawScenegraph(Renderer *renderer, ID3D10Device *device, const SceneGraph& s, const mat4& p = identity4(), bool onDown = true, int matIndex = 0);
	RESULT GCModel::initTextures(Renderer *renderer);
};