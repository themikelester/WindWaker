#include "evp1.h"

#include <iostream>
#include <assert.h>
using namespace std;

namespace bmd
{

struct Evp1Header
{
  char tag[4]; //'EVP1'
  u32 sizeOfSection;
  u16 count;
  u16 pad;

  //0 - count many bytes, each byte describes how many bones belong to this index
  //1 - sum over all bytes in 0 many shorts (index into some joint stuff? into matrix table?)
  //2 - bone weights table (as many floats as shorts in 1)
  //3 - matrix table (matrix is 3x4 float array)
  u32 offsets[4];
};

};

void readEvp1Header(Chunk* f, bmd::Evp1Header& h)
{
  DRead(h.tag, 1, 4, f);
  readDWORD(f, h.sizeOfSection);
  readWORD(f, h.count);
  readWORD(f, h.pad);
  for(int i = 0; i < 4; ++i)
    readDWORD(f, h.offsets[i]);
}

void readArray8(Chunk* f, vector<int>& arr)
{
  for(size_t i = 0; i < arr.size(); ++i)
  {
    u8 v; DRead(&v, 1, 1, f);
    arr[i] = v;
  }
}

void readMatrix(Chunk* c, mat4& m)
{
	assert(sizeof(vec4) == 16);
	for(int j = 0; j < 3; ++j)
		DRead(&(m.rows[j]), 4, 4, c);
	// This isn't flipping endianness (GC is big-endian, PC's are little-endian)
	// But it doesn't seem to matter, we must flip when we read it later
}

void dumpEvp1(Chunk* f, Evp1& dst)
{
  int evp1Offset = Dtell(f), i;

  //read header
  bmd::Evp1Header h;
  readEvp1Header(f, h);

  //read counts array
  DSeek(f, evp1Offset + h.offsets[0], SEEK_SET);
  vector<int> counts(h.count);
  readArray8(f, counts);

  //read indices of weighted matrices
  dst.weightedIndices.resize(h.count);
  DSeek(f, evp1Offset + h.offsets[1], SEEK_SET);
  int numMatrices = 0;
  for(i = 0; i < h.count; ++i)
  {
    dst.weightedIndices[i].indices.resize(counts[i]);
    for(int j = 0; j < counts[i]; ++j)
    {
      u16 d; DRead(&d, 2, 1, f); toWORD(d);
      dst.weightedIndices[i].indices[j] = d;
      numMatrices = max(numMatrices, d + 1);
     }
  }

  //read weights of weighted matrices
  DSeek(f, evp1Offset + h.offsets[2], SEEK_SET);
  for(i = 0; i < h.count; ++i)
  {
    dst.weightedIndices[i].weights.resize(counts[i]);
    for(int j = 0; j < counts[i]; ++j)
    {
      float fl; DRead(&fl, 4, 1, f); toFLOAT(fl);
      dst.weightedIndices[i].weights[j] = fl;
    }
  }

  //read matrices
  dst.matrices.resize(numMatrices);
  DSeek(f, evp1Offset + h.offsets[3], SEEK_SET);
  for(i = 0; i < numMatrices; ++i)
  {
    readMatrix(f, dst.matrices[i]);
  }
}

void writeEvp1Info(Chunk* f, ostream& out)
{
  out << string(50, '/') << endl
      << "//Evp1 section (incomplete)" << endl
      << string(50, '/') << endl << endl;
  
  bmd::Evp1Header h;
  readEvp1Header(f, h);
  
  out << h.count << " many" << endl << endl;
}
