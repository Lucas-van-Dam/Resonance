#include "EditorLayer.h"
#include "imgui/imgui.h"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "ImGuizmo.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "Events/EditorUIEvent.h"
#include "ProjectManagement/AssetScanner.h"
#include "ProjectManagement/MetadataGenerator.h"
#include <REON/AssetManagement/Processors/GLTFProcessor.h>

namespace REON::EDITOR {

	EditorLayer::EditorLayer() : Layer("Inspector")
	{
		REON::AssetRegistry::RegisterProcessor(".gltf", std::make_unique<REON::GLTFProcessor>());
	}

	void EditorLayer::OnUpdate()
	{
		if (!projectLoaded)
			return;
		ProcessMouseMove();
		if (!m_SelectedObject)
			m_SelectedObject = REON::SceneManager::Get()->GetCurrentScene()->GetGameObject(0);
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

		if (!projectLoaded)
			return;

		//ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		//ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

		if (ImGui::Begin("Scene", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse | ImGuiWindowFlags_NoCollapse))
		{
			auto scene = REON::SceneManager::Get()->GetCurrentScene();

			auto size = ImGui::GetContentRegionAvail();
			scene->renderManager->SetRenderDimensions(size.x, size.y);
			m_SceneHovered = ImGui::IsWindowHovered();

			ImGui::Image((void*)(intptr_t)scene->renderManager->GetEndBuffer(), size, ImVec2(0, 1), ImVec2(1, 0));
			if (m_SelectedObject) {
				ImVec2 windowSize = ImGui::GetWindowSize();
				ImVec2 pos = ImGui::GetWindowPos();

				ImGuizmo::SetDrawlist();

				ImGuizmo::SetRect(pos.x, pos.y, windowSize.x, windowSize.y);

				auto camera = scene->GetEditorCamera();
				glm::mat4 viewMatrix = camera->GetViewMatrix();
				glm::mat4 projMatrix = camera->GetProjectionMatrix();
				glm::mat4 objectMatrix = m_SelectedObject->GetTransform()->GetTransformationMatrix();

				if (ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix),
					ImGuizmo::TRANSLATE, ImGuizmo::WORLD, glm::value_ptr(objectMatrix)))
				{
					m_SelectedObject->GetTransform()->SetWorldTransform(objectMatrix);
				}
			}
		}
		ImGui::End();

		//ImGui::PopStyleVar(2);
		if (m_SelectedObject) {
			Inspector::InspectObject(*m_SelectedObject);
		}
		SceneHierarchy::RenderSceneHierarchy(REON::SceneManager::Get()->GetCurrentScene()->GetRootObjects(), m_SelectedObject);
	}

	void EditorLayer::OnEvent(REON::Event& event)
	{
		REON::EventDispatcher dispatcher(event);
		dispatcher.Dispatch<REON::KeyPressedEvent>(REON_BIND_EVENT_FN(EditorLayer::ProcessKeyPress));
		dispatcher.Dispatch<ProjectOpenedEvent>(REON_BIND_EVENT_FN(EditorLayer::OnProjectLoaded));
	}

	bool EditorLayer::ProcessKeyPress(REON::KeyPressedEvent& event)
	{
		if (event.GetKeyCode() == REON_KEY_F2 && event.GetRepeatCount() == 0) {
			REON::SceneManager::Get()->GetCurrentScene()->renderManager->HotReloadShaders();
		}
		return false;
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

	bool EditorLayer::OnProjectLoaded(ProjectOpenedEvent& event)
	{
		auto assets = AssetScanner::scanAssets(event.GetProjectDirectory());
		for (const auto& asset : assets) {
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
					REON::AssetRegistry::ProcessAsset(assetInfo);

				}
			}

		}
		//auto list = REON::AssetRegistry::Instance().GetAllAssets();
		Inspector::Initialize();
		projectLoaded = true;
		return false;
	}
}
