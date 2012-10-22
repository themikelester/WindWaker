#include "BMDLoader.h"

#include <iostream>
#include <assert.h>

using namespace std;

// Need to modify this to only read n bytes if we're reading from an archive
void readBmd(Chunk* d, BModel* dst)
{
  //Make sure this is actually a BMD/BDL file
  DSeek(d, 0x04, SEEK_CUR);
  char hdr[3];
  DRead(hdr, 1, 3, d);
  assert( memcmp(hdr, "bdl", 3) == 0 || memcmp(hdr, "bmd", 3) == 0 );
	  
  //skip rest of file header
  DSeek(d, 0x19, SEEK_CUR);

  u32 size = 0;
  char tag[4];
  int t;

  do
  {
    DSeek(d, size, SEEK_CUR);
    t = Dtell(d);

    DRead(tag, 1, 4, d);
    DRead(&size, 4, 1, d);
    toDWORD(size);
    if(size < 8) size = 8; //prevent endless loop on corrupt data

    if(DEOD(d))
      break;

    DSeek(d, t, SEEK_SET);

    setStartupText("Parsing " + std::string(tag, 4) + "...");

    if(strncmp(tag, "INF1", 4) == 0)
      dumpInf1(d, dst->inf1);
    else if(strncmp(tag, "VTX1", 4) == 0)
      dumpVtx1(d, dst->vtx1);
    else if(strncmp(tag, "EVP1", 4) == 0)
      dumpEvp1(d, dst->evp1);
    else if(strncmp(tag, "DRW1", 4) == 0)
      dumpDrw1(d, dst->drw1);
    else if(strncmp(tag, "JNT1", 4) == 0)
      dumpJnt1(d, dst->jnt1);
    else if(strncmp(tag, "SHP1", 4) == 0)
      dumpShp1(d, dst->shp1);
    //else if(strncmp(tag, "MAT3", 4) == 0)
	else if(strncmp(tag, "MAT", 3) == 0) //s_forest.bmd has a MAT2 section
      dumpMat3(d, dst->mat3);
    else if(strncmp(tag, "TEX1", 4) == 0)
      dumpTex1(d, dst->tex1);
    else
      warn("readBmd(): Unsupported section \'%c%c%c%c\'",
        tag[0], tag[1], tag[2], tag[3]);

    DSeek(d, t, SEEK_SET);

  } while(!DEOD(d));
}

BModel* loadBmd(Chunk* f)
{
  BModel* ret = new BModel;
  readBmd(f, ret);
  return ret;
}

void writeBmdInfo(Chunk* f, std::ostream& out)
{
  //skip file header
  DSeek(f, 0x20, SEEK_SET);

  u32 size = 0;
  char tag[4];
  int t;

  do
  {
    DSeek(f, size, SEEK_CUR);
    t = Dtell(f);

    DRead(tag, 1, 4, f);
    DRead(&size, 4, 1, f);
    toDWORD(size);
    if(size < 8) size = 8; //prevent endless loop on corrupt data

    if(DEOD(f))
      break;

    DSeek(f, t, SEEK_SET);

    if(strncmp(tag, "INF1", 4) == 0)
      writeInf1Info(f, out);
    else if(strncmp(tag, "VTX1", 4) == 0)
      writeVtx1Info(f, out);
    else if(strncmp(tag, "EVP1", 4) == 0)
      writeEvp1Info(f, out);
    else if(strncmp(tag, "DRW1", 4) == 0)
      writeDrw1Info(f, out);
    else if(strncmp(tag, "JNT1", 4) == 0)
      writeJnt1Info(f, out);
    else if(strncmp(tag, "SHP1", 4) == 0)
      writeShp1Info(f, out);
    //else if(strncmp(tag, "MAT3", 4) == 0)
	else if(strncmp(tag, "MAT", 3) == 0) //s_forest.bmd has a MAT2 section
      writeMat3Info(f, out);
    else if(strncmp(tag, "TEX1", 4) == 0)
      writeTex1Info(f, out);
    else if(strncmp(tag, "MDL3", 4) == 0)
      writeMdl3Info(f, out);

    DSeek(f, t, SEEK_SET);
  } while(!DEOD(f));
}
