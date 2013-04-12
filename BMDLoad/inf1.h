#ifndef BMD_INF1_H
#define BMD_INF1_H BMD_INF1_H

#include "common.h"
#include <vector>
#include <iosfwd>

enum SGNode_t {
	SG_JOINT	= 0x10,
	SG_MATERIAL	= 0x11,
	SG_PRIM		= 0x12,
	SG_DOWN		= 0x01,
	SG_UP		= 0x02,
	SG_END		= 0x00
};

struct Node //same as Inf1Entry
{
  u16 type, index;
};

struct Inf1
{
  int numVertices; //no idea what's this good for ;-)

  std::vector<Node> scenegraph;
};

void dumpInf1(Chunk* f, Inf1& dst);


//the following is only convenience stuff

struct SceneGraph
{
	int type;
	int index;
	std::vector<SceneGraph> children;
};

int buildSceneGraph(/*in*/ const Inf1& inf1, /*out*/ SceneGraph& sg, int j = 0 /* used internally */);
void writeInf1Info(Chunk* f, std::ostream& out);

#endif //BMD_INF1_H
