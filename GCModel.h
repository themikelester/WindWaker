#pragma once

#include "Common\common.h"
#include "Asset.h"
#include "BMDLoader\BMDLoader.h"
#include <Foundation\memory_types.h>
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

class GCModel : public Asset
{
private:
	SceneGraph		m_Scenegraph;
	BModel*			m_BDL;
	
	std::vector<IndexBufferID>	m_IndexBuffers;
	std::vector<VertexBufferID>	m_VertBuffers;
	std::vector<void*>			m_VertexData;
	std::vector<void*>			m_IndexData;
	std::vector<ShaderID>		m_Shaders;
	std::vector<VertexFormatID>	m_VertFormats;

protected:
	RESULT GCModel::Load(Chunk* data);
	RESULT GCModel::Reload();
	RESULT GCModel::Unload();

public:
	~GCModel() 
	{
		for (uint i = 0; i < m_VertexData.size(); i++) {	free(m_VertexData[i]); }
		for (uint i = 0; i < m_IndexData.size(); i++) {	free(m_IndexData[i]); }
	}

	RESULT Init(Renderer *renderer);
	RESULT Draw(Renderer *renderer, ID3D10Device *device);

	int _debugDrawBatch;

private:
	RESULT GCModel::initBatches(Renderer *renderer, const SceneGraph& scenegraph);

	void GCModel::drawScenegraph(Renderer *renderer, ID3D10Device *device, const SceneGraph& s, const mat4& p = identity4(), bool onDown = true, int matIndex = 0);
	void GCModel::drawBatch(Renderer *renderer, ID3D10Device *device, int batchIndex, const mat4 &parentMatrix);
	
	RESULT GCModel::findMatchingIndex(Index &point, int* index);

};