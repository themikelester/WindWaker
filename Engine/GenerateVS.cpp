#include "common.h"
#include "gx.h"
#include "BMDLoad\mat3.h"
#include "BMDLoad\shp1.h"
#include <sstream>

inline bool hasAttrib(int flags, BatchAttributeFlags attrib)
{
	return (flags & attrib) == attrib;
}

// TODO: Diffuse lighting functions
std::string getLightCalcString(uint litMask, uint attenuationFunc, uint diffuseFunc)
{
	if (litMask == 0)
		return "0.0f";
	else	
		return "0.5f";
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
	out << "  float4 ambLightColor;" << "\n";
	out << "}" << "\n";
	out << "\n";

	out << "cbuffer PerMaterial" << "\n";
	out << "{" << "\n";
	out << "  float4 matColor0; //Material" << "\n";
	out << "  float4 matColor1; //Material" << "\n";
	out << "  float4 ambColor0; //Ambient" << "\n";
	out << "  float4 ambColor1; //Ambient" << "\n";
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
	out << "float4 VtxColor0: Color0;" << "\n";
	out << "float4 VtxColor1: Color1;" << "\n";
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
	
	// chanSel is 0, 1, or 2
	for (uint chanSel = 0; chanSel < matInfo->numChans[mat.numChansIndex]; chanSel++)
	{
		ColorChanInfo& chanInfo = matInfo->colorChanInfos[mat.chanControls[chanSel]];
		std::string chanTarget, vtxColor, ambColor, matColor, ambLight, diffLight;
		std::string chan = (chanSel == 0 ? "0" : "1");


		chanTarget = "Out.Color" + chan;
		ambColor = (chanInfo.ambColorSource == GX_SRC_VTX ? "In.VtxColor" : "ambColor") + chan;
		matColor = (chanInfo.matColorSource == GX_SRC_VTX ? "In.VtxColor" : "matColor") + chan;
		ambLight = "ambLightColor";
		diffLight = getLightCalcString(chanInfo.litMask, chanInfo.attenuationFracFunc, chanInfo.diffuseAttenuationFunc);

		//Out.Color0 = ambient * ambLightColor + material * light;
		out << chanTarget << " = " << ambColor << " * " << ambLight << " + " << matColor << " * " << diffLight << ";\n";
		out << "\n";
	}

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