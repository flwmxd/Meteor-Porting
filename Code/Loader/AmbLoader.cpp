//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////
#include "AmbLoader.h"
#include <Animation/Animation.h>
#include <FileSystem/MeshResource.h>
#include <FileSystem/Skeleton.h>
#include <Others/StringUtils.h>
#include <Others/Console.h>
#include <Engine/Core.h>
#include <Engine/Material.h>
#include <Engine/Profiler.h>
#include <Engine/Buffer.h>

#include <fstream>
#include <sstream>
#include <mio/mmap.hpp>


namespace meteor
{
	auto AmbLoader::load(const std::string& fileName, const std::string& extension, std::vector<std::shared_ptr<maple::IResource>>& out) const -> void
	{
		mio::mmap_source mmap(fileName);
		MAPLE_ASSERT(mmap.is_open(), "open animation file failed");
		maple::BinaryReader binaryReader((const uint8_t*)mmap.data(), mmap.length());
		binaryReader.skip(5);//header -> BANIM 
		
		auto bone = binaryReader.read<int32_t>();
		auto dummy = binaryReader.read<int32_t>();
		auto frames = binaryReader.read<int32_t>();

		auto fps = binaryReader.read<int32_t>();
		auto length = 1.0f / fps;


		auto animation = std::make_shared<maple::Animation>(fileName);
		std::vector<std::shared_ptr<maple::AnimationClip>> clips;

		for (int32_t i = 0; i < frames; i++)
		{
			auto clip = std::make_shared<maple::AnimationClip>();
			clips.emplace_back(clip);

			auto flag = binaryReader.read<int32_t>();

			if (flag != -1) 
			{
				LOGE("frame: {} flag:{}",i, flag);
			}

			int32_t frameindex = binaryReader.read<int32_t>();

			clip->wrapMode = maple::AnimationWrapMode::Loop;
			clip->length = length;//animationDuration;

			MAPLE_ASSERT(i == frameindex, "");

			float x = binaryReader.read<float>();
			float y = binaryReader.read<float>();
			float z = binaryReader.read<float>();
			glm::vec3 bonePos = { x, z, y };//首骨骼的相对坐标.
			
			for (int32_t j = 0; j < bone; j++)
			{
				float w = binaryReader.read<float>();
				float xx = - binaryReader.read<float>();
				float zz = - binaryReader.read<float>();
				float yy = - binaryReader.read<float>();
				//clip.boneQuat.emplace_back();
				glm::quat rotation = { w,xx,yy,zz };

				auto euler = glm::eulerAngles(rotation);

				auto& curve0 = clip->curves.emplace_back();
				curve0.boneIndex = j;
				auto& cur0 = curve0.properties.emplace_back();
				cur0.type = maple::AnimationCurvePropertyType::LocalRotationX;
				
				cur0.curve.addKey(length, euler.x , 1, 1);

				auto& curve1 = clip->curves.emplace_back();
				curve1.boneIndex = j;
				auto& cur1 = curve1.properties.emplace_back();
				cur1.type = maple::AnimationCurvePropertyType::LocalRotationY;
				cur1.curve.addKey(length, euler.y, 1, 1);


				auto& curve2 = clip->curves.emplace_back();
				curve2.boneIndex = j;
				auto& cur2 = curve2.properties.emplace_back();
				cur2.type = maple::AnimationCurvePropertyType::LocalRotationZ;
				cur2.curve.addKey(length, euler.z, 1, 1);
			}

			for (int32_t k = 0; k < dummy; k++)
			{
				binaryReader.skip(5);
				float dx = binaryReader.read<float>();
				float dy = binaryReader.read<float>();
				float dz = binaryReader.read<float>();
				float dw = binaryReader.read<float>();
				float dxx = -binaryReader.read<float>();
				float dzz = -binaryReader.read<float>();
				float dyy = -binaryReader.read<float>();
				//clip.dummyPos.emplace_back(dx, dz, dy);
				//clip.dummyQuat.emplace_back(dw,dxx, dyy, dzz);
			}
		}
		animation->setClips(clips);
		out.emplace_back(animation);
	}
}
