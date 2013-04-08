#include "common.h"
#include "gx.h"
#include "BMDLoader\mat3.h"
#include <sstream>
#include "GC3D.h"


const std::string varResultName = "out";
const std::string varRegisterName[3] = {"r0", "r1", "r2"};

std::string GetRegisterString(int regIndex)
{
	if (regIndex == 0) return varResultName;
	else return varRegisterName[regIndex-1];
}

std::string GetTexTapString(const TevOrderInfo* texMapping)
{
	// "Texture0.Sample( Sampler0, In.TexCoord0 )"
	std::ostringstream out;
	out << "Texture" << (int)texMapping->texMap << ".Sample( Sampler" << (int)texMapping->texMap 
		<< ", In.TexCoord" << (int)texMapping->texCoordId << " )";
	return out.str();
}

std::string GetVertColorString(const TevOrderInfo* texMapping)
{
	// "In.Color0"
	switch (texMapping->chanId)
	{
		case GX_COLOR0: return "In.Color0.rgb";
		case GX_COLOR1: return "In.Color1.rgb";
		case GX_ALPHA0: return "In.Color0.a";
		case GX_ALPHA1: return "In.Color1.a";
		case GX_COLOR0A0: return "In.Color0.rgba";
		case GX_COLOR1A1: return "In.Color1.rgba";
		case GX_COLORZERO: return "float3(0.0f)";
		case GX_BUMP:
		case GX_BUMPN:
		case GX_COLORNULL:  
		default:
			{
			  WARN("GetVertColorString(): Unknown chanId 0x%x", texMapping->chanId);
			  return "float4(0.0f, 1.0f, 0.0f, 1.0f)";
			}
	}
}

std::string GetKonstColorString(int konst)
{
	switch (konst)
	{
		case GX_TEV_KCSEL_1:	return "float4(1.0f)";
		case GX_TEV_KCSEL_7_8:	return "float4(0.875f)";
		case GX_TEV_KCSEL_3_4:	return "float4(0.75f)";
		case GX_TEV_KCSEL_5_8:	return "float4(0.625f)";
		case GX_TEV_KCSEL_1_2:	return "float4(0.5f)";
		case GX_TEV_KCSEL_3_8:	return "float4(0.375f)";
		case GX_TEV_KCSEL_1_4:	return "float4(0.25f)";
		case GX_TEV_KCSEL_1_8:	return "float4(0.125f)";
		case GX_TEV_KCSEL_K0:	return "konst0.rgba";
		case GX_TEV_KCSEL_K1:	return "konst1.rgba";
		case GX_TEV_KCSEL_K2:	return "konst2.rgba";
		case GX_TEV_KCSEL_K3:	return "konst3.rgba";
		case GX_TEV_KCSEL_K0_R: return "konst0.r";
		case GX_TEV_KCSEL_K1_R:	return "konst1.r";
		case GX_TEV_KCSEL_K2_R:	return "konst2.r";
		case GX_TEV_KCSEL_K3_R:	return "konst3.r";
		case GX_TEV_KCSEL_K0_G:	return "konst0.g";
		case GX_TEV_KCSEL_K1_G:	return "konst1.g";
		case GX_TEV_KCSEL_K2_G:	return "konst2.g";
		case GX_TEV_KCSEL_K3_G:	return "konst3.g";
		case GX_TEV_KCSEL_K0_B:	return "konst0.b";
		case GX_TEV_KCSEL_K1_B:	return "konst1.b";
		case GX_TEV_KCSEL_K2_B:	return "konst2.b";
		case GX_TEV_KCSEL_K3_B:	return "konst3.b";
		case GX_TEV_KCSEL_K0_A:	return "konst0.a";
		case GX_TEV_KCSEL_K1_A:	return "konst1.a";
		case GX_TEV_KCSEL_K2_A:	return "konst2.a";
		case GX_TEV_KCSEL_K3_A:	return "konst3.a";
		default:
			WARN("GetKonstColorString(): Unknown konstIndex 0x%x", konst);
			return "float4(0.0f, 1.0f, 0.0f, 1.0f)";
	}
}

std::string GetKonstAlphaString(int konst)
{
	switch (konst)
	{
	case GX_TEV_KASEL_1:	return "1.0f";
	case GX_TEV_KASEL_7_8:	return "0.875f";
	case GX_TEV_KASEL_3_4:	return "0.75f";
	case GX_TEV_KASEL_5_8:	return "0.625f";
	case GX_TEV_KASEL_1_2:	return "0.5f";
	case GX_TEV_KASEL_3_8:	return "0.375f";
	case GX_TEV_KASEL_1_4:	return "0.25f";
	case GX_TEV_KASEL_1_8:	return "0.125f";
	case GX_TEV_KASEL_K0_R:	return "konst0.r";
	case GX_TEV_KASEL_K1_R:	return "konst1.r";
	case GX_TEV_KASEL_K2_R:	return "konst2.r";
	case GX_TEV_KASEL_K3_R:	return "konst3.r";
	case GX_TEV_KASEL_K0_G:	return "konst0.g";
	case GX_TEV_KASEL_K1_G:	return "konst1.g";
	case GX_TEV_KASEL_K2_G:	return "konst2.g";
	case GX_TEV_KASEL_K3_G:	return "konst3.g";
	case GX_TEV_KASEL_K0_B:	return "konst0.b";
	case GX_TEV_KASEL_K1_B:	return "konst1.b";
	case GX_TEV_KASEL_K2_B:	return "konst2.b";
	case GX_TEV_KASEL_K3_B:	return "konst3.b";
	case GX_TEV_KASEL_K0_A:	return "konst0.a";
	case GX_TEV_KASEL_K1_A:	return "konst1.a";
	case GX_TEV_KASEL_K2_A:	return "konst2.a";
	case GX_TEV_KASEL_K3_A:	return "konst2.a";
	default:
		WARN("GetKonstAlphaString(): Unknown konstIndex 0x%x", konst);
		return "float(1.0f)";
	}						
}							
							
std::string GetColorInString(int inputType, int konst, const TevOrderInfo* texMapping)
{
	switch (inputType)
	{
		case GX_CC_CPREV:	return varResultName + ".rgb";
		case GX_CC_APREV:	return varResultName + ".aaa";
		case GX_CC_C0:		return varRegisterName[0] + ".rgb";
		case GX_CC_A0:		return varRegisterName[0] + ".aaa";
		case GX_CC_C1:		return varRegisterName[1] + ".rgb";
		case GX_CC_A1:		return varRegisterName[1] + ".aaa";
		case GX_CC_C2:		return varRegisterName[2] + ".rgb";
		case GX_CC_A2:		return varRegisterName[2] + ".aaa";
		case GX_CC_TEXC:	return GetTexTapString(texMapping) + ".rgb";
		case GX_CC_TEXA:	return GetTexTapString(texMapping) + ".aaa";
		case GX_CC_RASC:	return GetVertColorString(texMapping) + ".rgb";
		case GX_CC_RASA:	return GetVertColorString(texMapping) + ".aaa";
		case GX_CC_ONE:		return "float3(1.0f)";
		case GX_CC_HALF:	return "float3(0.5f)";
		case GX_CC_KONST:	return GetKonstColorString(konst) + ".rgb";
		case GX_CC_ZERO:	return "float3(0.0f)";
		default:
			WARN("GetColorInString(): Unknown inputType %d", (u32)inputType);
			return "float4(0.0f)";
	}
}

std::string GetAlphaInString(int inputType, int konst, const TevOrderInfo* texMapping)
{
	switch (inputType)
	{
	case GX_CA_APREV:	return GetRegisterString(0) + ".a";
	case GX_CA_A0:		return GetRegisterString(1) + ".a";
	case GX_CA_A1:		return GetRegisterString(2) + ".a";
	case GX_CA_A2:		return GetRegisterString(3) + ".a";
	case GX_CA_TEXA:	return GetTexTapString(texMapping) + ".a";
	case GX_CA_RASA:	return GetVertColorString(texMapping) + ".a";
	case GX_CA_KONST:	return GetKonstAlphaString(konst) + ".a";
	case GX_CA_ZERO:	return "0.0f";
	default:
		WARN("GetAlphaInString(): Unknown inputType %d", (u32)inputType);
		return "float4(0.0f)";
	}
}

std::string GetModString(int outputRegIndex, int bias, int scale, bool clamp, bool isAlpha)
{
	static const float biasLUT[3] = {0, 0.5f, -0.5f};
	static const float scaleLUT[4] = {1, 2, 4, 0.5f};
	
	std::string channelSelect = (isAlpha ? ".a" : ".rgb");
	std::ostringstream out;
	std::string dest = GetRegisterString(outputRegIndex) + channelSelect;

	// result = saturate(result * scale  + bias * scale);
	out << dest << " = " << (clamp ? "saturate(" : "(") << dest << " * " << scaleLUT[scale] 
		<< " + " << biasLUT[bias] * scaleLUT[scale] << ");";

	return out.str();
}

std::string GetColorOpString(int op, int bias, int scale, bool clamp, int outputRegIndex, std::string input[4])
{
	std::string channelSelect = ".rgb";
	std::ostringstream str;
	std::string dest = GetRegisterString(outputRegIndex) + channelSelect;

	switch (op)
	{
	case GX_TEV_ADD: 
		//out = (d + lerp(a, b, c));
		str << dest << " = " << "(" << input[3] << " + lerp(" 
			<< input[0] + ", " << input[1] + ", "  << input[2] << "));\n";
		str << GetModString(outputRegIndex, bias, scale, clamp, false) + "\n";
		break;

	case GX_TEV_SUB:		
		//out = (d - lerp(a, b, c));
		//out = saturate(out * scale + result * scale);
		str << dest << " = " << "(" << input[3] << " - lerp(" << input[0] + ", " << input[1] + ", " << input[2] << "));\n";
		str << GetModString(outputRegIndex, bias, scale, clamp, false) + "\n";
		break;

	case GX_TEV_COMP_R8_GT:
	case GX_TEV_COMP_R8_EQ:	
		//out = (d + ((a.r > b.r) ? c : 0))
		str << dest << " = " << "(" << input[3] << " + ((" 
			<< input[0]+".r" 
			<< (op == GX_TEV_COMP_R8_GT ? " > " : " == ") 
			<< input[1]+".r" 
			<< ") ? " << input[2] << " : 0));\n";
		break;

	case GX_TEV_COMP_GR16_GT:
	case GX_TEV_COMP_GR16_EQ:	
	{
		//out = (d + (dot(a.gr, rgTo16Bit) > dot(b.gr, rgTo16Bit) ? c : 0));
		std::string grTo16Bit = "vec2(255.0/65535.0, 255.0*256.0/65535.0)";
		str << dest << " = " << "(" << input[3] << " + (" 
			<< " dot(" << input[0]+".gr" << ", " << grTo16Bit << ")"
			<< (op == GX_TEV_COMP_GR16_GT ? " > " : " == ") 
			<< " dot(" << input[1]+".gr" << ", " << grTo16Bit << ")"
			<< " ? " << input[2] << " : 0));\n";
		break;
	}

	case GX_TEV_COMP_BGR24_GT:
	case GX_TEV_COMP_BGR24_EQ:
	{
		//out = (d + (dot(a.bgr, rgTo16Bit) > dot(b.bgr, rgTo16Bit) ? c : 0));
		std::string bgrTo16Bit = "vec3(255.0/16777215.0, 255.0*256.0/16777215.0, 255.0*65536.0/16777215.0)";
		str << dest << " = " << "(" << input[3] << " + (" 
			<< " dot(" << input[0]+".bgr" << ", " << bgrTo16Bit << ")"
			<< (op == GX_TEV_COMP_BGR24_GT ? " > " : " == ") 
			<< " dot(" << input[1]+".bgr" << ", " << bgrTo16Bit << ")"
			<< " ? " << input[2] << " : 0));\n";
		break;
	}

	case GX_TEV_COMP_RGB8_GT:	
	case GX_TEV_COMP_RGB8_EQ:
		//out.r = d.r + ((a.r > b.r) ? c.r : 0);
		//out.g = d.g + ((a.g > b.g) ? c.g : 0);
		//out.b = d.b + ((a.b > b.b) ? c.b : 0);
		str << dest+".r" << " = " << input[3]+".r" << " + ((" << input[0]+".r" << (op == GX_TEV_COMP_RGB8_GT ? " > " : " == ") << input[1]+".r" << ") ? " << input[2]+".r" << " : 0);\n";
		str << dest+".g" << " = " << input[3]+".g" << " + ((" << input[0]+".g" << (op == GX_TEV_COMP_RGB8_GT ? " > " : " == ") << input[1]+".g" << ") ? " << input[2]+".g" << " : 0);\n";
		str << dest+".b" << " = " << input[3]+".b" << " + ((" << input[0]+".b" << (op == GX_TEV_COMP_RGB8_GT ? " > " : " == ") << input[1]+".b" << ") ? " << input[2]+".b" << " : 0);\n";
		break;

	default:
		WARN("GetColorOpString(): Unsupported op %d", op);
		str << "ERROR! Unsupported TEV operation. Aborting\n";
	}
	
	if (op > GX_TEV_SUB)
	{
        //if(bias != 3 || scale != 1 || clamp != 1)
		if(bias != 3 || scale != 0 || clamp != true)
          WARN("GetOpString(): Unexpected bias %d, scale %d, clamp %d for Comparison Operation", bias, scale, clamp);
	}

	return str.str();
}

std::string GetAlphaOpString(int op, int bias, int scale, bool clamp, int outputRegIndex, std::string input[4])
{
	std::string channelSelect = ".a";
	std::ostringstream str;
	std::string dest = GetRegisterString(outputRegIndex) + channelSelect;

	switch (op)
	{
	case GX_TEV_ADD: 
		//out = (d + lerp(a, b, c));
		str << dest << " = " << "(" << input[3] << " + lerp(" 
			<< input[0] + ", " << input[1] + ", "  << input[2] << "));\n";
		str << GetModString(outputRegIndex, bias, scale, clamp, true) + "\n";
		break;

	case GX_TEV_SUB:		
		//out = (d - lerp(a, b, c));
		//out = saturate(out * scale + result * scale);
		str << dest << " = " << "(" << input[3] << " - lerp(" << input[0] + ", "  << input[1] + ", "  << input[2] << "));\n";
		str << GetModString(outputRegIndex, bias, scale, clamp, true) + "\n";
		break;

	case GX_TEV_COMP_A8_GT:	
	case GX_TEV_COMP_A8_EQ:
		//out = (d + ((a.a > b.a) ? c : 0))
		str << dest << " = " << "(" << input[3] << " + ((" 
			<< input[0]+".a" 
			<< (op == GX_TEV_COMP_R8_GT ? " > " : " == ") 
			<< input[1]+".a" 
			<< ") ? " << input[2] << " : 0));\n";
		break;

	default:
		WARN("GetColorOpString(): Unsupported op %d", op);
		str << "ERROR! Unsupported TEV operation. Aborting\n";
	}
	
	if (op == GX_TEV_COMP_A8_GT || GX_TEV_COMP_A8_EQ)
	{
        //if(bias != 3 || scale != 1 || clamp != 1)
		if(bias != 3 || scale != 0 || clamp != true)
          WARN("GetOpString(): Unexpected bias %d, scale %d, clamp %d for Comparison Operation", bias, scale, clamp);
	}

	return str.str();
}

std::string GC3D::GeneratePS(Mat3* matInfo, int index)
{
	Material& mat = matInfo->materials[index];

	std::ostringstream out;
	out.setf(std::ios::fixed, std::ios::floatfield);
	out.setf(std::ios::showpoint);

	// Scale and Bias lookup tables
	out << "const float bias[3] = {0, 0.5f, -0.5f};" << "\n";
	out << "const float scale[4] = {1, 2, 4, 0.5f};" << "\n";
	out << "\n";

	out << "float4 main(PsIn In) : SV_Target" << "\n";
	out << "{" << "\n";

	// Output registers. There are only 4.
	for (uint i = 0; i < 4; i++)
	{
		// TODO: Verify that this is correct
		const Color16& c = matInfo->colorS10[mat.colorS10[i==0?3:i-1]];
		out << "float4 " << GetRegisterString(i);
		out << " = float4("
        << c.r/255.f << ", " << c.g/255.f << ", "
        << c.b/255.f << ", " << c.a/255.f << ");\n";
	}
	out << "\n";

	// Constant color (Konst) registers. There are only 4.
	for (uint i = 0; i < 4; i++)
	{
		const MColor& c = matInfo->color3[mat.color3[i]];
		out << "float4 konst" << i << " = float4(" 
			<< c.r/255.0f << ", " << c.g/255.0f << ", " 
			<< c.b/255.0f << ", " << c.a/255.0f << ");\n";
	}
	out << "\n";

	for(uint i = 0; i < matInfo->tevCounts[mat.tevCountIndex]; ++i)
	{
		const TevOrderInfo& order = matInfo->tevOrderInfos[mat.tevOrderInfo[i]];
		const TevStageInfo& stage = matInfo->tevStageInfos[mat.tevStageInfo[i]];

		std::string colorInputs[4];
		colorInputs[0] = GetColorInString(stage.colorIn[0], mat.constColorSel[i], &order);
		colorInputs[1] = GetColorInString(stage.colorIn[1], mat.constColorSel[i], &order);
		colorInputs[2] = GetColorInString(stage.colorIn[2], mat.constColorSel[i], &order);
		colorInputs[3] = GetColorInString(stage.colorIn[3], mat.constColorSel[i], &order);

		out << GetColorOpString(stage.colorOp, stage.colorBias, stage.colorScale, stage.colorClamp, stage.colorRegId, colorInputs);

		std::string alphaInputs[4];
		alphaInputs[0] = GetAlphaInString(stage.alphaIn[0], mat.constAlphaSel[i], &order);
		alphaInputs[1] = GetAlphaInString(stage.alphaIn[1], mat.constAlphaSel[i], &order);
		alphaInputs[2] = GetAlphaInString(stage.alphaIn[2], mat.constAlphaSel[i], &order);
		alphaInputs[3] = GetAlphaInString(stage.alphaIn[3], mat.constAlphaSel[i], &order);

		out << GetAlphaOpString(stage.alphaOp, stage.alphaBias, stage.alphaScale, stage.alphaClamp, stage.alphaRegId, alphaInputs);
	}

	//TODO: Alpha testing

	out << "return out;\n" << "}";

	return out.str();
}