#ifndef BMD_SHP1_H
#define BMD_SHP1_H BMD_SHP1_H

#include <vector>
#include <iosfwd>
#include "common.h"
#include <Framework3/Math/Vector.h>

struct Index
{
  u16 matrixIndex;
  u16 posIndex;
  u16 normalIndex;
  u16 colorIndex[2];
  u16 texCoordIndex[8];
};

enum PrimType_t
{
  PRIM_TRI_STRIP = 0x98,
  PRIM_TRI_FAN = 0xa0
};

struct Primitive
{
  u8 type; //See enum PrimType_t
  std::vector<Index> points;
};

struct Packet
{
  std::vector<Primitive> primitives;

  std::vector<u16> matrixTable; //maps attribute matrix index to draw array index
};

struct Attributes
{
  bool hasMatrixIndices, hasPositions, hasNormals, hasColors[2], hasTexCoords[8];
};

struct Batch1
{
  Attributes attribs;
  std::vector<Packet> packets;

  vec3 bbMin, bbMax; //experimental
  u8 matrixType; //experimental

  //Internal use only
  u8 batchID;
};

struct Shp1
{
  std::vector<Batch1> batches;

  //TODO: unknown data is missing, ...
};

void dumpShp1(Chunk* f, Shp1& dst);
void writeShp1Info(Chunk* f, std::ostream& out);

#endif //BMD_SHP1_H
