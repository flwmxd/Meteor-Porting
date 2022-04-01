//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "MeteorResources.h"

namespace meteor
{
	class BakedPose
	{
	public:
		BakedPose(const Pose& pose,int32_t fps);
		auto getFrame(float t)->int32_t;
	private:
		std::vector<float> keyFrames;
		std::pair<float, float> cache;
		std::pair<float, float> loopCache;
	};
};
