#include "GCModel.h"
#include "util.h"
#include "GC3D.h"

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
	delete m_BDL;
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

void GCModel::drawBatch(Renderer *renderer, ID3D10Device *device, int batchIndex, const mat4 &parentMatrix)
{
	Batch1& batch = m_BDL->shp1.batches[batchIndex];

	// Vertex format and buffer are uniform for an entire batch
	renderer->reset();
	renderer->setShader( m_Shaders[batchIndex] );
	renderer->setVertexFormat( m_VertFormats[batchIndex] );
	renderer->setVertexBuffer(0, m_VertBuffers[batchIndex]);
	renderer->setIndexBuffer(m_IndexBuffers[batchIndex]);
	renderer->setShaderConstant1f("scale", 1.0f);
	renderer->apply();

	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
	
	int numIndicesSoFar = 0;
	STL_FOR_EACH(packet, batch.packets)
	{
		// Setup Matrix table and apply billboarding to matrices here
	
		device->DrawIndexed(packet->indexCount, numIndicesSoFar, 0);	
		numIndicesSoFar = packet->indexCount;
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

static RESULT buildVertex(ubyte* dst, Index &point, u16 attribs, BModel* bdl)
{
	// For now, only use position
	if ( attribs != HAS_POSITIONS )
		WARN("Model does not have required attributes");

	if (attribs & HAS_POSITIONS) {
		memcpy(dst, bdl->vtx1.positions[point.posIndex], sizeof(float3));
		dst += sizeof(float3);
	}

	if (attribs & HAS_NORMALS) {
		memcpy(dst, bdl->vtx1.normals[point.normalIndex], sizeof(float3));
		dst += sizeof(float3);
	}

	return S_OK;
}

RESULT GCModel::initBatches(Renderer *renderer, const SceneGraph& scenegraph)
{
	int numBatches = m_BDL->shp1.batches.size();

	m_VertexData.resize(numBatches);
	m_IndexData.resize(numBatches);
	m_VertBuffers.resize(numBatches);
	m_IndexBuffers.resize(numBatches);
	m_Shaders.resize(numBatches);
	m_VertFormats.resize(numBatches);

	// Create a vertex and index buffer for every batch in this model
	STL_FOR_EACH(node, m_BDL->inf1.scenegraph)
	{
		if (node->type != SG_PRIM) continue;

		int batchIndex = node->index;
		Batch1& batch = m_BDL->shp1.batches[batchIndex];
		
		// FORCE BATCH ATTRIBUTES TO BE ONLY VERTICES RIGHT NOW
		batch.attribs = HAS_POSITIONS;

		int pointCount = 0;
		int primCount = 0;

		// Count "points" in this batch
		// A point is a struct of indexes that point to each attribute of the 3D vertex
		// pointCount represents the maximum number of vertices needed. If we find dups, there may be less.
		// TODO: Move this to loading code
		STL_FOR_EACH(packet, batch.packets)
		{
			STL_FOR_EACH(prim, packet->primitives)
			{
				switch(prim->type)
				{
					case PRIM_TRI_STRIP: 
						pointCount += prim->points.size();
						primCount += 1; 
						break;

					case PRIM_TRI_FAN: 
						WARN("Triangle fans have been deprecated in D3D10. Won't draw!"); 
						continue;

					default:
						WARN("unknown primitive type %x", prim->type);
						continue;
				}
			}
		}

		// Create vertex buffer for this batch based on available attributes
		int vertexSize = GC3D::GetVertexSize(renderer, batch.attribs);
		int bufferSize = pointCount * vertexSize; // we may not need all this space, see pointCount above
		
		ubyte* vertexBuffer = (ubyte*)malloc( bufferSize );
		u16* indexBuffer = (u16*)malloc( (pointCount + primCount) * sizeof(u16));
		
		// Interlace each attribute into a single vertex stream 
		// (may be duplicates because it's using indices into the vtx1 buffer) 
		int indexCount = 0;
		int vertexCount = 0; 
		int packetIndexOffset = 0;
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
					indexBuffer[indexCount++] = vbIndex;
				}

				// Add a strip-cut index to reset to a new triangle strip
				indexBuffer[indexCount++] = STRIP_CUT_INDEX;
			}
			
			// Might as well remove the last strip-cut index
			indexCount -= 1;

			packet->indexCount = indexCount - packetIndexOffset;
			packetIndexOffset = indexCount;
		}
		
		// Register our vertex buffer
		VertexBufferID VBID = renderer->addVertexBuffer(bufferSize, STATIC, vertexBuffer);
		IndexBufferID IBID =  renderer->addIndexBuffer(indexCount, 2, STATIC, indexBuffer);

		// Save data so that we can use/free it later
		m_VertexData[batchIndex] = vertexBuffer;
		m_IndexData[batchIndex] = indexBuffer;
		m_VertBuffers[batchIndex] = VBID;
		m_IndexBuffers[batchIndex] = IBID;

		// Load the vertex format and shader for this vertex type (so it doesn't have to load at draw time)
		m_Shaders[batchIndex] = GC3D::GetShader(renderer, batch.attribs);
		m_VertFormats[batchIndex] = GC3D::GetVertexFormat(renderer, batch.attribs, m_Shaders[batchIndex]);
	}

	return S_OK;
}

RESULT GCModel::Init(Renderer *renderer)
{	
	// First things first
	buildSceneGraph(m_BDL->inf1, m_Scenegraph);
	return initBatches(renderer, m_Scenegraph);
}

RESULT GCModel::Draw(Renderer *renderer, ID3D10Device *device)
{
	drawScenegraph(renderer, device, m_Scenegraph);

	return S_OK;
}
