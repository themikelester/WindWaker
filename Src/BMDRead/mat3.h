#ifndef BMD_MAT3_H
#define BMD_MAT3_H BMD_MAT3_H

#include "common.h"

#include <iosfwd>
  
// Serialize std::vectors of types that have serialize()
#define SERIALIZE_VECTOR(jsonValue, vector) \
	for (uint i = 0; i < vector.size(); i++) \
		{ jsonValue[#vector][i] = vector[i].serialize(); }
  
// Serialize std::vectors of primitive types (no serialize())
#define SERIALIZE_PVECTOR(jsonValue, vector) \
	for (uint i = 0; i < vector.size(); i++) \
		{ jsonValue[#vector][i] = vector[i]; }

// Serialize an array (of size num) of types that have serialize()
#define SERIALIZE_ARRAY(jsonValue, arr, num) \
	for (uint i = 0; i < num; i++) \
		{ jsonValue[#arr][i] = arr[i].serialize(); }

// Serialize an array (of size num) of primitive (no serialize())
#define SERIALIZE_PARRAY(jsonValue, arr, num) \
	for (uint i = 0; i < num; i++) \
		{ jsonValue[#arr][i] = arr[i]; }

// Serialize a variable
#define SERIALIZE(jsonValue, var) \
	jsonValue[#var] = var;

struct MColor
{
  u8 r, g, b, a;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["r"] = r;
	  val["g"] = g;
	  val["b"] = b;
	  val["a"] = a;
	  return val;
  }
};

struct Color16
{
  s16 r, g, b, a;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["r"] = r;
	  val["g"] = g;
	  val["b"] = b;
	  val["a"] = a;
	  return val;
  }
};

// MikeLest: See this site for details http://www.gamasutra.com/view/feature/2945/shader_integration_merging_.php?print=1
// MikeLest: This has examples of GXSetChanCtrl: https://wire3d.googlecode.com/svn-history/r416/trunk/Wire/Renderer/GXRenderer/WireGXLight.cpp
// MikeLest: Full gx.h documentation! http://libogc.devkitpro.org/gx_8h.html
// These are the parameters to GXSetChanCtrl
struct ColorChanInfo
{
  u8 enable;
  u8 matColorSource;
  u8 litMask;

  //GX_DF_NONE, GX_DF_SIGNED, GX_DF_CLAMP	
  u8 diffuseAttenuationFunc;

  //GX_AF_SPEC, GX_AF_SPOT, GX_AF_NONE	
  u8 attenuationFracFunc;
  u8 ambColorSource;
  u8 pad[2];

  Json::Value serialize()
  {
	Json::Value val;
	val["enable"] = enable;
	val["matColorSource"] = matColorSource; 
	val["litMask"] = litMask;
	val["diffuseAttenuationFunc"] = diffuseAttenuationFunc;
	val["attenuationFracFunc"] = attenuationFracFunc;
	val["ambColorSource"] = ambColorSource;
	return val;
  }
};

struct TexGenInfo
{
  u8 texGenType;
  u8 texGenSrc;
  u8 matrix;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["texGenType"] = texGenType;
	  val["texGenSrc"] = texGenSrc;
	  val["matrix"] = matrix;
	  return val;
  }
};

// This is unique to .bdl/.bmd, not GX
enum bdmTexMtxType
{
	TEXMTX_TEXCOORD = 0x00,
	TEXMTX_NORM		= 0x06,
	TEXMTX_TNORM	= 0x07,
	TEXMTX_POS		= 0x08,
	TEXMTX_TPOS		= 0x09,
	TEXMTX_UNK		= 0x80
};

struct TexMtxInfo
{
  u8 projection; // One of GX_TG_MTX3x4 or GX_TG_MTX2x4.
  u8 type; // bmdTexMtxType

  f32 center_s,center_t;
  f32 unknown0;
  f32 scale_s,scale_t;

  u16 rotate; // -32768 = -180 deg, 32768 = 180 deg
  f32 translate_s;
  f32 translate_t;

  f32 prematrix[4][4];
  
  Json::Value serialize()
  {
	Json::Value val;
	val["projection"] = projection;
	val["type"] = type;
	val["center_s"] = center_s;
	val["center_t"] = center_t;
	val["scale_s"] = scale_s;
	val["scale_t"] = scale_t;
	val["rotate"] = rotate;
	val["translate_s"] = translate_s;
	val["translate_t"] = translate_t;

	for (uint i = 0; i < 16; i++)
	{
		val["prematrix"][i/4][i%4] = prematrix[i/4][i%4];
	}
	return val;
  }
};

struct TevOrderInfo
{
  u8 texCoordId;
  u8 texMap;
  u8 chanId;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["texCoordId"] = texCoordId;
	  val["texMap"] = texMap;
	  val["chanId"] = chanId;
	  return val;
  }
};

struct TevSwapModeInfo
{
  u8 rasSel;
  u8 texSel;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["rasSel"] = rasSel;
	  val["texSel"] = texSel;
	  return val;
  }
};

struct TevSwapModeTable
{
  u8 r;
  u8 g;
  u8 b;
  u8 a;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["r"] = r;
	  val["g"] = g;
	  val["b"] = b;
	  val["a"] = a;
	  return val;
  }
};

struct AlphaCompare
{
  u8 comp0, ref0;
  u8 alphaOp;
  u8 comp1, ref1;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["comp0"] = comp0;
	  val["ref0"] = ref0;
	  val["alphaOp"] = alphaOp;
	  val["comp1"] = comp1;
	  val["ref1"] = ref1;
	  return val;
  }
};

struct BlendInfo
{
  u8 blendMode;
  u8 srcFactor, dstFactor;
  u8 logicOp;

  Json::Value serialize()
  {
	  Json::Value val;
	  val["blendMode"] = blendMode;
	  val["srcFactor"] = srcFactor;
	  val["dstFactor"] = dstFactor;
	  val["logicOp"] = logicOp;
	  return val;
  }
};

struct ZMode
{
  bool enable;
  u8 zFunc;
  bool enableUpdate;

  Json::Value serialize()
  {
	  Json::Value val;
	  val["enable"] = enable;
	  val["zFunc"] = zFunc;
	  val["enableUpdate"] = enableUpdate;
	  return val;
  }
};


struct TevStageInfo
{
  //GX_SetTevColorIn() arguments
  u8 colorIn[4]; //GX_CC_*

  //GX_SetTevColorOp() arguments
  u8 colorOp;
  u8 colorBias;
  u8 colorScale;
  u8 colorClamp;
  u8 colorRegId;

  //GX_SetTevAlphaIn() arguments
  u8 alphaIn[4]; //GC_CA_*

  //GX_SetTevAlphaOp() arguments
  u8 alphaOp;
  u8 alphaBias;
  u8 alphaScale;
  u8 alphaClamp;
  u8 alphaRegId;

  Json::Value serialize()
  {
	  Json::Value val;

	  SERIALIZE_PARRAY(val, colorIn, 4);

	  //GX_SetTevColorOp() arguments
	  SERIALIZE(val, colorOp);
	  SERIALIZE(val, colorBias);
	  SERIALIZE(val, colorScale);
	  SERIALIZE(val, colorClamp);
	  SERIALIZE(val, colorRegId);

	  //GX_SetTevAlphaIn() arguments
	  SERIALIZE_PARRAY(val, alphaIn, 4); //GC_CA_*

	  //GX_SetTevAlphaOp() arguments
	  SERIALIZE(val, alphaOp);
	  SERIALIZE(val, alphaBias);
	  SERIALIZE(val, alphaScale);
	  SERIALIZE(val, alphaClamp);
	  SERIALIZE(val, alphaRegId);

	  return val;
  }
};

struct Material
{
  u8 flag;

  // Lighting Pipeline 
  u16 chanControls[4]; // index into colorChanInfos. 
  //These indices are for GX_COLOR0, GX_ALPHA0, GX_COLOR1, GX_ALPHA1. 
  //According to http://kuribo64.net/?page=thread&id=532&from=20

  u8 numChansIndex;	   // index into numChans
  u16 ambColor[2];		
  u16 matColor[2];
  
  // State settings
  u16 alphaCompIndex;
  u16 blendIndex;
  u8 zModeIndex;
  u8 cullIndex;

  // Texture Units (8)
  u8 texGenCountIndex;
  u16 texGenInfos[8];
  u16 texMtxInfos[8];
  u16 texStages[8];

  // TEV Units (16)
  u8 tevCountIndex;
  u16 tevStageInfo[16];
  u16 tevOrderInfo[16];
  u16 tevSwapModeInfo[16]; // TODO: Add WARN messages that this is unsupported
  u16 tevSwapModeTable[4]; // TODO: Add WARN messages that this is unsupported
  //this is to be loaded into
  //GX_CC_CPREV - GX_CC_A2??
  u16 registerColor[4];

  u16 konstColor[4]; // The Konst Color Registers. This is an index into the actual colors stored in Mat3::konstColor
  u8 constColorSel[16]; //Inputs to GX_SetTevKColorSel. For each TEV Stage, binds a constant or a Konst Color Register value to the KONST register.
  u8 constAlphaSel[16]; //Inputs to GX_SetTevKColorSel. For each TEV Stage, binds a constant or a Konst Color Register value to the KONST register.
 
  Json::Value serialize()
  {
	  Json::Value mat;

	  SERIALIZE(mat, flag);
	  SERIALIZE(mat, cullIndex);
	  SERIALIZE(mat, numChansIndex);
	  SERIALIZE(mat, texGenCountIndex);
	  SERIALIZE(mat, tevCountIndex);

	  SERIALIZE(mat, zModeIndex);

	  SERIALIZE_PARRAY(mat, ambColor, 2);
	  SERIALIZE_PARRAY(mat, chanControls, 4);
	  SERIALIZE_PARRAY(mat, matColor, 2);

	  SERIALIZE_PARRAY(mat, texGenInfos, 8);

	  SERIALIZE_PARRAY(mat, texMtxInfos, 8);

	  SERIALIZE_PARRAY(mat, texStages, 8);
		//constColor (GX_TEV_KCSEL_K0-3)
	  SERIALIZE_PARRAY(mat, konstColor, 4);
	  SERIALIZE_PARRAY(mat, constColorSel, 16); //0x0c most of the time (const color sel, GX_TEV_KCSEL_*)
	  SERIALIZE_PARRAY(mat, constAlphaSel, 16); //0x1c most of the time (const alpha sel, GX_TEV_KASEL_*)
	  SERIALIZE_PARRAY(mat, tevOrderInfo, 16);
		//this is to be loaded into
		//GX_CC_CPREV - GX_CC_A2??
	  SERIALIZE_PARRAY(mat, registerColor, 4);
	  SERIALIZE_PARRAY(mat, tevStageInfo, 16);
	  SERIALIZE_PARRAY(mat, tevSwapModeInfo, 16);
	  SERIALIZE_PARRAY(mat, tevSwapModeTable, 4);

	  SERIALIZE(mat, alphaCompIndex);
	  SERIALIZE(mat, blendIndex);

	  return mat;
  }
};

struct Mat3
{
  // Controls the Lighting/Color calculations that feed in to the 2 TEV RAS color channels 
  std::vector<ColorChanInfo> colorChanInfos;
  std::vector<u8> numChans;		// Number of these channels to enable (0, 1, or 2)
  std::vector<MColor> matColor; // Each color channel has a mat color 
  std::vector<MColor> ambColor;

  std::vector<Material> materials;
  std::vector<int> indexToMatIndex;
  std::vector<std::string> stringtable;

  std::vector<u32> cullModes;

  std::vector<u8> texGenCounts;
  std::vector<TexGenInfo> texGenInfos;
  std::vector<TexMtxInfo> texMtxInfos;

  std::vector<int> texStageIndexToTextureIndex;
  std::vector<TevOrderInfo> tevOrderInfos;
  std::vector<Color16> registerColor;
  std::vector<MColor> konstColor;
  std::vector<u8> tevCounts;
  std::vector<TevStageInfo> tevStageInfos;
  std::vector<TevSwapModeInfo> tevSwapModeInfos;
  std::vector<TevSwapModeTable> tevSwapModeTables;
  std::vector<AlphaCompare> alphaCompares;
  std::vector<BlendInfo> blendInfos;
  std::vector<ZMode> zModes;

  Json::Value& serialize(Json::Value& mat)
  {
	  SERIALIZE_VECTOR(mat, ambColor);
	  SERIALIZE_PVECTOR(mat, numChans);
	  SERIALIZE_VECTOR(mat, colorChanInfos);
	  SERIALIZE_VECTOR(mat, matColor);

	  SERIALIZE_VECTOR(mat, materials);
	  SERIALIZE_PVECTOR(mat, indexToMatIndex);
	  SERIALIZE_PVECTOR(mat, stringtable);

	  SERIALIZE_PVECTOR(mat, cullModes);

	  SERIALIZE_PVECTOR(mat, texGenCounts);
	  SERIALIZE_VECTOR(mat, texGenInfos);
	  SERIALIZE_VECTOR(mat, texMtxInfos);
	  
	  SERIALIZE_PVECTOR(mat, texStageIndexToTextureIndex);
	  SERIALIZE_VECTOR(mat, tevOrderInfos);
	  SERIALIZE_VECTOR(mat, registerColor);
	  SERIALIZE_VECTOR(mat, konstColor);
	  SERIALIZE_PVECTOR(mat, tevCounts);
	  SERIALIZE_VECTOR(mat, tevStageInfos);
	  SERIALIZE_VECTOR(mat, tevSwapModeInfos);
	  SERIALIZE_VECTOR(mat, tevSwapModeTables);
	  SERIALIZE_VECTOR(mat, alphaCompares);
	  SERIALIZE_VECTOR(mat, blendInfos);
	  SERIALIZE_VECTOR(mat, zModes);

	  return mat;
  }
};

void dumpMat3(FILE* f, Mat3& dst);
void writeMat3Info(FILE* f, std::ostream& out);

#endif //BMD_MAT3_H
