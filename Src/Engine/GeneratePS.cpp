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

const std::string varResultName = "result";
const std::string varRegisterName[3] = {"r0", "r1", "r2"};

bool IsNewTexCombo(uint texMap, uint texCoordId, bool oldCombos[] )
{
	uint8 index = (texMap << 4 | texCoordId);
	if (oldCombos[index])
		return false;
	else
	{
		oldCombos[index] = true;
		return true;
	}
}

std::string GetSwapModeSwizzleString(const TevSwapModeTable& table)
{
	const char* components[4] = { "r", "g", "b", "a" };
	return std::string(".") + components[table.r] + components[table.g] + components[table.b] + components[table.a];
}

std::string getCompareString(int comp, std::string a, u8 ref)
{
	std::ostringstream out;
	float fRef = float(ref / 255.0f);

	if (comp != GX_ALWAYS)
		WARN("Alpha test functionality is untested\n");

	switch(comp)
	{
	case GX_NEVER:	return "false";	
	case GX_LESS:	out << a << " < " << fRef;  break;
	case GX_EQUAL:	out << a << " == " << fRef; break;
	case GX_LEQUAL:	out << a << " <= " << fRef; break;
	case GX_GREATER:out << a << " > " << fRef;  break;
	case GX_NEQUAL:	out << a << " != " << fRef; break;
	case GX_GEQUAL: out << a << " >= " << fRef; break;
	case GX_ALWAYS: return "true";
	default:
		WARN("Invalid comparison function %u. Defaulting to 'ALWAYS'\n", comp);
		return "true";
	}

	return out.str();
}

std::string GetRegisterString(uint regIndex)
{
	if (regIndex == 0) return varResultName;
	else return varRegisterName[regIndex-1];
}

std::string GetTexTapString(const TevOrderInfo* texMapping, uint formats[8])
{
	// "tex00"
	std::ostringstream out;
	out << "tex" << (int)texMapping->texMap << (int)texMapping->texCoordId;
	return out.str();
}

std::string GetVertColorString(const TevOrderInfo* texMapping)
{
	// "In.Color0"
	switch (texMapping->chanId)
	{
		case GX_COLOR0: return "In.Color0.rgb";
		case GX_COLOR1: return "In.Color1.rgb";
		case GX_ALPHA0: return "In.Color0.aaaa";
		case GX_ALPHA1: return "In.Color1.aaaa";
		case GX_COLOR0A0: return "In.Color0.rgba";
		case GX_COLOR1A1: return "In.Color1.rgba";
		case GX_COLORZERO: return "0.0f.rrrr";
		case GX_BUMP:
		case GX_BUMPN:
		case GX_COLORNULL:  
		default:
			{
			  WARN("GetVertColorString(): Unknown chanId 0x%x\n", texMapping->chanId);
			  return "float4(0.0f, 1.0f, 0.0f, 1.0f)";
			}
	}
}

std::string GetKonstColorString(uint konst)
{
	switch (konst)
	{
		case GX_TEV_KCSEL_1:	return "1.0f.rrrr";
		case GX_TEV_KCSEL_7_8:	return "0.875f.rrrr";
		case GX_TEV_KCSEL_3_4:	return "0.75f.rrrr";
		case GX_TEV_KCSEL_5_8:	return "0.625f.rrrr";
		case GX_TEV_KCSEL_1_2:	return "0.5f.rrrr";	
		case GX_TEV_KCSEL_3_8:	return "0.375f.rrrr";
		case GX_TEV_KCSEL_1_4:	return "0.25f.rrrr";
		case GX_TEV_KCSEL_1_8:	return "0.125f.rrrr";
		case GX_TEV_KCSEL_K0:	return "konst0.rgba";
		case GX_TEV_KCSEL_K1:	return "konst1.rgba";
		case GX_TEV_KCSEL_K2:	return "konst2.rgba";
		case GX_TEV_KCSEL_K3:	return "konst3.rgba";
		case GX_TEV_KCSEL_K0_R: return "konst0.rrrr";
		case GX_TEV_KCSEL_K1_R:	return "konst1.rrrr";
		case GX_TEV_KCSEL_K2_R:	return "konst2.rrrr";
		case GX_TEV_KCSEL_K3_R:	return "konst3.rrrr";
		case GX_TEV_KCSEL_K0_G:	return "konst0.gggg";
		case GX_TEV_KCSEL_K1_G:	return "konst1.gggg";
		case GX_TEV_KCSEL_K2_G:	return "konst2.gggg";
		case GX_TEV_KCSEL_K3_G:	return "konst3.gggg";
		case GX_TEV_KCSEL_K0_B:	return "konst0.bbbb";
		case GX_TEV_KCSEL_K1_B:	return "konst1.bbbb";
		case GX_TEV_KCSEL_K2_B:	return "konst2.bbbb";
		case GX_TEV_KCSEL_K3_B:	return "konst3.bbbb";
		case GX_TEV_KCSEL_K0_A:	return "konst0.aaaa";
		case GX_TEV_KCSEL_K1_A:	return "konst1.aaaa";
		case GX_TEV_KCSEL_K2_A:	return "konst2.aaaa";
		case GX_TEV_KCSEL_K3_A:	return "konst3.aaaa";
		default:
			WARN("GetKonstColorString(): Unknown konstIndex 0x%x\n", konst);
			return "float4(0.0f, 1.0f, 0.0f, 1.0f)";
	}
}

std::string GetKonstAlphaString(uint konst)
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
	case GX_TEV_KASEL_K3_A:	return "konst3.a";
	default:
		WARN("GetKonstAlphaString(): Unknown konstIndex 0x%x\n", konst);
		return "float(1.0f)";
	}						
}							
							
std::string GetColorInString(uint inputType, uint konst, const TevOrderInfo* texMapping, uint formats[8])
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
		case GX_CC_TEXC:	return GetTexTapString(texMapping, formats) + ".rgb";
		case GX_CC_TEXA:	return GetTexTapString(texMapping, formats) + ".aaa";
		case GX_CC_RASC:	return GetVertColorString(texMapping) + ".rgb";
		case GX_CC_RASA:	return GetVertColorString(texMapping) + ".aaa";
		case GX_CC_ONE:		return "1.0f.rrr";
		case GX_CC_HALF:	return "0.5f.rrr";
		case GX_CC_KONST:	return GetKonstColorString(konst) + ".rgb";
		case GX_CC_ZERO:	return "0.0f.rrr";
		default:
			WARN("GetColorInString(): Unknown inputType %d\n", (u32)inputType);
			return "0.0f.rrr";
	}
}

std::string GetAlphaInString(uint inputType, uint konst, const TevOrderInfo* texMapping, uint formats[8])
{
	switch (inputType)
	{
	case GX_CA_APREV:	return GetRegisterString(0) + ".a";
	case GX_CA_A0:		return GetRegisterString(1) + ".a";
	case GX_CA_A1:		return GetRegisterString(2) + ".a";
	case GX_CA_A2:		return GetRegisterString(3) + ".a";
	case GX_CA_TEXA:	return GetTexTapString(texMapping, formats) + ".a";
	case GX_CA_RASA:	return GetVertColorString(texMapping) + ".a";
	case GX_CA_KONST:	return GetKonstAlphaString(konst);
	case GX_CA_ZERO:	return "0.0f";
	default:
		WARN("GetAlphaInString(): Unknown inputType %d\n", (u32)inputType);
			return "0.0f";
	}
}

std::string GetModString(uint outputRegIndex, uint bias, uint scale, uint clamp, bool isAlpha)
{     
	float biasVal = 0.0f;
	float scaleVal = 1.0f;

	switch (bias)
	{
	case GX_TB_ZERO:	 biasVal = 0.0f;  break;
	case GX_TB_ADDHALF:	 biasVal = 0.5f;  break;
	case GX_TB_SUBHALF:	 biasVal = -0.5f; break;
	case GX_MAX_TEVBIAS: 
	default: 
		WARN("GetModString(): Unrecognized bias value. Defaulting to 0.0f\n");
		biasVal = 0.0f;
	}

	switch (scale)
	{
	case GX_CS_SCALE_1:	 scaleVal = 1.0f; break;
	case GX_CS_SCALE_2:	 scaleVal = 2.0f; break;
	case GX_CS_SCALE_4:	 scaleVal = 4.0f; break;
	case GX_CS_DIVIDE_2: scaleVal = 0.5f; break;
	case GX_MAX_TEVSCALE:
	default: 
		WARN("GetModString(): Unrecognized scale value. Defaulting to 1.0f\n");
		scaleVal = 1.0f;
	}

	if (scaleVal == 1.0f && biasVal == 0.0f && clamp == 0)
		return "";
	
	std::string channelSelect = (isAlpha ? ".a" : ".rgb");
	std::ostringstream out;
	std::string dest = GetRegisterString(outputRegIndex) + channelSelect;
	
	// result = saturate(result);
	if (scaleVal == 1.0f && biasVal == 0.0f)
		out << dest << " = saturate(" << dest << ");";
	else
	{
		// result = saturate(result * scale  + bias * scale);
		out << dest << " = " << (clamp ? "saturate(" : "(") << dest << " * " 
			<< scaleVal << " + " << biasVal * scaleVal << ");";
	}

	return out.str() + "\n";
}

// TODO: Detect and remove NOPs
std::string GetColorOpString(uint op, uint bias, uint scale, uint clamp, uint outputRegIndex, std::string input[4])
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
		str << GetModString(outputRegIndex, bias, scale, clamp, false);
		break;

	case GX_TEV_SUB:		
		//out = (d - lerp(a, b, c));
		//out = saturate(out * scale + result * scale);
		str << dest << " = " << "(" << input[3] << " - lerp(" << input[0] + ", " << input[1] + ", " << input[2] << "));\n";
		str << GetModString(outputRegIndex, bias, scale, clamp, false);
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
		WARN("GetColorOpString(): Unsupported op %d\n", op);
		str << "ERROR! Unsupported TEV operation. Aborting\n";
	}
	
	if (op > GX_TEV_SUB)
	{
        //if(bias != 3 || scale != 1 || clamp != 1)
		if(bias != 3 || scale != 0 || clamp != 1)
          WARN("GetOpString(): Unexpected bias %d, scale %d, clamp %d for Comparison Operation\n", bias, scale, clamp);
	}

	return str.str() + "\n";
}

// TODO: Detect and remove NOPs
std::string GetAlphaOpString(uint op, uint bias, uint scale, uint clamp, uint outputRegIndex, std::string input[4])
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
		str << GetModString(outputRegIndex, bias, scale, clamp, true);
		break;

	case GX_TEV_SUB:		
		//out = (d - lerp(a, b, c));
		//out = saturate(out * scale + result * scale);
		str << dest << " = " << "(" << input[3] << " - lerp(" << input[0] + ", "  << input[1] + ", "  << input[2] << "));\n";
		str << GetModString(outputRegIndex, bias, scale, clamp, true);
		break;

	case GX_TEV_COMP_A8_GT:	
	case GX_TEV_COMP_A8_EQ:
		//out = (d + ((a.a > b.a) ? c : 0))
		str << dest << " = " << "(" << input[3] << " + ((" 
			<< input[0] // This is guaranteed to be a single float, so we don't need a ".a"
			<< (op == GX_TEV_COMP_A8_GT ? " > " : " == ") 
			<< input[1] // This is guaranteed to be a single float, so we don't need a ".a"
			<< ") ? " << input[2] << " : 0));\n";
		break;

	default:
		WARN("GetColorOpString(): Unsupported op %d\n", op);
		str << "ERROR! Unsupported TEV operation. Aborting\n";
	}
	
	if (op == GX_TEV_COMP_A8_GT || op == GX_TEV_COMP_A8_EQ)
	{
        //if(bias != 3 || scale != 1 || clamp != 1)
		if(bias != 3 || scale != 0 || clamp != 1)
          WARN("GetOpString(): Unexpected bias %d, scale %d, clamp %d for Comparison Operation\n", bias, scale, clamp);
	}

	return str.str() + "\n";
}

std::string GeneratePS(const Tex1* texInfo, const Mat3* matInfo, int index)
{
	const Material& mat = matInfo->materials[index];

	std::ostringstream out;
	out.setf(std::ios::fixed, std::ios::floatfield);
	out.setf(std::ios::showpoint);

	// Helper macros
	out << "#define SAMPLE(texIdx, uvIdx) Texture##texIdx.Sample( Sampler##texIdx, In.TexCoord##uvIdx )\n";
	out << "\n";

	// Input structure
	out << "struct PsIn" << "\n";
	out << "{" << "\n";
	out << "float4 Position : SV_Position;" << "\n";
	
	uint nChans = matInfo->numChans[mat.numChansIndex];
	uint nTexGens = matInfo->texGenCounts[mat.texGenCountIndex];
	uint nTevStages = matInfo->tevCounts[mat.tevCountIndex];

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

	uint texFormats[8];

	// Textures and Samplers
	for(uint i = 0; i < 8; i++)
	{
		if(mat.texStages[i] == 0xffff)
			continue;

		uint texHdrIndex = matInfo->texStageIndexToTextureIndex[mat.texStages[i]];
		uint texImgIndex = texInfo->imageHeaders[texHdrIndex].imageIndex;
		texFormats[i] = texInfo->images[texImgIndex].format;

		//Texture2D Texture0;
		//SamplerState Sampler0;
		out << "SamplerState Sampler" << i << ";\n";
		out << "Texture2D Texture" << i << "; //"
			<< mat.texStages[i] << " -> "
			<< texHdrIndex << ", " << texInfo->imageHeaders[texHdrIndex].name << "\n";
		out << "\n";
	}

	out << "float4 main(PsIn In) : SV_Target" << "\n";
	out << "{" << "\n";

	// Define initial values of the TEV registers. There are only 4.
	for (uint i = 0; i < 4; i++)
	{
		// TODO: Verify that this is correct
		const Color16& c = matInfo->registerColor[mat.registerColor[i==0?3:i-1]];
		out << "float4 " << GetRegisterString(i);
		out << " = float4("
        << c.r/255.f << ", " << c.g/255.f << ", "
        << c.b/255.f << ", " << c.a/255.f << ");\n";
	}
	out << "\n";

	// Constant color (Konst) registers. There are only 4.
	for (uint i = 0; i < 4; i++)
	{
		const MColor& c = matInfo->konstColor[mat.konstColor[i]];
		out << "float4 konst" << i << " = float4(" 
			<< c.r/255.0f << ", " << c.g/255.0f << ", " 
			<< c.b/255.0f << ", " << c.a/255.0f << ");\n";
	}
	out << "\n";

	// Make our texture samples
	bool oldCombos[256] = {};
	for (uint i = 0, j = 0; i < nTevStages; i++)
	{
		const TevOrderInfo& order = matInfo->tevOrderInfos[mat.tevOrderInfo[i]];
		int tex = (int)order.texMap;
		int coord = (int)order.texCoordId;

		if (tex == 0xff || coord == 0xff)
			continue;	// This TEV probably doesn't use the textures

		if (IsNewTexCombo(tex, coord, oldCombos))
		{
			std::string swizzle;
			switch (texFormats[tex])
			{
			case I8: swizzle = ".rrrr"; break;
			case I8_A8: swizzle = ".rrrg"; break;
			case RGBA8: swizzle = ""; break;
			case DXT1: swizzle = ""; break;
			default:
				WARN("Unknown texture format %u. Defaulting to tap with no swizzling\n", 
					texFormats[tex]);
				swizzle = "";
			}

			out << "float4 tex" << tex << coord << " = SAMPLE(" 
				<< tex << ", " << coord << ")" << swizzle << ";\n";
		}
	}
	out << "\n";

	// TODO: Implement indirect texturing

	for(uint i = 0; i < nTevStages; ++i)
	{
		const TevOrderInfo& order = matInfo->tevOrderInfos[mat.tevOrderInfo[i]];
		const TevStageInfo& stage = matInfo->tevStageInfos[mat.tevStageInfo[i]];
		
		const TevSwapModeInfo& swap = matInfo->tevSwapModeInfos[mat.tevSwapModeInfo[i]];
		const TevSwapModeTable& rasTable = matInfo->tevSwapModeTables[mat.tevSwapModeTable[swap.rasSel]];
		const TevSwapModeTable& texTable = matInfo->tevSwapModeTables[mat.tevSwapModeTable[swap.texSel]];
		uint8 noSwap[4] = {0,1,2,3};

		if (memcmp(&rasTable, noSwap, 4) != 0)
			out << GetVertColorString(&order) << " = " 
				<< GetVertColorString(&order) << GetSwapModeSwizzleString(rasTable) << ";\n";
		if (memcmp(&texTable, noSwap, 4) != 0)
			out << GetTexTapString(&order, texFormats) << " = " 
				<< GetTexTapString(&order, texFormats) << GetSwapModeSwizzleString(texTable) << ";\n";
	
		std::string colorInputs[4];
		colorInputs[0] = GetColorInString(stage.colorIn[0], mat.constColorSel[i], &order, texFormats);
		colorInputs[1] = GetColorInString(stage.colorIn[1], mat.constColorSel[i], &order, texFormats);
		colorInputs[2] = GetColorInString(stage.colorIn[2], mat.constColorSel[i], &order, texFormats);
		colorInputs[3] = GetColorInString(stage.colorIn[3], mat.constColorSel[i], &order, texFormats);

		out << GetColorOpString(uint(stage.colorOp), uint(stage.colorBias), uint(stage.colorScale), uint(stage.colorClamp), uint(stage.colorRegId), colorInputs);
		
		std::string alphaInputs[4];
		alphaInputs[0] = GetAlphaInString(stage.alphaIn[0], mat.constAlphaSel[i], &order, texFormats);
		alphaInputs[1] = GetAlphaInString(stage.alphaIn[1], mat.constAlphaSel[i], &order, texFormats);
		alphaInputs[2] = GetAlphaInString(stage.alphaIn[2], mat.constAlphaSel[i], &order, texFormats);
		alphaInputs[3] = GetAlphaInString(stage.alphaIn[3], mat.constAlphaSel[i], &order, texFormats);

		out << GetAlphaOpString(uint(stage.alphaOp), uint(stage.alphaBias), uint(stage.alphaScale), uint(stage.alphaClamp), uint(stage.alphaRegId), alphaInputs);
	}

	const AlphaCompare& cmpInfo = matInfo->alphaCompares[mat.alphaCompIndex];
	std::string op;

	switch (cmpInfo.alphaOp)
	{
		case GX_AOP_AND:	op = " && "; break;
		case GX_AOP_OR:		op = " || "; break;
		case GX_AOP_XOR:	
		case GX_AOP_XNOR:
		default:
			WARN("Unsupported alpha operation %u. Defaulting to 'OR'\n", cmpInfo.alphaOp);
			op = " || ";
	}

	// clip( result.a < 0.5f && result a > 0.2 ? -1 : 1) 
	out << "clip( (" << getCompareString(cmpInfo.comp0, "result.a", cmpInfo.ref0) 
		<< op << getCompareString(cmpInfo.comp1, "result.a", cmpInfo.ref1)
		<< ") ? 1 : -1 );\n";


	out << "return " << GetRegisterString(0) <<";\n" << "}";

	return out.str();
}