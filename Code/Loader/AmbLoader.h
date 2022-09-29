//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Cache.h"
#include "MeteorResources.h"
#include <Loaders/Loader.h>

/**
 * Meteor Animation Loader
 */
namespace meteor
{
	class AmbLoader : public maple::io::AssetsArchive
	{
	public:
		static constexpr char* EXTENSIONS[] = { "amb" };
		auto load(const std::string& obj, const std::string& extension, std::vector<std::shared_ptr<maple::IResource>>& out) const -> void override;
		auto save(const std::string &fileName) const -> void override;
	};

	DEFINE_CACHE(MeteorAnimation);
}