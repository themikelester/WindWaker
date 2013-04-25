#include "Framework3\Renderer.h"
#include "GDModel.h"
#include "Compile.h"
#include "GC3D.h"
#include "util.h"

#include <sstream>
#include <fstream>

#define COMPILER_VERSION 1;
#define WRITE(val) { s.write( (char*)&val, sizeof(val) ); totalSize += sizeof(val); }
#define WRITE_ARRAY(arr, size) { s.write((char*)arr, size); totalSize += size; }
#define READ(type) *(type*)head; head += sizeof(type);
#define READ_ARRAY(type, count) (type*)head; head += sizeof(type) * count;

//TODO: HACK: Remove this!
#define FULL_VERTEX_ATTRIBS 0x1fff

#define MAX_NAME_LENGTH 16

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

	if (s != identity4())
		WARN("This model is using joint scaling. This is not yet tested!");

  //return t*rz*ry*rx*s; //scales seem to be local only
  *matrix = t*rz*ry*rx;
}

// Read and interpret Batch1 vertex attribs
u16 compileAttribs(Json::Value& attribsNode)
{
	u16 attribFlags = 0;

	if (attribsNode.get("mtx", false).asBool()) attribFlags |= HAS_MATRIX_INDICES;
	if (attribsNode.get("pos", false).asBool()) attribFlags |= HAS_POSITIONS;
	if (attribsNode.get("nrm", false).asBool()) attribFlags |= HAS_NORMALS;

	if (attribsNode["clr"].get(uint(0), false).asBool()) attribFlags |= HAS_COLORS0;
	if (attribsNode["clr"].get(uint(1), false).asBool()) attribFlags |= HAS_COLORS1;
	
	if (attribsNode["tex"].get(uint(0), false).asBool()) attribFlags |= HAS_TEXCOORDS0;
	if (attribsNode["tex"].get(uint(1), false).asBool()) attribFlags |= HAS_TEXCOORDS1;
	if (attribsNode["tex"].get(uint(2), false).asBool()) attribFlags |= HAS_TEXCOORDS2;
	if (attribsNode["tex"].get(uint(3), false).asBool()) attribFlags |= HAS_TEXCOORDS3;
	if (attribsNode["tex"].get(uint(4), false).asBool()) attribFlags |= HAS_TEXCOORDS4;
	if (attribsNode["tex"].get(uint(5), false).asBool()) attribFlags |= HAS_TEXCOORDS5;
	if (attribsNode["tex"].get(uint(6), false).asBool()) attribFlags |= HAS_TEXCOORDS6;
	if (attribsNode["tex"].get(uint(7), false).asBool()) attribFlags |= HAS_TEXCOORDS7;

	return attribFlags;
}

RESULT buildInflatedVertex(ubyte* dst, Point& point, u16 attribs, const Json::Value& vtx)
{
	uint attribSize;
	uint vtxSize = GC3D::GetVertexSize(FULL_VERTEX_ATTRIBS);

	// Clear the vertex so that all of our missing data is zero'd
	memset(dst, 0, vtxSize);
	
	if ( (attribs & HAS_POSITIONS) == false )
		WARN("Model does not have required attributes");

	// TODO: We can save a uint in the vertex structure if we remove this force
	// Always set this attribute. The default is 0.
	attribSize = GC3D::GetAttributeSize(HAS_MATRIX_INDICES);
	if (attribs & HAS_MATRIX_INDICES) {
		ASSERT(point.mtxIdx/3 < 10); 
		uint matrixIndex = point.mtxIdx/3;
		memcpy(dst, &matrixIndex, attribSize);
	}
	else
	{
		memset(dst, 0, attribSize);
	}
	dst += attribSize;
	
	attribSize = GC3D::GetAttributeSize(HAS_POSITIONS);
	if (attribs & HAS_POSITIONS) {
		float3 pos;
		pos.x = float(vtx["positions"][point.posIdx].get(uint(0), 0).asDouble());
		pos.y = float(vtx["positions"][point.posIdx].get(uint(1), 0).asDouble());
		pos.z = float(vtx["positions"][point.posIdx].get(uint(2), 0).asDouble());
		memcpy(dst, &pos, attribSize);
	}
	dst += attribSize;
	
	attribSize = GC3D::GetAttributeSize(HAS_NORMALS);
	if (attribs & HAS_NORMALS) {
		float3 nrm;
		nrm.x = float(vtx["normals"][point.nrmIdx].get(uint(0), 0).asDouble());
		nrm.y = float(vtx["normals"][point.nrmIdx].get(uint(1), 0).asDouble());
		nrm.z = float(vtx["normals"][point.nrmIdx].get(uint(2), 0).asDouble());
		memcpy(dst, &nrm, attribSize);
	}
	dst += attribSize;

	for (uint i = 0; i < 2; i++)
	{
		u16 colorAttrib = HAS_COLORS0 << i;
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
		u16 texAttrib = HAS_TEXCOORDS0 << i;
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
	std::map<u64, u16> indexSet;
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
				uint index;
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
				uint64_t hashKey = util::hash64(&p, sizeof(p), seed);
				auto indexPair = indexSet.find(hashKey);
				if (indexPair != indexSet.end())
				{
					// An equivalent vertex already exists, use that index
					index = indexPair->second;
				} else {
					// This points to a new vertex. Construct it.
					index = vertexCount++;
					buildInflatedVertex(vertices + vertexSize*index, p, vertexAttributes, vtx);
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

	// Post-processing data
	const uint numSections = 9;
	uint sectionHeaderOffsets[numSections];
	SectionHeader sectionHeaders[numSections];
	
	uint batchOffsetsOffset;
	
	VertexBuffer* vertexBuffers;
	IndexBuffer* indexBuffers;
	u16 nVertexIndexBuffers = 0;

	std::vector<u16> jointParents;

	uint totalTexSize = 0;

	// Scenegraph
	BEGIN_SECTION("scn1");
		Scenegraph sg;
		// TODO: Switch all of the Json::Value references to use &! This should speed it up
		Json::Value sgNode = root["Inf1"]["scenegraph"];
		uint nNodes = sgNode.size();
		WRITE(nNodes);

		// Table to convert scene node indexes into material indexes
		Json::Value indexToMatIndex = root["Mat3"]["indexToMatIndex"];

		std::stack<u16> parentJoints;
		parentJoints.push(-1);
		u16 tempParentJoint = parentJoints.top();
		for (uint i = 0; i < nNodes; i++)
		{
			sg.index = u16(sgNode[i].get("index", 0).asUInt());
			sg.type = u16(sgNode[i].get("type", 0).asUInt());

			switch(sg.type)
			{
			case SG_MATERIAL:
				sg.index = indexToMatIndex.get(sg.index, 0).asUInt();		
				break;

			case SG_JOINT:
				jointParents.push_back(parentJoints.top());
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
		// TODO: These should be offsets from the start of the batch list. Not _asset.
		batchOffsetsOffset = totalSize;
		uint* batchOffsets = (uint*) malloc(sizeof(uint) * nBatches);
		WRITE_ARRAY(batchOffsets, sizeof(uint) * nBatches);

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

	// Materials
	BEGIN_SECTION("mat1");
	{		
		// Depth Modes
		Json::Value depthNodes = root["Mat3"]["zModes"];
		u16 nDepthStates = depthNodes.size();
		WRITE(nDepthStates);
		for (uint i = 0; i < nDepthStates; i++)
		{
			DepthMode dm;
			dm.testEnable = depthNodes[i].get("enable", true).asBool();
			dm.writeEnable = depthNodes[i].get("enableUpdate", true).asBool();
			dm.func = GC3D::ConvertGCDepthFunction(depthNodes[i].get("zFunc", 3).asUInt());
			WRITE(dm);
		}
		
		// Blend Modes
		Json::Value blendNodes = root["Mat3"]["blendInfos"];
		u16 nBlendStates = blendNodes.size();
		WRITE(nBlendStates);
		for (uint i = 0; i < nBlendStates; i++)
		{
			BlendMode bm;
			bm.blendOp = GC3D::ConvertGCBlendOp(blendNodes[i].get("blendMode", 0).asUInt());
			bm.srcFactor = GC3D::ConvertGCBlendFactor(blendNodes[i].get("srcFactor", 0).asUInt());
			bm.dstFactor = GC3D::ConvertGCBlendFactor(blendNodes[i].get("dstFactor", 0).asUInt());
			WRITE(bm);
		}

		// Cull Modes
		Json::Value cullNodes = root["Mat3"]["cullModes"];
		u16 nCullStates = cullNodes.size();
		WRITE(nCullStates);
		for (uint i = 0; i < nCullStates; i++)
		{
			u8 cullMode = GC3D::ConvertGCCullMode(cullNodes[i].asUInt());
			WRITE(cullMode);
		}

		// MaterialInfo info
		Json::Value matNodes = root["Mat3"]["materials"];
		Json::Value texLookupNodes = root["Mat3"]["texStageIndexToTextureIndex"];
		u16 nMaterials = matNodes.size();
		WRITE(nMaterials);
		for (uint i = 0; i < nMaterials; i++)
		{
			MaterialInfo mat;
			mat.shader = i;
			mat.depthMode = matNodes[i].get("zModeIndex", 0).asUInt();
			mat.blendMode = matNodes[i].get("blendIndex", 0).asUInt();
			mat.rasterMode = matNodes[i].get("cullIndex", 0).asUInt();

			for (uint j = 0; j < 8; j++)
			{
				int stageIndex = matNodes[i]["texStages"].get(uint(j), 0xffff).asUInt();
				mat.samplers[j] = texLookupNodes.get(stageIndex, 0xffff).asUInt();
				mat.textures[j] = 0;
			}

			WRITE(mat);
		}

		// MaterialInfo names
		Json::Value nameNodes = root["Mat3"]["stringtable"];
		u16 nMaterialNames = nameNodes.size();
		WRITE(nMaterialNames);
		for (uint i = 0; i < nMaterialNames; i++)
		{
			std::string name = nameNodes.get(uint(0), "UNKNOWN").asString();
			WRITE_ARRAY(name.c_str(), name.size());
		}
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
		std::streamoff offsetsPos = s.tellp();
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

		free(weightedIndexOffsets);
		free(weightedIndexSizes);
	}
	END_SECTION();	

	// Shader HLSL
	BEGIN_SECTION("shd1");
	{
		Json::Value hlslNodes = root["Shd1"];
		u16 nNodes = hlslNodes.size();
		WRITE(nNodes);
		
		std::streamoff offsetsPos = s.tellp();
		uint* vsOffsets = (uint*) malloc(nNodes * sizeof(uint));
		uint* psOffsets = (uint*) malloc(nNodes * sizeof(uint));
		uint vsTotalSize = 0;
		uint psTotalSize = 0;
		WRITE_ARRAY(vsOffsets, nNodes * sizeof(uint));
		WRITE_ARRAY(psOffsets, nNodes * sizeof(uint));
		WRITE(vsTotalSize);
		WRITE(psTotalSize);

		for (uint i = 0; i < nNodes; i++)
		{
			vsOffsets[i] = vsTotalSize;
			std::string vs = hlslNodes[i].get("VS", "").asString();
			WRITE_ARRAY(vs.c_str(), vs.size()+1);
			vsTotalSize += vs.size()+1;
		}
		
		for (uint i = 0; i < nNodes; i++)
		{
			psOffsets[i] = psTotalSize;
			std::string ps = hlslNodes[i].get("PS", "").asString();
			WRITE_ARRAY(ps.c_str(), ps.size()+1);
			psTotalSize += ps.size()+1;
		}
		
		s.seekp(offsetsPos);
		s.write((char*)vsOffsets, nNodes * sizeof(uint));
		s.write((char*)psOffsets, nNodes * sizeof(uint));
		s.write((char*)&vsTotalSize, sizeof(uint));
		s.write((char*)&psTotalSize, sizeof(uint));
		s.seekp(0, std::ios_base::end);

		free(vsOffsets);
		free(psOffsets);
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

	// Textures and samplers
	BEGIN_SECTION("tex1");
	{
		// TODO: Turn the samplers into a set (using hashing) and simplify this
		Json::Value hdrsNode = root["Tex1"]["imageHdrs"];
		u16 nHdrs = hdrsNode.size();
		WRITE(nHdrs);
		for (uint i = 0; i < nHdrs; i++)
		{
			TextureResource res;

			const char* name = hdrsNode[i].get("name", "UNKNOWN").asCString();
			memcpy(res.name, name, 16);

			u8 magFilter = hdrsNode[i].get("magFilter", 0).asUInt();
			u8 minFilter = hdrsNode[i].get("minFilter", 0).asUInt();
			res.filter = GC3D::ConvertGCTexFilter(magFilter, minFilter);
			res.wrapS = GC3D::ConvertGCTexWrap(hdrsNode[i].get("wrapS", 0).asUInt());
			res.wrapT = GC3D::ConvertGCTexWrap(hdrsNode[i].get("wrapT", 0).asUInt());
			res.texIndex = hdrsNode[i].get("imageIdx", 0).asUInt();
			
			WRITE(res);
		}

		Json::Value imgsNode = root["Tex1"]["images"];
		u16 nImgs = imgsNode.size();
		WRITE(nImgs);
		for (uint i = 0; i < nImgs; i++)
		{
			TextureDesc tex;
			uint gcFormat = imgsNode[i].get("format", 0).asUInt();
			tex.format = GC3D::ConvertGCTextureFormat(gcFormat);
			tex.width = imgsNode[i].get("width", 0).asUInt();
			tex.height = imgsNode[i].get("height", 0).asUInt();

			Json::Value mipSizes = imgsNode[i]["mipmapSizes"];
			tex.numMips = mipSizes.size();
			
			uint texSize = 0;
			for (uint j = 0; j < tex.numMips; j++) { texSize += mipSizes[j].asUInt(); }
			tex.sizeBytes = texSize;
			totalTexSize += texSize;

			tex.texDataOffset = imgsNode[i].get("imageDataOffset", 0).asUInt();

			WRITE(tex);
		}
	}
	END_SECTION();

	uint blobSize = totalSize + totalTexSize;

	*data = (char*) malloc(blobSize);
	s.read(*data, totalSize);

	//Append our binary texture data
	std::string filename = root["Info"]["name"].asString();
	std::ifstream texFile(filename + ".tex", std::ios::in | std::ios::binary);
	if (!texFile.is_open())
	{
		//TODO: Test this
		WARN("Failed to open texture file %s, no textures will be loaded!", filename + ".tex");
		memset(*data + totalSize, 0, totalTexSize);
	}
	else
	{
		texFile.seekg (0, std::ios::end);
		std::streamoff length = texFile.tellg();
		texFile.seekg (0, std::ios::beg);
		ASSERT(length == totalTexSize);
		texFile.read(*data + totalSize, totalTexSize);
	}

	//Post-processing
	for (uint i = 0; i < numSections; i++)
	{
		SectionHeader* pHdr = (SectionHeader*)(*data + sectionHeaderOffsets[i]);
		memcpy(pHdr, &sectionHeaders[i], sizeof(SectionHeader));
	}

	uint* pBatchOffsetsTable = (uint*)(*data + batchOffsetsOffset);
	memcpy(pBatchOffsetsTable, batchOffsets, nBatches * sizeof(uint));

	memcpy(hdr.fourCC, "bmd1", 4);
	hdr.version = COMPILER_VERSION;
	hdr.sizeBytes = blobSize;

	for (uint i = 0; i < nVertexIndexBuffers; i++)
	{
		free(vertexBuffers[i].vertexBuf);
		free(indexBuffers[i].indexBuf);
	}

	free(vertexBuffers);
	free(indexBuffers);
	free(batchOffsets);
	
	texFile.close();

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

		renderer->reset();
			ApplyMaterial(renderer, model->materials[matIndex]);
			renderer->setVertexFormat(model->vertFormat);
			renderer->setVertexBuffer(0, vbID);
			renderer->setIndexBuffer(ibID);
			renderer->setShaderConstantArray4x4f("ModelMat", matrixTable, nMatrixIndices);
		renderer->apply();

		u16 indexCount = READ(u16);
		renderer->drawElements(PRIM_TRIANGLE_STRIP, numIndicesSoFar, indexCount, 0, -1);
		numIndicesSoFar += indexCount;
	}
}

const Scenegraph* DrawScenegraph(
		Renderer *renderer, GDModel::GDModel* model, const Scenegraph* node, 
		uint matIndex = 0, bool drawOnDown = true )
{
	//TODO: implement drawOnDown. The joint and material states match up with the onDown implementation.
	//		 the only thing that changes is the draw order. Does this make a difference?
	
	//TODO: Remove drawing with scenegraph. We should assign materials to batches and then draw batches linearly
	//		We don't even need batches, just draw packets in sequence, occasionally switching materials
	while (node->type != SG_END)
	{
		switch(node->type)
		{	
		case SG_MATERIAL: 
			WARN("Applying material %u", node->index); 
			matIndex = node->index;
			break;

		case SG_PRIM:
			if (drawOnDown)
			{
				WARN("Drawing batch %u with MaterialInfo%u", node->index, matIndex);
				DrawBatch(renderer, model, node->index, matIndex);
			}	
			break;	

		case SG_DOWN: 
			node = DrawScenegraph(renderer, model, node+1, matIndex, drawOnDown);
			break;

		case SG_UP:
			return node;
		}

		node++;
	}

	return nullptr;
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
			
		imgResource.loadFromMemory(gfxData.textureData + tex.texDataOffset, tex.format, 
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

	// TODO: This is currently hacked to be the full vertex every time
	FormatDesc formatBuf[13];
	GC3D::ConvertGCVertexFormat(FULL_VERTEX_ATTRIBS, formatBuf);
	model->vertFormat = renderer->addVertexFormat(formatBuf, util::bitcount(FULL_VERTEX_ATTRIBS), shaders[0]);

	// Register vertex and index buffers
	ubyte* head = gfxData.vertexIndexBuffers;
	for (uint i = 0; i < gfxData.nVertexIndexBuffers; i++)
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
			ubyte* batch = model->_asset + model->batchOffsetTable[i];
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
	free(model->emptyAnim);
	
	// Clear the whole model for safety
	memset(model, 0, sizeof(model));

	return r;
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
		model->batchOffsetTable = READ_ARRAY(uint, nBatches);
	END_READ_SECTION();
	
	BEGIN_READ_SECTION("mat1");
		u16 nDepthModes = READ(u16);
		model->gfxData.nDepthModes = nDepthModes;
		model->gfxData.depthModes = READ_ARRAY(DepthMode, nDepthModes);
		
		u16 nBlendModes = READ(u16);
		model->gfxData.nBlendModes = nBlendModes;
		model->gfxData.blendModes = READ_ARRAY(BlendMode, nBlendModes);

		u16 nCullModes = READ(u16);
		model->gfxData.nCullModes = nCullModes;
		model->gfxData.cullModes = READ_ARRAY(u8, nCullModes);

		u16 nMaterials = READ(u16);
		model->nMaterials = nMaterials;
		model->materials = READ_ARRAY(MaterialInfo, nMaterials);
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

	BEGIN_READ_SECTION("shd1");
		u16 nShaders = READ(u16);
		model->gfxData.nShaders = nShaders;

		model->gfxData.vsOffsets = READ_ARRAY(uint, nShaders);
		model->gfxData.psOffsets = READ_ARRAY(uint, nShaders);
		uint vsTotalSize = READ(uint);
		uint psTotalSize = READ(uint);
		model->gfxData.vsShaders = READ_ARRAY(char, vsTotalSize);
		model->gfxData.psShaders = READ_ARRAY(char, psTotalSize);
	END_READ_SECTION();

	// Vertex, Index Buffer registration is handled on the next draw
	BEGIN_READ_SECTION("vib1");
		model->gfxData.nVertexIndexBuffers = READ(u16);
		model->gfxData.vertexIndexBuffers = head;
	END_READ_SECTION();

	BEGIN_READ_SECTION("tex1");
		u16 nTextureResources = READ(u16);
		model->gfxData.nTextureResources = nTextureResources;
		model->gfxData.textureResources = READ_ARRAY(TextureResource, nTextureResources);

		u16 nTextures = READ(u16);
		model->gfxData.nTextures = nTextures;
		model->gfxData.textures = READ_ARRAY(TextureDesc, nTextures);
	END_READ_SECTION();

	model->gfxData.textureData = head;

	model->loadGPU = true;

	return S_OK;
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

RESULT GDModel::Draw(Renderer* renderer, GDModel* model)
{
	if (model->loadGPU)
	{
		RegisterGFX(renderer, model);
		model->loadGPU = false;
	}

	DrawScenegraph(renderer, model, model->scenegraph);

	return S_OK; 
}