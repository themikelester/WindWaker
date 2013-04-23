#pragma once

#include <Framework3\Renderer.h>
#include "Types.h"

namespace GC3D
{
	int GetAttributeSize (u16 attrib);
	int GetVertexSize (u16 attribFlags);
	
	int ConvertGCDepthFunction (u8 gcDepthFunc);
	int ConvertGCBlendFactor(u8 gcBlendFactor);
	int ConvertGCBlendOp (u8 gcBlendOp);
	int ConvertGCCullMode(u8 gcCullMode);
	FORMAT ConvertGCTextureFormat(u8 format);
	Filter ConvertGCTexFilter(u8 magFilter, u8 minFilter);
	AddressMode ConvertGCTexWrap(u8 wrapMode);
	void ConvertGCVertexFormat (u16 attribFlags, FormatDesc* formatBuf);

} // namespace GC3D