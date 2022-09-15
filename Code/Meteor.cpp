//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "Meteor.h"
#include <Engine/Profiler.h>
#include <Others/Console.h>
#include <ImGui/ImNotification.h>

#include "Loader/GmbLoader.h"
#include "Loader/SkcLoader.h"
#include "Loader/AmbLoader.h"

namespace meteor
{
	MeteorDelegate::MeteorDelegate()
	{
	}

	auto MeteorDelegate::onInit() -> void
	{
		maple::ImNotification::makeNotification("Loading...", "Meteor", maple::ImNotification::Type::Info);
		maple::loaders::addLoader<GmbLoader>();
		maple::loaders::addLoader<SkcLoader>();
		maple::loaders::addLoader<AmbLoader>();
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
