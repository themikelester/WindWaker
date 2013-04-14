#ifndef BMD_SHP1_H
#define BMD_SHP1_H BMD_SHP1_H

#include <vector>
#include <iosfwd>
#include "common.h"
#include "Vector3.h"

struct Index
{
  u16 matrixIndex;
  u16 posIndex;
  u16 normalIndex;
  u16 colorIndex[2];
  u16 texCoordIndex[8];

  Json::Value serialize()
  {
	Json::Value index;

	index["mtx"] = matrixIndex;
	index["pos"] = posIndex;
	index["nrm"] = normalIndex;
	index["clr"][uint(0)] = colorIndex[0];
	index["clr"][uint(1)] = colorIndex[1];
	for (uint i = 0; i < 8; i++)
		index["tex"][uint(i)] = texCoordIndex[i];
			
	return index;
  }
};

struct Primitive
{
  u8 type;
  std::vector<Index> points;
  
  Json::Value serialize()
  {
	Json::Value prim;

	prim["type"] = type;
	for (uint i = 0; i < points.size(); i++)
		prim["points"][i] = points[i].serialize();
			
	return prim;
  }
};

enum
{
  GX_TRIANGLE_STRIP = 0x98,
  GX_TRIANGLE_FAN = 0xa0
};

struct Packet
{
  std::vector<Primitive> primitives;

  std::vector<u16> matrixTable; //maps attribute matrix index to draw array index

  Json::Value serialize()
  {
	Json::Value packet;
	for (uint i = 0; i < primitives.size(); i++)
		packet["primitives"][i] = primitives[i].serialize();

	for (uint i = 0; i < matrixTable.size(); i++)
		packet["matrixTable"][i] = matrixTable[i];
		
	return packet;
  }
};

struct Attributes
{
  bool hasMatrixIndices, hasPositions, hasNormals, hasColors[2], hasTexCoords[8];
  
  Json::Value serialize()
  {
	Json::Value attribs;

	attribs["mtx"] = hasMatrixIndices;
	attribs["pos"] = hasPositions;
	attribs["nrm"] = hasNormals;
	attribs["clr"][uint(0)] = hasColors[0];
	attribs["clr"][uint(1)] = hasColors[1];
	for (uint i = 0; i < 8; i++)
		attribs["tex"][uint(i)] = hasTexCoords[i];
			
	return attribs;
  }
};

struct Batch
{
  Attributes attribs;
  std::vector<Packet> packets;

  Vector3f bbMin, bbMax; //experimental
  u8 matrixType; //experimental

  Json::Value serialize()
  {
	  Json::Value batch;
	  batch["attribs"] = attribs.serialize();
	  batch["matrixType"] = matrixType;
	  for (uint i = 0; i < packets.size(); i++)
		batch["packets"][i] = packets[i].serialize();

	  return batch;
  }
};

struct Shp1
{
  std::vector<Batch> batches;

  //TODO: unknown data is missing, ...

  Json::Value serialize()
  {
	  Json::Value shp1;

	  for (uint i = 0; i < batches.size(); i++)
		shp1["batches"][i] = batches[i].serialize();

	  return shp1;
  }
};

void dumpShp1(FILE* f, Shp1& dst);
void writeShp1Info(FILE* f, std::ostream& out);

#endif //BMD_SHP1_H
