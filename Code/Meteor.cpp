//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "Meteor.h"
#include <Engine/Profiler.h>
#include <Others/Console.h>
#include <ImGui/ImNotification.h>

#include "Loader/GmbLoader.h"
#include "Loader/SkcLoader.h"

namespace meteor
{
	MeteorDelegate::MeteorDelegate()
	{
	}

	auto MeteorDelegate::onInit() -> void
	{
		maple::ImNotification::makeNotification("Loading...", "Meteor", maple::ImNotification::Type::Info);
		maple::Application::getAssetsLoaderFactory()->addModelLoader<GmbLoader>();
		maple::Application::getAssetsLoaderFactory()->addModelLoader<SkcLoader>();
	}
}


maple::Application *createApplication()
{
#ifdef BUILD_EDITOR
	return new maple::Editor(new meteor::MeteorDelegate());
#else
	return new maple::Application(new meteor::MeteorDelegate());
#endif
};
