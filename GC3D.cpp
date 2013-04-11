#include "GC3D.h"

#include <Framework3\Renderer.h>
#include "Foundation\hash.h"
#include "Foundation\murmur_hash.h"
#include "Foundation\string_stream.h"
#include "Foundation\memory.h"
#include "Types.h"
#include "util.h"
#include "gx.h"

using namespace foundation;

#define ALLOC_SCRATCH memory_globals::default_scratch_allocator()
#define ALLOC_DEFAULT memory_globals::default_allocator()

static Hash<ShaderID>* __shaderMap;
static Hash<VertexFormatID>* __vertexFormatMap;

// These are in separate files because they're quite large
extern std::string GenerateVS(Mat3* matInfo, int index);
extern std::string GeneratePS(Tex1* texInfo, Mat3* matInfo, int index);

namespace GC3D
{
	const int MAX_VERTEX_ATTRIBS = 13;
	static const u64 hashSeed = 0x12345678;

	FormatDesc __GCformat[MAX_VERTEX_ATTRIBS] =
	{
		// Stream, Type, Format, Size
		{ 0, TYPE_GENERIC,	FORMAT_UINT,  1 }, // Matrix Index
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, 3 }, // Position
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, 3 }, // Normal
		{ 0, TYPE_COLOR,	FORMAT_UBYTE, 4 }, // Color 0
		{ 0, TYPE_COLOR,	FORMAT_UBYTE, 4 }, // Color 1
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 }, // Texture Coordinate 0
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 }, // Texture Coordinate 1
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 }, // Texture Coordinate 2
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 }, // Texture Coordinate 3
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 }, // Texture Coordinate 4
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 }, // Texture Coordinate 5
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 }, // Texture Coordinate 6
		{ 0, TYPE_TEXCOORD, FORMAT_FLOAT, 2 }, // Texture Coordinate 7
	};

	uint __AttribComponentSize[MAX_VERTEX_ATTRIBS] =
	{
		sizeof(uint) , // Matrix Index
		sizeof(float), // Position
		sizeof(float), // Normal
		sizeof(ubyte), // Color 0
		sizeof(ubyte), // Color 1
		sizeof(float), // Texture Coordinate 0
		sizeof(float), // Texture Coordinate 1
		sizeof(float), // Texture Coordinate 2
		sizeof(float), // Texture Coordinate 3
		sizeof(float), // Texture Coordinate 4
		sizeof(float), // Texture Coordinate 5
		sizeof(float), // Texture Coordinate 6
		sizeof(float), // Texture Coordinate 7
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

	int GetAttributeSize (Renderer* renderer, u16 attrib)
	{
		uint attribIndex = uint(log10(attrib) / log10(2));
		return __GCformat[attribIndex].size * renderer->getFormatSize(__GCformat[attribIndex].format);
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
	
	ShaderID CreateShader (Renderer* renderer, Tex1* texInfo, Mat3* matInfo, int matIndex)
	{
		std::string vs = GenerateVS(matInfo, matIndex);
		std::string ps = GeneratePS(texInfo, matInfo, matIndex);
		return renderer->addShader(vs.c_str(), nullptr, ps.c_str(), 0, 0, 0); 
	}

	VertexFormatID CreateVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader)
	{
		int numAttribs = util::bitcount(attribFlags);
		FormatDesc formatBuf[MAX_VERTEX_ATTRIBS];

		int formatBufIndex = 0;
		for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) 
		{
			if ( attribFlags & (1 << i) )
			{
				formatBuf[formatBufIndex++] = __GCformat[i];
			}
		}

		return renderer->addVertexFormat(formatBuf, numAttribs, shader); 
	}
	
	DepthStateID CreateDepthState (Renderer* renderer, ZMode mode)
	{
		bool depthTestEnable = mode.enable;
		bool depthWriteEnable = mode.enableUpdate;
		int depthFunc;

		switch(mode.zFunc)
		{
		case GX_NEVER:	 depthFunc = NEVER;	   break;
		case GX_LESS:	 depthFunc = LESS;	   break;
		case GX_EQUAL:	 depthFunc = EQUAL;	   break;
		case GX_LEQUAL:	 depthFunc = LEQUAL;   break;
		case GX_GREATER: depthFunc = GREATER;  break;
		case GX_NEQUAL:	 depthFunc = NOTEQUAL; break;
		case GX_GEQUAL:	 depthFunc = GEQUAL;   break;
		case GX_ALWAYS:	 depthFunc = ALWAYS;   break;
		default:
			WARN("Unknown compare mode %d. Defaulting to 'ALWAYS'", mode.zFunc);
			depthFunc = ALWAYS;
		}

		return renderer->addDepthState(depthTestEnable, depthWriteEnable, depthFunc);
	}
	
	RasterizerStateID CreateRasterizerState (Renderer* renderer, uint gcCullMode)
	{
		uint cullMode;

		switch(gcCullMode)
		{
		case GX_CULL_NONE:  cullMode = CULL_NONE; break;
		case GX_CULL_BACK:  cullMode = CULL_FRONT; break;
		case GX_CULL_FRONT: cullMode = CULL_BACK; break;
		case GX_CULL_ALL:   
		default:
			WARN("Unsupported cull mode %u. Defaulting to 'CULL_NONE'", gcCullMode);
			cullMode = CULL_NONE;
		}

		return renderer->addRasterizerState(cullMode);
	}

	int ConvertGCBlendFactor(int blendFactor)
	{
		switch (blendFactor)
		{
			case GX_BL_ZERO			: return ZERO;
			case GX_BL_ONE			: return ONE;
			case GX_BL_SRCCLR		: return SRC_COLOR;
			case GX_BL_INVSRCCLR	: return ONE_MINUS_SRC_COLOR;
			case GX_BL_SRCALPHA		: return SRC_ALPHA;
			case GX_BL_INVSRCALPHA	: return ONE_MINUS_SRC_ALPHA;
			case GX_BL_DSTALPHA		: return DST_ALPHA;
			case GX_BL_INVDSTALPHA	: return ONE_MINUS_DST_ALPHA;
			default:
				WARN("Unknown blend factor %u. Defaulting to 'ONE'", blendFactor);
				return ONE;
		}
	}

	BlendStateID CreateBlendState (Renderer* renderer, BlendInfo blendInfo)
	{
		int srcFactor = ConvertGCBlendFactor(blendInfo.srcFactor);
		int dstFactor = ConvertGCBlendFactor(blendInfo.dstFactor);
		int blendOp;

		switch (blendInfo.blendMode)
		{
			case GX_BM_NONE: 
				srcFactor = ONE;
				dstFactor = ZERO;
				blendOp = BM_ADD;
				break;

			case GX_BM_BLEND: 
				blendOp = BM_ADD; 
				break;

			case GX_BM_SUBSTRACT: 
				blendOp = BM_SUBTRACT; 
				break;

			case GX_BM_LOGIC:
			case GX_MAX_BLENDMODE:
			default:
				WARN("Unsupported blend mode %u. Defaulting to 'BM_ADD'", blendInfo.blendMode);
				blendOp = BM_ADD;
		}

		return renderer->addBlendState(srcFactor, dstFactor, blendOp);
	}

	ShaderID GetShader (Renderer* renderer, u16 attribFlags)
	{
		ShaderID shdID;
		u64 hashKey;
		
		//TODO: Move all hashing functionality like this up one level
		//		GC3D should only be handling the creation. This is app logic.
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
		
		//TODO: Move all hashing functionality like this up one level
		//		GC3D should only be handling the creation. This is app logic.
		hashKey = murmur_hash_64(&attribFlags, sizeof(attribFlags), hashSeed);
		vfID = hash::get(*__vertexFormatMap, hashKey, VF_NONE);

		if (vfID == VF_NONE)
		{
			vfID = CreateVertexFormat(renderer, attribFlags, shader);
			hash::set(*__vertexFormatMap, hashKey, vfID);
		}
		
		return vfID;
	}

	FORMAT ConvertGCFormat(int format)
	{
		switch (format)
		{
		//TODO: Are these supposed to be signed formats or not?
		case I8:	return FORMAT_R8;
		case I8_A8:	return FORMAT_RG8;
		case RGBA8: return FORMAT_RGBA8;
		case DXT1:	return FORMAT_DXT1;
		case 0xff: //Error case, fall through to default
		default: WARN("Unknown texture format %u. Refusing to load", format); return FORMAT_NONE;
		}
	}
	
	// TODO: DX has support for Mirror address mode. Add support for this in the renderer
	AddressMode ConvertGCAddressMode(int addressMode)
	{
	    //from gx.h:
	    //0: clamp to edge
	    //1: repeat
	    //2: mirror
		switch (addressMode)
		{
		case 0: return CLAMP;
		case 1: return WRAP;
		case 2: WARN("Address mode 'Mirror' is currently unsupported. Defaulting to 'Clamp'"); return CLAMP;
		default: WARN("Unsupported Address mode %u. Defaulting to 'Clamp'", addressMode); return CLAMP;
		}
	}
	
	// TODO: DX supports min and mag filters. Add support to the renderer so that we can use both.
	Filter ConvertGCTextureFilters(int minFilter, int magFilter)
	{
		if (magFilter != minFilter)
			WARN("Renderer does not support different texture filter types for Minification and Magnification. Rendering may be incorrect");

		//0 - nearest
		//1 - linear
		//2 - near_mip_near
		//3 - lin_mip_near
		//4 - near_mip_lin
		//5 - lin_mip_lin
		switch (magFilter)
		{
		case 0: return NEAREST;
		case 1: return LINEAR; 
		case 2: return NEAREST;
		case 3: return LINEAR; 
		case 4: return TRILINEAR;
		case 5: return TRILINEAR;
		default: WARN("Unknown filter type %u. Reverting to NEAREST", magFilter); 
			return NEAREST; 
		}
	}

	SamplerStateID CreateSamplerState(Renderer* renderer, ImageHeader* imgHdr)
	{
		return renderer->addSamplerState(
					ConvertGCTextureFilters(imgHdr->magFilter, imgHdr->minFilter), 
					ConvertGCAddressMode(imgHdr->wrapS), 
					ConvertGCAddressMode(imgHdr->wrapT),
					CLAMP);
	}

	TextureID CreateTexture (Renderer* renderer, Image1* img)
	{
		Image imgResource;

		FORMAT format = ConvertGCFormat(img->format);
		if (format == FORMAT_NONE)
		{
			// Fail if we can't determine the format
			return TEXTURE_NONE;
		}

		// This is probably reading the mipmap data incorrectly
		imgResource.loadFromMemory(&img->imageData[0], format, 
			img->width, img->height, 1, img->mipmaps.size(), false);
		
		return renderer->addTexture(imgResource);
	}

} // namespace GC3D