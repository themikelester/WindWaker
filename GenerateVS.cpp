#include "common.h"
#include "gx.h"
#include "BMDLoader\mat3.h"
#include "BMDLoader\shp1.h"
#include <sstream>

inline bool hasAttrib(int flags, BatchAttributeFlags attrib)
{
	return (flags & attrib) == attrib;
}

std::string GenerateVS(Mat3* matInfo, int index)
{
	Material& mat = matInfo->materials[index];

	std::ostringstream out;
	out.setf(std::ios::fixed, std::ios::floatfield);
	out.setf(std::ios::showpoint);

	// Constant buffers
	out << "cbuffer g_PerFrame" << "\n";
	out << "{" << "\n";
	out << "  float4x4 WorldViewProj;" << "\n";
	out << "}" << "\n";
	out << "\n";
	
	out << "cbuffer PerPacket" << "\n";
	out << "{" << "\n";
	out << "  float4x4 ModelMat[10];" << "\n";
	out << "}" << "\n";
	out << "\n";

	// In/Out structures
	out << "struct VsIn" << "\n";
	out << "{" << "\n";
	out << "uint   MatIndex : Generic;" << "\n";
	out << "float3 Position : Position;" << "\n";
	out << "float4 Color0	: Color0;" << "\n";
	out << "float4 Color1	: Color1;" << "\n";
	out << "float2 TexCoord0: Texcoord0;" << "\n";
	out << "float2 TexCoord1: Texcoord1;" << "\n";
	out << "float2 TexCoord2: Texcoord2;" << "\n";
	out << "float2 TexCoord3: Texcoord3;" << "\n";
	out << "float2 TexCoord4: Texcoord4;" << "\n";
	out << "float2 TexCoord5: Texcoord5;" << "\n";
	out << "float2 TexCoord6: Texcoord6;" << "\n";
	out << "float2 TexCoord7: Texcoord7;" << "\n";
	out << "};" << "\n";
	out << "\n";
	
	out << "struct VsOut" << "\n";
	out << "{" << "\n";
	out << "float4 Position : SV_Position;" << "\n";
	out << "uint4  Color0	: Color0;" << "\n";
	out << "uint4  Color1	: Color1;" << "\n";
	out << "float2 TexCoord0: Texcoord0;" << "\n";
	out << "float2 TexCoord1: Texcoord1;" << "\n";
	out << "float2 TexCoord2: Texcoord2;" << "\n";
	out << "float2 TexCoord3: Texcoord3;" << "\n";
	out << "float2 TexCoord4: Texcoord4;" << "\n";
	out << "float2 TexCoord5: Texcoord5;" << "\n";
	out << "float2 TexCoord6: Texcoord6;" << "\n";
	out << "float2 TexCoord7: Texcoord7;" << "\n";
	out << "};" << "\n";
	out << "\n";

	// Main function
	out << "VsOut main(VsIn In)" << "\n";
	out << "{" << "\n";
	out << "VsOut Out;" << "\n";
	out << "\n";

	// Transformation
	out << "Out.Position = mul(ModelMat[In.MatIndex], float4(In.Position, 1.0));" << "\n";
	out << "Out.Position = mul(WorldViewProj, Out.Position);" << "\n";
	out << "\n";

	// TODO: Handle TexGen
	//Attribute pass-through
	for (uint i = 0; i < 8; i++)
	{
		// TODO: Replace all magic numbers like '8' here with macros defined in some common file
		// Pass through all of the texcoords that we have
		out << "Out.TexCoord" << i << " = In.TexCoord" << i << ";\n";
	}
	out << "\n";

	// TODO: Handle Color transforms
	//Color pass-through
	ColorChanInfo& chanInfo = matInfo->colorChanInfos[mat.chanControls[0]];
    if(chanInfo.matColorSource == 1)
      out << "Out.Color0 = In.Color0;\n";
    else
    {
      const MColor& c = matInfo->ambColor[mat.ambColor[0]];
      out << "Out.Color0 = float4("
          << c.r/255.f << ", " << c.g/255.f << ", " << c.b/255.f << ", "
          << c.a/255.f << ");\n";
    }

	out << "Out.Color1 = In.Color1;\n";
	out << "\n";

	out << "return Out;" << "\n";
	out << "}\n";

	return out.str();
}

	//// In/Out structures
	//out << "struct VsIn" << "\n";
	//out << "{" << "\n";
	//out << "uint   MatIndex : Generic;" << "\n";
	//out << "float3 Position : Position;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_COLORS0))		out << "uint4  Color0	: Color0;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_COLORS1))		out << "uint4  Color1	: Color1;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS0))		out << "float2 TexCoord0: Texcoord0;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS1))		out << "float2 TexCoord1: Texcoord1;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS2))		out << "float2 TexCoord2: Texcoord2;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS3))		out << "float2 TexCoord3: Texcoord3;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS4))		out << "float2 TexCoord4: Texcoord4;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS5))		out << "float2 TexCoord5: Texcoord5;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS6))		out << "float2 TexCoord6: Texcoord6;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS7))		out << "float2 TexCoord7: Texcoord7;" << "\n";
	//out << "};" << "\n";
	//out << "\n";
	//
	//out << "struct VsOut" << "\n";
	//out << "{" << "\n";
	//out << "float4 Position : SV_Position;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_COLORS0))		out << "uint4  Color0	: Color0;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_COLORS1))		out << "uint4  Color1	: Color1;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS0))		out << "float2 TexCoord0: Texcoord0;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS1))		out << "float2 TexCoord1: Texcoord1;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS2))		out << "float2 TexCoord2: Texcoord2;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS3))		out << "float2 TexCoord3: Texcoord3;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS4))		out << "float2 TexCoord4: Texcoord4;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS5))		out << "float2 TexCoord5: Texcoord5;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS6))		out << "float2 TexCoord6: Texcoord6;" << "\n";
	//if (hasAttrib(vertAttribs, HAS_TEXCOORDS7))		out << "float2 TexCoord7: Texcoord7;" << "\n";
	//out << "};" << "\n";
	//out << "\n";