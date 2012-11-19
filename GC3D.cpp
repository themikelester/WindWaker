#include "GC3D.h"

#include <Framework3\Renderer.h>
#include "Foundation\hash.h"
#include "Foundation\murmur_hash.h"
#include "Foundation\string_stream.h"
#include "Foundation\memory.h"
#include "Types.h"
#include "util.h"

using namespace foundation;

#define ALLOC_SCRATCH memory_globals::default_scratch_allocator()
#define ALLOC_DEFAULT memory_globals::default_allocator()

static Hash<ShaderID>* __shaderMap;
static Hash<VertexFormatID>* __vertexFormatMap;

namespace GC3D
{
	const int MAX_VERTEX_ATTRIBS = 13;
	static const u64 hashSeed = 0x12345678;

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

	const char* __ShaderSwitches[MAX_VERTEX_ATTRIBS] =
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

	void Init()
	{
		__shaderMap = MAKE_NEW(ALLOC_DEFAULT, Hash<ShaderID>, ALLOC_DEFAULT);
		__vertexFormatMap = MAKE_NEW(ALLOC_DEFAULT, Hash<VertexFormatID>, ALLOC_DEFAULT);
	}

	void Shutdown()
	{
		MAKE_DELETE(ALLOC_DEFAULT, Hash<ShaderID>, __shaderMap);
		MAKE_DELETE(ALLOC_DEFAULT, Hash<VertexFormatID>, __vertexFormatMap);
	}

	int GetVertexSize (Renderer* renderer, u16 attribFlags)
	{
		int vertSize = 0;
		for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) 
		{
			if ( attribFlags & (1 << i) )
			{
				vertSize += __GCformat[i].size * renderer->getFormatSize(__GCformat[i].format);
			}
		}

		return vertSize;
	}

	ShaderID CreateShader (Renderer* renderer, u16 attribFlags)
	{
		using namespace string_stream;
		
		Buffer shaderDefines(ALLOC_SCRATCH);
		
		// TODO: Could probably clean this up by moving the #define's inside of here 
		// and listing them explicitly
		int formatBufIndex = 0;
		for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) 
		{
			if ( attribFlags & (1 << i) )
			{
				shaderDefines << __ShaderSwitches[i];
			}
		}

		return renderer->addShader("../Vertices.shd", string_stream::c_str(shaderDefines));
	}

	VertexFormatID CreateVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader)
	{
		int numAttribs = util::bitcount(attribFlags & ~1);
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
		u64 hashKey;

		hashKey = murmur_hash_64(&attribFlags, sizeof(attribFlags), hashSeed);
		shdID = hash::get(*__shaderMap, hashKey, SHADER_NONE);

		if (shdID == SHADER_NONE)
		{
			shdID = CreateShader(renderer, attribFlags);
			hash::set(*__shaderMap, hashKey, shdID);
		}
		
		return shdID;
	}

	VertexFormatID GetVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader)
	{
		VertexFormatID vfID;
		u64 hashKey;
		
		hashKey = murmur_hash_64(&attribFlags, sizeof(attribFlags), hashSeed);
		vfID = hash::get(*__vertexFormatMap, hashKey, VF_NONE);

		if (vfID == VF_NONE)
		{
			vfID = CreateVertexFormat(renderer, attribFlags, shader);
			hash::set(*__vertexFormatMap, hashKey, vfID);
		}
		
		return vfID;
	}

} // namespace GC3D