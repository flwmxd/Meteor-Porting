//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Cache.h"
#include "MeteorResources.h"
#include <FileSystem/MeshLoader.h>

#include <vector>
#include <memory>


namespace meteor 
{
	class GmbLoader : public maple::ModelLoader
	{
	public:
		static constexpr char* EXTENSIONS[] = { "gmb" };
		auto load(const std::string& obj, std::unordered_map<std::string, std::shared_ptr<maple::Mesh>>&)-> void override;
	};

	struct GmbFile 
	{
	public:
		int32_t texturesCount = 0;
		int32_t shaderCount = 0;
		int32_t sceneObjectsCount = 0;
		int32_t dummeyObjectsCount = 0;
		int32_t verticesCount = 0;
		int32_t facesCount = 0;
		std::string fileName;
		std::vector<MeteorShader> shaders;
		std::vector<std::string> texturesNames;
		std::vector<std::shared_ptr<maple::Mesh>> meshes;
	};
}