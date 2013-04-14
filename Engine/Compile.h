#pragma once
#include "json\json.h"

struct Blob
{
	char* data;
	size_t size;
};

int _Compile(std::string filename);