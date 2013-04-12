#ifndef BMD_DRW1_H
#define BMD_DRW1_H BMD_DRW1_H

#include "common.h"
#include <vector>
#include <iosfwd>

struct Drw1
{
  std::vector<bool> isWeighted;
  std::vector<u16> data;
};

void dumpDrw1(Chunk* f, Drw1& dst);
void writeDrw1Info(Chunk* f, std::ostream& out);

#endif //BMD_DRW1_H
