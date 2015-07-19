#include "Framework3\Renderer.h"
#include "GDModel.h"
#include "GC3D.h"
#include "util.h"
#include "BMDRead\bmdread.h"

#define READ(type) *(type*)head; head += sizeof(type);
#define READ_ARRAY(type, count) (type*)head; head += sizeof(type) * count;

#define MAX_NAME_LENGTH 16

struct VertexBuffer 
{
	u16 vertexAttributes;
	u16 vertexCount;
	ubyte* vertexBuf;
};

struct IndexBuffer
{
	u16 indexCount;
	u16* indexBuf;
};

struct Point
{
	u16 mtxIdx;
	u16 posIdx;
	u16 nrmIdx;
	u16 clrIdx[2];
	u16 texIdx[8];
};

struct _Packet
{
	u16 indexCount;
	u16 matrixCount;
	u16* matrixIndices;
};

struct _Batch
{
	VertexBufferID vbID;
	IndexBufferID ibID;
	VertexFormatID vfID;
	u16 numPackets;
	_Packet* packets;
};

struct TextureResource
{
	char name[MAX_NAME_LENGTH];
	u16 texIndex;

	Filter filter;
	AddressMode wrapS;
	AddressMode wrapT;
};

struct TextureDesc
{
	FORMAT format;
	uint width;
	uint height;
	uint numMips;

	uint sizeBytes;
	uint texDataOffset;
	u8* imgData;
};

struct MaterialInfo
{
	ShaderID shader;
	DepthStateID depthMode;
	BlendStateID blendMode;
	RasterizerStateID rasterMode;
		
	TextureID textures[8];
	SamplerStateID samplers[8];
};

struct DepthMode
{
	bool testEnable;
	bool writeEnable;
	u8 func;
};

struct BlendMode
{
	u8 blendOp;
	u8 srcFactor;
	u8 dstFactor;
};

struct DrwElement
{
	u16 index;
	bool isWeighted;
};

struct JointElement
{
	mat4 matrix;
	char name[MAX_NAME_LENGTH];
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

void loadFrame(const Frame& frame, mat4* matrix)
{
	mat4 t, rx, ry, rz, s;

	t = translate(frame.t[0], frame.t[1], frame.t[2]);

	rx = rotateX(frame.rx / 360.0 * 2 * PI);
	ry = rotateY(frame.ry / 360.0 * 2 * PI);
	rz = rotateZ(frame.rz / 360.0 * 2 * PI);

	s = scale(frame.sx, frame.sy, frame.sz);

	if (s != identity4())
		WARN("This model is using joint scaling. This is not yet implemented!\n");

	//return t*rz*ry*rx*s; //scales seem to be local only
	*matrix = t*rz*ry*rx;
}

u16 loadAttribs(const Attributes& attribs)
{
	u16 attribFlags = 0;
	if (attribs.hasMatrixIndices) { attribFlags |= HAS_MATRIX_INDICES; }
	if (attribs.hasPositions) { attribFlags |= HAS_POSITIONS; }
	if (attribs.hasNormals) { attribFlags |= HAS_NORMALS; }
	if (attribs.hasColors[0]) { attribFlags |= HAS_COLORS0; }
	if (attribs.hasColors[1]) { attribFlags |= HAS_COLORS1; }
	if (attribs.hasTexCoords[0]) { attribFlags |= HAS_TEXCOORDS0; }
	if (attribs.hasTexCoords[1]) { attribFlags |= HAS_TEXCOORDS1; }
	if (attribs.hasTexCoords[2]) { attribFlags |= HAS_TEXCOORDS2; }
	if (attribs.hasTexCoords[3]) { attribFlags |= HAS_TEXCOORDS3; }
	if (attribs.hasTexCoords[4]) { attribFlags |= HAS_TEXCOORDS4; }
	if (attribs.hasTexCoords[5]) { attribFlags |= HAS_TEXCOORDS5; }
	if (attribs.hasTexCoords[6]) { attribFlags |= HAS_TEXCOORDS6; }
	if (attribs.hasTexCoords[7]) { attribFlags |= HAS_TEXCOORDS7; }
	return attribFlags;
}

RESULT buildVertex(ubyte* dst, const Index& point, u16 attribs, const Vtx1& vtx)
{
	uint attribSize;
	uint vtxSize = GC3D::GetVertexSize(attribs);

	ASSERT(attribs & HAS_POSITIONS);

	// TODO: We can save a uint in the vertex structure if we remove this force
	// Always set this attribute. The default is 0.
	attribSize = GC3D::GetAttributeSize(HAS_MATRIX_INDICES);
	if (attribs & HAS_MATRIX_INDICES) {
		ASSERT(point.matrixIndex / 3 < 10);
		uint matrixIndex = point.matrixIndex / 3;
		memcpy(dst, &matrixIndex, attribSize);
	}
	else
	{
		memset(dst, 0, attribSize);
	}
	dst += attribSize;

	attribSize = GC3D::GetAttributeSize(HAS_POSITIONS);
	if (attribs & HAS_POSITIONS) {
		Vector3f pos = vtx.positions[point.posIndex];
		memcpy(dst, &pos, attribSize);
		dst += attribSize;
	}

	attribSize = GC3D::GetAttributeSize(HAS_NORMALS);
	if (attribs & HAS_NORMALS) {
		Vector3f nrm = vtx.normals[point.normalIndex];
		memcpy(dst, &nrm, attribSize);
		dst += attribSize;
	}

	for (uint i = 0; i < 2; i++)
	{
		u16 colorAttrib = HAS_COLORS0 << i;
		attribSize = GC3D::GetAttributeSize(colorAttrib);

		if (attribs & colorAttrib) {
			Color color = vtx.colors[i][point.colorIndex[i]];
			memcpy(dst, &color, attribSize);
			ASSERT(attribSize == sizeof(color));
			dst += attribSize;
		}
	}

	for (uint i = 0; i < 8; i++)
	{
		u16 texAttrib = HAS_TEXCOORDS0 << i;
		attribSize = GC3D::GetAttributeSize(texAttrib);

		if (attribs & texAttrib) {
			TexCoord st = vtx.texCoords[i][point.texCoordIndex[i]];
			memcpy(dst, &st, attribSize);
			ASSERT(attribSize == sizeof(st));
			dst += attribSize;
		}
	}

	return S_OK;
}

void loadVertexIndexBuffers(const Batch& batch, const Vtx1& vtx,
	VertexBuffer* vb, IndexBuffer* ib, u16* packetIndexCounts)
{
	std::map<u64, u16> indexSet;
	int pointCount = 0;
	int primCount = 0;

	u16 vertexAttributes = loadAttribs(batch.attribs);

	// Count "points" in this batch
	// A point is a struct of indexes that point to each attribute of the 3D vertex
	// pointCount represents the maximum number of vertices needed. If we find dups, there may be less.
	for (uint i = 0; i < batch.packets.size(); i++)
	{
		const std::vector<Primitive>& prims = batch.packets[i].primitives;
		for (uint j = 0; j < prims.size(); j++)
		{
			primCount += 1;
			pointCount += prims[j].points.size();

			if (prims[j].type != 0x98)
				WARN("Unsupported primitive type detected! This will probably not draw correctly");
		}
	}

	// Create vertex buffer for this batch based on available attributes
	int vertexSize = GC3D::GetVertexSize(vertexAttributes);
	int bufferSize = pointCount * vertexSize; // we may not need all this space, see pointCount above

	ubyte*	vertices = (ubyte*)malloc(bufferSize);
	u16*	indices = (u16*)malloc((pointCount + primCount) * sizeof(u16));

	// Interlace each attribute into a single vertex stream 
	// (may be duplicates because it's using indices into the vtx1 buffer) 
	int indexCount = 0;
	int vertexCount = 0;
	int packetCount = 0;
	int packetIndexOffset = 0;
	STL_FOR_EACH(packet, batch.packets)
	{
		STL_FOR_EACH(prim, packet->primitives)
		{
			STL_FOR_EACH(point, prim->points)
			{
				uint index;
				static const uint64_t seed = 101;
				
				// Add the index of this vertex to our index buffer (every time)
				uint64_t hashKey = util::hash64(&point, sizeof(point), seed);
				auto indexPair = indexSet.find(hashKey);
				if (indexPair != indexSet.end())
				{
					// An equivalent vertex already exists, use that index
					index = indexPair->second;
				}
				else {
					// This points to a new vertex. Construct it.
					index = vertexCount++;
					const Index& p = point[0];
					buildVertex(vertices + vertexSize*index, p, vertexAttributes, vtx);
					indexSet[hashKey] = index;
				}

				// Always add a new index to our index buffer
				indices[indexCount++] = index;
			}

			// Add a strip-cut index to reset to a new triangle strip
			indices[indexCount++] = STRIP_CUT_INDEX;
		}

		// Might as well remove the last strip-cut index
		indexCount -= 1;

		packetIndexCounts[packetCount] = indexCount - packetIndexOffset;
		packetCount++;
		packetIndexOffset = indexCount;
	}

	vb->vertexAttributes = vertexAttributes;
	vb->vertexCount = vertexCount;
	vb->vertexBuf = vertices;

	ib->indexCount = indexCount;
	ib->indexBuf = indices;
}

uint RecordScenegraph( const BModel* bmodel, std::vector< Scenegraph >& scenelist, std::vector<u16>& jointParents, uint nodeIndex = 0, uint matIndex = -1, 
	                  bool onDown = true, uint parentJoint = -1) 
{
	//static Json::Value sgNode = root["Inf1"]["scenegraph"];
	//static uint nNodes = sgNode.size();
	//static Json::Value indexToMatIndex = root["Mat3"]["indexToMatIndex"];
	//static uint lastMatIndex = uint(-1);

	// Table to convert scene node indexes into material indexes
	const std::vector<Node>& scenegraph = bmodel->inf1.scenegraph;
	const std::vector<int>& indexToMatIndex = bmodel->mat3.indexToMatIndex;
	static uint lastMatIndex = uint(-1);

	uint tmpJoint = parentJoint;
	uint tmpMat = matIndex;

	Scenegraph prevPrim = {-1, -1};

	for (uint i = nodeIndex; i < scenegraph.size(); i++)
	{
		Scenegraph sg;
		sg.index = scenegraph[i].index;
		sg.type = scenegraph[i].type;

		switch(sg.type)
		{
		case SG_MATERIAL:
			tmpMat = indexToMatIndex[sg.index];
			ASSERT(tmpMat != uint(-1));
			onDown = (bmodel->mat3.materials[ tmpMat ].flag == 1);
			break;

		case SG_PRIM:
			if (onDown)
			{
				if (matIndex != lastMatIndex)
				{
					Scenegraph material = {matIndex, SG_MATERIAL};
					scenelist.push_back(material);
					lastMatIndex = matIndex;
				}
				scenelist.push_back(sg);
			}
			else
			{
				prevPrim = sg;
			}
			break;

		case SG_JOINT:
			jointParents.push_back(parentJoint);
			tmpJoint = sg.index;
			break;

		case SG_DOWN:
			i += RecordScenegraph(bmodel, scenelist, jointParents, i+1, tmpMat, onDown, tmpJoint); 
			
			if ( prevPrim.type == SG_PRIM && !onDown) 
			{
				if (matIndex != lastMatIndex)
				{
					Scenegraph material = { matIndex, SG_MATERIAL };
					scenelist.push_back(material);
					lastMatIndex = matIndex;
				}
				scenelist.push_back(prevPrim);
			}
			break;

		case SG_UP:
			return (i - nodeIndex) + 1;
			break;
				
		case SG_END:
			scenelist.push_back(sg);
			break;

		default:
			WARN("Unrecognized scenegraph node type");
			return (i - nodeIndex) + 1;
		}
	}

	return 0;
}

void ApplyMaterial(Renderer* renderer, MaterialInfo mat)
{
	static char samplerName[9] = { 'S', 'a', 'm', 'p', 'l', 'e', 'r', 'I' };
	static char textureName[9] = { 'T', 'e', 'x', 't', 'u', 'r', 'e', 'I' };
	
	// Shader
	renderer->setShader(mat.shader);
	renderer->setVertexFormat(0);
	
	renderer->setBlendState(mat.blendMode);
	renderer->setDepthState(mat.depthMode);
	renderer->setRasterizerState(mat.rasterMode);

	// Textures
	for (uint i = 0; mat.samplers[i] != 0xffff; i++)
	{
		samplerName[7] = '0' + i;
		textureName[7] = '0' + i;
		renderer->setSamplerState(samplerName, mat.samplers[i]);
		renderer->setTexture(textureName, mat.textures[i]);
	}
}

void FillMatrixTable(GDModel::GDModel* model, mat4* matrixTable, u16* matrixIndices, u16 nMatrixIndices)
{
	for (uint i = 0; i < nMatrixIndices; i++)
	{
		mat4& matrix = matrixTable[i];
		u16 drwIndex = matrixIndices[i];

		if (drwIndex == 0xffff)
			continue; // keep matrix set by previous packet
		
		DrwElement& drw = model->drwTable[drwIndex];
		if (drw.isWeighted)
		{
			// TODO: Optimize this
			memset(&matrix, 0, sizeof(mat4));
			u8 nMatrices = model->evpWeightedIndexSizesTable[drw.index];
			WeightedIndex* weightedIndices = 
				model->evpWeightedIndexTable + model->evpWeightedIndexOffsetTable[drw.index];
			
			for (uint j = 0; j < nMatrices; j++)
			{
				uint evpAndJntIndex = weightedIndices[j].index;
				float evpAndJntWeight = weightedIndices[j].weight;
				const mat4& evpMatrix = model->evpMatrixTable[evpAndJntIndex];
				const mat4& jntMatrix = model->jointTable[evpAndJntIndex].matrix;
				matrix = (jntMatrix*evpMatrix) * evpAndJntWeight + matrix;
			}
			matrix.rows[3] = vec4(0, 0, 0, 1.0f);
		}
		else
		{
			matrix = model->jointTable[drw.index].matrix;
		}

		// TODO: Implement MatrixType (Billboard, Y-Billboard)
	}
}

void DrawBatch(Renderer* renderer, GDModel::GDModel* model, u16 batchIndex, u16 matIndex)
{
	_Batch* batch = (_Batch*)model->batchPtrs[batchIndex];
	
	VertexBufferID vbID = batch->vbID;
	IndexBufferID ibID = batch->ibID;
	VertexFormatID vfID = batch->vfID;
	u16 numPackets = batch->numPackets;
	
	// These are partially updated by each packet
	mat4 matrixTable[10];

	int numIndicesSoFar = 0;
	for (uint i = 0; i < numPackets; i++)
	{
		u16 nMatrixIndices = batch->packets[ i ].matrixCount;
		u16* matrixIndices = batch->packets[ i ].matrixIndices;

		// Setup Matrix table
		FillMatrixTable(model, matrixTable, matrixIndices, nMatrixIndices);	

		renderer->reset();
			ApplyMaterial(renderer, model->materials[matIndex]);
			renderer->setVertexBuffer(0, vbID);
			renderer->setVertexFormat(vfID);
			renderer->setIndexBuffer(ibID);
			renderer->setShaderConstantArray4x4f("ModelMat", matrixTable, nMatrixIndices);
		renderer->apply();

		u16 indexCount = batch->packets[ i ].indexCount;
		renderer->drawElements(PRIM_TRIANGLE_STRIP, numIndicesSoFar, indexCount, 0, -1);
		numIndicesSoFar += indexCount;
	}
}

RESULT RegisterGFX(Renderer* renderer, GDModel::GDModel* model)
{
	GDModel::TemporaryGFXData& gfxData = model->gfxData;
	uint samplers[256];
	uint blendModes[256];
	uint cullModes[256];
	uint depthModes[256];

	uint shaders[256];
	uint textures[256];
	
	// Remember our creator
	gfxData.renderer = renderer;

	// Register our samplers
	for (uint i = 0; i < gfxData.nTextureResources; i++)
	{
		TextureResource& res = gfxData.textureResources[i];
		samplers[i] = renderer->addSamplerState(res.filter, res.wrapS, res.wrapT, CLAMP);
	}
		
	// Register our textures
	Image imgResource;
	for (uint i = 0; i < gfxData.nTextures; i++)
	{
		TextureDesc& tex = gfxData.textures[i];
			
		imgResource.loadFromMemory(tex.imgData, tex.format, 
			tex.width, tex.height, 1, tex.numMips, true);

		textures[i] = renderer->addTexture(imgResource);
	}

	// Register our depth, blend, cull modes
	for (uint i = 0; i < gfxData.nBlendModes; i++)
	{
		BlendMode& bm = gfxData.blendModes[i];
		blendModes[i] = renderer->addBlendState(bm.srcFactor, bm.dstFactor, bm.blendOp);
	}
	for (uint i = 0; i < gfxData.nDepthModes; i++)
	{
		DepthMode& dm = gfxData.depthModes[i];
		depthModes[i] = renderer->addDepthState(dm.testEnable, dm.writeEnable, dm.func);
	}
	for (uint i = 0; i < gfxData.nCullModes; i++)
	{
		u8 cm = gfxData.cullModes[i];
		cullModes[i] = renderer->addRasterizerState(cm);
	}

	// Register our shaders
	for (uint i = 0; i < gfxData.nShaders; i++)
	{
		char* vsText = gfxData.vsShaders + gfxData.vsOffsets[i];
		char* psText = gfxData.psShaders + gfxData.psOffsets[i];
		shaders[i] = renderer->addShader(vsText, nullptr, psText, 0, 0, 0);
	}

	// Fixup our Materials with their runtime IDs
	for (uint i = 0; i < model->nMaterials; i++)
	{
		MaterialInfo& mat = model->materials[i];
		mat.blendMode = blendModes[mat.blendMode];
		mat.depthMode = depthModes[mat.depthMode];
		mat.rasterMode = cullModes[mat.rasterMode];
		mat.shader = shaders[mat.shader];
			
		for (uint i = 0; i < 8; i++)
		{
			u16 texIndex = mat.samplers[i];

			if (texIndex == 0xffff)
				break;

			mat.samplers[i] = samplers[texIndex];
			TextureResource& res = gfxData.textureResources[texIndex];
			mat.textures[i] = textures[res.texIndex];
		}
	}

	// Register vertex and index buffers
	ubyte* head = gfxData.vertexIndexBuffers;
	for (uint i = 0; i < gfxData.nVertexIndexBuffers; i++)
	{
		ubyte* batch = model->batchPtrs[i];
		VertexBufferID* batchVBID = (VertexBufferID*)batch;
		IndexBufferID*  batchIBID = (IndexBufferID*)(batch + sizeof(batchVBID));
		VertexFormatID* batchVFID = (VertexFormatID*)(batch + sizeof(batchVBID) + sizeof(batchIBID));
		assert(*batchVBID == -1 && *batchIBID == -1 && *batchVFID == -1);

		u16 attributes = READ(u16);
		int numVertices = READ(u16);
		int vbSize = numVertices * GC3D::GetVertexSize(attributes);
		void* vertices = READ_ARRAY(ubyte, vbSize);

		int numIndices = READ(u16);
		int ibSize = numIndices * sizeof(u16);
		void* indices = READ_ARRAY(ubyte, ibSize);

		FormatDesc formatBuf[MAX_VERTEX_ATTRIBS];
		GC3D::ConvertGCVertexFormat(attributes, formatBuf);

		*batchVBID = renderer->addVertexBuffer(vbSize, STATIC, vertices);
		*batchIBID = renderer->addIndexBuffer(ibSize, 2, STATIC, indices);
		*batchVFID = renderer->addVertexFormat(formatBuf, MAX_VERTEX_ATTRIBS, shaders[0]);
	}

	// Cleanup
	free(model->gfxData.depthModes);
	free(model->gfxData.blendModes);
	free(model->gfxData.cullModes);
	free(model->gfxData.vsOffsets);
	free(model->gfxData.psOffsets);
	free(model->gfxData.vsShaders);
	free(model->gfxData.psShaders);
	free(model->gfxData.vertexIndexBuffers);
	for (uint i = 0; i < gfxData.nTextures; i++)
	{
		free(gfxData.textures[i].imgData);
	}
	free(model->gfxData.textureResources);
	free(model->gfxData.textures);

	return S_OK;
}

RESULT UnregisterGFX(Renderer* renderer, GDModel::GDModel* model)
{
	for (uint i = 0; i < model->nMaterials; i++)
	{
		MaterialInfo& mat = model->materials[i];
		//renderer->removeBlendState(mat.blendMode);
		//renderer->removeDepthState(mat.depthMode);
		//renderer->removeRasterizerState(mat.rasterMode);
		//renderer->removeShader(mat.shader);
			
		for (uint i = 0; i < 8; i++)
		{
			u16 texIndex = mat.samplers[i];
			if (texIndex == 0xffff)
				break;

			//renderer->removeSamplerState(mat.samplers[i]);
			renderer->removeTexture(mat.textures[i]);
		}

		//renderer->removeVertexFormat(model->vertFormat);
		
		for (uint i = 0; i < model->gfxData.nVertexIndexBuffers; i++)
		{
			ubyte* batch = model->batchPtrs[i];
			VertexBufferID* batchVBID = (VertexBufferID*)batch;
			IndexBufferID*  batchIBID = (IndexBufferID*)(batch + sizeof(batchVBID));

			//renderer->removeVertexBuffer(batchVBID);
			//renderer->changeIndexBuffer(batchIBID);
		}
	}

	return S_OK;
}

RESULT GDModel::Unload(GDModel* model)
{
	RESULT r;
	
	r = UnregisterGFX(model->gfxData.renderer, model);
	
	// Clear the whole model for safety
	memset(model, 0, sizeof(model));
	 
	free(model->scenegraph);

	for (uint i = 0; i < model->batchCount; i++)
	{
		_Batch* batch = (_Batch*) model->batchPtrs[i];
		for (uint j = 0; j < batch->numPackets; j++)
		{
			free(batch->packets[j].matrixIndices);
		}
		free(batch->packets);
		free(batch);
	}
	free(model->batchPtrs);

	free(model->materials);
	free(model->drwTable);
	free(model->jointTable);
	free(model->defaultPose);

	free(model->evpWeightedIndexSizesTable);
	free(model->evpWeightedIndexOffsetTable);
	free(model->evpMatrixTable);
	free(model->evpWeightedIndexTable);

	return r;
}

extern std::string GenerateVS(const Mat3* matInfo, int index);
extern std::string GeneratePS(const Tex1* texInfo, const Mat3* matInfo, int index);

RESULT GDModel::Load(GDModel* model, const BModel* bdl)
{
	VertexBuffer* vertexBuffers;
	IndexBuffer* indexBuffers;

	// Scenegraph first		
	std::vector<u16> jointParents;
	std::vector<Scenegraph> scenelist;
	RecordScenegraph(bdl, scenelist, jointParents);
	model->scenegraph = (Scenegraph*)malloc(scenelist.size() * sizeof(Scenegraph));
	memcpy(model->scenegraph, scenelist.data(), scenelist.size() * sizeof(Scenegraph));

	// Batches	
	{
		const std::vector< Batch >& batches = bdl->shp1.batches;
		
		vertexBuffers = (VertexBuffer*)malloc(sizeof(VertexBuffer) * batches.size());
		indexBuffers = (IndexBuffer*)malloc(sizeof(IndexBuffer) * batches.size());

		model->batchCount = batches.size();
		model->batchPtrs = (ubyte**)malloc(sizeof(uint) * batches.size());
		for (uint i = 0; i < batches.size(); i++)
		{
			_Batch* batch = (_Batch*)malloc(sizeof(_Batch) * batches.size());
			memset(batch, 0xff, sizeof(_Batch));
			model->batchPtrs[i] = (ubyte*)batch;

			if (batches[i].matrixType != 0)
			{
				WARN("Batch matrix type %u not yet supported!\n", batches[i].matrixType);
			}

			const u32 kMaxPackets = 1024;
			u16 packetIdxCounts[ kMaxPackets ];
			loadVertexIndexBuffers(batches[i], bdl->vtx1, 
				&vertexBuffers[i], &indexBuffers[i], packetIdxCounts);
						
			batch->numPackets = batches[i].packets.size();
			ASSERT(batch->numPackets <= kMaxPackets);
			batch->packets = (_Packet*)malloc(sizeof(_Packet) * batch->numPackets);
			for (uint32_t j = 0; j < batch->numPackets; j++)
			{
				const std::vector< u16 >& mtxTable = batches[i].packets[j].matrixTable;
				batch->packets[j].matrixCount = mtxTable.size();
				batch->packets[j].matrixIndices = (u16*)malloc(sizeof(u16) * mtxTable.size());
				memcpy(batch->packets[j].matrixIndices, mtxTable.data(), sizeof(u16) * mtxTable.size());
				batch->packets[j].indexCount = packetIdxCounts[ j ];
			}
		}
	}

	// Materials
	{
		// Depth Modes
		u32 zModeCount = bdl->mat3.zModes.size();
		DepthMode* zModes = (DepthMode*)malloc(zModeCount * sizeof(DepthMode));
		for (uint i = 0; i < zModeCount; i++)
		{
			zModes[i].testEnable = bdl->mat3.zModes[i].enable;
			zModes[i].writeEnable = bdl->mat3.zModes[i].enableUpdate;
			zModes[i].func = GC3D::ConvertGCDepthFunction(bdl->mat3.zModes[i].zFunc);
		}
		model->gfxData.nDepthModes = zModeCount;
		model->gfxData.depthModes = zModes;

		// Blend Modes
		u32 bModeCount = bdl->mat3.blendInfos.size();
		BlendMode* bModes = (BlendMode*)malloc(bModeCount * sizeof(BlendMode));
		for (uint i = 0; i < bModeCount; i++)
		{
			bModes[i].blendOp = GC3D::ConvertGCBlendOp(bdl->mat3.blendInfos[i].blendMode);
			bModes[i].srcFactor = GC3D::ConvertGCBlendFactor(bdl->mat3.blendInfos[i].srcFactor);
			bModes[i].dstFactor = GC3D::ConvertGCBlendFactor(bdl->mat3.blendInfos[i].dstFactor);
		}
		model->gfxData.nBlendModes = bModeCount;
		model->gfxData.blendModes = bModes;

		// Cull Modes
		u32 cModeCount = bdl->mat3.cullModes.size();
		u8* cModes = (u8*)malloc(cModeCount * sizeof(u8));
		for (uint i = 0; i < cModeCount; i++)
		{
			cModes[i] = GC3D::ConvertGCCullMode(bdl->mat3.cullModes[i]);
		}
		model->gfxData.nCullModes = cModeCount;
		model->gfxData.cullModes = cModes;

		// MaterialInfo info
		u32 matCount = bdl->mat3.materials.size();
		MaterialInfo* matInfo = (MaterialInfo*) malloc(sizeof(MaterialInfo) * matCount);
		for (uint i = 0; i < matCount; i++)
		{
			MaterialInfo& mat = matInfo[i];
			mat.shader = i;
			mat.depthMode = bdl->mat3.materials[i].zModeIndex;
			mat.blendMode = bdl->mat3.materials[i].blendIndex;
			mat.rasterMode = bdl->mat3.materials[i].cullIndex;

			for (uint j = 0; j < 8; j++)
			{
				u16 stageIndex = bdl->mat3.materials[i].texStages[j];
				mat.samplers[j] = stageIndex == 0xffff ? 0xffff : bdl->mat3.texStageIndexToTextureIndex[stageIndex];
				mat.textures[j] = 0;
			}
		}
		model->nMaterials = matCount;
		model->materials = matInfo;

		// MaterialInfo names
		// @TODO: Add these to MaterialInfo
		/*Json::Value nameNodes = root["Mat3"]["stringtable"];
		u16 nMaterialNames = nameNodes.size();
		WRITE(nMaterialNames);
		for (uint i = 0; i < nMaterialNames; i++)
		{
			std::string name = nameNodes.get(uint(0), "UNKNOWN").asString();
			WRITE_ARRAY(name.c_str(), name.size());
		}*/
	}

	// Draw table
	{
		u32 drwCount = bdl->drw1.data.size();
		DrwElement* drwTable = (DrwElement*)malloc(sizeof(DrwElement) * drwCount);
		for (uint i = 0; i < drwCount; i++)
		{
			drwTable[i].index = bdl->drw1.data[i];
			drwTable[i].isWeighted = bdl->drw1.isWeighted[i];
		}
		model->drwTable = drwTable;
	}
	
	// Joints
	{
		u32 jointCount = bdl->jnt1.frames.size();
		JointElement* joints = (JointElement*)malloc(jointCount * sizeof(JointElement));
		for (uint i = 0; i < jointCount; i++)
		{
			JointElement& joint = joints[i];
			loadFrame(bdl->jnt1.frames[i], &joint.matrix);
			strncpy_s(joint.name, bdl->jnt1.frames[i].name.c_str(), 16);
			joint.parent = jointParents[i];
		}
		model->numJoints = jointCount;
		model->jointTable = joints;

		uint jointTableSize = sizeof(JointElement) * jointCount;
		model->defaultPose = (JointElement*)malloc(jointTableSize);
		memcpy(model->defaultPose, model->jointTable, jointTableSize);
	}

	// Envelope
	{
		u32 mtxCount = bdl->evp1.matrices.size();
		u32 weightCount = bdl->evp1.weightedIndices.size();
		u32 maxWeightedIdxs = weightCount * 8;

		u8* sizesTable = (u8*)malloc(sizeof(u8)*weightCount);
		u16* offsetsTable = (u16*)malloc(sizeof(u16)*weightCount);
		mat4* mtxTable = (mat4*)malloc(sizeof(mat4)*mtxCount);
		WeightedIndex* idxTable = (WeightedIndex*)malloc(sizeof(WeightedIndex) * maxWeightedIdxs);

		memcpy(&mtxTable[0], &bdl->evp1.matrices[0], mtxCount * 16 * sizeof(float));

		u32 offset = 0;
		for (uint i = 0; i < weightCount; i++)
		{
			u32 idxCount = bdl->evp1.weightedIndices[i].indices.size();
			sizesTable[i] = idxCount;
			offsetsTable[i] = offset;
			for (uint j = 0; j < idxCount; j++)
			{
				idxTable[offset + j].weight = bdl->evp1.weightedIndices[i].weights[j];
				idxTable[offset + j].index = bdl->evp1.weightedIndices[i].indices[j];
			}
			offset += idxCount;
		}
		ASSERT(offset <= maxWeightedIdxs);

		model->evpWeightedIndexSizesTable = sizesTable;
		model->evpWeightedIndexOffsetTable = offsetsTable;
		model->evpMatrixTable = mtxTable;
		model->evpWeightedIndexTable = idxTable;
	}

	// Shader HLSL
	{
		const u32 kMaxShaderLength = 64 * 1024;
		u32 shaderCount = bdl->mat3.materials.size();
		uint* vsOffsets = (uint*)malloc(sizeof(uint) * shaderCount);
		uint* psOffsets = (uint*)malloc(sizeof(uint) * shaderCount);
		char* vsShaders = (char*)malloc(sizeof(char) * kMaxShaderLength);
		char* psShaders = (char*)malloc(sizeof(char) * kMaxShaderLength);

		u32 vsOffset = 0;
		u32 psOffset = 0;
		for (uint i = 0; i < shaderCount; i++)
		{
			std::string vs = GenerateVS(&bdl->mat3, i);
			std::string ps = GeneratePS(&bdl->tex1, &bdl->mat3, i);

			vsOffsets[i] = vsOffset;
			psOffsets[i] = psOffset;
			memcpy(&vsShaders[vsOffset], vs.c_str(), vs.length() + 1);
			memcpy(&psShaders[psOffset], ps.c_str(), ps.length() + 1);
			vsOffset += vs.length() + 1;
			psOffset += ps.length() + 1;
		}
		ASSERT(vsOffset <= kMaxShaderLength);
		ASSERT(psOffset <= kMaxShaderLength);

		model->gfxData.nShaders = shaderCount;
		model->gfxData.vsOffsets = vsOffsets;
		model->gfxData.psOffsets = psOffsets;
		model->gfxData.vsShaders = vsShaders;
		model->gfxData.psShaders = psShaders;
	}

	{
		const u32 kMaxVertexIndexBufSize = 1024 * 1024;
		u32 bufCount = bdl->shp1.batches.size();
		ubyte* viBuf = (ubyte*)malloc(kMaxVertexIndexBufSize);
		ubyte* viHead = viBuf;
		for (uint i = 0; i < bufCount; i++)
		{
			memcpy(viHead, &vertexBuffers[i].vertexAttributes, sizeof(vertexBuffers[i].vertexAttributes));
			viHead += sizeof(vertexBuffers[i].vertexAttributes);
			memcpy(viHead, &vertexBuffers[i].vertexCount, sizeof(vertexBuffers[i].vertexCount));
			viHead += sizeof(vertexBuffers[i].vertexCount);
			uint vertBufSize = vertexBuffers[i].vertexCount * GC3D::GetVertexSize(vertexBuffers[i].vertexAttributes);
			memcpy(viHead, vertexBuffers[i].vertexBuf, vertBufSize);
			viHead += vertBufSize;

			memcpy(viHead, &indexBuffers[i].indexCount, sizeof(indexBuffers[i].indexCount));
			viHead += sizeof(indexBuffers[i].indexCount);
			memcpy(viHead, indexBuffers[i].indexBuf, indexBuffers[i].indexCount * sizeof(u16));
			viHead += indexBuffers[i].indexCount * sizeof(u16);
		}
		ASSERT(viHead - viBuf <= kMaxVertexIndexBufSize);
		viBuf = (ubyte*) realloc(viBuf, viHead - viBuf);
		model->gfxData.nVertexIndexBuffers = bufCount;
		model->gfxData.vertexIndexBuffers = viBuf;

		for (uint i = 0; i < bufCount; i++)
		{
			free(vertexBuffers[i].vertexBuf);
			free(indexBuffers[i].indexBuf);
		}
		free(vertexBuffers);
		free(indexBuffers);
	}

	{
		u32 texCount = bdl->tex1.imageHeaders.size();
		u32 imgCount = bdl->tex1.images.size();
		TextureResource* texs = (TextureResource*)malloc(sizeof(TextureResource) * texCount);
		TextureDesc* imgs = (TextureDesc*)malloc(sizeof(TextureDesc) * imgCount);

		// TODO: Turn the samplers into a set (using hashing) and simplify this
		for (uint i = 0; i < texCount; i++)
		{
			TextureResource& tex = texs[i];

			const char* name = bdl->tex1.imageHeaders[i].name.c_str();
			memcpy(tex.name, name, 16);

			u8 magFilter = bdl->tex1.imageHeaders[i].magFilter;
			u8 minFilter = bdl->tex1.imageHeaders[i].minFilter;
			tex.filter = GC3D::ConvertGCTexFilter(magFilter, minFilter);
			tex.wrapS = GC3D::ConvertGCTexWrap(bdl->tex1.imageHeaders[i].wrapS);
			tex.wrapT = GC3D::ConvertGCTexWrap(bdl->tex1.imageHeaders[i].wrapT);
			tex.texIndex = bdl->tex1.imageHeaders[i].imageIndex;
		}

		for (uint i = 0; i < imgCount; i++)
		{
			TextureDesc& img = imgs[i];
			
			uint gcFormat = bdl->tex1.images[i].format;
			img.format = GC3D::ConvertGCTextureFormat(gcFormat);
			img.width = bdl->tex1.images[i].width;
			img.height = bdl->tex1.images[i].height;

			img.numMips = bdl->tex1.images[i].sizes.size();

			uint imgSize = 0;
			for (uint j = 0; j < img.numMips; j++) { imgSize += bdl->tex1.images[i].sizes[j]; }
			img.sizeBytes = imgSize;

			img.imgData = (u8*)malloc(imgSize);
			memcpy(img.imgData, bdl->tex1.images[i].imageData.data(), imgSize);
		}

		model->gfxData.nTextureResources = texCount;
		model->gfxData.nTextures = imgCount;
		model->gfxData.textureResources = texs;
		model->gfxData.textures = imgs;
	}

	model->loadGPU = true;

	return S_OK;
}

RESULT GDModel::Update(GDModel* model, GDAnim::GDAnim* anim, float time)
{
	// Grab the root joint straight from the animation. It's parent is the identity
	float weights[1] = {1.0f};
	//model->jointTable[0].matrix = model->defaultPose[0].matrix;
	model->jointTable[0].matrix = GDAnim::GetJoint(&anim, weights, 1, 0, time);

	for (uint i = 1; i < model->numJoints; i++)
	{
		JointElement& joint = model->jointTable[i];

		// Calculate joint from animation at current time
		//mat4 animMatrix = model->defaultPose[i].matrix;
		mat4 animMatrix = GDAnim::GetJoint(&anim, weights, 1, i, time);

		// Put in parent's frame
		joint.matrix = model->jointTable[joint.parent].matrix * animMatrix;
	}

	return S_OK;
}

RESULT GDModel::Draw(Renderer* renderer, GDModel* model)
{
	u16 matIndex = -1;

	if (model->loadGPU)
	{
		RegisterGFX(renderer, model);
		model->loadGPU = false;
	}

	for (Scenegraph* node = model->scenegraph; node->type != SG_END; node++)
	{
		switch(node->type)
		{	
		case SG_MATERIAL: 
			matIndex = node->index;
			break;

		case SG_PRIM:
			LOG("Drawing Batch %u with Material %u\n", node->index, matIndex); 
			DrawBatch(renderer, model, node->index, matIndex);
			break;	
		}
	}

	return S_OK; 
}