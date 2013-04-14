#include "Common\common.h"

#include "json\json.h"
#include <iostream>
#include <fstream>

#include "Compile.h"
#include "GCModel.h"

using namespace std;

RESULT _WriteBlob(string filename, Blob& blob)
{
	ofstream file (filename, ios::out | ios::binary);
	if (file.is_open())
	{
		file.write(blob.data, blob.size);
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
	Blob blob = {};
	RESULT r = S_OK;

	ifstream file (filename);
	if (file.is_open())
	{
		bool parsingSuccessful = reader.parse(file, root, true);
		if ( !parsingSuccessful )
		{
			std::cout << "Failed to parse JSON from " << filename << endl;
			std::cout << reader.getFormatedErrorMessages();
			IFC(-1); //TODO: Define enums for these failure values
		}

		string type = root["Info"].get("type", "unknown").asString();
		if (type == "bmd")
		{
			IFC( GDModel::Compile(root, blob) );
		}
		else 
		{
			std::cout << "Asset type " << type << " unsupported. Exiting." << endl;
			IFC(-3);
		}

		int lastindex = filename.find_last_of("."); 
		string rawname = filename.substr(0, lastindex); 
		string blobname = rawname + ".blob";

		IFC( _WriteBlob(blobname, blob) );
	}
	else
	{
		std::cout << "Failed to open file " << filename << endl;
		IFC(-2);
	}

cleanup:
	if (file.is_open()) file.close();
	if (blob.data != nullptr) free(blob.data);

	return r;
}