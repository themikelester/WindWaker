#include "GDModel.h"
#include "Compile.h"
#include "Framework3\Renderer.h"
#include "GC3D.h"
#include "util.h"

#include <sstream>

#include "Foundation\hash.h"
#include "Foundation\murmur_hash.h"

//TODO: Remove the need for the D3D reference
#include <Framework3\Direct3D10\Direct3D10Renderer.h>

#define COMPILER_VERSION 1;
#define WRITE(val) { s.write( (char*)&val, sizeof(val) ); totalSize += sizeof(val); }
#define WRITE_ARRAY(arr, size) { s.write((char*)arr, size); totalSize += size; }
#define READ(type) *(type*)head; head += sizeof(type);
#define READ_ARRAY(type, count) (type*)head; head += sizeof(type) * count;

//TODO: HACK: Remove this!
#define FULL_VERTEX_ATTRIBS 0x1fff


struct SectionHeader
{
	char fourcc[4];
	int size; // Including this header
};

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

// Compile a coordinate frame into an affine matrix
void compileFrame(Json::Value& frameNode, mat4* matrix)
{
	mat4 t, rx, ry, rz, s;

	Json::Value& transNode = frameNode["translation"];
	t = translate(float(transNode.get("x", 0).asDouble()), 
				  float(transNode.get("y", 0).asDouble()), 
				  float(transNode.get("z", 0).asDouble()));

	Json::Value& rotNode = frameNode["rotation"];
	rx = rotateX(float(rotNode.get("x", 0).asDouble()/360.0 * 2*PI));
	ry = rotateY(float(rotNode.get("y", 0).asDouble()/360.0 * 2*PI));
	rz = rotateZ(float(rotNode.get("z", 0).asDouble()/360.0 * 2*PI));
	
	Json::Value& scaleNode = frameNode["scale"];
	s = scale(float(scaleNode.get("x", 0).asDouble()), 
			  float(scaleNode.get("y", 0).asDouble()), 
			  float(scaleNode.get("z", 0).asDouble()));

	// TODO: Implement == and != for mat4s 
	if (s != identity4())
		WARN("This model is using joint scaling. This is not yet tested!");

  //this is probably right this way:
  //return t*rz*ry*rx*s; //scales seem to be local only
  *matrix = t*rz*ry*rx;
}

// Read and interpret Batch1 vertex attribs
u16 compileAttribs(Json::Value& attribsNode)
{
	u16 attribFlags = 0;

	if (attribsNode.get("mtx", false).asBool()) attribFlags |= GDModel::HAS_MATRIX_INDICES;
	if (attribsNode.get("pos", false).asBool()) attribFlags |= GDModel::HAS_POSITIONS;
	if (attribsNode.get("nrm", false).asBool()) attribFlags |= GDModel::HAS_NORMALS;

	if (attribsNode["clr"].get(uint(0), false).asBool()) attribFlags |= GDModel::HAS_COLORS0;
	if (attribsNode["clr"].get(uint(1), false).asBool()) attribFlags |= GDModel::HAS_COLORS1;
	
	if (attribsNode["tex"].get(uint(0), false).asBool()) attribFlags |= GDModel::HAS_TEXCOORDS0;
	if (attribsNode["tex"].get(uint(1), false).asBool()) attribFlags |= GDModel::HAS_TEXCOORDS1;
	if (attribsNode["tex"].get(uint(2), false).asBool()) attribFlags |= GDModel::HAS_TEXCOORDS2;
	if (attribsNode["tex"].get(uint(3), false).asBool()) attribFlags |= GDModel::HAS_TEXCOORDS3;
	if (attribsNode["tex"].get(uint(4), false).asBool()) attribFlags |= GDModel::HAS_TEXCOORDS4;
	if (attribsNode["tex"].get(uint(5), false).asBool()) attribFlags |= GDModel::HAS_TEXCOORDS5;
	if (attribsNode["tex"].get(uint(6), false).asBool()) attribFlags |= GDModel::HAS_TEXCOORDS6;
	if (attribsNode["tex"].get(uint(7), false).asBool()) attribFlags |= GDModel::HAS_TEXCOORDS7;

	return attribFlags;
}

RESULT buildInflatedVertex(ubyte* dst, Point& point, u16 attribs, const Json::Value& vtx)
{
	uint attribSize;
	uint vtxSize = GC3D::GetVertexSize(FULL_VERTEX_ATTRIBS);

	// Clear the vertex so that all of our missing data is zero'd
	memset(dst, 0, vtxSize);
	
	if ( (attribs & GDModel::HAS_POSITIONS) == false )
		WARN("Model does not have required attributes");

	// TODO: We can save a uint in the vertex structure if we remove this force
	// Always set this attribute. The default is 0.
	attribSize = GC3D::GetAttributeSize(GDModel::HAS_MATRIX_INDICES);
	if (attribs & GDModel::HAS_MATRIX_INDICES) {
		ASSERT(point.mtxIdx/3 < 10); 
		uint matrixIndex = point.mtxIdx/3;
		memcpy(dst, &matrixIndex, attribSize);
	}
	else
	{
		memset(dst, 0, attribSize);
	}
	dst += attribSize;
	
	attribSize = GC3D::GetAttributeSize(GDModel::HAS_POSITIONS);
	if (attribs & GDModel::HAS_POSITIONS) {
		float3 pos;
		pos.x = float(vtx["positions"][point.posIdx].get(uint(0), 0).asDouble());
		pos.y = float(vtx["positions"][point.posIdx].get(uint(1), 0).asDouble());
		pos.z = float(vtx["positions"][point.posIdx].get(uint(2), 0).asDouble());
		memcpy(dst, &pos, attribSize);
	}
	dst += attribSize;
	
	attribSize = GC3D::GetAttributeSize(GDModel::HAS_NORMALS);
	if (attribs & GDModel::HAS_NORMALS) {
		float3 nrm;
		nrm.x = float(vtx["normals"][point.nrmIdx].get(uint(0), 0).asDouble());
		nrm.y = float(vtx["normals"][point.nrmIdx].get(uint(1), 0).asDouble());
		nrm.z = float(vtx["normals"][point.nrmIdx].get(uint(2), 0).asDouble());
		memcpy(dst, &nrm, attribSize);
	}
	dst += attribSize;

	for (uint i = 0; i < 2; i++)
	{
		u16 colorAttrib = GDModel::HAS_COLORS0 << i;
		attribSize = GC3D::GetAttributeSize(colorAttrib);
		
		if (attribs & colorAttrib) {
			ubyte clr[4];
			clr[0] = ubyte(vtx["colors"][point.clrIdx[i]].get("r", 0).asUInt());
			clr[1] = ubyte(vtx["colors"][point.clrIdx[i]].get("g", 0).asUInt());
			clr[2] = ubyte(vtx["colors"][point.clrIdx[i]].get("b", 0).asUInt());
			clr[3] = ubyte(vtx["colors"][point.clrIdx[i]].get("a", 0).asUInt());
			memcpy(dst, clr, attribSize);
			ASSERT(attribSize == sizeof(clr));
		}
		dst += attribSize;
	}

	for (uint i = 0; i < 8; i++)
	{
		u16 texAttrib = GDModel::HAS_TEXCOORDS0 << i;
		attribSize = GC3D::GetAttributeSize(texAttrib);

		if (attribs & texAttrib) {
			float2 st;
			st.x = float(vtx["texcoords"][i][point.texIdx[i]].get("s", 0).asDouble());
			st.y = float(vtx["texcoords"][i][point.texIdx[i]].get("t", 0).asDouble());
			memcpy(dst, st, attribSize);
			ASSERT(attribSize == sizeof(st));
		}
		dst += attribSize;
	}

	return S_OK;
}

void compileVertexIndexBuffers(Json::Value& batch, const Json::Value& vtx, 
							   VertexBuffer* vb, IndexBuffer* ib, u16* packetIndexCounts)
{
	foundation::Hash<u16>* indexSet = new foundation::Hash<u16>(foundation::memory_globals::default_allocator());
	int pointCount = 0;
	int primCount = 0;
			
	u16 vertexAttributes = compileAttribs(batch["attribs"]);

	// Count "points" in this batch
	// A point is a struct of indexes that point to each attribute of the 3D vertex
	// pointCount represents the maximum number of vertices needed. If we find dups, there may be less.
	Json::Value packetNodes = batch["packets"];
	for (uint i = 0; i < packetNodes.size(); i++)
	{				
		Json::Value primNodes = packetNodes[i]["primitives"];
		for (uint j = 0; j < primNodes.size(); j++)
		{
			primCount += 1;
			pointCount += primNodes[j]["points"].size();

			// TODO: We're still writing the crazy type version to the intermediate file
			//			Change this to something nice like 0 = TRISTRIP, 1 = TRIFAN, >1 = UNKNOWN 
			if (primNodes[j]["type"].asInt() != 152)
				WARN("Unsupported primitive type detected! This will probably not draw correctly");
		}
	}
	
	// Create vertex buffer for this batch based on available attributes
	int vertexSize = GC3D::GetVertexSize(FULL_VERTEX_ATTRIBS);
	int bufferSize = pointCount * vertexSize; // we may not need all this space, see pointCount above

	ubyte*	vertices = (ubyte*)malloc( bufferSize );
	u16*	indices = (u16*)malloc( (pointCount + primCount) * sizeof(u16));

	// Interlace each attribute into a single vertex stream 
	// (may be duplicates because it's using indices into the vtx1 buffer) 
	int indexCount = 0;
	int vertexCount = 0; 
	int packetCount = 0;
	int packetIndexOffset = 0;
	STL_FOR_EACH(packet, batch["packets"])
	{
		STL_FOR_EACH(prim, (*packet)["primitives"])
		{
			STL_FOR_EACH(point, (*prim)["points"])
			{
				u16 index; 
				static const uint64_t seed = 101;

				Point p = {};
				p.mtxIdx = (*point).get("mtx", 0).asUInt();
				p.posIdx = (*point).get("pos", 0).asUInt();
				p.nrmIdx = (*point).get("nrm", 0).asUInt();
				p.clrIdx[0] = (*point)["clr"].get(uint(0), 0).asUInt();
				p.clrIdx[1] = (*point)["clr"].get(uint(1), 0).asUInt();
				p.texIdx[0] = (*point)["tex"].get(uint(0), 0).asUInt();
				p.texIdx[1] = (*point)["tex"].get(uint(1), 0).asUInt();
				p.texIdx[2] = (*point)["tex"].get(uint(2), 0).asUInt();
				p.texIdx[3] = (*point)["tex"].get(uint(3), 0).asUInt();
				p.texIdx[4] = (*point)["tex"].get(uint(4), 0).asUInt();
				p.texIdx[5] = (*point)["tex"].get(uint(5), 0).asUInt();
				p.texIdx[6] = (*point)["tex"].get(uint(6), 0).asUInt();
				p.texIdx[7] = (*point)["tex"].get(uint(7), 0).asUInt();

				// Add the index of this vertex to our index buffer (every time)
				uint64_t hashKey = foundation::murmur_hash_64(&p, sizeof(p), seed);
				if (foundation::hash::has(*indexSet, hashKey))
				{
					// An equivalent vertex already exists, use that index
					index = foundation::hash::get(*indexSet, hashKey, u16(0));
				} else {
					// This points to a new vertex. Construct it.
					index = vertexCount++;
					buildInflatedVertex(vertices + vertexSize*index, p, vertexAttributes, vtx);
					foundation::hash::set(*indexSet, hashKey, index);
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

#define BEGIN_SECTION(FOURCC) {	\
	memcpy(sectionHeaders[currentSection].fourcc, FOURCC, 4); \
	sectionHeaderOffsets[currentSection] = totalSize; \
	WRITE(sectionHeaders[0]); }

#define END_SECTION() { \
	sectionHeaders[currentSection].size = totalSize - sectionHeaderOffsets[currentSection]; \
	currentSection++; }


RESULT GDModel::Compile(const Json::Value& root, Header& hdr, char** data)
{
	std::stringstream s;
	uint totalSize = 0;
	uint currentSection = 0;

	// Disable the debug heap. JsonCpp uses a ton of malloc/free's.
	DEBUG_ONLY(_CrtSetDbgFlag(0));

	//TODO: HACK: Remove this
	foundation::memory_globals::init(4 * 1024 * 1024);

	// Post-processing data
	u16 sectionHeaderOffsets[8];
	SectionHeader sectionHeaders[8];
	
	u16 batchOffsetsOffset;
	
	VertexBuffer* vertexBuffers;
	IndexBuffer* indexBuffers;
	u16 nVertexIndexBuffers = 0;

	std::vector<u16> jointParents;

	// Scenegraph
	BEGIN_SECTION("scn1");
		Scenegraph sg;
		Json::Value sgNode = root["Inf1"]["scenegraph"];
		uint nNodes = sgNode.size();
		WRITE(nNodes);

		std::stack<u16> parentJoints;
		parentJoints.push(-1);
		u16 tempParentJoint = parentJoints.top();
		for (uint i = 0; i < nNodes; i++)
		{
			sg.index = u16(sgNode[i].get("index", 0).asUInt());
			sg.type = u16(sgNode[i].get("type", 0).asUInt());

			switch(sg.type)
			{
			case SG_JOINT:
				jointParents.push_back(tempParentJoint);
				tempParentJoint = sg.index;
				break;

			case SG_DOWN:
				parentJoints.push(tempParentJoint);
				tempParentJoint = tempParentJoint;
				break;

			case SG_UP:
				parentJoints.pop();
				tempParentJoint = parentJoints.top();
				break;
			}

			WRITE(sg);
		}
	END_SECTION();

	// Batches
	BEGIN_SECTION("bch1");
		Json::Value batchNodes = root["Shp1"]["batches"];
		u16 nBatches = batchNodes.size();
		WRITE(nBatches);
		
		// Save some space for our batch offsets
		batchOffsetsOffset = totalSize;
		u16* batchOffsets = (u16*) malloc(sizeof(u16) * nBatches);
		WRITE_ARRAY(batchOffsets, sizeof(u16) * nBatches);

		nVertexIndexBuffers = nBatches;
		vertexBuffers = (VertexBuffer*) malloc(sizeof(VertexBuffer) * nBatches);
		indexBuffers = (IndexBuffer*) malloc(sizeof(IndexBuffer) * nBatches);

		for (uint i = 0; i < nBatches; i++)
		{
			batchOffsets[i] = totalSize;
		
			// Save space for our vertexBufferID and indexBufferID
			int vertexIndexBufferInvalidID = -1;
			WRITE(vertexIndexBufferInvalidID);
			WRITE(vertexIndexBufferInvalidID);

			Json::Value packetNodes = batchNodes[i]["packets"];
			u16 nPackets = packetNodes.size();
			WRITE(nPackets);
			
			u16* packetIndexCounts = (u16*)malloc(nPackets * sizeof(u16));
			compileVertexIndexBuffers(batchNodes[i], root["Vtx1"], &vertexBuffers[i], 
				&indexBuffers[i], packetIndexCounts);

			for (uint j = 0; j < nPackets; j++)
			{
				Json::Value matrixTableNodes = packetNodes[j]["matrixTable"];
				u16 nMatrixIndexes = matrixTableNodes.size();
				WRITE(nMatrixIndexes);

				for (uint k = 0; k < nMatrixIndexes; k++)
				{
					u16 mtxIdx = matrixTableNodes.get(k, 0).asInt();
					WRITE(mtxIdx);
				}

				WRITE(packetIndexCounts[j]);
			}

			free(packetIndexCounts);
		}
	END_SECTION();

	// Skeleton joints
	BEGIN_SECTION("drw1");
	{
		Json::Value weightedNode = root["Drw1"]["isWeighted"];
		Json::Value indexNode = root["Drw1"]["data"];
		uint nNodes = indexNode.size();
		WRITE(nNodes);
		for (uint i = 0; i < nNodes; i++)
		{
			DrwElement drw;
			drw.index = indexNode.get(i, 0).asUInt();
			drw.isWeighted = weightedNode.get(i, false).asBool();
			WRITE(drw);
		}
	}
	END_SECTION();	
	BEGIN_SECTION("jnt1");
	{
		Json::Value frameNode = root["Jnt1"]["frames"];
		uint nNodes = frameNode.size();
		WRITE(nNodes);
		for (uint i = 0; i < nNodes; i++)
		{
			JointElement joint;
			compileFrame(frameNode[i], &joint.matrix);
			strncpy_s(joint.name, frameNode[i].get("name", "UnknownName").asCString(), 16);
			joint.parent = jointParents[i];
			WRITE(joint);
		}
	}
	END_SECTION();	
	BEGIN_SECTION("evp1");
	{
		Json::Value matNode = root["Evp1"]["matrices"];
		Json::Value weightIndexNode = root["Evp1"]["weightedIndices"];
		uint nMatrices = matNode.size();
		uint nWeights = weightIndexNode.size();

		WRITE(nMatrices);
		WRITE(nWeights);

		// These elements will be filled as a postprocess
		long offsetsPos = s.tellp();
		u8*  weightedIndexSizes = (u8*)malloc(sizeof(u8) * nWeights);
		u16* weightedIndexOffsets = (u16*)malloc(sizeof(uint) * nWeights);
		WRITE_ARRAY(weightedIndexSizes, nWeights * sizeof(u8));
		WRITE_ARRAY(weightedIndexOffsets, nWeights * sizeof(u16));

		for (uint i = 0; i < nMatrices; i++)
		{
			mat4 matrix;
			for (uint j = 0; j < 16; j += 4)
			{
				matrix.rows[j/4].x = float(matNode[i].get(uint(j+0), 0).asDouble());
				matrix.rows[j/4].y = float(matNode[i].get(uint(j+1), 0).asDouble());
				matrix.rows[j/4].z = float(matNode[i].get(uint(j+2), 0).asDouble());
				matrix.rows[j/4].w = float(matNode[i].get(uint(j+3), 0).asDouble());
			}
			WRITE(matrix);
		}
		
		for (uint i = 0; i < nWeights; i++)
		{
			Json::Value weightsNode = weightIndexNode[i]["weights"];
			Json::Value indicesNode = weightIndexNode[i]["indices"];
			uint nWeightedIndices = weightsNode.size();

			weightedIndexSizes[i] = nWeightedIndices;
			weightedIndexOffsets[i] = (i == 0) ? 0 : weightedIndexOffsets[i-1] + weightedIndexSizes[i-1];

			for (uint j = 0; j < nWeightedIndices; j++)
			{
				WeightedIndex wi;
				wi.weight = float(weightsNode.get(uint(j), 0).asDouble());
				wi.index = indicesNode.get(uint(j), 0).asUInt();
				WRITE(wi);
			}
		}

		s.seekp(offsetsPos);
		s.write((char*)weightedIndexSizes, nWeights * sizeof(u8));
		s.write((char*)weightedIndexOffsets, nWeights * sizeof(weightedIndexOffsets[0]));
		s.seekp(0, std::ios_base::end);
	}
	END_SECTION();	

	// Vertex, Index Buffers
	BEGIN_SECTION("vib1");
		WRITE(nVertexIndexBuffers);
		for (uint i = 0; i < nVertexIndexBuffers; i++)
		{
			WRITE(vertexBuffers[i].vertexAttributes);
			WRITE(vertexBuffers[i].vertexCount);
			uint vertBufSize = vertexBuffers[i].vertexCount * GC3D::GetVertexSize(FULL_VERTEX_ATTRIBS);
			WRITE_ARRAY(vertexBuffers[i].vertexBuf, vertBufSize);

			WRITE(indexBuffers[i].indexCount);
			WRITE_ARRAY(indexBuffers[i].indexBuf, indexBuffers[i].indexCount * sizeof(u16));
		}
	END_SECTION();

	memcpy(hdr.fourCC, "bmd1", 4);
	hdr.version = COMPILER_VERSION;
	hdr.sizeBytes = totalSize;
	
	*data = (char*) malloc(totalSize);
	s.read(*data, totalSize);

	//Post-processing
	for (uint i = 0; i < currentSection; i++)
	{
		SectionHeader* pHdr = (SectionHeader*)(*data + sectionHeaderOffsets[i]);
		memcpy(pHdr, &sectionHeaders[i], sizeof(SectionHeader));
	}

	u16* pBatchOffsetsTable = (u16*)(*data + batchOffsetsOffset);
	memcpy(pBatchOffsetsTable, batchOffsets, nBatches * sizeof(u16));

cleanup:
	for (uint i = 0; i < nVertexIndexBuffers; i++)
	{
		free(vertexBuffers[i].vertexBuf);
		free(indexBuffers[i].indexBuf);
	}

	free(vertexBuffers);
	free(indexBuffers);
	free(batchOffsets);
	
	DEBUG_ONLY(_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF));

	return S_OK;
}

#define BEGIN_READ_SECTION(FOURCC) { \
	sectionHead = head; \
	section = READ(SectionHeader); \
	assert(memcmp(section.fourcc, FOURCC, 4) == 0); \
}

#define END_READ_SECTION() { \
	head = sectionHead + section.size; \
}

RESULT GDModel::Load(GDModel* model, ModelAsset* asset)
{
	model->_asset = asset;
	ubyte* head = asset;
	ubyte* sectionHead = head;
	SectionHeader section;

	// Scenegraph first	
	BEGIN_READ_SECTION("scn1");
		uint nNodes = READ(uint);
		model->scenegraph = READ_ARRAY(Scenegraph, nNodes);
	END_READ_SECTION();

	// Batches
	BEGIN_READ_SECTION("bch1");
		uint nBatches = READ(u16);
		model->batchOffsetTable = READ_ARRAY(u16, nBatches);
	END_READ_SECTION();
	
	BEGIN_READ_SECTION("drw1");
		uint nDrw = READ(uint);
		model->drwTable = READ_ARRAY(DrwElement, nDrw);
	END_READ_SECTION();

	BEGIN_READ_SECTION("jnt1");
		uint nJnt = READ(uint);
		model->numJoints = nJnt;
		model->jointTable = READ_ARRAY(JointElement, nJnt);

		// TODO: This is temporary until we start loading animations
		// TODO: This is a memory leak, but we're going to leave it since it's temporary
		// Store a copy of the joints as our "default" animation
		uint jointTableSize = sizeof(JointElement) * nJnt;
		model->emptyAnim = (JointElement*) malloc(jointTableSize);
		memcpy(model->emptyAnim, model->jointTable, jointTableSize);
	END_READ_SECTION();
	
	BEGIN_READ_SECTION("evp1");
		uint nMatrices = READ(uint);
		uint nWeights = READ(uint);
		model->evpWeightedIndexSizesTable = READ_ARRAY(u8, nWeights);
		model->evpWeightedIndexOffsetTable = READ_ARRAY(u16, nWeights);
		model->evpMatrixTable = READ_ARRAY(mat4, nMatrices);
		model->evpWeightedIndexTable = READ_ARRAY(WeightedIndex, 0);
	END_READ_SECTION();

	// Vertex, Index Buffer registration is handled on the next draw
	BEGIN_READ_SECTION("vib1");
		model->nVertexIndexBuffers = READ(u16);
		model->vertexIndexBuffers = head;
	END_READ_SECTION();

	model->loadGPU = true;

	return S_OK;
}

void FillMatrixTable(GDModel::GDModel* model, mat4* matrixTable, u16* matrixIndices, u16 nMatrixIndices)
{
	for (uint i = 0; i < nMatrixIndices; i++)
	{
		mat4& matrix = matrixTable[i];
		u16 drwIndex = matrixIndices[i];

		if (drwIndex == 0xffff)
			continue; // keep matrix set by previous packet
		
		GDModel::DrwElement& drw = model->drwTable[drwIndex];
		if (drw.isWeighted)
		{
			// TODO: Optimize this
			memset(&matrix, 0, sizeof(mat4));
			u8 nMatrices = model->evpWeightedIndexSizesTable[drw.index];
			GDModel::WeightedIndex* weightedIndices = 
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

void DrawBatch(Renderer* renderer, ID3D10Device* device, GDModel::GDModel* model, 
			   u16 batchIndex, u16 matIndex)
{
	ubyte* head = model->_asset + model->batchOffsetTable[batchIndex];
	
	VertexBufferID vbID = READ(int);
	IndexBufferID ibID = READ(int);
	u16 numPackets = READ(u16);
	
	// These are partially updated by each packet
	mat4 matrixTable[10];

	int numIndicesSoFar = 0;
	for (uint i = 0; i < numPackets; i++)
	{
		u16 nMatrixIndices = READ(u16);
		u16* matrixIndices = READ_ARRAY(u16, nMatrixIndices);

		// Setup Matrix table
		FillMatrixTable(model, matrixTable, matrixIndices, nMatrixIndices);	
		
		device->IASetPrimitiveTopology(D3D10_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);  

		renderer->reset();
			//applyMaterial(renderer, matIndex);
			renderer->setVertexFormat(model->vertFormat);
			renderer->setShader(model->shaderID);
			renderer->setVertexBuffer(0, vbID);
			renderer->setIndexBuffer(ibID);
			renderer->setShaderConstantArray4x4f("ModelMat", matrixTable, nMatrixIndices);
		renderer->apply();

		u16 indexCount = READ(u16);
		device->DrawIndexed(indexCount, numIndicesSoFar, 0);	
		numIndicesSoFar += indexCount;
	}
}

RESULT GDModel::Update(GDModel* model)
{
	// Grab the root joint straight from the animation. It's parent is the identity
	model->jointTable[0].matrix = model->emptyAnim[0].matrix;

	for (uint i = 1; i < model->numJoints; i++)
	{
		JointElement& joint = model->jointTable[i];

		// Calculate joint from animation at current time
		mat4& animMatrix = model->emptyAnim[i].matrix;

		// Put in parent's frame
		joint.matrix = model->jointTable[joint.parent].matrix * animMatrix;
	}

	return S_OK;
}

//TODO: Remove the need for the D3D reference
const GDModel::Scenegraph* DrawScenegraph(Renderer *renderer, ID3D10Device *device, GDModel::GDModel* model, 
										  const GDModel::Scenegraph* node, uint matIndex = 0, 
										  bool drawOnDown = true)
{
	//TODO: implement drawOnDown. The joint and material states match up with the onDown implementation.
	//		 the only thing that changes is the draw order. Does this make a difference?
	while (node->type != GDModel::SG_END)
	{
		switch(node->type)
		{	
		case GDModel::SG_MATERIAL: 
			WARN("Applying material %u", node->index); 
			matIndex = node->index;
			break;

		case GDModel::SG_PRIM:
			if (drawOnDown)
			{
				WARN("Drawing batch %u with Material%u", node->index, matIndex);
				DrawBatch(renderer, device, model, node->index, matIndex);
			}	
			break;	

		case GDModel::SG_DOWN: 
			node = DrawScenegraph(renderer, device, model, node+1, matIndex, drawOnDown);
			break;

		case GDModel::SG_UP:
			return node;
		}

		node++;
	}

	return nullptr;
}

RESULT GDModel::Draw(Renderer* renderer, ID3D10Device *device, GDModel* model)
{
	if (model->loadGPU)
	{
		// Register vertex and index buffers
		ubyte* head = model->vertexIndexBuffers;
		for (uint i = 0; i < model->nVertexIndexBuffers; i++)
		{
			ubyte* batch = model->_asset + model->batchOffsetTable[i];
			VertexBufferID* batchVBID = (VertexBufferID*)batch;
			IndexBufferID*  batchIBID = (IndexBufferID*)(batch + sizeof(batchVBID));
			assert(*batchVBID == -1 && *batchIBID == -1);

			u16 attributes = READ(u16);
			int numVertices = READ(u16);
			int vbSize = numVertices * GC3D::GetVertexSize(FULL_VERTEX_ATTRIBS);
			void* vertices = READ_ARRAY(ubyte, vbSize);

			int numIndices = READ(u16);
			int ibSize = numIndices * sizeof(u16);
			void* indices = READ_ARRAY(ubyte, ibSize);

			*batchVBID = renderer->addVertexBuffer(vbSize, STATIC, vertices);
			*batchIBID = renderer->addIndexBuffer(ibSize, 2, STATIC, indices);
		}

		// Register our shaders
		model->shaderID = renderer->addShader("Test.shd");

		// TODO: This is currently hacked to be the full vertex every time
		model->vertFormat = GC3D::CreateVertexFormat(renderer, FULL_VERTEX_ATTRIBS, model->shaderID);

		model->loadGPU = false;
	}

	DrawScenegraph(renderer, device, model, model->scenegraph);
	return S_OK; 
}