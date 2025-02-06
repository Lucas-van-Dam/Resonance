#include "reonpch.h"
#include "GameLogicLayer.h"

#include "REON/GameHierarchy/SceneManager.h"
#include "REON/Rendering/Model.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "REON/GameHierarchy/Components/Light.h"
#include "REON/Application.h"

#include "REON/KeyCodes.h"
#include "REON/MouseButtonCodes.h"
#include <GLFW/glfw3.h>

namespace REON {

	GameLogicLayer::GameLogicLayer()
	{
	}

	GameLogicLayer::~GameLogicLayer()
	{

	}

	void GameLogicLayer::OnDetach()
	{
		EventBus::Get().unsubscribe<KeyPressedEvent>(m_KeyPressedCallbackID);

	}

	void GameLogicLayer::OnAttach()
	{
		m_KeyPressedCallbackID = EventBus::Get().subscribe<KeyPressedEvent>(REON_BIND_EVENT_FN(GameLogicLayer::OnKeyPressed));
		InitializeTestScene();
		lastTime = std::chrono::high_resolution_clock::now();
	}

	void GameLogicLayer::OnUpdate()
	{
		CheckKeyPressed();
		SceneManager::Get()->GetCurrentScene()->UpdateScene(deltaTime);
		auto currentTime = std::chrono::high_resolution_clock::now();
		deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
		lastTime = currentTime;
	}

	void GameLogicLayer::OnImGuiRender()
	{

	}

	void GameLogicLayer::InitializeTestScene()
	{
		char path[] = "Assets/Models/nissan_gt-r_r35_gt_v2/scene.gltf";

		// PBR_Textures_ORCA

		//std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;

		std::shared_ptr<GameObject> backPack = std::make_shared<GameObject>();
		backPack->SetName("Television");

		std::shared_ptr<Scene> m_Scene = SceneManager::Get()->GetCurrentScene();

		m_Scene->AddGameObject(backPack);
		Model::LoadModelToGameObject(path, backPack);



		//    std::shared_ptr<GameObject> cube = std::make_shared<GameObject>();
		//    scene->AddGameObject(cube);
		//    Model::LoadModelToGameObject(path, cube);
		////
		//    cube->GetTransform()->localPosition = glm::vec3(0.0f, 7.5f, 0.0f);
		//    cube->GetTransform()->localScale = glm::vec3(0.5f, 0.5f, 0.5f);
		//    cube->GetTransform()->localRotation.setFromEulerAngles(0.0f, 12.5f, 0.0f);

		backPack->GetTransform()->localPosition = glm::vec3(0.0f, 0.0f, 0.0f);
		//backPack->GetTransform()->localScale = glm::vec3(0.01f, 0.01f, 0.01f);

		std::shared_ptr<GameObject> light = std::make_shared<GameObject>();
		std::shared_ptr<Light> lightComponent = std::make_shared<Light>(LightType::Point, 10, glm::vec3(30.0f, 0.5f, 0.5f));
		m_Scene->AddGameObject(light);
		light->AddComponent<Light>(lightComponent);
		light->GetTransform()->localPosition = glm::vec3(-7, 3, 3);
		light->SetName("light1");
		m_Scene->lightManager->AddLight(lightComponent);

		std::shared_ptr<GameObject> light2 = std::make_shared<GameObject>();
		std::shared_ptr<Light> lightComponent2 = std::make_shared<Light>(LightType::Point, 3, glm::vec3(1.0f, 1.0f, 1.0f));
		m_Scene->AddGameObject(light2);
		light2->AddComponent<Light>(lightComponent2);
		light2->GetTransform()->localPosition = glm::vec3(-5, 4, 3);
		light2->SetName("light2");
		m_Scene->lightManager->AddLight(lightComponent2);

		std::shared_ptr<GameObject> light3 = std::make_shared<GameObject>();
		std::shared_ptr<Light> lightComponent4 = std::make_shared<Light>(LightType::Directional, 3, glm::vec3(1, 1, 1));
		m_Scene->AddGameObject(light3);
		light3->AddComponent<Light>(lightComponent4);
		light3->GetTransform()->localRotation.setFromEulerAngles(110, 0, 0);
		light3->SetName("light3");
		m_Scene->lightManager->AddLight(lightComponent4);
	}

	void GameLogicLayer::CheckKeyPressed()
	{
		if (!Input::IsMouseButtonPressed(REON_MOUSE_BUTTON_2))
			return;
		auto camera = SceneManager::Get()->GetCurrentScene()->GetEditorCamera();

		if (Input::IsKeyPressed(REON_KEY_W)) {
			camera->ProcessKeyboard(FORWARD, deltaTime);
		}
		if (Input::IsKeyPressed(REON_KEY_D)) {
			camera->ProcessKeyboard(RIGHT, deltaTime);
		}
		if (Input::IsKeyPressed(REON_KEY_S)) {
			camera->ProcessKeyboard(BACKWARD, deltaTime);
		}
		if (Input::IsKeyPressed(REON_KEY_A)) {
			camera->ProcessKeyboard(LEFT, deltaTime);
		}
		if (Input::IsKeyPressed(REON_KEY_E)) {
			camera->ProcessKeyboard(UP, deltaTime);
		}
		if (Input::IsKeyPressed(REON_KEY_Q)) {
			camera->ProcessKeyboard(DOWN, deltaTime);
		}
		
		camera->ProcessShiftKey(Input::IsKeyPressed(REON_KEY_LEFT_SHIFT));
	}

	void GameLogicLayer::OnKeyPressed(const KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == REON_KEY_DELETE && event.GetRepeatCount() == 0) {
			auto obj = SceneManager::Get()->GetCurrentScene()->GetGameObject(0);
			if (obj != nullptr) {
				SceneManager::Get()->GetCurrentScene()->DeleteGameObject(obj);
			}
		}
	}
}