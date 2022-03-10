//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "Meteor.h"
#include "Engine/Camera.h"
#include "Engine/Profiler.h"
#include "Others/Console.h"
#include "Scene/Entity/Entity.h"
#include "Scene/Scene.h"
#include "ImGui/ImNotification.h"


MeteorDelegate::MeteorDelegate()
{
}

auto MeteorDelegate::onInit() -> void
{
	maple::ImNotification::makeNotification("Loading...", "Meteor", maple::ImNotification::Type::Info);
}

maple::Application *createApplication()
{
#ifdef BUILD_EDITOR
	return new maple::Editor(new MeteorDelegate());
#else
	return new maple::Application(new MeteorDelegate());
#endif
};
