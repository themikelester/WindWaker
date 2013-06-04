#pragma once
#include "json\json.h"

struct Header
{
	char fourCC[4]; // Four characters describing the data type of this blob
	int version;	// Version number of the compiler used to create this blob
	int sizeBytes;	// Size in bytes of the blob data
};


int _Compile(std::string filename);