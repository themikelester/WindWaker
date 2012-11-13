#pragma once

#include <Framework3\Renderer.h>
#include "Types.h"
#include "util.h"
#include "Mem.h"
#include <string>
#include <hash_map>


int vertexSize;

#define VERTEX_SIZE(x) 1

namespace GC3D
{
	#define MAX_VERTEX_ATTRIBS 13

	FormatDesc __GCformat[MAX_VERTEX_ATTRIBS] =
	{
		{ 0, TYPE_GENERIC,	FORMAT_FLOAT, 0 }, // Empty, because the first attribute is MATRICES which is not a vertex attrib
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, 3 },
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, 3 },
		{ 0, TYPE_COLOR,	FORMAT_UBYTE, 4 },
		{ 0, TYPE_COLOR,	FORMAT_UBYTE, 4 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 },
	};

	char* __ShaderSwitches[MAX_VERTEX_ATTRIBS] =
	{
		"#define HAS_MATRICES\n",
		"#define HAS_POSITIONS\n",
		"#define HAS_NORMALS\n",
		"#define HAS_COLORS0\n",
		"#define HAS_COLORS1\n",
		"#define HAS_TEXCOORDS0\n",
		"#define HAS_TEXCOORDS1\n",
		"#define HAS_TEXCOORDS2\n",
		"#define HAS_TEXCOORDS3\n",
		"#define HAS_TEXCOORDS4\n",
		"#define HAS_TEXCOORDS5\n",
		"#define HAS_TEXCOORDS6\n",
		"#define HAS_TEXCOORDS7\n",
	};

	int GetVertexSize (Renderer* renderer, u16 attribFlags)
	{
		int vertSize = 0;
		for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) 
		{
			if ( attribFlags | (1 << i) )
			{
				vertSize += __GCformat[i].size * renderer->getFormatSize(__GCformat[i].format);
			}
		}

		return vertSize;
	}

	std::hash_map<u16, ShaderID> shaderMap;
	std::hash_map<u16, VertexFormatID> vertexFormatMap;

	ShaderID CreateShader (Renderer* renderer, u16 attribFlags)
	{
		std::string extra;
		
		int formatBufIndex = 0;
		for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) 
		{
			if ( attribFlags & (1 << i) )
			{
				extra.append(__ShaderSwitches[i]);
			}
		}

		return renderer->addShader("../Vertices.shd", extra.c_str());
	}

	VertexFormatID CreateVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader)
	{
		int numAttribs = bitcount(attribFlags & ~1);
		FormatDesc formatBuf[MAX_VERTEX_ATTRIBS];

		int formatBufIndex = 0;
		// Start i at 1 to skip MATRICES, which is a patch attribute but not a vertex attribute
		for (int i = 1; i < MAX_VERTEX_ATTRIBS; ++i) 
		{
			if ( attribFlags & (1 << i) )
			{
				formatBuf[formatBufIndex++] = __GCformat[i];
			}
		}

		return renderer->addVertexFormat(formatBuf, numAttribs, shader); 
	}

	ShaderID GetShader (Renderer* renderer, u16 attribFlags)
	{
		ShaderID shdID;

		auto iter = shaderMap.find(attribFlags);
		if ( iter == shaderMap.end( ) )
		{
			shdID = CreateShader(renderer, attribFlags);
			shaderMap.insert( std::pair<u16, ShaderID> (attribFlags, shdID) );
		}
		else 
		{
			shdID = iter->second;
		}
		
		return shdID;
	}

	VertexFormatID GetVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader)
	{
		VertexFormatID vfID;

		auto iter = vertexFormatMap.find(attribFlags);
		if ( iter == vertexFormatMap.end( ) )
		{
			vfID = CreateVertexFormat(renderer, attribFlags, shader);
			vertexFormatMap.insert( std::pair<u16, VertexFormatID> (attribFlags, vfID) );
		}
		else 
		{
			vfID = iter->second;
		}
		
		return vfID;
	}

} // namespace GC3D