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

		auto animation = std::make_shared<maple::Animation>(fileName);

		std::shared_ptr<maple::AnimationClip> animClip = std::make_shared<maple::AnimationClip>();
		float timer = 0.f;
		animClip->length = length * frames;
		animClip->fps = fps;
		animation->addClip(animClip);

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

				maple::AnimationCurveWrapper* curve = nullptr;

				if (animClip->curves.size() < bone)
				{
					auto& curve0 = animClip->curves.emplace_back();
					curve = &curve0;
					curve0.boneIndex = j;

					auto& cur0 = curve->properties.emplace_back();
					cur0.type = maple::AnimationCurvePropertyType::LocalQuaternionW;
					cur0.curve.addKey(timer, w, 1, 1);

					auto& cur1 = curve->properties.emplace_back();
					cur1.type = maple::AnimationCurvePropertyType::LocalQuaternionX;
					cur1.curve.addKey(timer, xx, 1, 1);

					auto& cur2 = curve->properties.emplace_back();
					cur2.type = maple::AnimationCurvePropertyType::LocalQuaternionY;
					cur2.curve.addKey(timer, yy, 1, 1);

					auto& cur3 = curve->properties.emplace_back();
					cur3.type = maple::AnimationCurvePropertyType::LocalQuaternionZ;
					cur3.curve.addKey(timer, zz, 1, 1);
				}
				else 
				{
					curve = &animClip->curves[j];

					auto& cur0 = curve->properties[0];
					cur0.curve.addKey(timer, w, 1, 1);

					auto& cur1 = curve->properties[1];
					cur1.curve.addKey(timer, xx, 1, 1);

					auto& cur2 = curve->properties[2];
					cur2.curve.addKey(timer, yy, 1, 1);

					auto& cur3 = curve->properties[3];
					cur3.curve.addKey(timer, zz, 1, 1);
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

		out.emplace_back(animation);
	}
}
