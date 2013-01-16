#include "GCModel.h"
#include "util.h"
#include "GC3D.h"

// ----- Static functions ------------------------------------------------------- //
mat4 frameMatrix(const Frame& f);
void adjustMatrix(mat4& mat, u8 matrixType);
mat4 localMatrix(int i, const BModel* bm);
void updateMatrixTable(const BModel* bmd, const Packet& packet, 
					   u8 matrixType, mat4* matrixTable, bool* isMatrixWeighted);
RESULT buildVertex(ubyte* dst, Index &point, u16 attribs, BModel* bdl);
// ------------------------------------------------------------------------------ //

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

void GCModel::drawBatch(Renderer *renderer, ID3D10Device *device, int batchIndex, const mat4 &parentMatrix)
{
#ifdef DEBUG
	// Only draw the selected batch
	if (_debugDrawBatch >= 0 && batchIndex != _debugDrawBatch)
		return;
#endif

	Batch1& batch = m_BDL->shp1.batches[batchIndex];
	mat4 matrixTable[10];
	bool isMatrixWeighted[10];
	
	// Vertex format and buffer are uniform for an entire batch
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	int numIndicesSoFar = 0;
	STL_FOR_EACH(packet, batch.packets)
	{
		// Setup Matrix table
		updateMatrixTable(m_BDL, *packet, batch.matrixType, matrixTable, isMatrixWeighted);	
		
		renderer->reset();
		renderer->setShader( m_Shaders[batchIndex] );
		renderer->setVertexFormat( m_VertFormats[batchIndex] );
		renderer->setVertexBuffer(0, m_VertBuffers[batchIndex]);
		renderer->setIndexBuffer(m_IndexBuffers[batchIndex]);
		renderer->setShaderConstantArray4x4f("ModelMat", matrixTable, packet->matrixTable.size());
		renderer->apply();

		device->DrawIndexed(packet->indexCount, numIndicesSoFar, 0);	
		numIndicesSoFar += packet->indexCount;
	}
}

void GCModel::drawScenegraph(Renderer *renderer, ID3D10Device *device, const SceneGraph& scenegraph, 
							 const mat4& parentMatrix, bool onDown, int matIndex)
{
	mat4 tempMat = parentMatrix;

	switch(scenegraph.type)
	{
	case SG_JOINT:
	{
		const Frame& f = m_BDL->jnt1.frames[scenegraph.index];
		m_BDL->jnt1.matrices[scenegraph.index] = parentMatrix * frameMatrix(f);
		tempMat = m_BDL->jnt1.matrices[scenegraph.index];
		
		// This should probably be here. See definition comment of isMatrixValid
		//m_BDL->jnt1.isMatrixValid[scenegraph.index] = true;
		
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
		
		// TODO: LIMIT BATCH ATTRIBUTES TO POSITION AND MATRICES RIGHT NOW
		batch.attribs &= (HAS_POSITIONS | HAS_MATRIX_INDICES);

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

	// Convert BMD/BDL "Frames" into matrices usable by the renderer
	for (int i = 0; i < m_BDL->jnt1.frames.size(); i++)
	{
		Frame& frame = m_BDL->jnt1.frames[i];
		mat4& mat = m_BDL->jnt1.matrices[i];
		mat = frameMatrix(frame);
	}

	DEBUG_ONLY( _debugDrawBatch = -1 );

	return initBatches(renderer, m_Scenegraph);
}

RESULT GCModel::Draw(Renderer *renderer, ID3D10Device *device)
{
#ifdef DEBUG
	for (uint i = 0; i < m_BDL->jnt1.isMatrixValid.size(); i++)
		m_BDL->jnt1.isMatrixValid[i] = false;
#endif 

	drawScenegraph(renderer, device, m_Scenegraph);

	return S_OK;
}

// Convert a "Frame" into a D3D usable matrix
mat4 frameToMatrix(const Frame& f)
{
  mat4 t, r, s;
  t = translate(f.t);
  //TODO: Double check that this is correct. May need rotateZYX
  r = identity4();//rotateZXY( DEGTORAD(f.rx), DEGTORAD(f.ry), DEGTORAD(f.rz) );
  s = scale(f.sx, f.sy, f.sz);

  //this is probably right this way:
  //return t*rz*ry*rx*s; //scales seem to be local only
  return t*r*s;

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

mat4 frameMatrix(const Frame& f)
{
  mat4 t, rx, ry, rz, s;
  t = translate(f.t);
  rx = rotateX(f.rx/360.f*2*PI);
  ry = rotateY(f.ry/360.f*2*PI);
  rz = rotateZ(f.rz/360.f*2*PI);
  s = scale(f.sx, f.sy, f.sz);

  //this is probably right this way:
  //return t*rz*ry*rx*s; //scales seem to be local only
  return t*rz*ry*rx;

  //experimental:
  if(f.unknown == 0)
    return t*rx*ry*rz;
  else if(f.unknown == 1)
    return t*ry*rz*rx;
  else if(f.unknown == 2)
    return t*rz*ry*rx;
  else
    assert(false);
}

void adjustMatrix(mat4& mat, u8 matrixType)
{
	switch(matrixType)
	{
		case 1: //billboard
			WARN("Billboards not yet supported");
			break;

		case 2: //y billboard
			WARN("Y-Billboards not yet supported");
			break;
	}
}

mat4 localMatrix(int i, const BModel* bm)
{
	DEBUG_ONLY( ASSERT(bm->jnt1.isMatrixValid[i]) );
	mat4 s = scale(bm->jnt1.frames[i].sx, bm->jnt1.frames[i].sy, bm->jnt1.frames[i].sz);

	//TODO: I don't know which of these two return values are the right ones
	//(if it's the first, then what is scale used for at all?)

	//looks wrong in certain circumstances...
	return bm->jnt1.matrices[i]; //this looks better with vf_064l.bdl (from zelda)
	return bm->jnt1.matrices[i]*s; //this looks a bit better with mario's bottle_in animation
}

mat4& mad(mat4& r, const mat4& m, float f)
{
	for(int j = 0; j < 3; ++j)
		for(int k = 0; k < 4; ++k)
			r.rows[j][k] += f*m.rows[j][k];
	return r;
}

void updateMatrixTable(const BModel* bmd, const Packet& packet, u8 matrixType, mat4* matrixTable,
                       bool* isMatrixWeighted)
{
	for(size_t i = 0; i < packet.matrixTable.size(); ++i)
	{
		if(packet.matrixTable[i] != 0xffff) //this means keep old entry
		{
			u16 index = packet.matrixTable[i];
			if(bmd->drw1.isWeighted[index])
			{
				//TODO: the EVP1 data should probably be used here,
				//figure out how this works (most files look ok
				//without this, but models/ji.bdl is for example
				//broken this way)
				//matrixTable[i] = def;
			
				//the following _does_ the right thing...it looks
				//ok for all files, but i don't understand why :-P
				//(and this code is slow as hell, so TODO: fix this)
			
				//NO idea if this is right this way...
				mat4 m(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
				const MultiMatrix& mm = bmd->evp1.weightedIndices[bmd->drw1.data[index]];
				for(size_t r = 0; r < mm.weights.size(); ++r)
				{
					const mat4 evpMat = bmd->evp1.matrices[mm.indices[r]];
					const mat4 localMat = localMatrix(mm.indices[r], bmd);
					mad(m, localMat*evpMat, mm.weights[r]);
				}
				m.rows[3] = vec4(0.0f, 0.0f, 0.0f, 1.0f);

				matrixTable[i] = m;
				if(isMatrixWeighted != NULL)
					isMatrixWeighted[i] = true;
			}
			else
			{
				//ASSERT(bmd->jnt1.isMatrixValid[bmd->drw1.data[index]]);
				matrixTable[i] = bmd->jnt1.matrices[bmd->drw1.data[index]];
								
				if(isMatrixWeighted != NULL)
					isMatrixWeighted[i] = false;
			}
			adjustMatrix(matrixTable[i], matrixType);
		}
	}
}

static RESULT buildVertex(ubyte* dst, Index &point, u16 attribs, BModel* bdl)
{
	// For now, only use position
	if ( (attribs & HAS_POSITIONS) == false )
		WARN("Model does not have required attributes");

	// TODO: Fix the uint cast. Perhaps we can keep it as a u16 somehow? Depends on HLSL
	if (attribs & HAS_MATRIX_INDICES) {
		ASSERT(point.matrixIndex/3 < 10); 
		uint matrixIndex = point.matrixIndex/3;
		memcpy(dst, &matrixIndex, sizeof(uint));
		dst += sizeof(uint);
	}

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