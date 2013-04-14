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

struct ColorChanInfo
{
  //not sure if this is right
  u8 ambColorSource;
  u8 matColorSource;
  u8 litMask;
  u8 attenuationFracFunc;
  u8 diffuseAttenuationFunc;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["ambColorSource"] = ambColorSource;
	  val["matColorSource"] = matColorSource;
	  val["litMask"] = litMask;
	  val["attenuationFracFunc"] = attenuationFracFunc;
	  val["diffuseAttenuationFunc"] = diffuseAttenuationFunc;
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

struct TexMtxInfo
{
  float scaleCenterX, scaleCenterY;
  float scaleU, scaleV;
  
  Json::Value serialize()
  {
	  Json::Value val;
	  val["scaleCenterX"] = scaleCenterX;
	  val["scaleCenterY"] = scaleCenterY;
	  val["scaleU"] = scaleU;
	  val["scaleV"] = scaleV;
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
  u8 cullIndex;
  u8 numChansIndex;
  u8 texGenCountIndex;
  u8 tevCountIndex;

  u8 zModeIndex;

  u16 color1[2];
  u16 chanControls[4];
  u16 color2[2];

  u16 texGenInfos[8];

  u16 texMtxInfos[8];

  u16 texStages[8];
  //constColor (GX_TEV_KCSEL_K0-3)
  u16 color3[4];
  u8 constColorSel[16]; //0x0c most of the time (const color sel, GX_TEV_KCSEL_*)
  u8 constAlphaSel[16]; //0x1c most of the time (const alpha sel, GX_TEV_KASEL_*)
  u16 tevOrderInfo[16];
  //this is to be loaded into
  //GX_CC_CPREV - GX_CC_A2??
  u16 colorS10[4];
  u16 tevStageInfo[16];
  u16 tevSwapModeInfo[16];
  u16 tevSwapModeTable[4];

  u16 alphaCompIndex;
  u16 blendIndex;

  Json::Value serialize()
  {
	  Json::Value mat;

	  SERIALIZE(mat, flag);
	  SERIALIZE(mat, cullIndex);
	  SERIALIZE(mat, numChansIndex);
	  SERIALIZE(mat, texGenCountIndex);
	  SERIALIZE(mat, tevCountIndex);

	  SERIALIZE(mat, zModeIndex);

	  SERIALIZE_PARRAY(mat, color1, 2);
	  SERIALIZE_PARRAY(mat, chanControls, 4);
	  SERIALIZE_PARRAY(mat, color2, 2);

	  SERIALIZE_PARRAY(mat, texGenInfos, 8);

	  SERIALIZE_PARRAY(mat, texMtxInfos, 8);

	  SERIALIZE_PARRAY(mat, texStages, 8);
		//constColor (GX_TEV_KCSEL_K0-3)
	  SERIALIZE_PARRAY(mat, color3, 4);
	  SERIALIZE_PARRAY(mat, constColorSel, 16); //0x0c most of the time (const color sel, GX_TEV_KCSEL_*)
	  SERIALIZE_PARRAY(mat, constAlphaSel, 16); //0x1c most of the time (const alpha sel, GX_TEV_KASEL_*)
	  SERIALIZE_PARRAY(mat, tevOrderInfo, 16);
		//this is to be loaded into
		//GX_CC_CPREV - GX_CC_A2??
	  SERIALIZE_PARRAY(mat, colorS10, 4);
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
  std::vector<MColor> color1;
  std::vector<u8> numChans;
  std::vector<ColorChanInfo> colorChanInfos;
  std::vector<MColor> color2;

  std::vector<Material> materials;
  std::vector<int> indexToMatIndex;
  std::vector<std::string> stringtable;

  std::vector<u32> cullModes;

  std::vector<u8> texGenCounts;
  std::vector<TexGenInfo> texGenInfos;
  std::vector<TexMtxInfo> texMtxInfos;

  std::vector<int> texStageIndexToTextureIndex;
  std::vector<TevOrderInfo> tevOrderInfos;
  std::vector<Color16> colorS10;
  std::vector<MColor> color3;
  std::vector<u8> tevCounts;
  std::vector<TevStageInfo> tevStageInfos;
  std::vector<TevSwapModeInfo> tevSwapModeInfos;
  std::vector<TevSwapModeTable> tevSwapModeTables;
  std::vector<AlphaCompare> alphaCompares;
  std::vector<BlendInfo> blendInfos;
  std::vector<ZMode> zModes;

  Json::Value& serialize(Json::Value& mat)
  {
	  SERIALIZE_VECTOR(mat, color1);
	  SERIALIZE_PVECTOR(mat, numChans);
	  SERIALIZE_VECTOR(mat, colorChanInfos);
	  SERIALIZE_VECTOR(mat, color2);

	  SERIALIZE_VECTOR(mat, materials);
	  SERIALIZE_PVECTOR(mat, indexToMatIndex);
	  SERIALIZE_PVECTOR(mat, stringtable);

	  SERIALIZE_PVECTOR(mat, cullModes);

	  SERIALIZE_PVECTOR(mat, texGenCounts);
	  SERIALIZE_VECTOR(mat, texGenInfos);
	  SERIALIZE_VECTOR(mat, texMtxInfos);
	  
	  SERIALIZE_PVECTOR(mat, texStageIndexToTextureIndex);
	  SERIALIZE_VECTOR(mat, tevOrderInfos);
	  SERIALIZE_VECTOR(mat, colorS10);
	  SERIALIZE_VECTOR(mat, color3);
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
