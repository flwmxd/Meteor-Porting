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

#define READ_TO(Stream,Name,Value) \
	if (maple::StringUtils::startWith(line, Name)) \
	{	\
		Stream >> Value; \
	}

	namespace 
	{
		inline auto readAttack(std::ifstream& fileIn, Pose& pose) 
		{
			auto & attackInfo = pose.attackInfos.emplace_back();
			std::string line;

			bool bone = false;

			std::string boneValue;

			while (std::getline(fileIn, line))
			{
				maple::StringUtils::trim(line);
				maple::StringUtils::trim(line,"\t");

				if (maple::StringUtils::startWith(line, "{"))
				{
					continue;
				}
				if (maple::StringUtils::endWith(line, "}"))
				{
					break;
				}

				if (maple::StringUtils::startWith(line, "bone"))
				{
					bone = true;
					boneValue = line;
				}
				else 
				{
					if (maple::StringUtils::startWith(line, "\"")) 
					{
						boneValue.append(line);
					}
					else 
					{
						if (bone)
						{
							bone = false;
							//handle bone.
							auto firstValue = boneValue.find_first_of("=");
							boneValue = boneValue.substr(firstValue, boneValue.length() - firstValue);
							LOGI(boneValue);
						}
					}
				}

				if (!bone) 
				{
					std::stringstream sstream(line);
					sstream >> line;
					READ_TO(sstream, "Start", attackInfo.start);
					READ_TO(sstream, "End", attackInfo.end);
					READ_TO(sstream, "AttackType", attackInfo.attackType);
					READ_TO(sstream, "CheckFriend", attackInfo.checkFriend);
					READ_TO(sstream, "DefenseValue", attackInfo.defenseValue);
					READ_TO(sstream, "DefenseMove", attackInfo.defenseMove);
					READ_TO(sstream, "TargetValue", attackInfo.targetValue);
					READ_TO(sstream, "TargetMove", attackInfo.targetMove);
					READ_TO(sstream, "TargetPose", attackInfo.targetPose);
					READ_TO(sstream, "TargetPoseFront", attackInfo.targetPoseFront);
					READ_TO(sstream, "TargetPoseBack", attackInfo.targetPoseBack);
					READ_TO(sstream, "TargetPoseLeft", attackInfo.targetPoseLeft);
					READ_TO(sstream, "TargetPoseRight", attackInfo.targetPoseRight);
				}
			}
		}

		inline auto readPoseAction(std::ifstream& fileIn, Pose& pose, const std::string & name) 
		{
			std::string line;
			auto& action = pose.actions.emplace_back();
			action.type = name == "Blend" ? PoseAction::Type::Blend : PoseAction::Type::Action;
			while (std::getline(fileIn, line))
			{
				if (maple::StringUtils::startWith(line, "{"))
				{
					continue;
				}
				if (maple::StringUtils::endWith(line, "}"))
				{
					break;
				}

				std::stringstream sstream(line);
				sstream >> line;

				if (maple::StringUtils::startWith(line, "Start"))
				{
					sstream >> action.start;
				}

				if (maple::StringUtils::startWith(line, "End"))
				{
					sstream >> action.end;
				}

				if (maple::StringUtils::startWith(line, "speed"))
				{
					sstream >> action.speed;
				}
			}
		}

		inline auto readPose(std::ifstream & fileIn, Pose & pose)
		{
			std::string line;
			fileIn >> line;
			int32_t value = 0;

			maple::StringUtils::trim(line);
			maple::StringUtils::trim(line,"\t");

			READ_TO(fileIn, "source", pose.source);
			READ_TO(fileIn, "Start", pose.start);
			READ_TO(fileIn, "End", pose.end);
			READ_TO(fileIn, "LoopStart", pose.loopStart);
			READ_TO(fileIn, "LoopEnd", pose.loopEnd);
			READ_TO(fileIn, "EffectType", pose.effectType);
			READ_TO(fileIn, "EffectID", pose.effectID);
			READ_TO(fileIn, "link", pose.link);
			READ_TO(fileIn, "link", pose.link);

			if (line == "Blend" || line == "Action")
			{
				readPoseAction(fileIn, pose, line);
			}

			if (line == "Attack") 
			{
				readAttack(fileIn, pose);
			}
		}

		inline auto loadPose(const std::string& fileName)
		{
			std::ifstream fileIn(fileName);
			std::string line;

			std::vector<Pose> poses;

			while (std::getline(fileIn, line))
			{
				maple::StringUtils::trim(line);

				if (maple::StringUtils::endWith(line, "{}"))
				{
					continue;
				}

				if (maple::StringUtils::startWith(line, "{") || maple::StringUtils::startWith(line, "#"))
				{
					continue;
				}
				if (maple::StringUtils::endWith(line, "}"))
				{
					continue;
				}

				if (maple::StringUtils::startWith(line, "Pose"))
				{
					std::stringstream sstream(line);
					int32_t pos;
					sstream >> line >> pos;
					readPose(fileIn, poses.emplace_back());
					continue;
				}
			}

			fileIn.close();
		}
	}

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

		std::vector<MeteorAnimationClip> clips;

		for (int32_t i = 0; i < frames; i++)
		{
			auto & clip = clips.emplace_back();

			auto flag = binaryReader.read<int32_t>();

			if (flag != -1) 
			{
				LOGE("frame: {} flag:{}",i, flag);
			}

			int32_t frameindex = binaryReader.read<int32_t>();

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
				clip.boneQuat.emplace_back(w, xx, yy, zz);
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
				clip.dummyPos.emplace_back(dx, dz, dy);
				clip.dummyQuat.emplace_back(dw, dxx, dyy, dzz);
			}
		}

		auto posFile = maple::StringUtils::removeExtension(fileName) + ".pos";

		loadPose(posFile);
	}
}
