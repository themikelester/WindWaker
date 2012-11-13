#include "shp1.h"
#include <cassert>
#include <iostream>

using namespace std;

namespace bmd
{

struct Shp1Header
{
  char tag[4];
  u32 sizeOfSection;
  u16 batchCount; //number of batches
  u16 pad; //??
  u32 offsetToBatches; //should be 0x2c (Batch1 info starts here)

  u32 offsetUnknown; //??
  u32 zero; //??
  u32 offsetToBatchAttribs;  //Batch1 vertex attrib start

  //The matrixTable is an array of u16, which maps from the matrix data indices
  //to Drw1Data arrays indices. If a Batch1 contains multiple packets, for the
  //2nd, 3rd, ... packet this array may contain 0xffff values, which means that
  //the corresponding index from the previous packet should be used.
  u32 offsetToMatrixTable;
  
  
  u32 offsetData; //start of the actual primitive data
  u32 offsetToMatrixData;
  u32 offsetToPacketLocations; //offset to packet start/length info
  
  //(all offsets relative to Shp1Header start)
};

//Shp1Header.batchCount many of these structs are
//stored at Shp1Header.offsetToBatches
struct Batch1
{
  u8 matrixType; //0, 1, 2, 3 "matrix type" according to yazor
            //0 seems to be the default value

            //1 could mean y-billboard (mo_fire02.bmd, default.bmd)
            //or billboard (geostar.bmd)
            //RESOLVED: I just checked in sunshine, default.bmd always
            //faces the viewer, so this means billboard
            //(camera plane billboards, not camera centered billboards)

            //2 is probably y-billboard (see snowman1.bmd), looks like
            //that in mario kart in-game as well

            //3 is about as frequent as 1, but i have no idea what this
            //could mean (don't inherit parent transform?) (hum, yaz0r's
            //code suggests that this means multimatrix transform...
            //but that would conlict with my understanding of DRW1 and EVP1...)
            //-> probably means multimatrix

  u8 unknown2; //seems to be always 0xff
  u16 packetCount; //number of packets belonging to this Batch1

  //attribs used for the strips in this Batch1. relative to
  //Shp1Header.offsetToBatchAttribs
  //Read StripTypes until you encounter an 0x000000ff/0x00000000,
  //for all these types indices are included. If, for example,
  //a Batch1 has types (9, 3), (a, 3), (0xff, 0), then for this Batch1 two shorts (= 3)
  //are stored per vertex: position index and normal index
  u16 offsetToAttribs; 

  u16 firstMatrixData; //index to first matrix data (packetCount consecutive indices)
  u16 firstPacketLocation; //index to first packet location (packetCount consecutive indices)


  u16 unknown3; //0xffff

  f32 unknown4;

  //some kind of bounding box (in which coord frame?)
  f32 bbMin[3];
  f32 bbMax[3];
};

struct BatchAttrib
{
  u32 attrib; //cf. ArrayFormat.arrayType
  u32 dataType; //cf. ArrayFormat.dataType (always bytes or shorts...)
};

//for every packet a PacketLocation struct is stored at
//Shp1Header.offsetToPacketLocation + Batch1.firstPacketLocation*sizeof(PacketLocation).
//This struct stores where the primitive data for this packet is stored in the
//data block.
struct PacketLocation
{
  u32 size; //size in bytes of packet
  u32 offset; //relative to Shp1Header.offsetData
};

struct Primitive
{
  u8 primitiveType; //see above
  u16 numVertices; //that many vertices included in this primitive - for
                   //each vertex indices are stored according to Batch1 type
};

//for every packet a MatrixData struct is stored at
//Shp1Header.offsetToMatrixData + Batch1.firstMatrixData*sizeof(MatrixData).
//This struct stores which part of the MatrixTable belongs to this packet
//(the matrix table is stored at Shp1Header.offsetToMatrixTable)
struct MatrixData //from yaz0r's source (animation stuff)
{
  u16 unknown1;
  u16 count; //count many consecutive indices into matrixTable
  u32 firstIndex; //first index into matrix table
};


typedef vector<BatchAttrib> BatchAttribs;

};

bmd::BatchAttribs getBatchAttribs(Chunk* f, int off)
{
  int old = Dtell(f);

  DSeek(f, off, SEEK_SET);

  bmd::BatchAttribs ret;

  bmd::BatchAttrib attrib;
  readDWORD(f, attrib.attrib);
  readDWORD(f, attrib.dataType);

  while(attrib.attrib != 0xff)
  {
    ret.push_back(attrib);

    readDWORD(f, attrib.attrib);
    readDWORD(f, attrib.dataType);
  }

  DSeek(f, old, SEEK_SET);
  return ret;
};

void dumpPacketPrimitives(const bmd::BatchAttribs& attribs, int dataSize, Chunk* f, Packet& dst)
{
  bool done = false;
  int readBytes = 0;

  while(!done)
  {
    u8 type;
    DRead(&type, 1, 1, f);
    ++readBytes;

    if(type == 0 || readBytes >= dataSize)
    {
      done = true;
      continue;
    }

    dst.primitives.push_back(Primitive());
    Primitive& currPrimitive = dst.primitives.back();
    currPrimitive.type = type;

    u16 count;
    readWORD(f, count);
    readBytes += 2;

    currPrimitive.points.resize(count);

    for(int j = 0; j < count; ++j)
    {
      Index& currPoint = currPrimitive.points[j];

      for(size_t k = 0; k < attribs.size(); ++k)
      {
        u16 val;

        //get value
        switch(attribs[k].dataType)
        {
          case 1: //s8
          {
            u8 tmp;
            DRead(&tmp, 1, 1, f);
            val = tmp;
            readBytes += 1;
          }break;

          case 3: //s16
          { 
            readWORD(f, val);
            readBytes += 2;
          }break;

          default:
            assert(false && "shp1: got invalid data type in packet. should never happen because "
            "dumpBatch() should check this before calling dumpPacket()");
        }

        //set appropriate index
        switch(attribs[k].attrib)
        {
          case 0:
            currPoint.matrixIndex = val;
            break;

          case 9:
            currPoint.posIndex = val;
            break;

          case 0xa:
            currPoint.normalIndex = val;
            break;

          case 0xb:
          case 0xc:
            currPoint.colorIndex[attribs[k].attrib - 0xb] = val;
            break;

          case 0xd:
          case 0xe:
          case 0xf:
          case 0x10:
          case 0x11:
          case 0x12:
          case 0x13:
          case 0x14:
            currPoint.texCoordIndex[attribs[k].attrib - 0xd] = val;
            break;

          default:
            //assert(false && "shp1: got invalid attrib in packet. should never happen because "
            //"dumpBatch() should check this before calling dumpPacket()");

            ; //ignore unknown types, it's enough to WARN() in dumpBatch
        }
      }
    }
  }
}

void dumpBatch(Batch1& dst, const bmd::Batch1& Batch1, const bmd::Shp1Header& h, Chunk* f, long baseOffset)
{
  size_t i;

  dst.bbMin = vec3(Batch1.bbMin[0], Batch1.bbMin[1], Batch1.bbMin[2]);
  dst.bbMax = vec3(Batch1.bbMax[0], Batch1.bbMax[1], Batch1.bbMax[2]);
  dst.matrixType = Batch1.matrixType;

  //read and interpret Batch1 vertex attribs
  bmd::BatchAttribs attribs = getBatchAttribs(f,
    baseOffset + h.offsetToBatchAttribs + Batch1.offsetToAttribs);
  
  dst.attribs = 0;
  for(i = 0; i < attribs.size(); ++i)
  {
    if(attribs[i].dataType != 1 && attribs[i].dataType != 3)
    {
      WARN("shp1, dumpBatch(): unknown attrib data type %d, skipping Batch1", attribs[i].dataType);
      return;
    }

    switch(attribs[i].attrib)
    {
      case 0x00: dst.attribs |= HAS_MATRIX_INDICES;	break;
	  case 0x09: dst.attribs |= HAS_POSITIONS;		break;
      case 0x0a: dst.attribs |= HAS_NORMALS;		break;
      case 0x0b: dst.attribs |= HAS_COLORS0;		break;
      case 0x0c: dst.attribs |= HAS_COLORS1;		break;
	  case 0x0d: dst.attribs |= HAS_TEXCOORDS0;		break;
      case 0x0e: dst.attribs |= HAS_TEXCOORDS1;		break;
      case 0x0f: dst.attribs |= HAS_TEXCOORDS2;		break;
      case 0x10: dst.attribs |= HAS_TEXCOORDS3;		break;
      case 0x11: dst.attribs |= HAS_TEXCOORDS4;		break;
      case 0x12: dst.attribs |= HAS_TEXCOORDS5;		break;
      case 0x13: dst.attribs |= HAS_TEXCOORDS6;		break;
      case 0x14: dst.attribs |= HAS_TEXCOORDS7;		break;

      default:
        WARN("shp1, dumpBatch(): unknown attrib %d in Batch1, it might not display correctly", attribs[i].attrib);
        //return; //it's enough to warn
    }
  }

  //read packets
  dst.packets.resize(Batch1.packetCount);
  for(i = 0; i < Batch1.packetCount; ++i)
  {
    //read packet location for current packet
    bmd::PacketLocation packetLocation;
    DSeek(f, baseOffset + h.offsetToPacketLocations
      + (Batch1.firstPacketLocation + i)*sizeof(packetLocation), SEEK_SET);
    readDWORD(f, packetLocation.size);
    readDWORD(f, packetLocation.offset);

    //read packet's primitives
    Packet& dstPacket = dst.packets[i];
    DSeek(f, baseOffset + h.offsetData + packetLocation.offset, SEEK_SET);
    dumpPacketPrimitives(attribs, packetLocation.size, f, dstPacket);

    //read matrix data for current packet
    bmd::MatrixData matrixData;
    DSeek(f, baseOffset + h.offsetToMatrixData
      + (Batch1.firstMatrixData + i)*sizeof(matrixData), SEEK_SET);
    readWORD(f, matrixData.unknown1); //TODO: figure this out...
    readWORD(f, matrixData.count);
    readDWORD(f, matrixData.firstIndex);

    //read packet's matrix table
    dstPacket.matrixTable.resize(matrixData.count);
    DSeek(f, baseOffset + h.offsetToMatrixTable
      + 2*matrixData.firstIndex, SEEK_SET);
    for(int j = 0; j < matrixData.count; ++j)
      readWORD(f, dstPacket.matrixTable[j]);
  }
}

void readShp1Header(Chunk* f, bmd::Shp1Header& h)
{
  DRead(h.tag, 1, 4, f);
  readDWORD(f, h.sizeOfSection);
  readWORD(f, h.batchCount);
  readWORD(f, h.pad);
  readDWORD(f, h.offsetToBatches);
  readDWORD(f, h.offsetUnknown);
  readDWORD(f, h.zero);
  readDWORD(f, h.offsetToBatchAttribs);
  readDWORD(f, h.offsetToMatrixTable);

  readDWORD(f, h.offsetData);
  readDWORD(f, h.offsetToMatrixData);
  readDWORD(f, h.offsetToPacketLocations);
}

void readBatch(Chunk* f, bmd::Batch1& d)
{
  DRead(&d.matrixType, 1, 1, f);
  DRead(&d.unknown2, 1, 1, f);
  readWORD(f, d.packetCount);
  readWORD(f, d.offsetToAttribs);
  readWORD(f, d.firstMatrixData);
  readWORD(f, d.firstPacketLocation);
  readWORD(f, d.unknown3);
  readFLOAT(f, d.unknown4);
  for(int i = 0; i < 3; ++i)
    readFLOAT(f, d.bbMin[i]);
  for(int j = 0; j < 3; ++j)
    readFLOAT(f, d.bbMax[j]);
}

void dumpShp1(Chunk* f, Shp1& dst)
{
  int shp1Offset = Dtell(f), i;

  bmd::Shp1Header h;
  readShp1Header(f, h);

  //read batches
  DSeek(f, h.offsetToBatches + shp1Offset, SEEK_SET);
  dst.batches.resize(h.batchCount);
  for(i = 0; i < h.batchCount; ++i)
  {
    bmd::Batch1 d;
    readBatch(f, d);

    Batch1& dstBatch = dst.batches[i];

    long filePos = Dtell(f);
    dumpBatch(dstBatch, d, h, f, shp1Offset);
    DSeek(f, filePos, SEEK_SET);
  }
}

void writeShp1Info(Chunk* f, ostream& out)
{
  out << string(50, '/') << endl
      << "//Shp1 section" << endl
      << string(50, '/') << endl << endl;


  int shp1Offset = Dtell(f), i;

  bmd::Shp1Header h;
  readShp1Header(f, h);

  //read batches
  out << "Batches (VERY incomplete)" << endl;
  DSeek(f, h.offsetToBatches + shp1Offset, SEEK_SET);
  for(i = 0; i < h.batchCount; ++i)
  {
    bmd::Batch1 d;
    readBatch(f, d);

    //very incomplete output ;-)
    out << "   matrixType(?) " << hex << (int)d.matrixType
        << " unknown2 " << (int)d.unknown2
        << " numPackets " << d.packetCount
        << endl << "  "
        << " unknown4 " << d.unknown4
        << endl << "  "
        << " bbMin " << d.bbMin[0] << ", " << d.bbMin[1] << ", " << d.bbMin[2]
        << endl << "  "
        << " bbMax " << d.bbMax[0] << ", " << d.bbMax[1] << ", " << d.bbMax[2]
        << endl << endl;
  }

  out << endl;
}
