//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "Meteor.h"
#include "Engine/Camera.h"
#include "Engine/Profiler.h"
#include "Engine/Mesh.h"

#include "Others/Console.h"
#include "Scene/Entity/Entity.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Scene.h"
#include "ImGui/ImNotification.h"

#include "Loader/Loader.h"

namespace meteor
{
	MeteorDelegate::MeteorDelegate()
	{
	}

	auto MeteorDelegate::onInit() -> void
	{
		maple::Application::getModelLoaderFactory()->addModelLoader<GmbLoader>();

/*
		auto scene = new maple::Scene("MeteorTest");

		auto modelEntity = scene->createEntity("meteor/sn08B01.gmb");
		auto& model = modelEntity.addComponent<maple::component::Model>("meteor/sn08B01.gmb");

		if (model.resource->getMeshes().size() == 1)
		{
			modelEntity.addComponent<maple::component::MeshRenderer>(model.resource->getMeshes().begin()->second);
		}
		else
		{
			for (auto& mesh : model.resource->getMeshes())
			{
				auto child = scene->createEntity(mesh.first);
				child.addComponent<maple::component::MeshRenderer>(mesh.second);
				child.setParent(modelEntity);
			}
		}
		model.type = maple::component::PrimitiveType::File;

	
		maple::ImNotification::makeNotification("Loading...", "Meteor", maple::ImNotification::Type::Info);
		maple::Application::getSceneManager()->addScene("MeteorTest", scene);
		maple::Application::getSceneManager()->switchScene("MeteorTest");*/
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
