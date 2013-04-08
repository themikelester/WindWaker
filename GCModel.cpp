#include "GCModel.h"
#include "util.h"
#include "GC3D.h"
#include "Foundation\hash.h"
#include "Foundation\murmur_hash.h"

// ----- Static functions ------------------------------------------------------- //
mat4 frameMatrix(const Frame& f);
void adjustMatrix(mat4& mat, u8 matrixType);
mat4 localMatrix(int i, const BModel* bm);
void updateMatrixTable(const BModel* bmd, const Packet& packet, 
					   u8 matrixType, mat4* matrixTable, bool* isMatrixWeighted);
RESULT buildVertex(Renderer *renderer, ubyte* dst, Index &point, u16 attribs, Vtx1* vtx);

// ----- Static initilizations ---------------------------------------------------//
DEBUG_ONLY(int GCModel::_debugDrawBatch = -1);
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
	RESULT r = S_OK;
	delete m_BDL;
	
	STL_FOR_EACH(batch, m_Batches)
	{
		RESULT res = batch->Shutdown();
		// Explicitly ignore r, attempt to shutdown all batches even if one fails
		if ( FAILED(res) ) r = res;
	}

	return r;
}

void GCBatch::applyMaterial(Renderer* renderer, int matIndex)
{
	static const char constSamplerName[] = "SamplerI";
	static const char constTextureName[] = "TextureI";
	
	char samplerName[9];
	char textureName[9];

	Material& mat = bmd->mat3.materials[bmd->mat3.indexToMatIndex[matIndex]];

	for (uint i = 0; i < 8; i++)
	{
		uint stageIndex = mat.texStages[i];
		if (stageIndex == 0xffff)
			continue;

		uint texIndex = bmd->mat3.texStageIndexToTextureIndex[stageIndex];

		memcpy(samplerName, constSamplerName, 9);
		memcpy(textureName, constTextureName, 9);
		samplerName[7] = '0' + i;
		textureName[7] = '0' + i;

		//TODO: we'll need separate indexes once we stop creating a texture for every sampler
		renderer->setSamplerState(samplerName, model->m_Samplers[texIndex]);
		renderer->setTexture(textureName, model->m_Textures[texIndex]);

		ImageHeader& tex = bmd->tex1.imageHeaders[texIndex];
	}
}

RESULT GCBatch::Draw(Renderer *renderer, ID3D10Device *device, const mat4 &parentMatrix, int matIndex)
{
#ifdef _DEBUG
	// Only draw the selected batch
	if (GCModel::_debugDrawBatch >= 0 && batchIndex != GCModel::_debugDrawBatch)
		return S_OK;
#endif

	mat4 matrixTable[10];
	bool isMatrixWeighted[10];
	
	// Vertex format and buffer are uniform for an entire batch
	device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

	int numIndicesSoFar = 0;
	STL_FOR_EACH(packet, packets)
	{
		// Setup Matrix table
		updateMatrixTable(bmd, *packet, matrixType, matrixTable, isMatrixWeighted);	
		
		renderer->reset();
			renderer->setShader(shader);
			renderer->setVertexFormat(vertexFormat);
			renderer->setVertexBuffer(0, vertexBuffer.id);
			renderer->setIndexBuffer(indexBuffer.id);
			renderer->setShaderConstantArray4x4f("ModelMat", matrixTable, packet->matrixTable.size());
			applyMaterial(renderer, matIndex);
		renderer->apply();

		device->DrawIndexed(packet->indexCount, numIndicesSoFar, 0);	
		numIndicesSoFar += packet->indexCount;
	}

	return S_OK;
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
			m_Batches[scenegraph.index].Draw(renderer, device, tempMat, matIndex);
		} 
	}
	}

	for(size_t i = 0; i < scenegraph.children.size(); ++i)
		drawScenegraph(renderer, device, scenegraph.children[i], tempMat, onDown, matIndex);

	if (scenegraph.type == SG_PRIM && !onDown) 
	{
		m_Batches[scenegraph.index].Draw(renderer, device, tempMat, matIndex);
	}
}

RESULT buildVertex(Renderer *renderer, ubyte* dst, Index &point, u16 attribs, Vtx1* vtx)
{
	uint attribSize;
	
	// For now, only use position
	if ( (attribs & HAS_POSITIONS) == false )
		WARN("Model does not have required attributes");

	// TODO: Fix the uint cast. Perhaps we can keep it as a u16 somehow? Depends on HLSL
	if (attribs & HAS_MATRIX_INDICES) {
		attribSize = GC3D::GetAttributeSize(renderer, HAS_MATRIX_INDICES);
		ASSERT(point.matrixIndex/3 < 10); 
		uint matrixIndex = point.matrixIndex/3;
		memcpy(dst, &matrixIndex, attribSize);
		dst += attribSize;
	}

	if (attribs & HAS_POSITIONS) {
		attribSize = GC3D::GetAttributeSize(renderer, HAS_POSITIONS);
		memcpy(dst, vtx->positions[point.posIndex], attribSize);
		dst += attribSize;
	}

	if (attribs & HAS_NORMALS) {
		attribSize = GC3D::GetAttributeSize(renderer, HAS_NORMALS);
		memcpy(dst, vtx->normals[point.normalIndex], attribSize);
		dst += attribSize;
	}

	for (uint i = 0; i < 2; i++)
	{
		u16 colorAttrib = HAS_COLORS0 << i;

		if (attribs & colorAttrib) {
			Color c = vtx->colors[i][point.colorIndex[i]];

			ASSERT(sizeof(c.a) == sizeof(dst[0]));
			dst[0] = c.r;
			dst[1] = c.g;
			dst[2] = c.b;
			dst[3] = c.a;
			
			attribSize = GC3D::GetAttributeSize(renderer, colorAttrib);
			ASSERT(attribSize == sizeof(c.r) + sizeof(c.g) + sizeof(c.b) + sizeof(c.a));
			dst += attribSize;
		}
	}

	for (uint i = 0; i < 8; i++)
	{
		u16 texAttrib = HAS_TEXCOORDS0 << i;
		float* dstFloat = (float*)dst;

		if (attribs & texAttrib) {
			TexCoord t = vtx->texCoords[i][point.texCoordIndex[i]];
			dstFloat[0] = t.s;
			dstFloat[1] = t.t;
			
			attribSize = GC3D::GetAttributeSize(renderer, texAttrib);
			ASSERT(attribSize == sizeof(t.s) + sizeof(t.t));
			dst += attribSize;
		}
	}

	return S_OK;
}

RESULT GCBatch::Init(uint index, BModel *bdl, Renderer *renderer, GCModel* model)
{
	// Initialize members that need initializing
	this->model = model;
	indexMap = new foundation::Hash<u16>(foundation::memory_globals::default_allocator());

	Batch1* batch = &bdl->shp1.batches[index];

	attribs = batch->attribs;
	batchIndex = index;
	bmd = bdl;
	packets = batch->packets;
	matrixType = batch->matrixType;

	// Load the vertex format and shader for this vertex type (so it doesn't have to load at draw time)
	shader = GC3D::GetShader(renderer, attribs);
	vertexFormat = GC3D::GetVertexFormat(renderer, attribs, shader);
	
	int pointCount = 0;
	int primCount = 0;

	// Count "points" in this batch
	// A point is a struct of indexes that point to each attribute of the 3D vertex
	// pointCount represents the maximum number of vertices needed. If we find dups, there may be less.
	// TODO: Move this to loading code
	STL_FOR_EACH(packet, packets)
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
	int vertexSize = GC3D::GetVertexSize(renderer, attribs);
	int bufferSize = pointCount * vertexSize; // we may not need all this space, see pointCount above
		
	ubyte*	vertices = (ubyte*)malloc( bufferSize );
	u16*	indices = (u16*)malloc( (pointCount + primCount) * sizeof(u16));
		
	// Interlace each attribute into a single vertex stream 
	// (may be duplicates because it's using indices into the vtx1 buffer) 
	int indexCount = 0;
	int vertexCount = 0; 
	int packetIndexOffset = 0;
	STL_FOR_EACH(packet, packets)
	{
		STL_FOR_EACH(prim, packet->primitives)
		{
			STL_FOR_EACH(point, prim->points)
			{
				u16 index; 
				static const uint64_t seed = 101;

				// Add the index of this vertex to our index buffer (every time)
				uint64_t hashKey = foundation::murmur_hash_64(point._Ptr, sizeof(Index), seed);
				if (foundation::hash::has(*indexMap, hashKey))
				{
					// An equivalent vertex already exists, use that index
					index = foundation::hash::get(*indexMap, hashKey, u16(0));
				} else {
					// This points to a new vertex. Construct it.
					index = vertexCount++;
					buildVertex(renderer, vertices + vertexSize*index, *point, attribs, &bdl->vtx1);
					foundation::hash::set(*indexMap, hashKey, index);
				}

				// Always add a new index to our index buffer
				indices[indexCount++] = index;
			}

			// Add a strip-cut index to reset to a new triangle strip
			indices[indexCount++] = STRIP_CUT_INDEX;
		}
			
		// Might as well remove the last strip-cut index
		indexCount -= 1;

		packet->indexCount = indexCount - packetIndexOffset;
		packetIndexOffset = indexCount;
	}
		
	// Register our vertex buffer
	vertexBuffer.id = renderer->addVertexBuffer(vertexCount * vertexSize, STATIC, vertices);
	vertexBuffer.data = vertices;
	vertexBuffer.count = vertexCount;

	indexBuffer.id = renderer->addIndexBuffer(indexCount, 2, STATIC, indices);
	indexBuffer.data = indices;
	indexBuffer.count = indexCount;

	return S_OK;
}

RESULT GCBatch::Shutdown()
{
	RESULT r = S_OK;

	delete indexMap;

	return r;
}


RESULT GCModel::initTextures(Renderer *renderer)
{
	HRESULT r = S_OK;

	// TODO: We don't need to create a new texture every time, because sometimes the imgHdr->data pointers will be duplicates
	STL_FOR_EACH(imgHdr, m_BDL->tex1.imageHeaders)
	{
		TextureID tid = GC3D::CreateTexture(renderer, imgHdr->data);
		if (tid == TEXTURE_NONE)
		{
			// TODO: Assign default texture
			WARN("Failed to load texture");
			//hr = S_FALSE;
			IFC(E_FAIL);
		}

		SamplerStateID ssid = GC3D::CreateSamplerState(renderer, imgHdr._Ptr);
		if (ssid == SS_NONE)
		{
			// TODO: Assign default sampler
			WARN("Failed to create sampler state");
			//hr = S_FALSE;
			IFC(E_FAIL);
		}

		m_Textures.push_back(tid);
		m_Samplers.push_back(ssid);
	}
cleanup:
	if FAILED(r)
	{
		STL_FOR_EACH(tex, m_Textures)
		{
			renderer->removeTexture(*tex);
		}
	}

	return r;
}

RESULT GCModel::Init(Renderer *renderer)
{	
	RESULT r = S_OK; 

	// First things first
	buildSceneGraph(m_BDL->inf1, m_Scenegraph);

	// Convert BMD/BDL "Frames" into matrices usable by the renderer
	for (uint i = 0; i < m_BDL->jnt1.frames.size(); i++)
	{
		Frame& frame = m_BDL->jnt1.frames[i];
		mat4& mat = m_BDL->jnt1.matrices[i];
		mat = frameMatrix(frame);
	}

	DEBUG_ONLY( GCModel::_debugDrawBatch = -1 );

	// Load textures
	initTextures(renderer);

	// Init all batches
	int numBatches = m_BDL->shp1.batches.size();
	m_Batches.resize(numBatches);

	STL_FOR_EACH(node, m_BDL->inf1.scenegraph)
	{
		if (node->type != SG_PRIM) continue;

		int batchIndex = node->index;
		IFC( m_Batches[batchIndex].Init(batchIndex, m_BDL, renderer, this) );
	}

cleanup:
	return r;
}

RESULT GCModel::Draw(Renderer *renderer, ID3D10Device *device)
{
#ifdef _DEBUG
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
  r = identity4();//rotateZXY( DEGTORAD(f.rx), DEGTORAD(f.ry), DjgvEGTORAD(f.rz) );
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
	//DEBUG_ONLY( ASSERT(bm->jnt1.isMatrixValid[i]) );
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

// TODO: Remove BModel as a dependency. Convert frames to matrices at load time and never look back
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