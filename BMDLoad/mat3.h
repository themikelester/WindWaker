#ifndef BMD_MAT3_H
#define BMD_MAT3_H BMD_MAT3_H

#include "common.h"

#include <iosfwd>

struct MColor
{
  u8 r, g, b, a;
};

struct Color16
{
  s16 r, g, b, a;
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
};

struct TexGenInfo
{
  u8 texGenType;
  u8 texGenSrc;
  u8 matrix;
};

struct TexMtxInfo
{
  float scaleCenterX, scaleCenterY;
  float scaleU, scaleV;
};

struct TevOrderInfo
{
  u8 texCoordId;
  u8 texMap;
  u8 chanId;
};

struct TevSwapModeInfo
{
  u8 rasSel;
  u8 texSel;
};

struct TevSwapModeTable
{
  u8 r;
  u8 g;
  u8 b;
  u8 a;
};

struct AlphaCompare
{
  u8 comp0, ref0;
  u8 alphaOp;
  u8 comp1, ref1;
};

struct BlendInfo
{
  u8 blendMode;
  u8 srcFactor, dstFactor;
  u8 logicOp;
};

struct ZMode
{
  bool enable;
  u8 zFunc;
  bool enableUpdate;
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
  u16 colorS10[4];

  u16 color3[4]; // The Konst Color Registers. This is an index into the actual colors stored in Mat3::color3
  u8 constColorSel[16]; //Inputs to GX_SetTevKColorSel. For each TEV Stage, binds a constant or a Konst Color Register value to the KONST register.
  u8 constAlphaSel[16]; //Inputs to GX_SetTevKColorSel. For each TEV Stage, binds a constant or a Konst Color Register value to the KONST register.
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
  std::vector<Color16> colorS10;
  std::vector<MColor> color3;
  std::vector<u8> tevCounts;
  std::vector<TevStageInfo> tevStageInfos;
  std::vector<TevSwapModeInfo> tevSwapModeInfos;
  std::vector<TevSwapModeTable> tevSwapModeTables;
  std::vector<AlphaCompare> alphaCompares;
  std::vector<BlendInfo> blendInfos;
  std::vector<ZMode> zModes;
};

void dumpMat3(Chunk* f, Mat3& dst);
void writeMat3Info(Chunk* f, std::ostream& out);;

#endif //BMD_MAT3_H
