//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#pragma once
#include "Cache.h"
#include "MeteorResources.h"
#include <IO/Loader.h>
/**
 * Meteor Skeleton/Bone/Character Loader
 */
namespace meteor
{
	class SkcLoader : public maple::io::AssetsArchive
	{
	public:
		static constexpr char* EXTENSIONS[] = { "skc" };
		auto load(const std::string& obj, const std::string& extension, std::vector<std::shared_ptr<maple::IResource>>& out) const -> void override;
	};

	DEFINE_CACHE(SkcFile);

	DEFINE_CACHE(BncFile);
}