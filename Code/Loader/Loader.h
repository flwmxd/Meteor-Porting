//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Cache.h"
#include "MeteorResources.h"
#include <Loaders/Loader.h>

#include <vector>
#include <memory>


namespace meteor 
{
	class GmbLoader : public maple::AssetsLoader
	{
	public:
		static constexpr char* EXTENSIONS[] = { "gmb","gmc","GMB","GMC" };
		auto load(const std::string& obj, const std::string & extension, std::vector<std::shared_ptr<maple::IResource>>& out)-> void override;
	};

	DEFINE_CACHE(MeteorSceneObject);
}