#include "GDAnim.h"
#include <sstream>

uint CompileChannel(Json::Value jointsRoot, char* channelName, GDAnim::KeyIndex* channelIndexSparseArray, std::stringstream& writer)
{
	GDAnim::Key key;
	uint keyCount = 0;
	uint numSubChannels = 3;

	for (uint subChannel = 0; subChannel < numSubChannels; subChannel++)
	{
		GDAnim::KeyIndex* subchannelIndex = &channelIndexSparseArray[subChannel]; 

		for (uint jointIndex = 0; jointIndex < jointsRoot.size(); jointIndex++)
		{
			Json::Value channelNode = jointsRoot[jointIndex][channelName][subChannel];

			subchannelIndex->count = channelNode.size();
			subchannelIndex->index = keyCount;

			for (uint i = 0; i < channelNode.size(); i++)
			{
				key.time =		float(channelNode[i]["Time"].asDouble());
				key.value =		float(channelNode[i]["Value"].asDouble());
				key.tangent =	float(channelNode[i]["Tangent"].asDouble());

				keyCount++;
				writer.write((char*)&key, sizeof(key));
			}
		
			// channelIndexSparseArray is a pointer to an element of Joint Timeline
			// the stride between the channel indices for this channel is thus the size of a JointTimeline
			static const uint channelIndexStride = sizeof(GDAnim::JointTimeline);
			subchannelIndex = (GDAnim::KeyIndex*)((char*)subchannelIndex + channelIndexStride);
		}
	}

	return keyCount;
}

RESULT GDAnim::Compile(const Json::Value& root, Header& hdr, char** data)
{
#	define COMPILER_VERSION 1

	RESULT r = S_OK;
	std::stringstream infoStream;
	std::stringstream dataStream;
	uint dataSize = 0;

	// Disable the debug heap. JsonCpp uses a ton of malloc/free's.
	DEBUG_ONLY(_CrtSetDbgFlag(0));

	AnimAsset anim = {0};
	
	Json::Value jointRoot = root["Anim"]["Joints"];
	uint numJoints = jointRoot.size();
	
	JointTimeline* timelines = (JointTimeline*) malloc(numJoints * sizeof(JointTimeline));
		
	anim.offsetToScales = dataStream.tellp();
	anim.scaleCount += CompileChannel(jointRoot, "Scales", timelines[0].s, dataStream);
	
	anim.offsetToRots = dataStream.tellp();
	anim.rotCount += CompileChannel(jointRoot, "Rotations", timelines[0].r, dataStream);
	
	anim.offsetToTrans = dataStream.tellp();
	anim.transCount += CompileChannel(jointRoot, "Translations", timelines[0].t, dataStream);

	anim.offsetToJoints = dataStream.tellp();
	anim.numJoints = numJoints;
	dataStream.write((char*) timelines, numJoints * sizeof(JointTimeline));
	
	anim.animationLength = root["Anim"]["AnimLength"].asUInt();
		
	infoStream.write((char*)&anim, sizeof(anim));

	// Copy our data to the new buffer
	uint blobSize = infoStream.tellp() + dataStream.tellp();
	*data = (char*) malloc(blobSize);
	infoStream.read(*data, infoStream.tellp());
	dataStream.read((*data + infoStream.tellp()), dataStream.tellp());

	memcpy(hdr.fourCC, "anm1", 4);
	hdr.version = COMPILER_VERSION;
	hdr.sizeBytes = blobSize;

	DEBUG_ONLY(_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF));

	return r;
}

RESULT GDAnim::Load(GDAnim* anim, ubyte* blob)
{
	anim->__asset = (AnimAsset*) blob;
	
	anim->scaleKeys = (Key*)(anim->__asset->data + anim->__asset->offsetToScales);
	anim->rotKeys   = (Key*)(anim->__asset->data + anim->__asset->offsetToRots);
	anim->transKeys = (Key*)(anim->__asset->data + anim->__asset->offsetToTrans);

	anim->jointTimelines = (JointTimeline*)(anim->__asset->data + anim->__asset->offsetToJoints);
	
	return S_OK;
}

//Our backing asset is about to be deleted. Do any necessary cleanup.
RESULT GDAnim::Unload(GDAnim* anim)
{
	memset(anim, 0, sizeof(anim));
	return S_OK;
}

//Initialize our new asset. The asset manager will then delete the old asset.
RESULT GDAnim::Reload(GDAnim* anim, ubyte* blob)
{
	anim->__asset = (AnimAsset*) blob;
	
	anim->scaleKeys = (Key*)(anim->__asset->data + anim->__asset->offsetToScales);
	anim->rotKeys   = (Key*)(anim->__asset->data + anim->__asset->offsetToRots);
	anim->transKeys = (Key*)(anim->__asset->data + anim->__asset->offsetToTrans);

	anim->jointTimelines = (JointTimeline*)(anim->__asset->data + anim->__asset->offsetToJoints);
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
		
	time = fmod(time, anim->__asset->animationLength);

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