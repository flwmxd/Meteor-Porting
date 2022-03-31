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

	struct CharacterMaterial
	{
		std::string texture;
		std::string option;
		glm::vec4 colorKey;
		glm::vec3 ambient;
		glm::vec3 diffuse;
		glm::vec3 specular;
		glm::vec3 emissive;
		float opacity;
		bool twoSide;
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

	struct FmcFrame
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
		std::vector<FmcFrame> fmcFrames;
	};

	struct MeteorSceneObject 
	{
		MeteorSceneObject(const std::string& name):name(name) {};
		std::string name;
		DesFile desFile; //SubMesh description;
		GModelFile gmbFile;// MeshObject
		std::shared_ptr<FmcFile> fmcFile;
	};

	//Character Skin
	struct SkcFile
	{
		SkcFile(const std::string& name) :name(name) {};
		std::string name;
		int32_t staticSkins = 0;
		int32_t dynmaicSkins = 0;
		std::string skin;
		std::vector<CharacterMaterial> materials;
	};

	struct MeteorBone 
	{
		std::string name;
		std::string parent;
		glm::vec3 offset;//local 
		glm::quat rotation;//local
		bool dummy;//unknown??
		uint32_t children;
	};

	struct BncFile//skeleton
	{
		BncFile(const std::string& name) :fileName(name) {};
		std::string fileName;

		std::vector<MeteorBone> bones;
		uint32_t boneSize = 0;
		uint32_t dummeySize = 0;
	};

/*
	struct MeteorAnimationClip
	{
		int32_t flag;
		glm::vec3 bonePos;//相对位置,每一帧只有首骨骼有
		std::vector<glm::vec3> dummyPos;//虚拟对象相对位置
		std::vector<glm::quat> boneQuat;//相对旋转.
		std::vector<glm::quat> dummyQuat;//虚拟对象相对旋转
	};*/
}