//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace meteor
{
	struct MeteorShader 
	{
		int32_t textureArg0;//textureId
		std::string textureArg1;//NORMAL 
		int32_t twoSideArg0;
		std::string blendArg0;
		std::string blendArg1;
		std::string blendArg2;
		float opaqueArg0;
	};

	//GModel Binary File and GModel Geometry File
	struct GModelFile// gmb and gmc
	{
		int32_t texturesCount = 0;
		int32_t shaderCount = 0;
		int32_t sceneObjectsCount = 0;
		int32_t dummeyObjectsCount = 0;
		int32_t verticesCount = 0;
		int32_t facesCount = 0;
		std::string fileName;
		std::vector<MeteorShader> shaders;
		std::vector<std::string> texturesNames;
	};

	struct DesItem
	{
		std::string name;
		glm::vec3 pos = {};
		glm::quat quat{};
		bool useTextAnimation = false; //是否使用uv动画
		glm::vec2 textAnimation = {}; // uv参数
		std::vector<std::string> custom;
	};

	//GModel Description File
	struct DesFile
	{
		int32_t dummyCount = 0;
		int32_t objectCount = 0;
		std::unordered_map<std::string, DesItem> sceneItems;
	};

	struct FMCFrame
	{
		int32_t frameIdx = 0;
		std::vector<glm::vec3> pos;//pos for every objects
		std::vector<glm::quat> quat;//rotation for every objects
	};

	//# GModel Animation File
	struct FmcFile
	{
		int32_t fps = 0;
		int32_t frames = 0;
		int32_t scemeObjCount = 0;
		int32_t dummyObjCount = 0;
		std::vector<FMCFrame> fmcFrames;
	};

	struct MeteorSceneObject 
	{
		MeteorSceneObject(const std::string& name):name(name) {};
		std::string name;
		DesFile desFile; //SubMesh description;
		GModelFile gmbFile;// MeshObject
		std::shared_ptr<FmcFile> fmcFile;
	};
}