//////////////////////////////////////////////////////////////////////////////
// This file is part of the Meteor-Remake                             		//
//////////////////////////////////////////////////////////////////////////////

#include "Meteor.h"
#include "Animation/AnimationSystem.h"
#include "Application.h"
#include "Assets/MeshResource.h"
#include "Devices/Input.h"
#include "Engine/Camera.h"
#include "Engine/Material.h"
#include "Engine/Renderer/RendererData.h"
#include "Engine/VirtualGeometry/VirtualGeometry.h"
#include "Loader/AmbLoader.h"
#include "Loader/GmbLoader.h"
#include "Loader/SkcLoader.h"
#include "Physics/PhysicsWorld.h"
#include "Physics/RigidBody.h"
#include "Scene/Component/MeshRenderer.h"
#include "Scene/Scene.h"
#include "Scene/System/ExecutePoint.h"
#include <Engine/Profiler.h>
#include <ImGui/ImNotification.h>
#include <Others/Console.h>
#include <ecs/ecs.h>

namespace meteor
{
	//namespace test
	//{
	//	// clang-format off
	//	using Entity = ecs::Registry
	//		::Read<maple::virtual_geometry::component::VirtualGeometryRenderer>
	//		::Read<maple::component::Transform>
	//		::To<ecs::Entity>;
	//	// clang-format on

	//	inline auto system(Entity entity, const maple::component::CameraView &cameraView, const maple::component::WindowSize &winSize)
	//	{
	//		auto [render, transform] = entity;
	//		if (cameraView.cameraTransform != nullptr)
	//		{
	//			const float cotHalfFov = 1.0f / glm::tan(glm::radians(cameraView.fov) / 2.0f);

	//			auto viewPos = cameraView.view * glm::vec4(transform.getWorldPosition(), 1.f);
	//			viewPos /= viewPos.w;
	//			auto d2 = glm::length(glm::vec3(viewPos));
	//			d2 *= d2;

	//			auto trans = cameraView.view * transform.getWorldMatrix();

	//			float     error = 0.0003f;
	//			glm::vec4 aaaa  = trans * glm::vec4{error, 0, 0, 0};
	//			float     r     = glm::length(glm::vec3{aaaa});

	//			float radius = winSize.height / 2.0f * cotHalfFov * r / glm::sqrt(d2 - r * r);

	//			LOGI("radius : {} ---- error on screen : {},  error : {}", radius, r, error);
	//		}
	//	}
	//}        // namespace test

	class MeteorDelegate : public maple::AppDelegate
	{
	  public:
		auto onInit() -> void override
		{
		/*	maple::ImNotification::makeNotification("Loading...", "Meteor", maple::ImNotification::Type::Info);
			maple::io::addLoader<GmbLoader>();
			maple::io::addLoader<SkcLoader>();
			maple::io::addLoader<AmbLoader>();

			auto resources = maple::io::load("virtualGeo/yodatrump_.vgeo");

			auto    scene = new maple::Scene("sn01");
			int32_t kkk   = 0;

			for (uint32_t i = 0; i < 1; i++)
			{
				for (uint32_t j = 0; j < 1; j++)
				{
					for (uint32_t k = 0; k < 1; k++)
					{
						// auto  mesh         = std::static_pointer_cast<maple::virtual_geometry::VirtualGeometry>(kkk % 2 == 0 ? resources2[0] : resources[0]);
						auto  mesh         = std::static_pointer_cast<maple::virtual_geometry::VirtualGeometry>(resources[0]);
						auto  child        = scene->createEntity("Test" + std::to_string(kkk++));
						auto &meshRenderer = child.addComponent<maple::virtual_geometry::component::VirtualGeometryRenderer>();
						auto &transform    = child.getComponent<maple::component::Transform>();

						transform.setLocalPosition({i * 20.0f, k * 25.0f, j * 25.0f});
						transform.setLocalOrientation(glm::vec3{-90, 0, 0});
						transform.setWorldMatrix(glm::mat4(1.f));

						meshRenderer.filePath = mesh->getPath();
						meshRenderer.mesh     = mesh;
						// meshRenderer.mesh->setMaterial(material);
					}
				}
			}
			maple::Application::getSceneManager()->addScene("sn01", scene);
			maple::Application::getSceneManager()->switchScene("sn01");*/
	/*		maple::Application::get()->getExecutePoint()->registerSystem<test::system>();*/
		}

		virtual auto onDestory() -> void override{};
	};

	using namespace maple;

	class Game : public Application
	{
	  public:
		Game() :
		    Application(new MeteorDelegate())
		{}
		auto init() -> void override
		{
			Application::init();
			if (File::fileExists("default.scene"))
			{
				sceneManager->addSceneFromFile("default.scene");
				sceneManager->switchScene("default.scene");
			}

			auto winSize = window->getWidth() / (float) window->getHeight();

			camera = std::make_unique<Camera>(
			    60, 0.1, 3000, winSize);
			editorCameraController.setCamera(camera.get());
			setEditorState(maple::EditorState::Play);
		};

		auto onUpdate(const Timestep &delta) -> void override
		{
			Application::onUpdate(delta);
			editorCameraTransform.setWorldMatrix(glm::mat4(1.f));
			const auto mousePos = Input::getInput()->getMousePosition();
			editorCameraController.handleMouse(editorCameraTransform, delta, mousePos.x, mousePos.y);
			editorCameraController.handleKeyboard(editorCameraTransform, delta);
			if (auto currentScene = getSceneManager()->getCurrentScene())
			{
				currentScene->setOverrideCamera(camera.get());
				currentScene->setOverrideTransform(&editorCameraTransform);
			}
		}

	  private:
		std::unique_ptr<Camera> camera;
		component::Transform    editorCameraTransform;
		EditorCameraController  editorCameraController;
	};

}        // namespace meteor

maple::Application *createApplication()
{
#ifdef BUILD_EDITOR
	return new maple::Editor(new meteor::MeteorDelegate());
	//return new meteor::Game();
#else
	return new maple::Application(new meteor::MeteorDelegate());
#endif
};
