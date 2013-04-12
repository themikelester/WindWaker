#ifndef BMD_EVP1_H
#define BMD_EVP1_H BMD_EVP1_H

#include "common.h"

#include <iosfwd>

#include <Framework3\Math\Vector.h>

struct MultiMatrix
{
  std::vector<float> weights;
  std::vector<u16> indices; //indices into Evp1.matrices (?)
};

struct Evp1
{
  std::vector<MultiMatrix> weightedIndices;
  std::vector<mat4> matrices;
};

void dumpEvp1(Chunk* f, Evp1& dst);
void writeEvp1Info(Chunk* f, std::ostream& out);

#endif //BMD_EVP1_H
