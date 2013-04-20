
/* * * * * * * * * * * * * Author's note * * * * * * * * * * * *\
*   _       _   _       _   _       _   _       _     _ _ _ _   *
*  |_|     |_| |_|     |_| |_|_   _|_| |_|     |_|  _|_|_|_|_|  *
*  |_|_ _ _|_| |_|     |_| |_|_|_|_|_| |_|     |_| |_|_ _ _     *
*  |_|_|_|_|_| |_|     |_| |_| |_| |_| |_|     |_|   |_|_|_|_   *
*  |_|     |_| |_|_ _ _|_| |_|     |_| |_|_ _ _|_|  _ _ _ _|_|  *
*  |_|     |_|   |_|_|_|   |_|     |_|   |_|_|_|   |_|_|_|_|    *
*                                                               *
*                     http://www.humus.name                     *
*                                                                *
* This file is a part of the work done by Humus. You are free to   *
* use the code in any way you like, modified, unmodified or copied   *
* into your own work. However, I expect you to respect these points:  *
*  - If you use this file and its contents unmodified, or use a major *
*    part of this file, please credit the author and leave this note. *
*  - For use in anything commercial, please request my approval.     *
*  - Share your work and ideas too as much as you can.             *
*                                                                *
\* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "App.h"
#include <Foundation\memory.h>
#include "GC3D.h"
#include "GDModel.h"

//TODO: Remove HACK
#include <fstream>

BaseApp *app = new App();

bool App::init()
{
	foundation::memory_globals::init(4 * 1024 * 1024);
	GC3D::Init();
	m_AssMan.Init();

	return true;
}

void App::exit()
{
	m_AssMan.Shutdown();
	GC3D::Shutdown();
	foundation::memory_globals::shutdown();
}

bool App::initAPI()
{
	return D3D10App::initAPI(DXGI_FORMAT_R8G8B8A8_UNORM_SRGB, DXGI_FORMAT_D32_FLOAT, max(antiAliasSamples, 1), 0);
}

void App::exitAPI()
{
	D3D10App::exitAPI();
}

bool App::load()
{
	RESULT r = S_OK;

	char* filename = "..\\Assets\\Link.rarc";
	char* nodeName = "/bdl/cl.bdl";

	std::ifstream file ("../assets/cl.bdl.blob", std::ios::in|std::ios::binary);
	if(file.is_open())
    {
		char fourcc[4];
		uint version;
		uint size;
        file.read(fourcc, 4);
        file.read((char*)&version, 4);
        file.read((char*)&size, 4);

		ubyte* blob = (ubyte*) malloc(size);
		file.read((char*)blob, size);

		GDModel::Load(&m_GDModel, blob);
		GDModel::Draw(renderer, device, &m_GDModel); 
	}
    file.close();

	//early out
	//return false;

	IFC(m_AssMan.OpenPkg(filename, &m_Pkg) );
	IFC(m_AssMan.Load(m_Pkg, nodeName));
	IFC(m_AssMan.Get(nodeName, &m_Model));

	defaultFont = renderer->addFont("../Assets/Fonts/Future.dds", "../Assets/Fonts/Future.font", linearClamp);

	m_Model->Init(renderer);

cleanup:
	return SUCCEEDED(r);
}

void App::unload()
{
	// Destructing an Asset pointer will cause it to be evicted if the refcount is 0
	// We have to do this goofily here because the model destructor won't be called until
	//   after the asset manager is shut down. 
	// The memset is to clear the m_AssetPtr attribute to negate the second destructor call
	m_Model.~AssetPtr();
	memset(&m_Model, 0, sizeof(m_Model));

	m_AssMan.ClosePkg(m_Pkg);
}

bool App::onKey(const uint key, const bool pressed)
{
	static int curModel = 0;
	int numAssets = 92;
	const char* nodepath;

	if (pressed && key == KEY_RIGHT)
	{
		do {
			++curModel;
		} while( (m_AssMan.Load(m_Pkg, (curModel % numAssets), 1, &nodepath) != S_OK) );
		
		m_Model.~AssetPtr();
		m_AssMan.Get(nodepath, &m_Model); 
		m_Model->Init(renderer);
	}

	if (pressed && key == KEY_LEFT)
	{
		do {
			if(--curModel == -1) curModel = 92;		
		} while( (m_AssMan.Load(m_Pkg, curModel, 1, &nodepath) != S_OK) );
		
		m_Model.~AssetPtr();
		m_AssMan.Get(nodepath, &m_Model); 
		m_Model->Init(renderer);
	}

	if ( pressed && (KEY_0 <= key && key <= KEY_9) )
	{
		int batchNum = key - KEY_0;
		DEBUG_ONLY(m_Model->_debugDrawBatch = batchNum);
	}

	return BaseApp::onKey(key, pressed);
}

void App::resetCamera(){
	camPos = vec3(0, 100.0f, -200.0f);
	wx = 0.1f;
	wy = 0;
}

void App::drawFrame()
{
	const float fov = 1.0f;

	float4x4 projection = toD3DProjection(perspectiveMatrixY(fov, width, height, 0.1f, 5000));
	float4x4 view = rotateXY(-wx, -wy);
	float4x4 inv_vp_env = !(projection * view);
	view.translate(-camPos);
 	float4x4 view_proj = projection * view;

	float clearColor[4] = {0.5f, 0.1f, 0.2f};
	renderer->clear(true, true, true, clearColor);
	
	renderer->reset();
		renderer->setGlobalConstant4x4f("WorldViewProj", view_proj);
		renderer->setGlobalConstant4f("ambLightColor", float4(0.2f, 0.2f, 0.2f, 1.0f));
	renderer->apply();

	GDModel::Update(&m_GDModel);
	GDModel::Draw(renderer, device, &m_GDModel);

	//m_Model->Draw(renderer, device);
	
	renderer->drawText(m_Model->nodepath, 8, 8, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);
}
