//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////
#include "AmbLoader.h"
#include <Assets/MeshResource.h>
#include <Others/StringUtils.h>
#include <Others/Console.h>
#include <Engine/Core.h>
#include <Engine/Material.h>
#include <Engine/Profiler.h>
#include <Engine/Buffer.h>
#include <Animation/Animation.h>

#include <fstream>
#include <sstream>
#include <mio/mmap.hpp>


namespace meteor
{

#define READ_TO(Stream,Name,Value) \
	if (maple::StringUtils::startWith(line, Name,true)) \
	{	\
		Stream >> Value; \
	}

	namespace 
	{
		inline auto readDrag(std::ifstream& fileIn, Pose& pose)
		{
			std::string line;
			while (std::getline(fileIn, line))
			{
				maple::StringUtils::trim(line);
				maple::StringUtils::trim(line, "\t");
				if (maple::StringUtils::startWith(line, "{"))
				{
					continue;
				}
				if (maple::StringUtils::endWith(line, "}"))
				{
					break;
				}

				if (maple::StringUtils::startWith(line, "Color",true))
				{
					int32_t r, g, b;
					sscanf(line.c_str(), "Color  %d,%d,%d", &r, &g, &b);
					pose.drag.color = {r/255.f,g/255.f,b/255.f};
					continue;
				}

				std::stringstream sstream(line);
				sstream >> line;
				READ_TO(sstream, "Start", pose.drag.start);
				READ_TO(sstream, "End", pose.drag.end);
				READ_TO(sstream, "Time", pose.drag.time);
			}
		}

		inline auto readNextPose(std::ifstream& fileIn, Pose& pose)
		{
			std::string line;
			while (std::getline(fileIn, line))
			{
				maple::StringUtils::trim(line);
				maple::StringUtils::trim(line, "\t");
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
				READ_TO(sstream, "start", pose.nextPose.start);
				READ_TO(sstream, "end", pose.nextPose.end);
				READ_TO(sstream, "Time", pose.nextPose.time);
			}
		}

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
							maple::StringUtils::trim(boneValue);
							maple::StringUtils::trim(boneValue,"=");
							maple::StringUtils::trim(boneValue);
							auto results = maple::StringUtils::split(boneValue, ",");

							for (auto str : results)
							{
								maple::StringUtils::trim(str);
								maple::StringUtils::trim(str, "\"");
								attackInfo.bones.emplace_back(str);
							}
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
				maple::StringUtils::trim(line);
				maple::StringUtils::trim(line, "\t");

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

				READ_TO(sstream, "Start", action.start);
				READ_TO(sstream, "End", action.end);
				READ_TO(sstream, "speed", action.speed);
			}
		}

		inline auto readPose(std::ifstream& fileIn, std::string line, Pose & pose)
		{

			maple::StringUtils::trim(line);
			maple::StringUtils::trim(line, "\t");

			std::stringstream sstream(line);
			sstream >> line;

			READ_TO(sstream, "source", pose.source);
			READ_TO(sstream, "Start", pose.start);
			READ_TO(sstream, "End", pose.end);
			READ_TO(sstream, "LoopStart", pose.loopStart);
			READ_TO(sstream, "LoopEnd", pose.loopEnd);
			READ_TO(sstream, "EffectType", pose.effectType);
			READ_TO(sstream, "EffectID", pose.effectID);
			READ_TO(sstream, "link", pose.link);

			if (line == "Blend" || line == "Action")
			{
				readPoseAction(fileIn, pose, line);
			}

			if (line == "Attack") 
			{
				readAttack(fileIn, pose);
			}

			if (line == "Drag")
			{
				readDrag(fileIn, pose);
			}

			if (line == "NextPose")
			{
				readNextPose(fileIn, pose);
			}
		}

		inline auto loadPose(const std::string& fileName, std::vector<Pose> & poses)
		{
			std::ifstream fileIn(fileName);
			std::string line;

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
					auto& back = poses.emplace_back();
					continue;
				}

				readPose(fileIn, line, poses.back());
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

		auto& meteorAnimation = MeteorAnimationCache::get(fileName);
		ozz::animation::offline::RawAnimation rawAnimation;

		float timer = 0.f;
		rawAnimation.duration = length * frames;

		for (int32_t i = 0; i < frames; i++)
		{
			auto & clip = meteorAnimation.clips.emplace_back();
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

				if (rawAnimation.num_tracks() < bone)
				{
					auto& curve0 = rawAnimation.tracks.emplace_back();
					auto& rotation = curve0.rotations.emplace_back();
					rotation.time = timer;
					rotation.value = {xx,yy,zz,w};
				}
				else 
				{
					auto& rotation = rawAnimation.tracks[j].rotations.emplace_back();
					rotation.time = timer;
					rotation.value = {xx,yy,zz,w};
				}
			}

			timer += length;
			
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

		loadPose(posFile, meteorAnimation.poses);
		auto animation = std::make_shared<maple::animation::Animation>(rawAnimation);
	}
}
