#include "common.h"
#include "util.h"
#include "GC3D.h"

//TODO: Remove all dependencies on gx.h
#include "gx.h"

#include <Framework3\Renderer.h>

namespace GC3D
{
	const int MAX_VERTEX_ATTRIBS = 13;

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
		
	static const int formatSize[] = { 
		sizeof(float), sizeof(half), sizeof(ubyte), sizeof(uint) 
	};
	
	int GetAttributeSize (u16 attrib)
	{
		uint attribIndex = uint(log10(float(attrib)) / log10(2.0f));
		return __GCformat[attribIndex].size * formatSize[__GCformat[attribIndex].format];
	}

	int GetVertexSize (u16 attribFlags)
	{

		int vertSize = 0;
		for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) 
		{
			if ( attribFlags & (1 << i) )
			{
				vertSize += __GCformat[i].size * formatSize[__GCformat[i].format];
			}
		}

		return vertSize;
	}

	void GC3D::ConvertGCVertexFormat (u16 attribFlags, FormatDesc* formatBuf)
	{
		int numAttribs = util::bitcount(attribFlags);

		int formatBufIndex = 0;
		for (int i = 0; i < MAX_VERTEX_ATTRIBS; ++i) 
		{
			if ( attribFlags & (1 << i) )
			{
				formatBuf[formatBufIndex++] = __GCformat[i];
			}
		}
	}

	int GC3D::ConvertGCDepthFunction (u8 gcDepthFunc)
	{
		switch(gcDepthFunc)
		{
		case GX_NEVER:	 return NEVER;	  
		case GX_LESS:	 return LESS;	  
		case GX_EQUAL:	 return EQUAL;	  
		case GX_LEQUAL:	 return LEQUAL;   
		case GX_GREATER: return GREATER;  
		case GX_NEQUAL:	 return NOTEQUAL; 
		case GX_GEQUAL:	 return GEQUAL;   
		case GX_ALWAYS:	 return ALWAYS;   
		default:
			WARN("Unknown compare mode %d. Defaulting to 'ALWAYS'", gcDepthFunc);
			return ALWAYS;
		}
	}
	
	int GC3D::ConvertGCCullMode(u8 gcCullMode)
	{
		switch(gcCullMode)
		{
		case GX_CULL_NONE:  return CULL_NONE; 
		case GX_CULL_BACK:  return CULL_FRONT;
		case GX_CULL_FRONT: return CULL_BACK; 
		case GX_CULL_ALL:   
		default:
			WARN("Unsupported cull mode %u. Defaulting to 'CULL_NONE'", gcCullMode);
			return CULL_NONE;
		}
	}

	int GC3D::ConvertGCBlendFactor(u8 gcBlendFactor)
	{
		switch (gcBlendFactor)
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
				WARN("Unknown blend factor %u. Defaulting to 'ONE'", gcBlendFactor);
				return ONE;
		}
	}

	int GC3D::ConvertGCBlendOp (u8 gcBlendOp)
	{
		switch (gcBlendOp)
		{
			case GX_BM_NONE: return BM_ADD;
			case GX_BM_BLEND: return BM_ADD; 
			case GX_BM_SUBSTRACT: return BM_SUBTRACT; 
			case GX_BM_LOGIC:
			case GX_MAX_BLENDMODE:
			default:
				WARN("Unsupported blend mode %u. Defaulting to 'BM_ADD'", gcBlendOp);
				return BM_ADD;
		}
	}

	FORMAT GC3D::ConvertGCTextureFormat(u8 format)
	{
		// TODO: Define these formats in the common folder and share with Interpreter
		switch (format)
		{
		case /*I8:	*/ 0:	return FORMAT_R8;
		case /*I8_A8*/ 1:	return FORMAT_RG8;
		case /*RGBA8*/ 2:	return FORMAT_RGBA8;
		case /*DXT1 */ 3:	return FORMAT_DXT1;
		case 0xff: //Error case, fall through to default
		default: WARN("Unknown texture format %u. Refusing to load", format); return FORMAT_NONE;
		}
	}
	
	// TODO: DX has support for Mirror address mode. Add support for this in the renderer
	AddressMode GC3D::ConvertGCTexWrap(u8 addressMode)
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
	Filter GC3D::ConvertGCTexFilter(u8 minFilter, u8 magFilter)
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
} // namespace GC3D