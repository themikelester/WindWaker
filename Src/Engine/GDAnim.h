#pragma once
#include "Common\common.h"
#include "Framework3\Math\Vector.h"

struct Bck;

namespace GDAnim
{	
	struct Key
	{
		float time;
		float value;
		float tangent;
	};

	struct KeyIndex
	{
	  u16 count; // number of keys
	  u16 index; // index into key array (either rotation, scale, or translation)
	};

	struct JointTimeline
	{
	  KeyIndex s[3]; //scales
	  KeyIndex r[3]; //rotations
	  KeyIndex t[3]; //translations
	};

	struct GDAnim
	{
		Key* scaleKeys;
		Key* rotKeys;
		Key* transKeys;

		u16 animLength; //in time units
		JointTimeline* jointTimelines;
	};

	mat4& GetJoint(GDAnim** anims, float* weights, uint numAnims, uint jointID, float time);

	//Save our asset reference and any other initialization
	RESULT Load(GDAnim* anim, const Bck* bck);
	
	//Our backing asset is about to be deleted. Do any necessary cleanup.
	RESULT Unload(GDAnim* anim);

	//Unregister our old asset with the renderer. Save our new reference.
	//Initialize our new asset with the renderer. The asset manager will then delete the old asset.
	RESULT Reload(GDAnim* anim, ubyte* blob);
}