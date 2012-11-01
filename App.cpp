
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

BaseApp *app = new App();

bool App::init()
{
	memMan.init();

	return true;
}

void App::exit()
{
	memMan.shutdown();
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
	m_Model = new GCModel;

	IFC( assMan.OpenPkg(filename, &m_Pkg) );
	IFC( assMan.Load(m_Pkg, nodeName, m_Model) );
	
	m_VerticesShader = renderer->addShader("../Vertices.shd");
	
	//TODO: Should only be done once when the renderer is init
	FormatDesc GCformat[] =
	{
		{ 0, TYPE_VERTEX,   FORMAT_FLOAT, 3 },
		{ 0, TYPE_NORMAL,   FORMAT_FLOAT, 3 },
	};

	m_GCVertFormat = renderer->addVertexFormat(GCformat, elementsOf(GCformat), m_VerticesShader); 
	if ( m_GCVertFormat == VF_NONE) 
			return false;

	void* test = Mem::defaultAllocator->Alloc(16);

	defaultFont = renderer->addFont("../Fonts/Future.dds", "../Fonts/Future.font", linearClamp);

	m_Model->Init(renderer, m_GCVertFormat);

	Mem::defaultAllocator->Free(test);

cleanup:
	return true;
}

void App::unload()
{
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
		} while( (assMan.Load(m_Pkg, (curModel % numAssets), (Asset**)&m_Model) != S_OK) );
		
		m_Model->Init(renderer, m_GCVertFormat);
	}

	if (pressed && key == KEY_LEFT)
	{
		do {
			if(--curModel == -1) curModel = 92;		
		} while( (assMan.Load(m_Pkg, curModel, (Asset**)&m_Model) != S_OK) );
		m_Model->Init(renderer, m_GCVertFormat);
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
	renderer->setShader(m_VerticesShader);
	renderer->setShaderConstant4x4f("WorldViewProj", view_proj);
	renderer->apply();

	m_Model->Draw(renderer, device);
	
	renderer->drawText(m_Model->nodepath, 8, 8, 30, 38, defaultFont, linearClamp, blendSrcAlpha, noDepthTest);
}
