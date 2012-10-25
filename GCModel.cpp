#include "GCModel.h"
#include "util.h"

RESULT GCModel::Load(Chunk* data) 
{
	m_BDL = loadBmd(data);	
	return (m_BDL == NULL ? E_FAIL : S_OK);
}

RESULT GCModel::Reload()
{	
	return S_OK;
}
	
RESULT GCModel::Unload() 
{	
	free(m_BDL);
	return S_OK;
}

// TODO: This should be done at load time!!
// Convert a "Frame" into a D3D usable matrix
mat4 frameMatrix(const Frame& f)
{
  mat4 t, r, s;
  t = translate(f.t);
  //TODO: Double check that this is correct. May need rotateZYX
  r = rotateZXY( DEGTORAD(f.rx), DEGTORAD(f.ry), DEGTORAD(f.rz) );
  s = scale(f.sx, f.sy, f.sz);

  //this is probably right this way:
  //return t*rz*ry*rx*s; //scales seem to be local only
  return t*r;

  //experimental: 
  /*
  if(f.unknown == 0)
    return t*rx*ry*rz;
  else if(f.unknown == 1)
    return t*ry*rz*rx;
  else if(f.unknown == 2)
    return t*rz*ry*rx;
  else
    assert(false);
  */
}

#define VERTEX_SIZE(x) 2*sizeof(float3);
#define VERTEX_FORMAT(x) m_VertexFormat

void GCModel::drawBatch(Renderer *renderer, ID3D10Device *device, int batchIndex, const mat4 &parentMatrix)
{
	Batch1& batch = m_BDL->shp1.batches[batchIndex];

	// Vertex format and buffer are uniform for an entire batch
	renderer->changeVertexFormat( VERTEX_FORMAT(batch.attribs) );
	renderer->changeVertexBuffer(0, m_VertBuffers[batch.batchID]);
	renderer->changeIndexBuffer(m_IndexBuffers[batch.batchID]);

	// TODO: Make a shader for each vertex format
	//renderer->setShader( SHADER( currBatch.attribs ) );

	int numIndices = 0;
	int numIndicesSoFar = 0;
	STL_FOR_EACH(packet, batch.packets)
	{
		// Setup Matrix table and apply billboarding to matrices here
		STL_FOR_EACH(prim, packet->primitives)
		{
			switch(prim->type)
			{
				case PRIM_TRI_STRIP: device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP); break;
				case PRIM_TRI_FAN: warn("Triangle fans have been deprecated in D3D10. Won't draw!"); return;
				default:
					warn("unknown primitive type %x", prim->type);
					continue;
			}
			
			numIndices = prim->points.size();
			device->DrawIndexed(numIndices, numIndicesSoFar, 0);
			numIndicesSoFar += numIndices;
		}
	}
}

void GCModel::drawScenegraph(Renderer *renderer, ID3D10Device *device, const SceneGraph& scenegraph, const mat4& parentMatrix, bool onDown, int matIndex)
{
	mat4 tempMat;

	switch(scenegraph.type)
	{
	case SG_JOINT:
	{
		const Frame& f = m_BDL->jnt1.frames[scenegraph.index];
		m_BDL->jnt1.matrices[scenegraph.index] = parentMatrix * frameMatrix(f);
		tempMat = m_BDL->jnt1.matrices[scenegraph.index];
		break;
	}

	case SG_MATERIAL:
	{
		//applyMaterial(s.index, m_BDL, *g_oglBlock);
		matIndex = scenegraph.index;
		onDown = ( m_BDL->mat3.materials[m_BDL->mat3.indexToMatIndex[matIndex]].flag == 1 );
		break;
	}

	case SG_PRIM:
	{
		if (onDown) 
		{
			//TODO: it's not sure that all matrices required by this call
			//are already calculated...
			//applyMaterial(matIndex, m, *m.oglBlock);
			drawBatch(renderer, device, scenegraph.index, tempMat);
		} 
	}
	}

	for(size_t i = 0; i < scenegraph.children.size(); ++i)
		drawScenegraph(renderer, device, scenegraph.children[i], tempMat, onDown, matIndex);

	if (scenegraph.type == SG_PRIM && !onDown) 
	{
		//applyMaterial(matIndex, m, *m.oglBlock);
		drawBatch(renderer, device, scenegraph.index, tempMat);
	}
}


// If the indices in "point" match a vertex in our vertex buffer, return the index of that vert
//
// point [in]	- an Index struct with which to search
// index [out]	- the index of the vertex that matches
// RETURN		- S_OK if a match was found, S_FALSE otherwise
RESULT GCModel::findMatchingIndex(Index &point, int* index)
{
	return S_FALSE;
}

static RESULT buildVertex(ubyte* dst, Index &point, Attributes &attribs, BModel* bdl)
{
	// For now, only use position and normal
	ASSERT(attribs.hasPositions && attribs.hasNormals);

	if(attribs.hasPositions) {
		memcpy(dst, bdl->vtx1.positions[point.posIndex], sizeof(float3));
		dst += sizeof(float3);
	}

	if(attribs.hasNormals) {
		memcpy(dst, bdl->vtx1.normals[point.normalIndex], sizeof(float3));
		dst += sizeof(float3);
	}

	return S_OK;
}

RESULT GCModel::initBatches(Renderer *renderer, const SceneGraph& scenegraph)
{
	// A UID for batches so that we can access the right batch data when drawing a specific batch
	// The scenegraph may be traversed in different order between init and draw time, so we can't trust the natural order
	// Hence we use this UID
	int batchID = 0;

	// Create a vertex and index buffer for every batch in this model
	STL_FOR_EACH(node, m_BDL->inf1.scenegraph)
	{
		if (node->type != SG_PRIM) continue;

		Batch1& batch = m_BDL->shp1.batches[node->index];
		batch.batchID = batchID++;

		int vertexCount = 0; 
		int pointCount = 0;

		// Count "points" in this batch
		// A point is a struct of indexes that point to each attribute of the 3D vertex
		// pointCount represents the maximum number of vertices needed. If we find dups, there may be less.
		// TODO: Move this to loading code
		STL_FOR_EACH(packet, batch.packets)
		{
			STL_FOR_EACH(prim, packet->primitives)
			{
				pointCount += prim->points.size();
			}
		}

		// Create vertex buffer for this batch based on available attributes
		int vertexSize = VERTEX_SIZE(currBatch.attribs);
		int bufferSize = pointCount * vertexSize; // we may not need all this space, see pointCount above
		
		ubyte* vertexBuffer = (ubyte*)malloc( bufferSize );
		u16* indexBuffer = (u16*)malloc( pointCount * sizeof(u16) );
		
		// Interlace each attribute into a single vertex stream 
		// (may be duplicates because it's using indices into the vtx1 buffer) 
		pointCount = 0;
		vertexCount = 0;
		STL_FOR_EACH(packet, batch.packets)
		{
			STL_FOR_EACH(prim, packet->primitives)
			{
				STL_FOR_EACH(point, prim->points)
				{
					int vbIndex = -1;
					if( findMatchingIndex(*point, &vbIndex) == S_FALSE )
					{
						// Build the vertex and add it to the buffer
						ubyte* nextVertex = &vertexBuffer[vertexSize * vertexCount];
						buildVertex(nextVertex, *point, batch.attribs, m_BDL);
						
						vertexCount += 1;
						vbIndex = vertexCount - 1;
					}

					// Set the index into our vertex buffer for this point
					indexBuffer[pointCount] = vbIndex;
					pointCount += 1;
				}
			}
		}
		
		// Register our vertex buffer
		VertexBufferID VBID = renderer->addVertexBuffer(bufferSize, STATIC, vertexBuffer);
		IndexBufferID IBID =  renderer->addIndexBuffer(pointCount, 2, STATIC, indexBuffer);

		// Save data so that we can use/free it later
		m_VertexData.push_back(vertexBuffer);
		m_IndexData.push_back(indexBuffer);
		m_VertBuffers.push_back(VBID);
		m_IndexBuffers.push_back(IBID);
	}

	return S_OK;
}

RESULT GCModel::Init(Renderer *renderer, VertexFormatID vertFormat)
{
	m_VertexFormat = vertFormat;
	
	// First things first
	buildSceneGraph(m_BDL->inf1, m_Scenegraph);
	initBatches(renderer, m_Scenegraph);
	
	struct Vertex
	{
		float3 Position;
		float3 Normal;
	};
	
	//if ((m_VertBuffer = renderer->addVertexBuffer(sizeof(Vertex) * m_NumVerts, STATIC, vertices)) == VB_NONE) return false;

Cleanup:
	return S_OK;
}

RESULT GCModel::Draw(Renderer *renderer, ID3D10Device *device)
{
	drawScenegraph(renderer, device, m_Scenegraph);

	return S_OK;
}
