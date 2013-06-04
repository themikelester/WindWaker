#include "Common\common.h"

#include "json\json.h"
#include <iostream>
#include <fstream>

#include "Compile.h"
#include "GDModel.h"
#include "GDAnim.h"

using namespace std;

RESULT _WriteBlob(string filename, Header& hdr, char* data)
{
	ofstream file (filename, ios::out | ios::binary);
	if (file.is_open())
	{
		file.write((char*)&hdr, sizeof(hdr));
		file.write(data, hdr.sizeBytes);
		file.close();
	}
	else
	{
		cout << "Failed to open binary file for writing: " << filename << endl;
		return E_FAIL;
	}

	return S_OK;
}

int _Compile(string filename)
{
	Json::Value root;
	Json::Reader reader;
	Header hdr = {};
	char* data = nullptr;
	RESULT r = S_OK;
	
	ifstream file (filename);
	if (file.is_open())
	{
		_CrtSetDbgFlag(0);
		bool parsingSuccessful = reader.parse(file, root, true);
		_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF);

		if ( !parsingSuccessful )
		{
			std::cout << "Failed to parse JSON from " << filename << endl;
			std::cout << reader.getFormatedErrorMessages();
			IFC(-1); //TODO: Define enums for these failure values
		}

		string type = root["Info"].get("type", "unknown").asString();
		if (type == "bmd")
		{
			IFC( GDModel::Compile(root, hdr, &data) );
		}
		else if (type == "bck")
		{
			IFC( GDAnim::Compile(root, hdr, &data) );
		}
		else 
		{
			WARN("Asset type \"%s\" unsupported. Exiting.\n", type.c_str());
			IFC(-3);
		}

		int lastindex = filename.find_last_of("."); 
		string rawname = filename.substr(0, lastindex); 
		string blobname = rawname + ".blob";

		IFC( _WriteBlob(blobname, hdr, data) );
	}
	else
	{
		std::cout << "Failed to open file " << filename << endl;
		IFC(-2);
	}

cleanup:
	if (file.is_open()) file.close();
	if (data != nullptr) free(data);

	return r;
}