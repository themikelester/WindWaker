// Interpreter.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "BMDRead\bck.h"

using namespace std;

typedef unsigned int uint;

extern std::string GenerateVS(Mat3* matInfo, int index);
extern std::string GeneratePS(Tex1* texInfo, Mat3* matInfo, int index);

std::ofstream texFile;
std::ofstream jsonFile;

uint serializeImage(int ID, u8* buf, int size)
{
	std::streamoff offset = texFile.tellp();
	texFile.write((char*) buf, size);
	return uint(offset);
}

char _DEBUG_BUFFER[256];
void warn(const char* msg, ...)
{
  va_list argList;
  va_start(argList, msg);
  char buff[161];
  vsnprintf_s(buff, 161, msg, argList);
  va_end(argList);
  cout << buff << endl;
}

Json::Value compileMaterials(Mat3& mat, Tex1& tex)
{
	Json::Value node;
	for (uint i = 0; i < mat.materials.size(); i++)
	{
		std::string vs = GenerateVS(&mat, i);
		std::string ps = GeneratePS(&tex, &mat, i);
		node[i]["VS"] = vs;
		node[i]["PS"] = ps;
	}
	return node;
}

Json::Value ParseBCK(FILE* f, const char* filename)
{
	Json::Value root;

	Bck* bck = readBck(f);
	root["Info"]["type"] = "bck";
	root["Info"]["version"] = 1.0;
	root["Info"]["name"] = filename;

	root["Anim"] = ( serializeBck(bck) );

	return root;
}

Json::Value ParseBMD(FILE* f, const char* filename)
{
	Json::Value root;

	BModel* bmd = loadBmd(f);

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
	
	root["Shd1"] = compileMaterials(bmd->mat3, bmd->tex1);

	texFile.close();

	return root;
}

int _tmain(int argc, TCHAR* argv[])
{
	Json::Value root;
	
	if(argc < 2)
		return -1;

	char* filename = argv[1];
	OpenedFile* f = openFile(filename);
	if(f == nullptr)
	{
		return -2;
	}
	else
	{
		cout << "Opened " << filename 
		<< " for interpreting to intermediate format" << endl;
	}

	// Read the file type
	char type[3];
	fseek(f->f, 0x04, SEEK_CUR);
	fread(type, 1, 3, f->f);

	if ( memcmp(type, "bmd", 3) == 0 || memcmp(type, "bdl", 3) == 0 )
		root = ParseBMD(f->f, filename);
	else if ( memcmp(type, "bck", 3) == 0 )
		root = ParseBCK(f->f, filename);
	
	closeFile(f);

	Json::StyledWriter writer;
	std::string outBmd = writer.write( root );

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

