
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

BaseApp *app = new App();

bool App::init()
{
	foundation::memory_globals::init();
	memMan.init();
	assMan.Init();

	return true;
}

void App::exit()
{
	memMan.shutdown();
	assMan.Shutdown();
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
	RESULT r;

	char* filename = "C:\\Users\\Michael\\Dropbox\\Code\\Wind Waker Assets\\Link.rarc";
	char* nodeName = "/bdl/bow.bdl";

	IFC( assMan.OpenPkg(filename, &m_Pkg) );
	IFC(assMan.Load(m_Pkg, nodeName));
	IFC(assMan.Get(nodeName, &m_Model)); 

	void* test = Mem::defaultAllocator->Alloc(16);

	defaultFont = renderer->addFont("../Fonts/Future.dds", "../Fonts/Future.font", linearClamp);

	m_Model->Init(renderer);

	Mem::defaultAllocator->Free(test);

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

	assMan.ClosePkg(m_Pkg);
}

bool App::onKey(const uint key, const bool pressed)
{
	static int curModel = 0;
	int numAssets = 92;

	if (pressed && key == KEY_RIGHT)
	{
		do {
			++curModel;
		} while( (assMan.Load(m_Pkg, (curModel % numAssets)) != S_OK) );

		m_Model->Init(renderer);
	}

	if (pressed && key == KEY_LEFT)
	{
		do {
			if(--curModel == -1) curModel = 92;		
		} while( (assMan.Load(m_Pkg, curModel) != S_OK) );
		m_Model->Init(renderer);
	}

	return BaseApp::onKey(key, pressed);
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
		renderer->setGlobalConstant1f("anotherConstantInSeperateCBuffer", 1.0f);
		renderer->setGlobalConstant4f("globalColor", float4(0, 1, 1, 1));
	renderer->apply();

	m_Model->Draw(renderer, device);
	
	renderer->drawText(m_Model->nodepath, 8, 8, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);
}
