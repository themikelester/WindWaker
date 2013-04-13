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
std::string getLightCalcString(uint litMask, uint attenuationFunc, uint diffuseFunc, bool alpha)
{
	if (litMask == 0)
		return "0.0f";
	else	
		return "0.5f";
}

std::string getMtxString(TexMtxInfo mtx, bool is4x3)
{
	std::ostringstream out;
	out.setf(std::ios::fixed, std::ios::floatfield);
	out.setf(std::ios::showpoint);

	if (is4x3)
	{
		WARN("Non-identity 4x3 texture matrices definitely aren't yet supported. Applying identity (!)");
		return "{ 1.0f, 0.0f, 0.0f, 0.0f,\n\
				  0.0f, 1.0f, 0.0f, 0.0f,\n\
				  0.0f, 0.0f, 1.0f, 0.0f }";
	}
	else
	{
		WARN("Non-identity texture matrices aren't yet fully supported. Applying best guess (!)");
		out << "{ " << mtx.scaleU << "f, 0.0f, 0.0f, " << mtx.scaleCenterX - 0.5f << "f,\n"
			<< "0.0f, " << mtx.scaleV << "f, 0.0f, " << mtx.scaleCenterY - 0.5f << "f }";
	}

	return out.str();
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

	// Transformation
	out << "Out.Position = mul(ModelMat[In.MatIndex], float4(In.Position, 1.0));" << "\n";
	out << "Out.Position = mul(WorldViewProj, Out.Position);" << "\n";
	out << "\n";
	
	for (uint chanSel = 0; chanSel < nChans; chanSel++)
	{
		ColorChanInfo& chanInfo = matInfo->colorChanInfos[mat.chanControls[chanSel]];
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
		out << chanTarget << " = " << ambColor << " * " << ambLight << " + " << matColor << " * " << diffLight << ";\n";
		out << "\n";
	}

	for (uint i = 0; i < nTexGens; i++)
	{
		TexGenInfo& texGen = matInfo->texGenInfos[mat.texGenInfos[i]];
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
			case GX_TG_MTX2x4:
				out << "float2x4 uvMatrix" << i << " = { "
					<< getMtxString(matInfo->texMtxInfos[mat.texMtxInfos[matIndex]], false) << " };\n";
				out << "Out.TexCoord" << i << " = mul( uvMatrix" << i << ", " << texGenSrc << ");\n";
				break;
			case GX_TG_MTX3x4:
				out << "float3x4 uvMatrix" << i << " = { " 
					<< getMtxString(matInfo->texMtxInfos[mat.texMtxInfos[matIndex]], true) << " };\n";
				out << "float3 uvw = mul( uvMatrix" << i << ", " << texGenSrc << ");\n";
				out << "Out.TexCoord" << i << " = (uvw / uvw.z).xy;\n";
				break;
			case GX_TG_SRTG:
				WARN("Invalid TexGen operation GX_TG_SRTG. A matrix is set but this op would ignore it!");
				out << "Out.TexCoord" << i << " = " << texGenSrc << ".rg;\n";
				break;
			default:
				WARN("Unsupported TexGen operation %u. Defaulting to MTX2x4", texGen.texGenType);
				out << "float2x4 uvMatrix" << i << " = { " 
					<< getMtxString(matInfo->texMtxInfos[mat.texMtxInfos[matIndex]], false) << " };\n";
				out << "Out.TexCoord" << i << " = mul( uvMatrix" << i << ", " << texGenSrc << ");\n";
			}
		}
	}
	out << "\n";

	out << "return Out;" << "\n";
	out << "}\n";

	return out.str();
}