
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
#include "GC3D.h"
#include "GDModel.h"

//TODO: Remove HACK
#include <fstream>

BaseApp *app = new App();

bool App::init()
{
	return true;
}

void App::exit()
{
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

	std::ifstream file ("../assets/cl.bdl.blob", std::ios::in|std::ios::binary);
	if(file.is_open())
    {
		char fourcc[4];
		uint version;
		uint size;
        file.read(fourcc, 4);
        file.read((char*)&version, 4);
        file.read((char*)&size, 4);

		m_Blob = (ubyte*) malloc(size);
		file.read((char*)m_Blob, size);

		GDModel::Load(&m_GDModel, m_Blob);
	}
	else
	{
		return false;
	}
    file.close();

	defaultFont = renderer->addFont("../Assets/Fonts/Future.dds", "../Assets/Fonts/Future.font", linearClamp);

cleanup:
	return SUCCEEDED(r);
}

void App::unload()
{
	GDModel::Unload(&m_GDModel);
	free(m_Blob);
}

bool App::onKey(const uint key, const bool pressed)
{
	if (pressed && key == KEY_RIGHT)
	{
	}

	if (pressed && key == KEY_LEFT)
	{
	}

	if ( pressed && (KEY_0 <= key && key <= KEY_9) )
	{
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
	renderer->apply();

	GDModel::Update(&m_GDModel);
	GDModel::Draw(renderer, &m_GDModel);
}
