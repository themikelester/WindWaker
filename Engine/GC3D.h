#pragma once

#include <Framework3\Renderer.h>
#include "Foundation\collection_types.h"
#include "Types.h"
#include "BMDLoad\tex1.h"
#include "BMDLoad\mat3.h"

namespace GC3D
{
	void Init();
	void Shutdown();
	
	int GetAttributeSize (u16 attrib);
	int GetVertexSize (u16 attribFlags);

	FORMAT ConvertGCTextureFormat(u8 format);
	int ConvertGCDepthFunction (u8 gcDepthFunc);
	int ConvertGCBlendFactor(u8 gcBlendFactor);
	int ConvertGCBlendOp (u8 gcBlendOp);
	int ConvertGCCullMode(u8 gcCullMode);
	Filter ConvertGCTexFilter(u8 magFilter, u8 minFilter);
	AddressMode ConvertGCTexWrap(u8 wrapMode);

	// TODO: Remove these, they can't be used at compile time
	SamplerStateID CreateSamplerState(Renderer* renderer, ImageHeader* imgHdr);
	TextureID CreateTexture (Renderer* renderer, Image1* imgHdr);
	
	ShaderID CreateShader (Renderer* renderer, Tex1* texInfo, Mat3* matInfo, int matIndex);
	VertexFormatID CreateVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader);
	VertexFormatID GetVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader);

	DepthStateID CreateDepthState (Renderer* renderer, ZMode mode);
	BlendStateID CreateBlendState (Renderer* renderer, BlendInfo blendInfo, u8 mask);
	RasterizerStateID CreateRasterizerState (Renderer* renderer, uint cullMode);

} // namespace GC3D