#include "EditorLayer.h"
#include "imgui/imgui.h"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>
#include "ImGuizmo.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "Events/EditorUIEvent.h"
#include "ProjectManagement/AssetScanner.h"
#include "ProjectManagement/MetadataGenerator.h"
#include "ProjectManagement/Processors/GLTFProcessor.h"
#include "ProjectManagement/Processors/PrimaryProcessors.h"
#include "REON/Rendering/PostProcessing/BloomEffect.h"

namespace REON::EDITOR {

	EditorLayer::EditorLayer() : Layer("EditorLayer")
	{
		REON::AssetRegistry::RegisterProcessor(".gltf", std::make_unique<GLTFProcessor>());
		REON::AssetRegistry::RegisterProcessor(".glb", std::make_unique<GLTFProcessor>());
		REON::AssetRegistry::RegisterProcessor("model", std::make_unique<ModelProcessor>());
		m_Gizmotype = GizmoType::Translate;
	}

	void EditorLayer::OnUpdate()
	{
		if (!m_ProjectLoaded)
			return;
		ProcessMouseMove();
	}

	void EditorLayer::OnAttach()
	{
		m_KeyPressedCallbackID = EventBus::Get().subscribe<REON::KeyPressedEvent>(REON_BIND_EVENT_FN(EditorLayer::ProcessKeyPress));
		m_ProjectOpenedCallbackID = EventBus::Get().subscribe<ProjectOpenedEvent>(REON_BIND_EVENT_FN(EditorLayer::OnProjectLoaded));
	}

	void EditorLayer::OnDetach()
	{
		EventBus::Get().unsubscribe<REON::KeyPressedEvent>(m_KeyPressedCallbackID);
		EventBus::Get().unsubscribe<ProjectOpenedEvent>(m_ProjectOpenedCallbackID);
	}

	void EditorLayer::OnImGuiRender()
	{
		// Set up the main window as a dockspace
		ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
		ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->Pos);
		ImGui::SetNextWindowSize(viewport->Size);
		ImGui::SetNextWindowViewport(viewport->ID);
		windowFlags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
		windowFlags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
		// Begin the dockspace window
		ImGui::Begin("DockSpace", nullptr, windowFlags);

		// Dockspace ID
		ImGuiID dockspaceID = ImGui::GetID("MyDockSpace");
		ImGui::DockSpace(dockspaceID, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem("New Project")) {
					ImGuiFileDialog::Instance()->OpenDialog(
						"ChooseProjectFolder",         // Key to identify the dialog
						"Select Project Folder",    // Title
						nullptr                     // File filter (nullptr for folders)
					);
					ProjectManager::GetInstance().CreateNewProject("VisualizerProject", "C:/Projects/Cuttlefish/TestProjects");
				}
				else if (ImGui::MenuItem("Open Project")) {
					ImGuiFileDialog::Instance()->OpenDialog(
						"ChooseProjectDir",         // Key to identify the dialog
						"Select Project Folder",    // Title
						nullptr                     // File filter (nullptr for folders)
					);
				}
				else if (ImGui::MenuItem("Save Project")) {
					ProjectManager::GetInstance().SaveProject();
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window")) {
			}
			ImGui::EndMenuBar();
		}

		ImGui::End();

		ImGuizmo::BeginFrame();

		if (ImGuiFileDialog::Instance()->Display("ChooseProjectDir")) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				// The user selected a valid folder
				std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
				std::cout << "Selected Path: " << selectedPath << std::endl;

				// Call your project loading function
				ProjectManager::GetInstance().OpenProject(selectedPath);
			}
			// Close the dialog after processing the results
			ImGuiFileDialog::Instance()->Close();
		}
		if (ImGuiFileDialog::Instance()->Display("ChooseProjectFolder")) {
			if (ImGuiFileDialog::Instance()->IsOk()) {
				// The user selected a valid folder
				std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();

				// Call your project loading function
				ProjectManager::GetInstance().CreateNewProject("TestProject", selectedPath);
			}
			// Close the dialog after processing the results
			ImGuiFileDialog::Instance()->Close();
		}

		if (!m_ProjectLoaded)
			return;

		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		auto scene = REON::SceneManager::Get()->GetCurrentScene();

		if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse))
		{
			auto size = ImGui::GetContentRegionAvail();
			scene->renderManager->SetRenderDimensions(size.x, size.y);
			m_SceneHovered = ImGui::IsWindowHovered();

			ImGui::Image((void*)(intptr_t)scene->renderManager->GetEndBuffer(), size, ImVec2(0, 1), ImVec2(1, 0));
			if (scene->selectedObject) {
				ImVec2 windowSize = ImGui::GetWindowSize();
				ImVec2 pos = ImGui::GetWindowPos();

				ImGuizmo::SetDrawlist();

				ImGuizmo::SetRect(pos.x, pos.y, windowSize.x, windowSize.y);

				auto camera = scene->GetEditorCamera();
				glm::mat4 viewMatrix = camera->GetViewMatrix();
				glm::mat4 projMatrix = camera->GetProjectionMatrix();
				glm::mat4 worldMatrix = scene->selectedObject->GetTransform()->GetWorldTransform();

				ImGuizmo::OPERATION operation;
				switch (m_Gizmotype) {
				case GizmoType::Translate:
					operation = ImGuizmo::TRANSLATE;
					break;
				case GizmoType::Rotate:
					operation = ImGuizmo::ROTATE;
					break;
				case GizmoType::Scale:
					operation = ImGuizmo::SCALE;
					break;
				}

				if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix),
					operation, ImGuizmo::LOCAL, glm::value_ptr(worldMatrix)))
				{
					scene->selectedObject->GetTransform()->SetWorldTransform(worldMatrix);
				}

			}
		}
		ImGui::End();

		if (ImGui::Begin("PostProcessTestWindow")) {
			bool bloom = RenderManager::m_BloomEffect->IsEnabled();
			if (ImGui::Checkbox("Enable Bloom", &bloom)) {
				RenderManager::m_BloomEffect->SetEnabled(bloom);
			}

			ImGui::DragFloat("Bloom Threshold", &RenderManager::m_BloomEffect->BloomThreshold, 0.01f, 0.0f, 3.0f);

			ImGui::InputInt("Bloom Passes", &RenderManager::m_BloomEffect->BloomPasses, 2);

			ImGui::DragFloat("Bloom Strength", &RenderManager::m_BloomEffect->BloomStrength, 0.01f, 0.0f, 1.0f);
		}

		ImGui::End();

		//ImGui::PopStyleVar(2);
		Inspector::InspectObject(scene->selectedObject);

		SceneHierarchy::RenderSceneHierarchy(REON::SceneManager::Get()->GetCurrentScene()->GetRootObjects(), scene->selectedObject);
		m_AssetBrowser.RenderAssetBrowser();
	}

	void EditorLayer::ProcessKeyPress(const REON::KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == REON_KEY_F2 && event.GetRepeatCount() == 0) {
			REON::SceneManager::Get()->GetCurrentScene()->renderManager->HotReloadShaders();
		}

		if (event.GetKeyCode() == REON_KEY_1) {
			m_Gizmotype = GizmoType::Translate;
		}
		if (event.GetKeyCode() == REON_KEY_2) {
			m_Gizmotype = GizmoType::Rotate;
		}
		if (event.GetKeyCode() == REON_KEY_3) {
			m_Gizmotype = GizmoType::Scale;
		}
	}

	void EditorLayer::ProcessMouseMove()
	{
		if (m_SceneHovered) {
			if (REON::Input::IsMouseButtonPressed(REON_MOUSE_BUTTON_2)) {
				if (!m_CursorLocked) {
					glfwGetCursorPos((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), &m_SavedX, &m_SavedY);
					glfwSetInputMode((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
				}

				float Xoffset = REON::Input::GetMouseX() - (float)m_SavedX;
				float Yoffset = m_SavedY - REON::Input::GetMouseY();
				REON::SceneManager::Get()->GetCurrentScene()->GetEditorCamera()->ProcessMouseMovement(Xoffset, Yoffset);
				glfwSetCursorPos((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), m_SavedX, m_SavedY);
				m_CursorLocked = true;
			}
			else {
				glfwSetInputMode((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
				m_CursorLocked = false;
			}
		}
	}



	void EditorLayer::OnProjectLoaded(const ProjectOpenedEvent& event)
	{
		auto assets = AssetScanner::scanAssets(event.GetProjectDirectory());
		for (const auto& asset : assets) {
			if (AssetScanner::primaryAssetExtensions.find(asset.extension().string()) != AssetScanner::primaryAssetExtensions.end()) {
				std::ifstream primaryFile(asset);
				if (primaryFile.is_open()) {
					nlohmann::json j;
					primaryFile >> j;
					REON::AssetInfo assetInfo;
					assetInfo.id = j["GUID"];
					auto extension = asset.extension().string();
					extension.erase(0, 1);
					assetInfo.type = extension;
					assetInfo.path = fs::relative(asset, event.GetProjectDirectory());
					REON::AssetRegistry::ProcessAsset(assetInfo);
					REON::AssetRegistry::Instance().RegisterAsset(assetInfo);
				}
			}
			else {
				MetadataGenerator::EnsureMetadataExists(asset, event.GetProjectDirectory());

				auto metaPath = asset.string() + ".meta";
				if (fs::exists(metaPath)) {
					std::ifstream metaFile(metaPath);
					if (metaFile.is_open()) {
						json metaData;
						metaFile >> metaData;

						REON::AssetInfo assetInfo;
						assetInfo.id = metaData["Id"].get<std::string>();
						assetInfo.type = metaData["Type"].get<std::string>();
						assetInfo.path = fs::relative(asset, event.GetProjectDirectory());

						REON::AssetRegistry::Instance().RegisterAsset(assetInfo);
					}
				}
			}
		}
		//auto list = REON::AssetRegistry::Instance().GetAllAssets();
		Inspector::Initialize();
		m_AssetBrowser.SetRootDirectory(event.GetProjectDirectory().string() + "/Assets");
		m_ProjectLoaded = true;
	}
}
