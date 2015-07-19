#include "GDAnim.h"
#include "BMDRead\bck.h"

RESULT GDAnim::Load(GDAnim* anim, const Bck* bck)
{
	u32 jointCount = bck->anims.size();
	anim->animLength = bck->animationLength;

	// jnt0.sx[0], jnt0.sx[1], ..., jnt1.sx[0], jnt1.sx[1], ... jnt0.sy[0], jnt0.sy[1], ...

	const u32 kMaxKeysPerChannel = 1024;
	anim->scaleKeys = (Key*)malloc(sizeof(Key) * kMaxKeysPerChannel);
	anim->transKeys = (Key*)malloc(sizeof(Key) * kMaxKeysPerChannel);
	anim->rotKeys = (Key*)malloc(sizeof(Key) * kMaxKeysPerChannel);
	anim->jointTimelines = (JointTimeline*)malloc(sizeof(JointTimeline) * jointCount);

	// Scale
	u32 keyOffset = 0;
	for (u32 i = 0; i < jointCount; i++)
	{
		JointTimeline& jt = anim->jointTimelines[i];

		jt.s[0].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].scalesX.size(); j++)
		{
			anim->scaleKeys[keyOffset++] = (Key&)bck->anims[i].scalesX[j];
		}
		jt.s[0].count = keyOffset - jt.s[0].index;

		jt.s[1].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].scalesY.size(); j++)
		{
			anim->scaleKeys[keyOffset++] = (Key&)bck->anims[i].scalesY[j];
		}
		jt.s[1].count = keyOffset - jt.s[1].index;

		jt.s[2].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].scalesZ.size(); j++)
		{
			anim->scaleKeys[keyOffset++] = (Key&)bck->anims[i].scalesZ[j];
		}
		jt.s[2].count = keyOffset - jt.s[2].index;
	}

	// Translation
	keyOffset = 0;
	for (u32 i = 0; i < jointCount; i++)
	{
		JointTimeline& jt = anim->jointTimelines[i];

		jt.t[0].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].translationsX.size(); j++)
		{
			anim->transKeys[keyOffset++] = (Key&)bck->anims[i].translationsX[j];
		}
		jt.t[0].count = keyOffset - jt.t[0].index;

		jt.t[1].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].translationsY.size(); j++)
		{
			anim->transKeys[keyOffset++] = (Key&)bck->anims[i].translationsY[j];
		}
		jt.t[1].count = keyOffset - jt.t[1].index;

		jt.t[2].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].translationsZ.size(); j++)
		{
			anim->transKeys[keyOffset++] = (Key&)bck->anims[i].translationsZ[j];
		}
		jt.t[2].count = keyOffset - jt.t[2].index;
	}

	// Rotation
	keyOffset = 0;
	for (u32 i = 0; i < jointCount; i++)
	{
		JointTimeline& jt = anim->jointTimelines[i];

		jt.r[0].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].rotationsX.size(); j++)
		{
			anim->rotKeys[keyOffset++] = (Key&)bck->anims[i].rotationsX[j];
		}
		jt.r[0].count = keyOffset - jt.r[0].index;

		jt.r[1].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].rotationsY.size(); j++)
		{
			anim->rotKeys[keyOffset++] = (Key&)bck->anims[i].rotationsY[j];
		}
		jt.r[1].count = keyOffset - jt.r[1].index;

		jt.r[2].index = keyOffset;
		for (u32 j = 0; j < bck->anims[i].rotationsZ.size(); j++)
		{
			anim->rotKeys[keyOffset++] = (Key&)bck->anims[i].rotationsZ[j];
		}
		jt.r[2].count = keyOffset - jt.r[2].index;
	}
	
	return S_OK;
}

//Our backing asset is about to be deleted. Do any necessary cleanup.
RESULT GDAnim::Unload(GDAnim* anim)
{
	free(anim->jointTimelines);
	free(anim->rotKeys);
	free(anim->scaleKeys);
	free(anim->transKeys);
	memset(anim, 0, sizeof(anim));
	return S_OK;
}

//Initialize our new asset. The asset manager will then delete the old asset.
RESULT GDAnim::Reload(GDAnim* anim, ubyte* blob)
{
	return S_OK;
}

template<class T>
T interpolate(T v1, T d1, T v2, T d2, T t) //t in [0, 1]
{
  //linear interpolation
  //return v1 + t*(v2 - v1);

  //cubic interpolation
  float a = 2*(v1 - v2) + d1 + d2;
  float b = -3*v1 + 3*v2 - 2*d1 - d2;
  float c = d1;
  float d = v1;
  //TODO: yoshi_walk.bck has strange-looking legs...not sure if
  //the following line is to blame, though
  return ((a*t + b)*t + c)*t + d;
}

float getAnimValue(GDAnim::Key* keyData, GDAnim::KeyIndex keyIndex, float t)
{
	if(keyIndex.count == 0)
		return 0.f;

	if(keyIndex.count == 1)
		return keyData[keyIndex.index].value;

	keyData += keyIndex.index;

	int i = 1;
	while(keyData[i].time < t)
	++i;

	float time = (t - keyData[i - 1].time)/(keyData[i].time - keyData[i - 1].time); //scale to [0, 1]
	return interpolate(keyData[i - 1].value, keyData[i - 1].tangent, keyData[i].value, keyData[i].tangent, time);
}

mat4& GDAnim::GetJoint(GDAnim** anims, float* weights, uint numAnims, uint jointID, float time)
{
	GDAnim* anim = anims[0];
		
	time = fmod(time, anim->animLength);

	vec3 vscale;
	vec3 rot;
	vec3 trans;
	
	vscale[0] = getAnimValue(anim->scaleKeys, anim->jointTimelines[jointID].s[0], time);
	vscale[1] = getAnimValue(anim->scaleKeys, anim->jointTimelines[jointID].s[1], time);
	vscale[2] = getAnimValue(anim->scaleKeys, anim->jointTimelines[jointID].s[2], time);
	
	rot[0] = getAnimValue(anim->rotKeys, anim->jointTimelines[jointID].r[0], time);
	rot[1] = getAnimValue(anim->rotKeys, anim->jointTimelines[jointID].r[1], time);
	rot[2] = getAnimValue(anim->rotKeys, anim->jointTimelines[jointID].r[2], time);
	
	trans[0] = getAnimValue(anim->transKeys, anim->jointTimelines[jointID].t[0], time);
	trans[1] = getAnimValue(anim->transKeys, anim->jointTimelines[jointID].t[1], time);
	trans[2] = getAnimValue(anim->transKeys, anim->jointTimelines[jointID].t[2], time);

	// TODO: Super slow! Fix this! Use SQT's probably
	mat4 t, rx, ry, rz, s;
	t = translate(trans);
	rx = rotateX(rot.x/360.f*2*PI);
	ry = rotateY(rot.y/360.f*2*PI);
	rz = rotateZ(rot.z/360.f*2*PI);
	s = scale(vscale.x, vscale.y, vscale.z);

	return t*rz*ry*rx;
}