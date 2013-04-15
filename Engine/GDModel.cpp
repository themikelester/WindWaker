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
#define WRITE(val) { s.write( (char*)&val, sizeof(val) ); _size += sizeof(val); }
#define WRITE_ARRAY(arr, size) { s.write((char*)arr, size); _size += size; }
#define READ(type) *(type*)head; head += sizeof(type);
#define READ_ARRAY(type, count) (type*)head; head += sizeof(type) * count;

//TODO: HACK: Remove this!
#define FULL_VERTEX_ATTRIBS 0x1fff

//read and interpret Batch1 vertex attribs
u16 compileAttribs(Json::Value attribsNode)
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
		pos.x = float(vtx["positions"][point.posIdx].get("x", 0).asDouble());
		pos.y = float(vtx["positions"][point.posIdx].get("y", 0).asDouble());
		pos.z = float(vtx["positions"][point.posIdx].get("z", 0).asDouble());
		memcpy(dst, &pos, attribSize);
	}
	dst += attribSize;
	
	attribSize = GC3D::GetAttributeSize(GDModel::HAS_NORMALS);
	if (attribs & GDModel::HAS_NORMALS) {
		float3 nrm;
		nrm.x = float(vtx["normals"][point.nrmIdx].get("x", 0).asDouble());
		nrm.y = float(vtx["normals"][point.nrmIdx].get("y", 0).asDouble());
		nrm.z = float(vtx["normals"][point.nrmIdx].get("z", 0).asDouble());
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
			st.x = float(vtx["texcoords"][point.texIdx[i]].get("s", 0).asDouble());
			st.y = float(vtx["texcoords"][point.texIdx[i]].get("t", 0).asDouble());
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

			// TODO: We're still writing the crazy ass type version to the intermediate file
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
				p.nrmIdx = (*point).get("pos", 0).asUInt();
				p.posIdx = (*point).get("nrm", 0).asUInt();
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

RESULT GDModel::Compile(const Json::Value& root, Header& hdr, char** data)
{
	std::stringstream s;
	uint _size = 0;
	
	//TODO: HACK: Remove this
	foundation::memory_globals::init(4 * 1024 * 1024);

	// We don't write these until the end, so save them
	VertexBuffer* vertexBuffers;
	IndexBuffer* indexBuffers;
	u16 nVertexIndexBuffers = 0;

	// Scenegraph
	{
		Scenegraph sg;
		Json::Value sgNode = root["Inf1"]["scenegraph"];
		uint nNodes = sgNode.size();
		WRITE(nNodes);
		for (uint i = 0; i < nNodes; i++)
		{
			sg.index = u16(sgNode[i].get("index", 0).asUInt());
			sg.type = u16(sgNode[i].get("type", 0).asUInt());
			WRITE(sg);
		}
	}

	// Batches
	{
		Json::Value batchNodes = root["Shp1"]["batches"];
		u16 nBatches = batchNodes.size();
		WRITE(nBatches);
		
		nVertexIndexBuffers = nBatches;
		vertexBuffers = (VertexBuffer*) malloc(sizeof(VertexBuffer) * nBatches);
		indexBuffers = (IndexBuffer*) malloc(sizeof(IndexBuffer) * nBatches);

		for (uint i = 0; i < nBatches; i++)
		{
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
	}

	// Vertex Buffers
	{
		WRITE(nVertexIndexBuffers);
		for (uint i = 0; i < nVertexIndexBuffers; i++)
		{
			WRITE(vertexBuffers[i].vertexAttributes);
			WRITE(vertexBuffers[i].vertexCount);
			uint vertBufSize = vertexBuffers[i].vertexCount * GC3D::GetVertexSize(vertexBuffers[i].vertexAttributes);
			WRITE_ARRAY(vertexBuffers[i].vertexBuf, vertBufSize);

			WRITE(indexBuffers[i].indexCount);
			WRITE_ARRAY(indexBuffers[i].indexBuf, indexBuffers[i].indexCount * sizeof(u16));
		}
	}

	memcpy(hdr.fourCC, "bmd1", 4);
	hdr.version = COMPILER_VERSION;
	hdr.sizeBytes = _size;
	
	*data = (char*) malloc(_size);
	s.read(*data, _size);

cleanup:
	for (uint i = 0; i < nVertexIndexBuffers; i++)
	{
		free(vertexBuffers[i].vertexBuf);
		free(indexBuffers[i].indexBuf);
	}

	free(vertexBuffers);
	free(indexBuffers);

	return S_OK;
}

RESULT GDModel::Load(GDModel* model, ModelAsset* asset)
{
	model->_asset = asset;
	ubyte* head = asset;

	// Scenegraph first	
	model->nScenegraphNodes = READ(uint);
	model->scenegraph = READ_ARRAY(Scenegraph, model->nScenegraphNodes);
	
	return S_OK;
}


//TODO: Remove the need for the D3D reference
const GDModel::Scenegraph* DrawScenegraph(Renderer *renderer, ID3D10Device *device, const GDModel::Scenegraph* node, 
									uint jointIdx = 0, uint materialIdx = 0, bool drawOnDown = true)
{
	//TODO: implement drawOnDown. The joint and material states match up with the onDown implementation.
	//		 the only thing that changes is the draw order. Does this make a difference?

	while (node->type != GDModel::SG_END)
	{
		switch(node->type)
		{
		case GDModel::SG_JOINT: 
			WARN("Applying joint %u", node->index); 
			jointIdx = node->index;
			break;
	
		case GDModel::SG_MATERIAL: 
			WARN("Applying material %u", node->index); 
			materialIdx = node->index;
			break;

		case GDModel::SG_PRIM:
			if (drawOnDown)
				WARN("Drawing batch %u with Joint=%u and Material%u", node->index, jointIdx, materialIdx);
			break;	

		case GDModel::SG_DOWN: 
			node = DrawScenegraph(renderer, device, node+1, jointIdx, materialIdx, drawOnDown);
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
	DrawScenegraph(renderer, device, model->scenegraph);
	return S_OK;
}