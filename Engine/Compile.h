#pragma once
#include "json\json.h"

struct Header
{
	char fourCC[4];
	int version;
	int sizeBytes;
};


int _Compile(std::string filename);