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
		static constexpr char* EXTENSIONS[] = { "gmb","gmc","GMB","GMC" };
		auto load(const std::string& obj, const std::string & extension, std::unordered_map<std::string, std::shared_ptr<maple::Mesh>>&)-> void override;
	};

	DEFINE_CACHE(MeteorSceneObject);
	DEFINE_CACHE(GmcFile);
}