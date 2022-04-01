//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "MeteorPose.h"

namespace meteor 
{
	BakedPose::BakedPose(const Pose& pose, int32_t fps)
	{
		float speedScale = 1.0f;
		int32_t prev = pose.start;
		float time = 0.0f;
		for (int32_t i = pose.start; i <= pose.end; i++)
		{
			speedScale = 1.0f;
			for (int32_t j = 0; j < pose.actions.size(); j++)
			{
				if (i >= pose.actions[j].start && i <= pose.actions[j].end)
				{
					speedScale = (pose.actions[j].speed == 0.0f ? 1.0f : pose.actions[j].speed);
					break;
				}
			}
			time += (i - prev) / ( speedScale * fps );
			prev = i;
			keyFrames.emplace_back(time);
		}

		cache = { keyFrames[0], keyFrames[keyFrames.size() - 1] };

		if (pose.loopStart != 0 && pose.loopEnd != 0)
			loopCache = { keyFrames[pose.loopStart - pose.start], keyFrames[pose.loopEnd - pose.end] };
	}

	auto BakedPose::getFrame(float t) ->int32_t
	{
		int32_t ret = 0;
		for (int32_t i = 0; i < keyFrames.size(); i++) {
			if (keyFrames[i] < t)
				ret = i;
			else
				break;
		}
		return ret;
	}
}

