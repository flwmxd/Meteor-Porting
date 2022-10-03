//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "Meteor.h"
#include <Engine/Profiler.h>
#include <ImGui/ImNotification.h>
#include <Others/Console.h>

#include "Loader/AmbLoader.h"
#include "Loader/GmbLoader.h"
#include "Loader/SkcLoader.h"

#include "Animation/AnimationSystem.h"
#include "Assets/MeshResource.h"
#include "Devices/Input.h"
#include "Physics/PhysicsWorld.h"
#include "Physics/RigidBody.h"

#include "Scene/Component/MeshRenderer.h"
#include "Scene/Component/Player.h"
#include "Scene/Scene.h"
#include "Scene/System/ExecutePoint.h"

#include "Application.h"

#include <ecs/ecs.h>

namespace meteor
{
	namespace base_update
	{
		// clang-format off
			using Entity = ecs::Registry 
				::Include<maple::component::Player>
				::Modify<maple::physics::component::RigidBody>
				::Modify<maple::animation::component::Animator>
				::Fetch<maple::component::Transform>
				::To<ecs::Entity>;
		// clang-format on

		inline auto system(Entity entity, const maple::physics::global::component::PhysicsWorld &world)
		{
			auto [ridigBody, animator, transform] = entity;
			if (ridigBody.physicsObject)
			{
				auto w = maple::Input::getInput()->isKeyHeld(maple::KeyCode::Id::W);
				ridigBody.physicsObject->setVelocity(w ? transform.getForwardDirection() * -100.f : glm::vec3{0, 0, 0});
			}
		}
	}        // namespace base_update

	MeteorDelegate::MeteorDelegate()
	{
	}

	auto MeteorDelegate::onInit() -> void
	{
		maple::ImNotification::makeNotification("Loading...", "Meteor", maple::ImNotification::Type::Info);
		maple::io::addLoader<GmbLoader>();
		maple::io::addLoader<SkcLoader>();
		maple::io::addLoader<AmbLoader>();

		auto resources = maple::io::load("meteor/level/sn01/sn01.gmb");

		auto scene       = new maple::Scene("sn01");
		auto modelEntity = scene->createEntity("sn01");

		for (auto &res : resources)
		{
			if (res->getResourceType() == maple::FileType::Model)
			{
				for (auto mesh : std::static_pointer_cast<maple::MeshResource>(res)->getMeshes())
				{
					auto  child           = scene->createEntity(mesh.first);
					auto &meshRenderer    = child.addComponent<maple::component::MeshRenderer>();
					meshRenderer.type     = maple::component::PrimitiveType::File;
					meshRenderer.mesh     = mesh.second;
					meshRenderer.meshName = mesh.first;
					auto &body            = child.addComponent<maple::physics::component::RigidBody>();
					body.type             = maple::physics::ColliderType::TriangleMeshCollider;
					body.dynamic          = false;
					child.setParent(modelEntity);
				}
			}
		}
		maple::Application::getSceneManager()->addScene("sn01", scene);
		maple::Application::getSceneManager()->switchScene("sn01");

		maple::Application::getExecutePoint()->registerSystem<base_update::system>();
	}
}        // namespace meteor

maple::Application *createApplication()
{
#ifdef BUILD_EDITOR
	return new maple::Editor(new meteor::MeteorDelegate());
#else
	return new maple::Application(new meteor::MeteorDelegate());
#endif
};
