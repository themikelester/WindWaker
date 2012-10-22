#pragma once

#include "Asset.h"
#include "BMDLoader\BMDLoader.h"
#include <Framework3\Util\Model.h>
#include <Framework3\Direct3D10\Direct3D10Renderer.h>

class Renderer;

class GCModel : public Asset
{
private:
	BModel* model;
	size_t m_NumVerts;
	VertexFormatID m_VertFormat;
	VertexBufferID m_VertBuffer;

protected:
	RESULT GCModel::Load(Chunk* data);
	RESULT GCModel::Reload();
	RESULT GCModel::Unload();

public:
	RESULT Init(Renderer *renderer, VertexFormatID vertFormat);
	RESULT Draw(Renderer *renderer, ID3D10Device *device);
};