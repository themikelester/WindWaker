#include "GCModel.h"

RESULT GCModel::Load(Chunk* data) 
{
	model = loadBmd(data);	
	return (model == NULL ? E_FAIL : S_OK);
}

RESULT GCModel::Reload()
{	
	return S_OK;
}
	
RESULT GCModel::Unload() 
{	
	free(model);
	return S_OK;
}

RESULT GCModel::Init(Renderer *renderer, VertexFormatID vertFormat)
{
	m_VertFormat = vertFormat;

	struct Vertex
	{
		float3 Position;
		float3 Normal;
	};

	m_NumVerts = model->vtx1.positions.size();
	int numNormals = model->vtx1.normals.size();
	Vertex *vertices = (Vertex*)malloc(m_NumVerts * sizeof(Vertex));

	for (int i = 0; i < m_NumVerts; i++)
	{
		vertices[i].Position = model->vtx1.positions[i];
		
		// Some models have fewer normals than verts?
		if (i < numNormals)
			vertices[i].Normal = model->vtx1.normals[i];			
	}

	if ((m_VertBuffer = renderer->addVertexBuffer(sizeof(Vertex) * m_NumVerts, STATIC, vertices)) == VB_NONE) return false;

Cleanup:
	return S_OK;
}

RESULT GCModel::Draw(Renderer *renderer, ID3D10Device *device)
{
	ASSERT(m_VertBuffer != VB_NONE);

	renderer->changeVertexFormat(m_VertFormat);
	renderer->changeVertexBuffer(0, m_VertBuffer);

	//renderer->drawElements(PRIM_POINTS, 0, 0, 0, m_NumVerts);
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);
	device->Draw(m_NumVerts, 0);

	return S_OK;
}