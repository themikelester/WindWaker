#pragma once

#include <Framework3\Renderer.h>
#include "Foundation\collection_types.h"
#include "Common\Types.h"

namespace GC3D
{
	void Init();
	void Shutdown();
	
	int GetAttributeSize (Renderer* renderer, u16 attrib);
	int GetVertexSize (Renderer* renderer, u16 attribFlags);

	ShaderID CreateShader (Renderer* renderer, u16 attribFlags);
	VertexFormatID CreateVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader);
	
	ShaderID GetShader (Renderer* renderer, u16 attribFlags);
	VertexFormatID GetVertexFormat (Renderer* renderer, u16 attribFlags, ShaderID shader);

} // namespace GC3D