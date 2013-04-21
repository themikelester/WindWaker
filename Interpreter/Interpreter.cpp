// Interpreter.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <stdarg.h>
#include <fstream>

#include "json\json.h"
#include "BMDRead\bmdread.h"
#include "BMDRead\openfile.h"

using namespace std;

typedef unsigned int uint;

std::ofstream texFile;
std::ofstream jsonFile;

uint serializeImage(int ID, u8* buf, int size)
{
	std::streamoff offset = texFile.tellp();
	texFile.write((char*) buf, size);
	return uint(offset);
}

void warn(const char* msg, ...)
{
  va_list argList;
  va_start(argList, msg);
  char buff[161];
  vsnprintf_s(buff, 161, msg, argList);
  va_end(argList);
  cout << buff << endl;
}

int _tmain(int argc, TCHAR* argv[])
{
	if(argc < 2)
		return -1;

	char* filename = argv[1];
	OpenedFile* f = openFile(filename);
	if(f == nullptr)
		return -2;
  
	BModel* bmd = loadBmd(f->f);
	if (bmd != nullptr)
	{
		cout << "Opened " << filename 
		<< " for interpreting to intermediate format" << endl;
	}

	closeFile(f);

	// Open our texture output file
	char texFilename[128];
	_snprintf_s(texFilename, 128, "%s.tex", filename);
	texFile.open(texFilename, ios::out | ios::binary);
	if(texFile.is_open())
	{
		cout << "Writing texture file to " << texFilename << endl;
	}
	else
	{
		cout << "Failed to open " << texFilename << " for writing" << endl;
		return -4;
	}

	Json::Value root;
	root["Info"]["type"] = "bmd";
	root["Info"]["version"] = 1.0;
	root["Info"]["name"] = filename;

	root["Inf1"] = serializeInf1(bmd->inf1);
	root["Vtx1"] = serializeVtx1(bmd->vtx1);
	root["Drw1"] = serializeDrw1(bmd->drw1);
	root["Evp1"] = serializeEvp1(bmd->evp1);
	root["Jnt1"] = serializeJnt1(bmd->jnt1);
	root["Shp1"] = bmd->shp1.serialize();

	Json::Value mat;
	root["Mat3"] = bmd->mat3.serialize(mat);
	root["Tex1"] = bmd->tex1.serialize();

	Json::StyledWriter writer;
	std::string outBmd = writer.write( root );

	texFile.close();

	char jsonFilename[128];
	_snprintf_s(jsonFilename, 128, "%s.json", filename);
	jsonFile.open(jsonFilename, ios::out);
	if(f != nullptr)
	{
		cout << "Writing JSON file to " << jsonFilename << endl;
		jsonFile.write(outBmd.c_str(), outBmd.size());
		jsonFile.close();
		return 0;
	}
	else
	{
		cout << "Failed to open " << jsonFilename << " for writing" << endl;
		return -3;
	}
}

