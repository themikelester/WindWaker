// Interpreter.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include <stdarg.h>
#include <iostream>

#include "json\json.h"
#include "BMDRead\bmdread.h"
#include "BMDRead\openfile.h"

using namespace std;

typedef unsigned int uint;

std::string serializeImage(int ID, u8* buf, int size)
{
	char filenameBuf[64];
	_snprintf_s(filenameBuf, 64, "%u.img", ID);

	return filenameBuf;
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
	if(f == 0)
		return -2;
  
	BModel* bmd = loadBmd(f->f);
	if (bmd != nullptr)
	{
		cout << "Opened " << filename 
		<< " for interpreting to intermediate format" << endl;
	}

	closeFile(f);

	Json::Value root;
	root["Info"]["type"] = "bmd";
	root["Info"]["version"] = 1.0;
	root["Info"]["name"] = filename;

	// Double check this
	

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

	char jsonFilename[128];
	_snprintf_s(jsonFilename, 128, "%s.json", filename);
	FILE* outFile;
	fopen_s(&outFile, jsonFilename, "w");
	fwrite(outBmd.c_str(), outBmd.size(), 1, outFile);
	fclose(outFile);

	return 0;
}

