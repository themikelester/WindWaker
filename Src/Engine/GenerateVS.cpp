#include <stdio.h>
#include <tchar.h>
#include <stdarg.h>
#include <fstream>
#include <sstream>

#include "json\json.h"

#include "common.h"
#include "gx.h"
#include "BMDRead\bmdread.h"
#include "BMDRead\bck.h"
#include "BMDRead\openfile.h"

#define PI 3.14159265358979323846f

// TODO: Diffuse lighting functions
std::string getLightCalcString(uint litMask, uint attenuationFunc, uint diffuseFunc, bool alpha)
{
	if (litMask == 0)
		return "0.0f";
	else	
		return "0.5f";
}

std::string getColorString(const MColor& color)
{
	std::ostringstream out;
	out.setf(std::ios::fixed, std::ios::floatfield);
	out.setf(std::ios::showpoint);
	out << "float4(" << color.r/255.0f << ", " << color.g/255.0f << ", " << color.b/255.0f << ", " << color.a/255.0f << ")";
	return out.str();
}

std::string getMtxString(uint slot, const TexMtxInfo mtx, std::string texGenSrc)
{
	std::ostringstream out;
	out.setf(std::ios::fixed, std::ios::floatfield);
	out.setf(std::ios::showpoint);
	
	if (mtx.projection == GX_TG_MTX3x4)
	{
		switch(mtx.type)
		{
		case TEXMTX_TNORM:
		case TEXMTX_POS:	
		case TEXMTX_TPOS:
			WARN("3x4 texture matrix type %u not yet supported. Applying identity", mtx.type);
			out << "Out.TexCoord" << slot << " = " << texGenSrc << ".xy;\n";
			break;
		default:
			WARN("Unexpected texture generation type %u for 3x4 projection. \
				 Applying identity", mtx.type);
			out << "Out.TexCoord" << slot << " = " << texGenSrc << ".xy;\n";
			break;
		}
	}
	else if (mtx.projection == GX_TG_MTX2x4)
	{
		float theta = float(mtx.rotate/32768) * PI;
		char matrix[128], scale[128], center[128], uv[128];

		snprintf(scale, 128, "float2 scale%u = float2(%f, %f);\n", slot, mtx.scale_s, mtx.scale_t);
		snprintf(center, 128, "float2 center%u = float2(%f, %f);\n", slot, mtx.center_s, mtx.center_t);
		snprintf(matrix, 128, "float2x3 uvMatrix%u = { %f, %f, %f, %f, %f, %f };\n",
			slot,
			cos(theta), -sin(theta), mtx.translate_s + mtx.center_s,
			sin(theta),  cos(theta), mtx.translate_t + mtx.center_t);
		
		if (mtx.scale_s == 1.0f && mtx.scale_t == 1.0f)
			snprintf(uv, 128, "float3 uv%u = float3( (%s - center%u), 1.0f );\n",
				slot, texGenSrc.c_str(), slot);
		else
		{
			out << scale;
			snprintf(uv, 128, "float3 uv%u = float3( scale%u * (%s - center%u), 1.0f );\n", 
				slot, slot, texGenSrc.c_str(), slot);
		}

		out << "\n";
		out << center;
		out << matrix;
		out << uv;

		switch(mtx.type)
		{
		case TEXMTX_TEXCOORD:
			out << "Out.TexCoord" << slot << " = mul( uvMatrix" << slot << ", uv" << slot << " );\n";
			break;
		case TEXMTX_NORM:		
		case TEXMTX_UNK:
			WARN("2x4 texture matrix type %u not yet supported. Applying identity", mtx.type);
			out << "Out.TexCoord" << slot << " = " << texGenSrc << ".xy;\n";
			break;
		default:
			WARN("Unexpected texture generation type %u for 2x4 projection. \
				 Applying identity", mtx.type);
			out << "Out.TexCoord" << slot << " = " << texGenSrc << ".xy;\n";
			break;
		}
	}

	return out.str();
}

std::string GenerateVS(const Mat3* matInfo, int index)
{
	const Material& mat = matInfo->materials[index];

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
	
	uint nChans = matInfo->numChans[mat.numChansIndex];
	uint nTexGens = matInfo->texGenCounts[mat.texGenCountIndex];

	for (uint i = 0; i < nChans; i++)
	{
		out << "float4 Color" << i << " : Color" << i << ";\n";
	}

	for (uint i = 0; i < nTexGens; i++)
	{
		out << "float2 TexCoord" << i << ": Texcoord" << i << ";\n";
	}

	out << "};" << "\n";
	out << "\n";

	// Main function
	out << "VsOut main(VsIn In)" << "\n";
	out << "{" << "\n";
	out << "VsOut Out;" << "\n";
	out << "\n";

	out << "float4 ambColor0 = " << getColorString(matInfo->ambColor[mat.ambColor[0]]) << ";\n";
	out << "float4 ambColor1 = " << getColorString(matInfo->ambColor[mat.ambColor[1]]) << ";\n";
	out << "float4 matColor0 = " << getColorString(matInfo->matColor[mat.matColor[0]]) << ";\n";
	out << "float4 matColor1 = " << getColorString(matInfo->matColor[mat.matColor[1]]) << ";\n";

	// Transformation
	out << "Out.Position = mul(ModelMat[In.MatIndex], float4(In.Position, 1.0));" << "\n";
	out << "Out.Position = mul(WorldViewProj, Out.Position);" << "\n";
	out << "\n";
	
	for (uint chanSel = 0; chanSel < nChans; chanSel++)
	{
		const ColorChanInfo& chanInfo = matInfo->colorChanInfos[mat.chanControls[chanSel]];
		std::string chanTarget, vtxColor, ambColor, matColor, ambLight, diffLight;
		std::string swizzle, chan;
		bool alpha;

		switch (chanSel)
		{
		case /* Color0 */ 0: chan = "0"; swizzle = ".rgb"; alpha = false; break;
		case /* Alpha0 */ 1: chan = "0"; swizzle = ".a";   alpha = true; break;
		case /* Color1 */ 2: chan = "1"; swizzle = ".rgb"; alpha = false; break;
		case /* Alpha1 */ 3: chan = "1"; swizzle = ".a";   alpha = true; break;
		default:
			WARN("Unknown vertex output color channel %u. Skipping", chanSel);
			continue;
		}

		chanTarget = "Out.Color" + chan + swizzle;
		ambColor = (chanInfo.ambColorSource == GX_SRC_VTX ? "In.VtxColor" : "ambColor") + chan + swizzle;
		matColor = (chanInfo.matColorSource == GX_SRC_VTX ? "In.VtxColor" : "matColor") + chan + swizzle;
		ambLight = "ambLightColor" + swizzle;
		diffLight = getLightCalcString(chanInfo.litMask, chanInfo.attenuationFracFunc, chanInfo.diffuseAttenuationFunc, alpha);

		//Out.Color0.rgb = ambient * ambLightColor + material * light;
		out << "Out.Color" + chan << " = 1.0f;\n";
		if (chanInfo.enable)
			out << chanTarget << " = " << ambColor << " * " << ambLight << " + " << matColor << " * " << diffLight << ";\n";
		else
			out << chanTarget << " = " << matColor << ";\n";
		out << "\n";
	}

	for (uint i = 0; i < nTexGens; i++)
	{
		const TexGenInfo& texGen = matInfo->texGenInfos[mat.texGenInfos[i]];
		std::string texGenSrc, texGenFunc, matrix;
		
		switch(texGen.texGenSrc)
		{
		case GX_TG_POS:  texGenSrc = "Position";	 break;
		case GX_TG_TEX0: texGenSrc = "In.TexCoord0"; break;
		case GX_TG_TEX1: texGenSrc = "In.TexCoord1"; break; 
		case GX_TG_TEX2: texGenSrc = "In.TexCoord2"; break; 
		case GX_TG_TEX3: texGenSrc = "In.TexCoord3"; break; 
		case GX_TG_TEX4: texGenSrc = "In.TexCoord4"; break; 
		case GX_TG_TEX5: texGenSrc = "In.TexCoord5"; break; 
		case GX_TG_TEX6: texGenSrc = "In.TexCoord6"; break; 
		case GX_TG_TEX7: texGenSrc = "In.TexCoord7"; break; 

		// TODO: Verify this is correct
		case GX_TG_TEXCOORD0: texGenSrc = "Out.TexCoord0"; WARN("NOT SURE IF GX_TG_TEXCOORD0 SRC IS CORRECT"); break;
		case GX_TG_TEXCOORD1: texGenSrc = "Out.TexCoord1"; WARN("NOT SURE IF GX_TG_TEXCOORD1 SRC IS CORRECT"); break;
		case GX_TG_TEXCOORD2: texGenSrc = "Out.TexCoord2"; WARN("NOT SURE IF GX_TG_TEXCOORD2 SRC IS CORRECT"); break;
		case GX_TG_TEXCOORD3: texGenSrc = "Out.TexCoord3"; WARN("NOT SURE IF GX_TG_TEXCOORD3 SRC IS CORRECT"); break;
		case GX_TG_TEXCOORD4: texGenSrc = "Out.TexCoord4"; WARN("NOT SURE IF GX_TG_TEXCOORD4 SRC IS CORRECT"); break;
		case GX_TG_TEXCOORD5: texGenSrc = "Out.TexCoord5"; WARN("NOT SURE IF GX_TG_TEXCOORD5 SRC IS CORRECT"); break;
		case GX_TG_TEXCOORD6: texGenSrc = "Out.TexCoord6"; WARN("NOT SURE IF GX_TG_TEXCOORD6 SRC IS CORRECT"); break;

		case GX_TG_COLOR0: texGenSrc = "Out.Color0"; break;
		case GX_TG_COLOR1: texGenSrc = "Out.Color1"; break;

		case GX_TG_NRM: 
		case GX_TG_TANGENT:  
		case GX_TG_BINRM:  
		default: 
			WARN("Unsupported TexGen source %u. Defaulting to GX_TG_TEXCOORD0", texGen.texGenSrc);
			texGenSrc = "In.TexCoord0";
		}

		if (texGen.matrix == GX_IDENTITY)
		{
			switch (texGen.texGenType)
			{
			case GX_TG_MTX2x4:
				out << "Out.TexCoord" << i << " = " << texGenSrc << ".xy;\n";
				break;
			case GX_TG_MTX3x4:
				out << "float3 uvw = " << texGenSrc << ".xyz;\n";
				out << "Out.TexCoord" << i << " = (uvw / uvw.z).xy;\n";
				break;
			case GX_TG_SRTG:
				out << "Out.TexCoord" << i << " = " << texGenSrc << ".rg;\n";
				break;
			default:
				WARN("Unsupported TexGen operation %u. Defaulting to MTX2x4", texGen.texGenType);
				out << "Out.TexCoord" << i << " = " << texGenSrc << ".xy;\n";
			}
		}
		else
		{
			uint matIndex = (texGen.matrix - 30) / 3; // See GX_TEXMTX0 - GX_TEXMTX9 in gx.h
			switch (texGen.texGenType)
			{
			default: WARN("Unsupported TexGen operation %u. Defaulting to MTX2x4", texGen.texGenType);
			case GX_TG_MTX2x4:
			case GX_TG_MTX3x4:
				out	<< getMtxString(i, matInfo->texMtxInfos[mat.texMtxInfos[matIndex]], texGenSrc);
				break;
			case GX_TG_SRTG:
				WARN("Invalid TexGen operation GX_TG_SRTG. A matrix is set but this op would ignore it!");
				out << "Out.TexCoord" << i << " = " << texGenSrc << ".rg;\n";
				break;
			}
		}
	}
	out << "\n";

	out << "return Out;" << "\n";
	out << "}\n";

	return out.str();
}