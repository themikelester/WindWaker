#ifndef BMD_SHP1_H
#define BMD_SHP1_H BMD_SHP1_H

#include <vector>
#include <iosfwd>
#include "common.h"
#include <Framework3/Math/Vector.h>

enum BatchAttributeFlags
{
	HAS_MATRIX_INDICES	= 1 << 0,
	HAS_POSITIONS		= 1 << 1,
	HAS_NORMALS			= 1 << 2,
	HAS_COLORS0			= 1 << 3,
	HAS_COLORS1			= 1 << 4,
	HAS_TEXCOORDS0		= 1 << 5,
	HAS_TEXCOORDS1		= 1 << 6,
	HAS_TEXCOORDS2		= 1 << 7,
	HAS_TEXCOORDS3		= 1 << 8,
	HAS_TEXCOORDS4		= 1 << 9,
	HAS_TEXCOORDS5		= 1 << 10,
	HAS_TEXCOORDS6		= 1 << 11,
	HAS_TEXCOORDS7		= 1 << 12,
};

struct Index
{
  u16 matrixIndex;		// Index into matrix table. Divide by 3 to find real index. 0 > i > 10
  u16 posIndex;			// Index into vertex list in vtx1.positions . 0 > i > vtx1.positions.length
  u16 normalIndex;		// Index into normal list in vtx1.normals . 0 > i > vtx1.normals.length
  u16 colorIndex[2];	// Index into colors list in vtx1.colors . 0 > i > vtx1.colors.length
  u16 texCoordIndex[8];	// Index into texture coordinate list in vtx1.texCoords . 0 > i > vtx1.texCoords.length
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

  // Internal use only
  u16 indexCount;
};

struct Batch1
{
  u16 attribs; //BatchAttributeFlags
  std::vector<Packet> packets;

  vec3 bbMin, bbMax; //experimental
  u8 matrixType; //experimental
};

struct Shp1
{
  std::vector<Batch1> batches;

  //TODO: unknown data is missing, ...
};

void dumpShp1(Chunk* f, Shp1& dst);
void writeShp1Info(Chunk* f, std::ostream& out);

#endif //BMD_SHP1_H
