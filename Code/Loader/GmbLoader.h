//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Cache.h"
#include "MeteorResources.h"
#include <IO/Loader.h>
/**
 * Meteor Map Loader
 */
namespace meteor 
{
	class GmbLoader : public maple::io::AssetsArchive
	{
	public:
		static constexpr char* EXTENSIONS[] = { "gmb","gmc","GMB","GMC" };
		auto load(const std::string& obj, const std::string & extension, std::vector<std::shared_ptr<maple::IResource>>& out) const -> void override;
	};

	DEFINE_CACHE(MeteorSceneObject);
}