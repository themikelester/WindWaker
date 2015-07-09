#include "mat3.h"

#include <string.h>
#include <memory.h>

#include <cassert>
#include <sstream> //for debug output
#include <iomanip> //dito
#include <fstream> //dito
#include <iostream>
using namespace std;

//this is still somewhat incomplete...

namespace bmd
{

struct Mat3Header
{
  char tag[4]; //'MAT3' or 'MAT2'
  u32 sizeOfSection;
  u16 count;
  u16 pad;

  /*
    Left-indented stuff not completely known

    The entries marked with * are missing in MAT2 blocks
    (ie. in a MAT2 block entry 3 is cull mode etc.), the
    Mat2Header is 12 bytes shorter because of this


    0 - MatEntry array (indexed by next array)
    1 - indexToMatEntryIndex (count many)
    2 - string table
*   3 - MatIndirectTexturingEntry array, count many for all files i've seen
    4 - cull mode                               (indexed by MatEntry.unk[1])
  5 - ambColor (rgba8) (amb color??)
  6 - numChans (?)                              (indexed by MatEntry.unk[2])
  7 - colorChanInfo
* 8 - matColor (rgba8) (mat color??)
* 9 - light
    10 - texgen counts (-> vr_back_cloud.bdl)   (indexed by MatEntry.unk[3])
    11 - TexGen
  12 - TexGen2
  13 - TexMtxInfo
  14 - TexMtxInfo2
    15 - index to texture table
    16 - TevOrderInfo array
    17 - registerColor (rgba16) (prev, c0-c2)
    18 - konstColor (rgba8) (konst0-3)
    19 - tev counts                             (indexed by MatEntry.unk[4])
    20 - tev stages
    21 - tev swap mode info
    22 - tev swap mode table
    23 - fog info            (indexed by MatEntry.indices2[0])
    24 - alphacomp array     (indexed by MatEntry.indices2[1])
    25 - blend info array    (indexed by MatEntry.indices2[2])
    26 - zmode info array                       (indexed by MatEntry.unk[6])

  The following fields are only here for MAT3 blocks, not for MAT2

  27 - matData6
  28 - matData7
  30 - NBTScale              (indexed by MatEntry.indices2[3])
  */
  u32 offsets[30];
};

struct MatEntry
{
  //(0 - possible values: 1, 4, 253 [1: draw on tree down, 4 on up??])
  //     - related to transparency sorting
  //1 - index into cullModes
  //2 - index into numChans
  //3 - index into texgen counts
  //4 - index into tev counts
  //5 - index into matData6 (?)
  //6 - index into zModes? (quite sure)
  //7 - index into matData7 (?)
  //(still missing stuff: isDirect, zCompLoc,
  //enable/disable blend alphatest depthtest, ...)
  u8 unk[8];

  // 0, 1 - index into ambColor (e.g. map_delfino3.bmd)
  // 6, 7 - index into matColor (e.g. mo.bdl)
  // 2, 3, 4, 5 - index into chanControls
  //u16 chanControls[8];

  u16 ambColor[2];
  u16 chanControls[4];
  u16 matColor[2]; //not in MAT2 block

  u16 lights[8]; //all 0xffff most of the time, not in MAT2 block

  u16 texGenInfo[8];
  u16 texGenInfo2[8];

  u16 texMatrices[10]; //direct index
  u16 dttMatrices[20]; //?? (I have no idea what dtt matrices do...)

  u16 texStages[8]; //indices into textureTable

  //constColor (GX_TEV_KCSEL_K0-3)
  u16 konstColor[4]; //direct index

  u8 constColorSel[16]; //0x0c most of the time (const color sel, GX_TEV_KCSEL_*)
  u8 constAlphaSel[16]; //0x1c most of the time (const alpha sel, GX_TEV_KASEL_*)

  u16 tevOrderInfo[16]; //direct index

  //this is to be loaded into
  //GX_CC_CPREV - GX_CC_A2??
  u16 registerColor[4]; //direct index


  //these two always contained the same data in all files
  //I've seen...
  u16 tevStageInfo[16]; //direct index
  u16 tevSwapModeInfo[16]; //direct index

  u16 tevSwapModeTable[4];
  
  
  u16 unknown6[12]; //vf_118 has a float in here (but only in one block...)
  //f32 unknown6[6];

  //0 - fog index (vf_117.bdl)
  //1 - alphaComp (vf_117.bdl, yoshi.bmd)
  //2 - blendInfo (cl.bdl)
  //3 - nbt scale?
  u16 indices2[4];
};


//structs below are wip

struct MatIndirectTexturingEntry
{
  //size = 312 = 0x138
  //(not always...see default.bmd <- but there 3 and 2 point to the same
  //location in file (string table). but default.bmd is the only file i know
  //where number of ind tex entries doesn't match number of mats)


  //this could be arguments to GX_SetIndTexOrder() plus some dummy values
  u16 unk[10];

  struct
  {
    float f[6]; //3x2 matrix? texmatrix?
    u8 b[4];
  } unk2[3];

  //probably the arguments to GX_SetIndTexOrder()
  //or GX_SetIndtexCoordScale() (index is first param)
  u32 unk3[4];

  struct
  {
    //the first 9 bytes of this array are probably the arguments to
    //GX_SetTevIndirect (index is the first argument), the
    //other three bytes are padding
    u16 unk[4 + 2];
  } unk4[16];
};

struct ColorChanInfo
{
  u8 enable;
  u8 matColorSource;
  u8 litMask;
  u8 diffuseAttenuationFunc;
  u8 attenuationFracFunc;
  u8 ambColorSource;
  u8 pad[2];
};

struct TexGenInfo
{
  u8 texGenType;
  u8 texGenSrc;
  u8 matrix;
  u8 pad;
};

struct TexMtxInfo
{
  u8 projection;
  u8 type;
  u16 padding0; // 0xFFFF

  f32 center_s,center_t;
  f32 unknown0;
  f32 scale_s,scale_t;

  u16 rotate; // -32768 = -180 deg, 32768 = 180 deg
  u16 padding1; // 0xFFFF

  f32 translate_s,translate_t;

  f32 prematrix[4][4];
};


struct TevOrderInfo
{
  u8 texCoordId;
  u8 texMap;
  u8 chanId;
  u8 pad;
};

struct TevStageInfo
{
  u8 unk; //always 0xff

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

  u8 unk2; //always 0xff
};

struct TevSwapModeInfo
{
  u8 rasSel;
  u8 texSel;
  u8 pad[2];
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
  u8 pad[3]; //??
};

struct BlendInfo
{
  u8 blendMode;
  u8 srcFactor, dstFactor;
  u8 logicOp;
};

struct ZModeInfo
{
  u8 enable;
  u8 func;
  u8 updateEnable;
  u8 pad; //(ref val?)
};

struct FogInfo
{
  u8 fogType;
  u8 enable;
  u16 center;

  f32 startZ;
  f32 endZ;
  f32 nearZ;
  f32 farZ;

  u32 color; //rgba
  u16 adjTable[10]; //????
};

};

bmd::MatIndirectTexturingEntry g_defaultIndirectEntry = 
{
  {
    0, 0xffff, 0xffff, 0xffff, 0xffff,
    0xffff, 0xffff, 0xffff, 0xffff, 0xffff
  }, //unk1

  {
    { 
     { .5f, 0.f, 0.f, 0.f, .5f, 0.f },
     { 1, 0xff, 0xff, 0xff }
    },
    {
      { .5f, 0.f, 0.f, 0.f, .5f, 0.f },
      { 1, 0xff, 0xff, 0xff }
    },
    {
      { .5f, 0.f, 0.f, 0.f, .5f, 0.f },
      { 1, 0xff, 0xff, 0xff }
    }
  },

  { 0x0000ffff, 0x0000ffff, 0x0000ffff, 0x0000ffff },

  {
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 

    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }, 
    { {0, 0, 0, 0, 0x00ff, 0xffff} }
  }
};

void displayData(ostream& out, const string& str, FILE* f, int offset, int size, int space = -1)
{
  long old = ftell(f);
  fseek(f, offset, SEEK_SET);

  out << str << ":";

  for(int i = 0, c = 1; i < size; ++i, ++c)
  {
    if(space > 0 && (i%space == 0))
      if(space >= 16)
        out << endl;
      else
        out << ' ';

    u8 v; fread(&v, 1, 1, f);
    out << hex << setw(2) << setfill('0') << (int)v;
  }

  if(space > 0)
    out << " (" << size/float(space) << " many)" << endl;

  //log("%s", textStream.str().c_str());

  fseek(f, old, SEEK_SET);
}

void readTevStageInfo(FILE* f, bmd::TevStageInfo& info);

void displayTevStage(ostream& out, FILE* f, int offset, int size)
{
  if(size%20 != 0)
  {
    warn("TevStage has wrong size");
    return;
  }

  long old = ftell(f);
  fseek(f, offset, SEEK_SET);

  out << "TevStageInfo (op, bias, scale, doClamp, tevRegId):";

  int count = size/20;
  for(int i = 0; i < count; ++i)
  {
    bmd::TevStageInfo info;
    readTevStageInfo(f, info);
    //textStream << hex << setw(2) << setfill('0') << (int)v;

    out << endl << hex << setfill('0') <<
      setw(2) << (int)info.unk << "  " <<
      setw(2) << (int)info.colorIn[0] << ' ' << 
      setw(2) << (int)info.colorIn[1] << ' ' << 
      setw(2) << (int)info.colorIn[2] << ' ' << 
      setw(2) << (int)info.colorIn[3] << "  " << 
      setw(2) << (int)info.colorOp << ' ' << 
      setw(2) << (int)info.colorBias << ' ' << 
      setw(2) << (int)info.colorScale << ' ' << 
      setw(2) << (int)info.colorClamp << ' ' << 
      setw(2) << (int)info.colorRegId << "  " << 
      setw(2) << (int)info.alphaIn[0] << ' ' << 
      setw(2) << (int)info.alphaIn[1] << ' ' << 
      setw(2) << (int)info.alphaIn[2] << ' ' << 
      setw(2) << (int)info.alphaIn[3] << "  " << 
      setw(2) << (int)info.alphaOp << ' ' << 
      setw(2) << (int)info.alphaBias << ' ' << 
      setw(2) << (int)info.alphaScale << ' ' << 
      setw(2) << (int)info.alphaClamp << ' ' << 
      setw(2) << (int)info.alphaRegId << "  " << 
      setw(2) << (int)info.unk2;
  }

  out << " (" << dec << count << " many)" << endl;

  //log("%s", textStream.str().c_str(), count);


  fseek(f, old, SEEK_SET);
}

void displaySize(ostream& out, const string& str, int len, int space = -1)
{
  float count = -1;
  if(space > 0)
    count = len/float(space);

  out << str << ": " << len << " bytes (" << count << " many)" << endl;
}

void writeMat3Data(ostream& debugOut, FILE* f, int mat3Offset,
                   bmd::Mat3Header& h, const vector<size_t>& lengths)
{
  //displayData(debugOut, "matEntries", f, mat3Offset + h.offsets[0], sizeof(bmd::MatEntry));
  displayData(debugOut, "indexToMatIndex", f, mat3Offset + h.offsets[1], 2*h.count, 2);
  //2: string table
  displaySize(debugOut, "IndirectTexturing", lengths[3], 312);
  displayData(debugOut, "cull mode", f, mat3Offset + h.offsets[4], lengths[4], 4);
  displayData(debugOut, "ambColor", f, mat3Offset + h.offsets[5], lengths[5], 4);
  displayData(debugOut, "numChans (?) (unk[2])", f, mat3Offset + h.offsets[6], lengths[6], 1);
  displayData(debugOut, "colorChanInfo", f, mat3Offset + h.offsets[7], lengths[7], 8);
  displayData(debugOut, "matColor", f, mat3Offset + h.offsets[8], lengths[8], 4);
  displayData(debugOut, "light", f, mat3Offset + h.offsets[9], lengths[9], 52); //there's one dr_comp.bdl
  displayData(debugOut, "texCounts (unk[3])", f, mat3Offset + h.offsets[10], lengths[10], 1);
  displayData(debugOut, "TexGen", f, mat3Offset + h.offsets[11], lengths[11], 4);
  displayData(debugOut, "TexGen2", f, mat3Offset + h.offsets[12], lengths[12], 4);
  displaySize(debugOut, "TexMtxInfo", lengths[13], 100);  //13: TexMtxInfo (below)
  displaySize(debugOut, "TexMtxInfo2", lengths[14]);
  displayData(debugOut, "indexToTexIndex", f, mat3Offset + h.offsets[15], lengths[15], 2);
  displayData(debugOut, "tevOrderInfo", f, mat3Offset + h.offsets[16], lengths[16], 4);
  displayData(debugOut, "registerColor", f, mat3Offset + h.offsets[17], lengths[17], 8);
  displayData(debugOut, "konstColor", f, mat3Offset + h.offsets[18], lengths[18], 4);
  displayData(debugOut, "tevStageCounts (unk[4])", f, mat3Offset + h.offsets[19], lengths[19], 1);
  displayTevStage(debugOut, f, mat3Offset + h.offsets[20], lengths[20]);
  displayData(debugOut, "tevSwapModeInfo", f, mat3Offset + h.offsets[21], lengths[21], 4);
  displayData(debugOut, "tevSwapModeTable", f, mat3Offset + h.offsets[22], lengths[22], 4);
  displayData(debugOut, "fog", f, mat3Offset + h.offsets[23], lengths[23], 44);
  displayData(debugOut, "alpha comp", f, mat3Offset + h.offsets[24], lengths[24], 8);
  displayData(debugOut, "blend info", f, mat3Offset + h.offsets[25], lengths[25], 4);
  displayData(debugOut, "z mode", f, mat3Offset + h.offsets[26], lengths[26], 4);
  displayData(debugOut, "matData6 (isIndirect (?))", f, mat3Offset + h.offsets[27], lengths[27], 1);
  displayData(debugOut, "matData7", f, mat3Offset + h.offsets[28], lengths[28], 1);
  displayData(debugOut, "NBTscale", f, mat3Offset + h.offsets[29], lengths[29]);
}

void writeMatEntry(ostream& debugOut, const bmd::MatEntry& init)
{
  int j;
  debugOut << setfill('0');
  debugOut << "unk: unk1, cull, numChans, texCounts, tevCounts, matData6Index, zMode, matData7Index:";
  for(j = 0; j < 8; ++j) debugOut << " " << hex << setw(2) << (int)init.unk[j]; debugOut << endl;

  debugOut << "ambColor (?): ";
  for(j = 0; j < 2; ++j) debugOut << setw(4) << init.ambColor[j]; debugOut << endl;

  debugOut << "chanControls (?): ";
  for(j = 0; j < 4; ++j) debugOut << setw(4) << init.chanControls[j]; debugOut << endl;

  debugOut << "matColor (?): ";
  for(j = 0; j < 2; ++j) debugOut << setw(4) << init.matColor[j]; debugOut << endl;

  debugOut << "lights: ";
  for(j = 0; j < 8; ++j) debugOut << setw(4) << init.lights[j]; debugOut << endl;

  debugOut << "texGenInfo: ";
  for(j = 0; j < 8; ++j) debugOut << setw(4) << init.texGenInfo[j]; debugOut << endl;

  debugOut << "texGenInfo2: ";
  for(j = 0; j < 8; ++j) debugOut << setw(4) << init.texGenInfo2[j]; debugOut << endl;

  debugOut << "texMatrices: ";
  for(j = 0; j < 10; ++j) debugOut << setw(4) << init.texMatrices[j]; debugOut << endl;

  debugOut << "dttMatrices: ";
  for(j = 0; j < 20; ++j) debugOut << setw(4) << init.dttMatrices[j]; debugOut << endl;

  debugOut << "Textures: ";
  for(j = 0; j < 8; ++j) debugOut << setw(4) << init.texStages[j]; debugOut << endl;

  debugOut << "konstColor: ";
  for(j = 0; j < 4; ++j) debugOut << setw(4) << init.konstColor[j]; debugOut << endl;

  debugOut << "constColorSel: ";
  for(j = 0; j < 16; ++j) debugOut << hex << setw(2) << (int)init.constColorSel[j]; debugOut << endl;

  debugOut << "constAlphaSel: ";
  for(j = 0; j < 16; ++j) debugOut << hex << setw(2) << (int)init.constAlphaSel[j]; debugOut << endl;

  debugOut << "tevOrderInfo: ";
  for(j = 0; j < 16; ++j) debugOut << setw(4) << init.tevOrderInfo[j]; debugOut << endl;

  debugOut << "registerColor: ";
  for(j = 0; j < 4; ++j) debugOut << setw(4) << init.registerColor[j]; debugOut << endl;

  debugOut << "tevStageInfo: ";
  for(j = 0; j < 16; ++j) debugOut << setw(4) << init.tevStageInfo[j]; debugOut << endl;

  debugOut << "tevSwapModeInfo: ";
  for(j = 0; j < 16; ++j) debugOut << setw(4) << init.tevSwapModeInfo[j]; debugOut << endl;

  debugOut << "tevSwapModeTable: ";
  for(j = 0; j < 4; ++j) debugOut << setw(4) << init.tevSwapModeTable[j]; debugOut << endl;

  debugOut << "unk6: ";
  for(j = 0; j < 12; ++j) debugOut << setw(4) << init.unknown6[j]; debugOut << endl;
  //for(j = 0; j < 6; ++j) debugOut << " " << init.unknown6[j]; debugOut << endl;

  debugOut << "index fog, alphaComp, blend, nbtScale: ";
  for(j = 0; j < 4; ++j) debugOut << setw(4) << init.indices2[j]; debugOut << endl;
  debugOut << endl;
}

void readMat3Header(FILE* f, bmd::Mat3Header& h)
{
  fread(h.tag, 1, 4, f);
  readDWORD(f, h.sizeOfSection);
  readWORD(f, h.count);
  readWORD(f, h.pad);
  for(int i = 0; i < 30; ++i)
    readDWORD(f, h.offsets[i]);

  if(strncmp(h.tag, "MAT2", 4) == 0)
  {
    //check if this is a MAT3 section (most of the time) or a MAT2 section
    //(TODO: probably there's also a MAT1 section - find one...)

    //if this is a mat2 section, convert header to a mat3 header
    for(int j = 29; j >= 0; --j)
    {
      u32 t;
      if(j < 3) t = h.offsets[j];
      else if(j == 3 || j == 8 || j == 9) t = 0;
      else if(j < 8) t = h.offsets[j - 1];
      else t = h.offsets[j - 3];
      h.offsets[j] = t;
    }
  }
}

void readMatIndirectTexturingEntry(FILE* f, bmd::MatIndirectTexturingEntry& indEntry)
{
  int k, m;
  for(k = 0; k < 10; ++k)
    readWORD(f, indEntry.unk[k]);
  for(k = 0; k < 3; ++k)
  {
    for(m = 0; m < 6; ++m)
      readFLOAT(f, indEntry.unk2[k].f[m]);
    fread(indEntry.unk2[k].b, 1, 4, f);
  }

  for(k = 0; k < 4; ++k)
    readDWORD(f, indEntry.unk3[k]);
  for(k = 0; k < 16; ++k)
    for(m = 0; m < 6; ++m)
      readWORD(f, indEntry.unk4[k].unk[m]);
}

void readMatEntry(FILE* f, bmd::MatEntry& init, bool isMat2)
{
  int j;
  fread(init.unk, 1, 8, f);
  for(j = 0; j < 2; ++j) readWORD(f, init.ambColor[j]);
  for(j = 0; j < 4; ++j) readWORD(f, init.chanControls[j]);
  
  //these two fields are only in mat3 headers, not in mat2
  if(!isMat2) for(j = 0; j < 2; ++j) readWORD(f, init.matColor[j]);
  else memset(init.matColor, 0xff, 2*2);
  if(!isMat2) for(j = 0; j < 8; ++j) readWORD(f, init.lights[j]);
  else memset(init.lights, 0xff, 8*2);
  
  for(j = 0; j < 8; ++j) readWORD(f, init.texGenInfo[j]);
  for(j = 0; j < 8; ++j) readWORD(f, init.texGenInfo2[j]);
  for(j = 0; j < 10; ++j) readWORD(f, init.texMatrices[j]);
  for(j = 0; j < 20; ++j) readWORD(f, init.dttMatrices[j]);
  for(j = 0; j < 8; ++j) readWORD(f, init.texStages[j]);
  for(j = 0; j < 4; ++j) readWORD(f, init.konstColor[j]);
  fread(init.constColorSel, 1, 16, f);
  fread(init.constAlphaSel, 1, 16, f);
  for(j = 0; j < 16; ++j) readWORD(f, init.tevOrderInfo[j]);
  for(j = 0; j < 4; ++j) readWORD(f, init.registerColor[j]);
  for(j = 0; j < 16; ++j) readWORD(f, init.tevStageInfo[j]);
  for(j = 0; j < 16; ++j) readWORD(f, init.tevSwapModeInfo[j]);
  for(j = 0; j < 4; ++j) readWORD(f, init.tevSwapModeTable[j]);
  for(j = 0; j < 12; ++j) readWORD(f, init.unknown6[j]);
  //for(j = 0; j < 6; ++j) readFLOAT(f, init.unknown6[j]);
  for(j = 0; j < 4; ++j) readWORD(f, init.indices2[j]);
}

void readTexMtxInfo(FILE* f, bmd::TexMtxInfo& info)
{
  int j;
  fread(&info.projection, 1, 1, f);
  fread(&info.type, 1, 1, f);
  readWORD(f, info.padding0);

  readFLOAT(f, info.center_s);
  readFLOAT(f, info.center_t);
  readFLOAT(f, info.unknown0);
  readFLOAT(f, info.scale_s);
  readFLOAT(f, info.scale_t);

  readWORD(f, info.rotate);
  readWORD(f, info.padding1);
  
  readFLOAT(f, info.translate_s);
  readFLOAT(f, info.translate_t);

  for(j = 0; j < 16; ++j)
    readFLOAT(f, info.prematrix[j/4][j%4]);
}

void readTevStageInfo(FILE* f, bmd::TevStageInfo& info)
{
  fread(&info.unk, 1, 1, f);
  
  fread(&info.colorIn, 1, 4, f);
  fread(&info.colorOp, 1, 1, f);
  fread(&info.colorBias, 1, 1, f);
  fread(&info.colorScale, 1, 1, f);
  fread(&info.colorClamp, 1, 1, f);
  fread(&info.colorRegId, 1, 1, f);
  
  fread(&info.alphaIn, 1, 4, f);
  fread(&info.alphaOp, 1, 1, f);
  fread(&info.alphaBias, 1, 1, f);
  fread(&info.alphaScale, 1, 1, f);
  fread(&info.alphaClamp, 1, 1, f);
  fread(&info.alphaRegId, 1, 1, f);
  
  fread(&info.unk2, 1, 1, f);
}

void computeSectionLengths(const bmd::Mat3Header& h, vector<size_t>& lengths)
{
  for(size_t i = 0; i < 30; ++i)
  {
    size_t length = 0;
    if(h.offsets[i] != 0)
    {
      size_t next = h.sizeOfSection;
      for(size_t j = i + 1; j < 30; ++j)
      {
        if(h.offsets[j] != 0)
        {
          next = h.offsets[j];
          break;
        }
      }
      length = next - h.offsets[i];
    }

    lengths[i] = length;
    if(i == 3)
    {
      //assert(length%h.count == 0); //violated by luigi's mansion files
      //assert(length/h.count == 312); //violated by quite a few files
    }
  }
}

void dumpMat3(FILE* f, Mat3& dst)
{
  //warn("Mat3 section support is incomplete");

  assert(sizeof(bmd::MatEntry) == 332);

  int mat3Offset = ftell(f);
  size_t i;

  //read header
  bmd::Mat3Header h;
  readMat3Header(f, h);

  bool isMat2 = strncmp(h.tag, "MAT2", 4) == 0;
  if(isMat2)
    warn("Model contains MAT2 block instead of MAT3");

  //read stringtable
  //vector<string> stringtable;
  readStringtable(mat3Offset + h.offsets[2], f, dst.stringtable);
  if(h.count != dst.stringtable.size())
    warn("mat3: number of strings (%d) doesn't match number of elements (%d)",
         dst.stringtable.size(), h.count);

  //compute max length of each subsection
  //(it's probably better to check the maximal indices
  //of every MatEntry and use these as counts, but
  //this works fine as well, so stick with it for now)
  vector<size_t> lengths(30);
  computeSectionLengths(h, lengths);

  //offset[1] (indirection table from indices to init data indices)
  fseek(f, mat3Offset + h.offsets[1], SEEK_SET);
  u16 maxIndex = 0;
  dst.indexToMatIndex.resize(h.count);
  for(i = 0; i < h.count; ++i)
  {
    u16 bla; readWORD(f, bla);
    maxIndex = max(maxIndex, bla);
    dst.indexToMatIndex[i] = bla;
  }

  //offset[4] (cull mode)
  fseek(f, mat3Offset + h.offsets[4], SEEK_SET);
  dst.cullModes.resize(lengths[4]/4);
  for(i = 0; i < dst.cullModes.size(); ++i)
  {
    u32 tmp; readDWORD(f, tmp);
    dst.cullModes[i] = tmp;
  }

  //offset[5] (ambColor)
  fseek(f, mat3Offset + h.offsets[5], SEEK_SET);
  dst.ambColor.resize(lengths[5]/4);
  for(i = 0; i < dst.ambColor.size(); ++i)
  {
    u8 col[4]; fread(col, 1, 4, f);
    dst.ambColor[i].r = col[0];
    dst.ambColor[i].g = col[1];
    dst.ambColor[i].b = col[2];
    dst.ambColor[i].a = col[3];
  }

  //offset[6] (numChans)
  fseek(f, mat3Offset + h.offsets[6], SEEK_SET);
  dst.numChans.resize(lengths[6]);
  fread(&dst.numChans[0], 1, lengths[6], f);

  //offset[7] (colorChanInfo)
  fseek(f, mat3Offset + h.offsets[7], SEEK_SET);
  dst.colorChanInfos.resize(lengths[7]/8);
  for(i = 0; i < dst.colorChanInfos.size(); ++i)
  {
    bmd::ColorChanInfo info;
    fread(&info.enable, 1, 1, f);
    fread(&info.matColorSource, 1, 1, f);
    fread(&info.litMask, 1, 1, f);
    fread(&info.diffuseAttenuationFunc, 1, 1, f);
    fread(&info.attenuationFracFunc, 1, 1, f);
    fread(&info.ambColorSource, 1, 1, f);
    fread(&info.pad[0], 1, 1, f);
    fread(&info.pad[1], 1, 1, f);
	
    ColorChanInfo& dstInfo = dst.colorChanInfos[i];

	dstInfo.enable = info.enable;
    dstInfo.ambColorSource = info.ambColorSource;
    dstInfo.matColorSource = info.matColorSource;
    dstInfo.litMask = info.litMask;
    dstInfo.attenuationFracFunc = info.attenuationFracFunc;
    dstInfo.diffuseAttenuationFunc = info.diffuseAttenuationFunc;
  }


  //offset[8] (matColor)
  fseek(f, mat3Offset + h.offsets[8], SEEK_SET);
  dst.matColor.resize(lengths[8]/4);
  for(i = 0; i < dst.matColor.size(); ++i)
  {
    u8 col[4]; fread(col, 1, 4, f);
    dst.matColor[i].r = col[0];
    dst.matColor[i].g = col[1];
    dst.matColor[i].b = col[2];
    dst.matColor[i].a = col[3];
  }

  //offset[0] (MatEntries)
  fseek(f, mat3Offset + h.offsets[0], SEEK_SET);
  dst.materials.resize(maxIndex + 1);
  for(i = 0; i <= maxIndex; ++i)
  {
    bmd::MatEntry init;
    readMatEntry(f, init, isMat2);
	
    //copy data
    //(this assumes that init has already been endian-fixed)
    Material& dstMat = dst.materials[i];

    dstMat.flag = init.unk[0];
    dstMat.cullIndex = init.unk[1];
    dstMat.numChansIndex = init.unk[2];
    dstMat.texGenCountIndex = init.unk[3];
    dstMat.tevCountIndex = init.unk[4];
    dstMat.zModeIndex = init.unk[6];

    int j;
    for(j = 0; j < 8; ++j)
    {
      dstMat.texGenInfos[j] = init.texGenInfo[j];
      dstMat.texMtxInfos[j] = init.texMatrices[j];
      dstMat.texStages[j] = init.texStages[j];
    }
    for(j = 0; j < 4; ++j)
    {
      dstMat.konstColor[j] = init.konstColor[j];
      dstMat.registerColor[j] = init.registerColor[j];

      dstMat.chanControls[j] = init.chanControls[j];

      dstMat.tevSwapModeTable[j] = init.tevSwapModeTable[j];
    }
    for(j = 0; j < 2; ++j)
    {
      dstMat.ambColor[j] = init.ambColor[j];
      dstMat.matColor[j] = init.matColor[j];
    }
    for(j = 0; j < 16; ++j)
    {
      dstMat.constColorSel[j] = init.constColorSel[j];
      dstMat.constAlphaSel[j] = init.constAlphaSel[j];
      dstMat.tevOrderInfo[j] = init.tevOrderInfo[j];
      dstMat.tevStageInfo[j] = init.tevStageInfo[j];
      dstMat.tevSwapModeInfo[j] = init.tevSwapModeInfo[j];
    }

    dstMat.alphaCompIndex = init.indices2[1];
    dstMat.blendIndex = init.indices2[2];
  }

  //offset[3] indirect texturing blocks (always as many as count)
  assert(sizeof(bmd::MatIndirectTexturingEntry) == 312);
  fseek(f, mat3Offset + h.offsets[3], SEEK_SET);
  if(lengths[3]%312 != 0)
    warn("mat3: indirect texturing block size no multiple of 312: %d", lengths[3]);
  else if(lengths[3]/312 != h.count)
    warn("mat3: number of ind texturing blocks (%d) doesn't match number of materials (%d)",
      lengths[3]/312, h.count);
  else
  {
    for(i = 0; i < h.count; ++i)
    {
      bmd::MatIndirectTexturingEntry indEntry;
      readMatIndirectTexturingEntry(f, indEntry);
      //...
      if(memcmp(&g_defaultIndirectEntry, &indEntry, sizeof(indEntry)) != 0)
        warn("found different ind tex block");
    }
  }

  //offsets[10] (read texGenCounts)
  fseek(f, mat3Offset + h.offsets[10], SEEK_SET);
  dst.texGenCounts.resize(lengths[10]);
  fread(&dst.texGenCounts[0], 1, dst.texGenCounts.size(), f);

  //offsets[11] (texGens)
  fseek(f, mat3Offset + h.offsets[11], SEEK_SET);
  dst.texGenInfos.resize(lengths[11]/4);
  for(i = 0; i < dst.texGenInfos.size(); ++i)
  {
    bmd::TexGenInfo info;
    fread(&info.texGenType, 1, 1, f);
    fread(&info.texGenSrc, 1, 1, f);
    fread(&info.matrix, 1, 1, f);
    fread(&info.pad, 1, 1, f);

    dst.texGenInfos[i].texGenType = info.texGenType;
    dst.texGenInfos[i].texGenSrc = info.texGenSrc;
    dst.texGenInfos[i].matrix = info.matrix;
  }

  //offsets[13] (read texMtxInfo)
  fseek(f, mat3Offset + h.offsets[13], SEEK_SET);
  dst.texMtxInfos.resize(lengths[13]/100);
  for(i = 0; i < dst.texMtxInfos.size(); ++i)
  {
    bmd::TexMtxInfo info;
    readTexMtxInfo(f, info);
    TexMtxInfo dstInfo = { 
		info.projection, info.type, 
		info.center_s, info.center_t, info.unknown0, info.scale_s, info.scale_t, 
		info.rotate, info.translate_s, info.translate_t };
	memcpy(dstInfo.prematrix, info.prematrix, sizeof(info.prematrix));
    dst.texMtxInfos[i] = dstInfo;
  }

  //offsets[15] (read texTable)
  fseek(f, mat3Offset + h.offsets[15], SEEK_SET);
  size_t texLength = lengths[15];
  dst.texStageIndexToTextureIndex.resize(texLength/2);
  for(i = 0; i < texLength/2; ++i)
  {
    u16 index; fread(&index, 2, 1, f); toWORD(index);
    dst.texStageIndexToTextureIndex[i] = index;
  }

  //offsets[16] (read TevOrderInfos)
  fseek(f, mat3Offset + h.offsets[16], SEEK_SET);
  dst.tevOrderInfos.resize(lengths[16]/4);
  for(i = 0; i < dst.tevOrderInfos.size(); ++i)
  {
    bmd::TevOrderInfo info;
    fread(&info.texCoordId, 1, 1, f);
    fread(&info.texMap, 1, 1, f);
    fread(&info.chanId, 1, 1, f);
    fread(&info.pad, 1, 1, f);

    dst.tevOrderInfos[i].texCoordId = info.texCoordId;
    dst.tevOrderInfos[i].texMap = info.texMap;
    dst.tevOrderInfos[i].chanId = info.chanId;
  }

  //offsets[17] (read registerColor)
  fseek(f, mat3Offset + h.offsets[17], SEEK_SET);
  dst.registerColor.resize(lengths[17]/(4*2));
  for(i = 0; i < dst.registerColor.size(); ++i)
  {
    s16 col[4]; fread(col, 2, 4, f);
    dst.registerColor[i].r = aSHORT(col[0]);
    dst.registerColor[i].g = aSHORT(col[1]);
    dst.registerColor[i].b = aSHORT(col[2]);
    dst.registerColor[i].a = aSHORT(col[3]);
  }

  //offsets[18] (konstColor)
  fseek(f, mat3Offset + h.offsets[18], SEEK_SET);
  dst.konstColor.resize(lengths[18]/4);
  for(i = 0; i < dst.konstColor.size(); ++i)
  {
    u8 col[4]; fread(col, 1, 4, f);
    dst.konstColor[i].r = col[0];
    dst.konstColor[i].g = col[1];
    dst.konstColor[i].b = col[2];
    dst.konstColor[i].a = col[3];
  }

  //offset[19] (tevCounts)
  fseek(f, mat3Offset + h.offsets[19], SEEK_SET);
  dst.tevCounts.resize(lengths[19]);
  fread(&dst.tevCounts[0], 1, dst.tevCounts.size(), f);

  //offset[20] (TevStageInfos)
  fseek(f, mat3Offset + h.offsets[20], SEEK_SET);
  dst.tevStageInfos.resize(lengths[20]/20);
  for(i = 0; i < dst.tevStageInfos.size(); ++i)
  {
    bmd::TevStageInfo info;
    readTevStageInfo(f, info);
    
    memcpy(&dst.tevStageInfos[i], ((u8*)&info) + 1, sizeof(info) - 2); //XXX: Hacky...
  }

  //offset[21] (TevSwapModeInfos)
  fseek(f, mat3Offset + h.offsets[21], SEEK_SET);
  dst.tevSwapModeInfos.resize(lengths[21]/4);
  for(i = 0; i < dst.tevSwapModeInfos.size(); ++i)
  {
    bmd::TevSwapModeInfo info;
    fread(&info.rasSel, 1, 1, f);
    fread(&info.texSel, 1, 1, f);
    fread(info.pad, 1, 2, f);

    dst.tevSwapModeInfos[i].rasSel = info.rasSel;
    dst.tevSwapModeInfos[i].texSel = info.texSel;
  }

  //offset[22] (TevSwapModeTable)
  fseek(f, mat3Offset + h.offsets[22], SEEK_SET);
  dst.tevSwapModeTables.resize(lengths[22]/4);
  for(i = 0; i < dst.tevSwapModeTables.size(); ++i)
  {
    bmd::TevSwapModeTable table;
    fread(&table.r, 1, 1, f);
    fread(&table.g, 1, 1, f);
    fread(&table.b, 1, 1, f);
    fread(&table.a, 1, 1, f);

    TevSwapModeTable dstTable = { table.r, table.g, table.b, table.a };
    dst.tevSwapModeTables[i] = dstTable;
  }

  //offset[24] (alphaCompares)
  fseek(f, mat3Offset + h.offsets[24], SEEK_SET);
  dst.alphaCompares.resize(lengths[24]/8);
  for(i = 0; i < dst.alphaCompares.size(); ++i)
  {
    bmd::AlphaCompare info;
    fread(&info.comp0, 1, 1, f);
    fread(&info.ref0, 1, 1, f);
    fread(&info.alphaOp, 1, 1, f);
    fread(&info.comp1, 1, 1, f);
    fread(&info.ref1, 1, 1, f);
    fread(info.pad, 1, 3, f);

    AlphaCompare dstInfo = { info.comp0, info.ref0, info.alphaOp, info.comp1, info.ref1 };
    dst.alphaCompares[i] = dstInfo;
  }

  //offset[25] (blendInfo)
  fseek(f, mat3Offset + h.offsets[25], SEEK_SET);
  dst.blendInfos.resize(lengths[25]/4);
  for(i = 0; i < dst.blendInfos.size(); ++i)
  {
    bmd::BlendInfo info;
    fread(&info.blendMode, 1, 1, f);
    fread(&info.srcFactor, 1, 1, f);
    fread(&info.dstFactor, 1, 1, f);
    fread(&info.logicOp, 1, 1, f);

    BlendInfo dstInfo = { info.blendMode, info.srcFactor, info.dstFactor, info.logicOp };
    dst.blendInfos[i] = dstInfo;
  }

  //offset[26] (z mode)
  fseek(f, mat3Offset + h.offsets[26], SEEK_SET);
  dst.zModes.resize(lengths[26]/4);
  for(i = 0; i < dst.zModes.size(); ++i)
  {
    bmd::ZModeInfo zInfo;
    fread(&zInfo.enable, 1, 1, f);
    fread(&zInfo.func, 1, 1, f);
    fread(&zInfo.updateEnable, 1, 1, f);
    fread(&zInfo.pad, 1, 1, f);

    ZMode m;
    m.enable = zInfo.enable != 0;
    m.zFunc = zInfo.func;
    m.enableUpdate = zInfo.updateEnable != 0;
    dst.zModes[i] = m;
  }
}

void writeMat3Info(FILE* f, ostream& out)
{
  int mat3Offset = ftell(f);
  size_t i;

  out << string(50, '/') << endl
      << "//Mat3 section" << endl
      << string(50, '/') << endl << endl;

  out << "(only partially implemented)" << endl << endl << endl;

  //read header
  bmd::Mat3Header h;
  readMat3Header(f, h);
  bool isMat2 = strncmp(h.tag, "MAT2", 4) == 0;

  //compute lengths (see comment above for some notes on this)
  vector<size_t> lengths(30);
  computeSectionLengths(h, lengths);

  //offset[1] (indirection table from indices to init data indices)
  fseek(f, mat3Offset + h.offsets[1], SEEK_SET);
  u16 maxIndex = 0;
  vector<u16> indexToMatIndex(h.count);
  for(i = 0; i < h.count; ++i)
  {
    u16 bla; fread(&bla, 2, 1, f); toWORD(bla);
    maxIndex = max(maxIndex, bla);
    indexToMatIndex[i] = bla;
  }

  //indexed data
  out << string(h.tag, 4) <<  " data" << endl << endl;
  writeMat3Data(out, f, mat3Offset, h, lengths);

  //read stringtable
  vector<string> stringtable;
  readStringtable(mat3Offset + h.offsets[2], f, stringtable);

  //offset[0] (MatEntries)
  out << endl << endl << "MatEntries" << endl << endl;
  fseek(f, mat3Offset + h.offsets[0], SEEK_SET);
  for(i = 0; i <= maxIndex; ++i)
  {
    bmd::MatEntry init;
    readMatEntry(f, init, isMat2);
	
    //dump names of this stage info block
    for(size_t m = 0; m < indexToMatIndex.size(); ++m)
      if(indexToMatIndex[m] == i)
        out << stringtable[m] << " ";
    out << endl;

    //dump block
    writeMatEntry(out, init);
  }

  //offset[3] indirect texturing blocks (always as many as count)
  out << endl << endl << "Indirect texturing entries" << endl;
  fseek(f, mat3Offset + h.offsets[3], SEEK_SET);
  if(lengths[3]%312 != 0)
    out << "mat3: indirect texturing block size no multiple of 312: " << lengths[3] << endl << endl;
  else if(lengths[3]/312 != h.count)
    out << "mat3: number of ind texturing blocks (" << lengths[3]/312
        << ") doesn't match number of materials (" << h.count << ")" << endl << endl;
  else
  {
    //dump data
    for(i = 0; i < h.count; ++i)
    {
      bmd::MatIndirectTexturingEntry indEntry;
      readMatIndirectTexturingEntry(f, indEntry);

      //dump block
      int k;
      if(i < stringtable.size())
        out << " " << stringtable[i] << endl;
      out << " Ten shorts:" << endl << "  ";
      for(k = 0; k < 10; ++k)
        out << hex << setw(4) << indEntry.unk[k] << " ";
      out << endl;
      out << " Three times 6 floats, 4 bytes:" << endl;
      for(k = 0; k < 3; ++k)
      {
        out << "  ";
        int m;
        for(m = 0; m < 3; ++m)
          out << setw(6) << indEntry.unk2[k].f[m] << " ";
        out << endl << "  ";
        for(m = 3; m < 6; ++m)
          out << setw(6) << indEntry.unk2[k].f[m] << " ";
        out << endl << "  ";
        for(m = 0; m < 4; ++m)
          out << hex << setw(2) << (int)indEntry.unk2[k].b[m] << " ";
        out << endl;
      }
      out << " 4 ints:" << endl;
      for(k = 0; k < 4; ++k)
        out << hex << "  " << setw(8) << indEntry.unk3[k] << endl;
      out << " 16 times 6 shorts:" << endl;
      for(k = 0; k < 16; ++k)
      {
        out << "  ";
        for(int m = 0; m < 6; ++m)
          out << hex << setw(4) << indEntry.unk4[k].unk[m] << " ";
        out << endl;
      }
      out << endl;

      if(memcmp(&g_defaultIndirectEntry, &indEntry, sizeof(indEntry)) != 0)
        out << " ->This was a different block." << endl;

      out << endl;
    }
  }

  //offset[13] (texmtxinfo)
  out << "TexMtxInfos" << endl << endl;
  fseek(f, mat3Offset + h.offsets[13], SEEK_SET);

  for(size_t m = 0; m < lengths[13]/100; ++m)
  {
    bmd::TexMtxInfo info;
    readTexMtxInfo(f, info);
  }
}


#if 0

/*
//from yaz0r:
struct J3DMaterialBlock
{
	unsigned long int header; //0
	unsigned long int size; //4
	unsigned short int numMaterial; //8
	unsigned short int unkA; //A


	unsigned long int offsetToMaterialInitData; //C
	unsigned long int offsetToMaterialData1; //10
	unsigned long int offsetToNameTable; //14
	unsigned long int var18; //18
	unsigned long int offsetToCullMode; // 1C
	unsigned long int offsetToColor; // 20
	unsigned long int offsetToMaterialData2; // 24
	unsigned long int offsetToColorChanInfo; // 28
	unsigned long int offsetToColor2; // 2C
	unsigned long int offsetToLightInfo; // 30
	unsigned long int offsetToMaterialData3; // 34
	unsigned long int offsetToTexCoordInfo; // 38
	unsigned long int offsetToTexCoord2Info; // 3C
	unsigned long int offsetToTexMtxInfo; // 40
	unsigned long int offsetToTexMtxInfo2; // 44
	unsigned long int offsetToMaterialData4; // 48
	unsigned long int offsetToTevOrderInfo; // 4C 
	unsigned long int offsetToColorS10; // 50
	unsigned long int offsetToColor3; // 54
	unsigned long int offsetToMaterialData5; // 58
	unsigned long int offsetToTevStageInfo; // 5C
	unsigned long int offsetToTevSwapModeInfo; // 60
	unsigned long int offsetToTevSwapModeTableInfo; // 64
	unsigned long int offsetToFogInfo; // 68
	unsigned long int offsetToAlphaCompInfo; // 6C
	unsigned long int offsetToBlendInfo; // 70
	unsigned long int offsetToZModeInfo; // 74
	unsigned long int offsetToMaterialData6; // 78
	unsigned long int offsetToMaterialData7; // 7C
	unsigned long int offsetToNBTScaleInfo; // 80
};
*/

#endif
