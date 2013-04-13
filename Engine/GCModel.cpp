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

	// TODO: Remove all drawtime dependencies on bmd. 
	//		 This information should all be loaded into a GCMaterial at load time
	Material& mat = bmd->mat3.materials[bmd->mat3.indexToMatIndex[matIndex]];
	GCMaterial gcMat = model->m_Materials[bmd->mat3.indexToMatIndex[matIndex]];

	// Shader
	renderer->setShader(gcMat.shader);
	renderer->setVertexFormat(model->hackFullVertFormat);

	renderer->setDepthState(gcMat.depthState);
	renderer->setBlendState(gcMat.blendState);
	renderer->setRasterizerState(gcMat.rasterState);

	// Colors
	renderer->setShaderConstant4f("matColor0", gcMat.matColor[0]);
	renderer->setShaderConstant4f("matColor1", gcMat.matColor[1]);
	renderer->setShaderConstant4f("ambColor0", gcMat.ambColor[0]);
	renderer->setShaderConstant4f("ambColor1", gcMat.ambColor[1]);

	// Textures
	for (uint i = 0; i < 8; i++)
	{
		uint stageIndex = mat.texStages[i];
		if (stageIndex == 0xffff)
			continue;

		uint texIndex = bmd->mat3.texStageIndexToTextureIndex[stageIndex];
		GCTexture tex = model->m_Textures[texIndex];

		memcpy(samplerName, constSamplerName, 9);
		memcpy(textureName, constTextureName, 9);
		samplerName[7] = '0' + i;
		textureName[7] = '0' + i;

		renderer->setSamplerState(samplerName, tex.sampler);
		renderer->setTexture(textureName, tex.tex);
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
			applyMaterial(renderer, matIndex);
			renderer->setVertexBuffer(0, vertexBuffer.id);
			renderer->setIndexBuffer(indexBuffer.id);
			renderer->setShaderConstantArray4x4f("ModelMat", matrixTable, packet->matrixTable.size());
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

#define FULL_VERTEX_ATTRIBS 0x1fff

RESULT buildInflatedVertex(Renderer *renderer, ubyte* dst, Index &point, u16 attribs, Vtx1* vtx)
{
	uint attribSize;
	uint vtxSize = GC3D::GetVertexSize(renderer, FULL_VERTEX_ATTRIBS);

	// Clear the vertex so that all of our missing data is zero'd
	memset(dst, 0, vtxSize);
	
	if ( (attribs & HAS_POSITIONS) == false )
		WARN("Model does not have required attributes");

	// TODO: Fix the uint cast. Perhaps we can keep it as a u16 somehow? Depends on HLSL
	// Always set this attribute. The default is 0.
	// TODO: We can save a uint in the vertex structure if we remove this force
	attribSize = GC3D::GetAttributeSize(renderer, HAS_MATRIX_INDICES);
	if (attribs & HAS_MATRIX_INDICES) {
		ASSERT(point.matrixIndex/3 < 10); 
		uint matrixIndex = point.matrixIndex/3;
		memcpy(dst, &matrixIndex, attribSize);
	}
	else
	{
		memset(dst, 0, attribSize);
	}
	dst += attribSize;
	
	attribSize = GC3D::GetAttributeSize(renderer, HAS_POSITIONS);
	if (attribs & HAS_POSITIONS) {
		memcpy(dst, vtx->positions[point.posIndex], attribSize);
	}
	dst += attribSize;
	
	attribSize = GC3D::GetAttributeSize(renderer, HAS_NORMALS);
	if (attribs & HAS_NORMALS) {
		memcpy(dst, vtx->normals[point.normalIndex], attribSize);
	}
	dst += attribSize;

	for (uint i = 0; i < 2; i++)
	{
		u16 colorAttrib = HAS_COLORS0 << i;
		attribSize = GC3D::GetAttributeSize(renderer, colorAttrib);
		
		if (attribs & colorAttrib) {
			Color c = vtx->colors[i][point.colorIndex[i]];

			ASSERT(sizeof(c.a) == sizeof(dst[0]));
			dst[0] = c.r;
			dst[1] = c.g;
			dst[2] = c.b;
			dst[3] = c.a;
			
			ASSERT(attribSize == sizeof(c.r) + sizeof(c.g) + sizeof(c.b) + sizeof(c.a));
		}
		dst += attribSize;
	}

	for (uint i = 0; i < 8; i++)
	{
		u16 texAttrib = HAS_TEXCOORDS0 << i;
		float* dstFloat = (float*)dst;
		attribSize = GC3D::GetAttributeSize(renderer, texAttrib);

		if (attribs & texAttrib) {
			TexCoord t = vtx->texCoords[i][point.texCoordIndex[i]];
			dstFloat[0] = t.s;
			dstFloat[1] = t.t;
			ASSERT(attribSize == sizeof(t.s) + sizeof(t.t));
		}
		dst += attribSize;
	}

	return S_OK;
}

RESULT buildVertex(Renderer *renderer, ubyte* dst, Index &point, u16 attribs, Vtx1* vtx)
{
	uint attribSize;
	
	// For now, only use position
	if ( (attribs & HAS_POSITIONS) == false )
		WARN("Model does not have required attributes");

	// TODO: Fix the uint cast. Perhaps we can keep it as a u16 somehow? Depends on HLSL
	// Always set this attribute. The default is 0.
	// TODO: We can save a uint in the vertex structure if we remove this force
	attribSize = GC3D::GetAttributeSize(renderer, HAS_MATRIX_INDICES);
	if (attribs & HAS_MATRIX_INDICES) {
		ASSERT(point.matrixIndex/3 < 10); 
		uint matrixIndex = point.matrixIndex/3;
		memcpy(dst, &matrixIndex, attribSize);
	}
	else
	{
		memset(dst, 0, attribSize);
	}
	dst += attribSize;

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
	int vertexSize = GC3D::GetVertexSize(renderer, FULL_VERTEX_ATTRIBS);
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
					buildInflatedVertex(renderer, vertices + vertexSize*index, *point, attribs, &bdl->vtx1);
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

	// For our purposes, a "Texture" is a sampler and texture data pairing
	uint nTextures = m_BDL->tex1.imageHeaders.size();
	m_Textures.resize(nTextures);

	std::vector<SamplerStateID> ssIDs;
	std::vector<TextureID> texIDs;

	ssIDs.resize(m_BDL->tex1.imageHeaders.size());
	for (uint i = 0; i < m_BDL->tex1.imageHeaders.size(); i++)
	{
		ImageHeader* hdr = &m_BDL->tex1.imageHeaders[i];
		ssIDs[i] = GC3D::CreateSamplerState(renderer, hdr);
	}

	texIDs.resize(m_BDL->tex1.images.size());
	for (uint i = 0; i < m_BDL->tex1.images.size(); i++)
	{
		texIDs[i] = GC3D::CreateTexture(renderer, &m_BDL->tex1.images[i]);
		if (texIDs[i] == TEXTURE_NONE)
		{
			// TODO: Assign default texture
			WARN("Failed to load texture data at index %u", i);
			//hr = S_FALSE;
			IFC(E_FAIL);
		}
	}

	for (uint i = 0; i < nTextures; i++)
	{
		ImageHeader* hdr = &m_BDL->tex1.imageHeaders[i];

		strncpy(m_Textures[i].name, hdr->name.c_str(), GCMODEL_NAME_MAX_CHARS);
		m_Textures[i].tex = texIDs[hdr->imageIndex];
		m_Textures[i].sampler = ssIDs[i];
		m_Textures[i].gcFormat = m_BDL->tex1.images[hdr->imageIndex].format;
	}

cleanup:
	if FAILED(r)
	{
		STL_FOR_EACH(tex, texIDs)
		{
			renderer->removeTexture(*tex);
		}
	}

	return r;
}

RESULT GCModel::initMaterials(Renderer* renderer)
{
	uint nMaterials = m_BDL->mat3.materials.size();
	m_Materials.resize(nMaterials);
	
	std::vector<ShaderID> shaders;
	std::vector<std::string> names;
	std::vector<DepthStateID> depthModes;
	std::vector<BlendStateID> blendModes;
	std::vector<RasterizerStateID> cullModes;
	std::vector<float4> matColors;
	std::vector<float4> ambColors;

	// Shaders
	for (uint i = 0; i < nMaterials; i++)
	{
		// TODO: Create default on fail
		shaders.push_back(GC3D::CreateShader(renderer, &m_BDL->tex1, &m_BDL->mat3, i));
	}
	
	// Depth State
	for (uint i = 0; i < m_BDL->mat3.zModes.size(); i++)
	{
		// TODO: Create default on fail
		ZMode gcZMode = m_BDL->mat3.zModes[i];
		depthModes.push_back(GC3D::CreateDepthState(renderer, gcZMode));
	}
	
	// Cull State
	for (uint i = 0; i < m_BDL->mat3.cullModes.size(); i++)
	{
		// TODO: Create default on fail
		uint gcCullMode = m_BDL->mat3.cullModes[i];
		cullModes.push_back(GC3D::CreateRasterizerState(renderer, gcCullMode));
	}

	// Blend State
	for (uint i = 0; i < m_BDL->mat3.blendInfos.size(); i++)
	{
		// TODO: Create default on fail
		BlendInfo gcBlendInfo = m_BDL->mat3.blendInfos[i];
		blendModes.push_back(GC3D::CreateBlendState(renderer, gcBlendInfo));
	}

	// Material Colors
	for (uint i = 0; i < m_BDL->mat3.matColor.size(); i++)
	{
		//TODO: This is only a 32-bit color, but we're packing it into a float4
		// This could be a lot more efficient! (but it is only per material)
		MColor mcol = m_BDL->mat3.matColor[i];
		matColors.push_back(float4(mcol.r/255.0f, mcol.g/255.0f, mcol.b/255.0f, mcol.a/255.0f));
	}

	// Ambient Colors
	for (uint i = 0; i < m_BDL->mat3.ambColor.size(); i++)
	{
		MColor mcol = m_BDL->mat3.ambColor[i];
		ambColors.push_back(float4(mcol.r/255.0f, mcol.g/255.0f, mcol.b/255.0f, mcol.a/255.0f));
	}

	// Fixup our actual materials
	for (uint i = 0; i < nMaterials; i++)
	{
		Material& mat = m_BDL->mat3.materials[i];

		strncpy(m_Materials[i].name, m_BDL->mat3.stringtable[i].c_str(), GCMODEL_NAME_MAX_CHARS);

		m_Materials[i].matColor[0] = matColors[mat.matColor[0]];
		m_Materials[i].matColor[1] = matColors[mat.matColor[1]];
		m_Materials[i].ambColor[0] = ambColors[mat.ambColor[0]];
		m_Materials[i].ambColor[1] = ambColors[mat.ambColor[1]];

		m_Materials[i].shader = shaders[i];
		m_Materials[i].depthState = depthModes[mat.zModeIndex];
		m_Materials[i].rasterState = cullModes[mat.cullIndex];
		m_Materials[i].blendState = blendModes[mat.blendIndex];
	}

	return S_OK;
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

	initTextures(renderer);
	initMaterials(renderer);
		
	// TODO: HACK: Force all vertices to be fully inflated (all attributes) and use a single vert format
	hackFullVertFormat = GC3D::CreateVertexFormat(renderer, FULL_VERTEX_ATTRIBS, m_Materials[0].shader);
	
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