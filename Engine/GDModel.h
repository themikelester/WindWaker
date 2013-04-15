#include "Common\common.h"
#include "Framework3\Renderer.h"
#include <d3d10.h>

struct Header;
namespace Json {
	class Value;
}

namespace GDModel
{
	
	enum SgNodeType {
		SG_END		= 0x00,
		SG_DOWN		= 0x01,
		SG_UP		= 0x02,
		SG_JOINT	= 0x10,
		SG_MATERIAL	= 0x11,
		SG_PRIM		= 0x12
	};
	
	enum VertexAttributeFlags
	{
		HAS_MATRIX_INDICES	= 1 << 0,
		HAS_POSITIONS		= 1 << 1,
		HAS_NORMALS			= 1 << 2,
		HAS_COLORS0			= 1 << 3,
		HAS_COLORS1			= 1 << 4,
		HAS_TEXCOORDS0		= 1 << 5,
		HAS_TEXCOORDS1		= 1 << 6,
		HAS_TEXCOORDS2		= 1 << 7,
		HAS_TEXCOORDS3		= 1 << 8,
		HAS_TEXCOORDS4		= 1 << 9,
		HAS_TEXCOORDS5		= 1 << 10,
		HAS_TEXCOORDS6		= 1 << 11,
		HAS_TEXCOORDS7		= 1 << 12,
	};

	struct Scenegraph
	{
		u16 index;
		u16 type; //One of SgNodeType
	};
	
	typedef ubyte ModelAsset;

	struct GDModel
	{
		ModelAsset* _asset;

		uint nScenegraphNodes;
		Scenegraph* scenegraph;
	};
	
	RESULT Draw(Renderer* renderer, ID3D10Device* device, GDModel* model);
	
	//Given a JSON object, compile a binary blob that can be loaded with Load() and Reload()
	RESULT Compile(const Json::Value& root, Header& hdr, char** data);

	//Save our asset reference and initialize the model in the renderer
	RESULT Load(GDModel* model, ModelAsset* blob);
	
	//Unregister our old asset with the renderer. Save our new reference.
	//Initialize our new asset with the renderer. The asset manager will then delete the old asset.
	RESULT Reload(GDModel* model, ModelAsset* blob);
}