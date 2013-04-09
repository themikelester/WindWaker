#pragma once

#include <Framework3\Renderer.h>
#include "Foundation\collection_types.h"
#include "Common\Types.h"
#include "BMDLoader\tex1.h"
#include "BMDLoader\mat3.h"

namespace GC3D
{
	void Init();
	void Shutdown();
	
	int GetAttributeSize (Renderer* renderer, u16 attrib);
	int GetVertexSize (Renderer* renderer, u16 attribFlags);
	
	SamplerStateID CreateSamplerState(Renderer* renderer, ImageHeader* imgHdr);
	TextureID CreateTexture (Renderer* renderer, Image1* imgHdr);
	
	ShaderID CreateShader (Renderer* renderer, Mat3* matInfo, int matIndex);
	ShaderID CreateShader (Renderer* renderer, u16 attribFlags);
	VertexFormatID CreateVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader);
	
	ShaderID GetShader (Renderer* renderer, u16 attribFlags);
	VertexFormatID GetVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader);

	DepthStateID CreateDepthState (Renderer* renderer, ZMode mode);

} // namespace GC3D