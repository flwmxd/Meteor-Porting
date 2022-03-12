//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>

namespace meteor
{
	struct MeteorShader 
	{
		int32_t textureArg0;
		std::string textureArg1;
		int32_t twoSideArg0;
		std::string blendArg0;
		std::string blendArg1;
		std::string blendArg2;
		float opaqueArg0;
	};
}