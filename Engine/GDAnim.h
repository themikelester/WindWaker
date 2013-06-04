#pragma once
#include "Common\common.h"
#include "Compile.h"
#include "Framework3\Math\Vector.h"


namespace GDAnim
{	
	typedef ubyte* AnimAssetPtr;

	struct AnimAsset
	{
		u16 animationLength; //in time units

		u16 numJoints;	//that many animated joints at offsetToJoints
		u16 scaleCount; //that many keys at offsetToScales
		u16 rotCount;   //that many keys at offsetToRots
		u16 transCount; //that many keys at offsetToTrans

		u32 offsetToJoints; // offset (from "data") to array of JointTimeline array
		u32 offsetToScales;	// offset (from "data") to array of scale Keys
		u32 offsetToRots;	// offset (from "data") to array of rotation Keys
		u32 offsetToTrans;	// offset (from "data") to array of translation Keys

		ubyte data[0];
	};

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
		AnimAsset* __asset;
		
		Key* scaleKeys;
		Key* rotKeys;
		Key* transKeys;

		JointTimeline* jointTimelines;
	};

	mat4& GetJoint(GDAnim** anims, float* weights, uint numAnims, uint jointID, float time);

	//Given a JSON object, compile a binary blob that can be loaded with Load() and Reload()
	RESULT Compile(const Json::Value& root, Header& hdr, char** data);

	//Save our asset reference and any other initialization
	RESULT Load(GDAnim* , ubyte* blob);
	
	//Our backing asset is about to be deleted. Do any necessary cleanup.
	RESULT Unload(GDAnim* anim);

	//Unregister our old asset with the renderer. Save our new reference.
	//Initialize our new asset with the renderer. The asset manager will then delete the old asset.
	RESULT Reload(GDAnim* anim, ubyte* blob);
}