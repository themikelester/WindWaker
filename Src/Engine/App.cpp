
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
#include "GDAnim.h"
#include "BMDRead\bck.h"
#include "BMDRead\bmdread.h"
#include "BMDRead\openfile.h"

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

bool App::load(int argc, char** argv)
{
	RESULT r = S_OK;

	// Parse arguments
	if (argc < 1) { return false; }
	char* filename = argv[0];

	// Load Model
	OpenedFile* file = openFile(filename);
	if(file)
    {
		char fourcc[4];
		fread(fourcc, 1, 4, file->f);
		if (memcmp(fourcc, "J3D", 3) == 0)
		{
			fseek(file->f, 0, SEEK_SET);
			BModel* bdl = loadBmd(file->f);
			GDModel::Load(&m_GDModel, bdl);
			delete bdl;
		}
		else if (memcmp(fourcc, "bmd1", 4) == 0)
		{
			WARN("bmd1 format no longer supported\n");
			closeFile(file);
			return false;
		}
	}
	else
	{
		return false;
	}
	closeFile(file);

	// Load Animation
	animLoaded = false;
	/*filename = "../../Data/Scratch/LkAnm/archive/bcks/walk.bck";
	file = openFile(filename);
	if(file)
	{
		char fourcc[4];
		fread(fourcc, 1, 4, file->f);
		if (memcmp(fourcc, "J3D", 3) == 0)
		{
			fseek(file->f, 0, SEEK_SET);
			Bck* bck = readBck(file->f);
			GDAnim::Load(&m_restAnim, bck);
			delete bck;
		}
		else
		{
			WARN("Unsupported animation format\n");
			return false;
		}
	}
	else
	{
		return false;
	}
	closeFile(file);*/

	// Load Font
	defaultFont = renderer->addFont("../../Data/Fonts/Future.dds", "../../Data/Fonts/Future.font", linearClamp);

cleanup:
	return SUCCEEDED(r);
}

void App::unload()
{
	GDModel::Unload(&m_GDModel);
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

	GDModel::Update(&m_GDModel, animLoaded ? &m_restAnim : nullptr, time*30);
	GDModel::Draw(renderer, &m_GDModel);
}
