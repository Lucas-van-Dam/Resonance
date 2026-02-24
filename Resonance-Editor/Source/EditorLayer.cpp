#include "EditorLayer.h"

#include "AssetManagement/AssetRegistry.h"
#include "Events/EditorUIEvent.h"
#include "ImGuizmo.h"
#include "ProjectManagement/AssetScanner.h"
#include "ProjectManagement/MetadataGenerator.h"
#include "ProjectManagement/ProjectManager.h"
#include "REON/GameHierarchy/Components/Transform.h"
#include "ShaderGraph/ShaderNodeLibrary.h"
#include "imgui/imgui.h"
#include "vulkan/vulkan.h"

#include <Commands/CommandManager.h>
#include <Commands/PropertyChangeCommand.h>
#include <Windows/ShaderGraph.h>
#include <filesystem>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/matrix_decompose.hpp>

namespace REON::EDITOR
{

EditorLayer::EditorLayer() : Layer("EditorLayer")
{
    // REON::AssetRegistry::RegisterProcessor(".gltf", std::make_unique<GLTFProcessor>());
    // REON::AssetRegistry::RegisterProcessor(".glb", std::make_unique<GLTFProcessor>());
    // REON::AssetRegistry::RegisterProcessor("model", std::make_unique<ModelProcessor>());
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
    m_KeyPressedCallbackID =
        EventBus::Get().subscribe<REON::KeyPressedEvent>(REON_BIND_EVENT_FN(EditorLayer::ProcessKeyPress));
    m_ProjectOpenedCallbackID =
        EventBus::Get().subscribe<ProjectOpenedEvent>(REON_BIND_EVENT_FN(EditorLayer::OnProjectLoaded));
    SG::ShaderNodeLibrary::GetInstance().Initialize("Assets/ShaderGraph/ShaderNodeTemplates.json");
    ShaderGraph::GetInstance().initialize();
}

void EditorLayer::OnDetach()
{
    EventBus::Get().unsubscribe<REON::KeyPressedEvent>(m_KeyPressedCallbackID);
    EventBus::Get().unsubscribe<ProjectOpenedEvent>(m_ProjectOpenedCallbackID);
    ShaderGraph::GetInstance().shutdown();
}

void EditorLayer::OnCleanup()
{
    // if (m_ProjectLoaded)
    //     AssetRegistry::Instance().SaveRegistryToFile(ProjectManager::GetInstance().GetCurrentProjectPath() +
    //                                                  "/EngineCache/AssetRegistry");
}

void EditorLayer::OnImGuiRender()
{
    // Set up the main window as a dockspace
    ImGuiWindowFlags windowFlags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);
    windowFlags |=
        ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
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
            if (ImGui::MenuItem("New Project"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("ChooseProjectFolder",   // Key to identify the dialog
                                                        "Select Project Folder", // Title
                                                        nullptr                  // File filter (nullptr for folders)
                );
                ProjectManager::GetInstance().CreateNewProject("VisualizerProject",
                                                               "C:/Projects/Cuttlefish/TestProjects");
            }
            else if (ImGui::MenuItem("Open Project"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("ChooseProjectDir",      // Key to identify the dialog
                                                        "Select Project Folder", // Title
                                                        nullptr                  // File filter (nullptr for folders)
                );
            }
            else if (ImGui::MenuItem("Save Project"))
            {
                ProjectManager::GetInstance().SaveProject();
            }
            else if (ImGui::MenuItem("Build Project"))
            {
                ImGuiFileDialog::Instance()->OpenDialog("ChooseBuildDirectory", // Key to identify the dialog
                                                        "Select Build Folder",  // Title
                                                        nullptr                 // File filter (nullptr for folders)
                );
            }
            ImGui::EndMenu();
        }
        if (m_ProjectLoaded)
        {
            if (ImGui::BeginMenu("Window"))
            {
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("GameObject"))
            {
                if (ImGui::Button("PointLight"))
                {
                    std::shared_ptr<GameObject> lightObject = std::make_shared<GameObject>();
                    SceneManager::Get()->GetCurrentScene()->AddGameObject(lightObject);
                    lightObject->SetName("point light");
                    std::shared_ptr<Light> light = std::make_shared<Light>();
                    light->type = LightType::Point;
                    lightObject->AddComponent<Light>(light);
                }
                ImGui::EndMenu();
            }
        }
        ImGui::EndMenuBar();
    }

    ImGui::End();

    ImGuizmo::BeginFrame();

    if (ImGuiFileDialog::Instance()->Display("ChooseProjectDir"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            // The user selected a valid folder
            std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            std::cout << "Selected Path: " << selectedPath << std::endl;

            // Call your project loading function
            ProjectManager::GetInstance().OpenProject(selectedPath);
        }
        // Close the dialog after processing the results
        ImGuiFileDialog::Instance()->Close();
    }
    if (ImGuiFileDialog::Instance()->Display("ChooseProjectFolder"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            // The user selected a valid folder
            std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();

            // Call your project loading function
            ProjectManager::GetInstance().CreateNewProject("TestProject", selectedPath);
        }
        // Close the dialog after processing the results
        ImGuiFileDialog::Instance()->Close();
    }
    if (ImGuiFileDialog::Instance()->Display("ChooseBuildDirectory"))
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string selectedPath = ImGuiFileDialog::Instance()->GetCurrentPath();
            ProjectManager::GetInstance().BuildProject(selectedPath);
        }
        // Close the dialog after processing the results
        ImGuiFileDialog::Instance()->Close();
    }

    if (!m_ProjectLoaded || m_FirstFrame)
    {
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

        VkSemaphore signalSemaphores[] = {context->getCurrentRenderFinishedSemaphore()};
        VkSemaphore waitSemaphores[] = {context->getCurrentImageAvailableSemaphore()};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        vkQueueSubmit(context->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        if (m_ProjectLoaded)
            m_FirstFrame = false;
        return;
    }

    // ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    // ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));

    auto scene = SceneManager::Get()->GetCurrentScene();

    bool ClearSelection = false;

    ImGui::Begin("Scene Hierarchy");
    if (ImGui::IsWindowHovered())
    {
        ClearSelection = true;
    }
    ImGui::End();

    ImGui::Begin("Asset Browser");
    if (ImGui::IsWindowHovered())
    {
        ClearSelection = true;
    }
    ImGui::End();

    if (ClearSelection && ImGui::IsMouseClicked(0))
    {
        scene->selectedObject = nullptr;
        m_AssetBrowser.clearSelectedFile();
    }

    if (ImGui::Begin("Scene", nullptr,
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                         ImGuiWindowFlags_NoCollapse))
    {
        static ImVec2 lastSize = ImVec2(0, 0);
        ImVec2 currentSize = ImGui::GetWindowSize();

        bool wasResized = (currentSize.x != lastSize.x || currentSize.y != lastSize.y);
        lastSize = currentSize;

        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(6, 4));

        ImGui::BeginChild("SceneToolbar", ImVec2(0, 32), false,
                          ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        {
            static const char* modeNames[] = {"Lit", "Unlit", "Wireframe"};
            int currentIndex = static_cast<int>(scene->renderManager->renderMode);
            ImGui::PushItemWidth(80);
            ImGui::SetNextWindowSizeConstraints(
                ImVec2(80, IM_ARRAYSIZE(modeNames) * ImGui::GetTextLineHeightWithSpacing() + 12),
                ImVec2(FLT_MAX, FLT_MAX));
            if (ImGui::BeginCombo("###ViewMode", modeNames[currentIndex], ImGuiComboFlags_NoArrowButton))
            {
                for (int i = 0; i < IM_ARRAYSIZE(modeNames); ++i)
                {
                    bool isSelected = (currentIndex == i);
                    if (ImGui::Selectable(modeNames[i], isSelected))
                    {
                        scene->renderManager->renderMode = static_cast<RenderMode>(i);
                    }
                    if (isSelected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndCombo();
            }
            if (ImGui::IsItemHovered())
                ImGui::SetTooltip("Choose how to render the scene view");
            ImGui::PopItemWidth();
        }
        ImGui::EndChild();

        ImGui::PopStyleVar(2);

        auto size = ImGui::GetContentRegionAvail();

        if (wasResized)
            scene->renderManager->SetRenderDimensions(scene->GetEditorCamera(), size.x, size.y);

        m_SceneHovered = ImGui::IsWindowHovered();

        ImVec2 viewportStart = ImGui::GetCursorScreenPos();

        auto texId = scene->renderManager->GetEndBuffer(scene->GetEditorCamera());

        if (texId != nullptr)
            ImGui::Image((ImTextureID)texId, size, ImVec2(0, 1), ImVec2(1, 0));
        if (scene->selectedObject)
        {
            ImVec2 windowSize = size;
            ImVec2 pos = viewportStart;

            ImGuizmo::SetDrawlist();

            ImGuizmo::SetRect(pos.x, pos.y, windowSize.x, windowSize.y);

            auto camera = scene->GetEditorCamera();
            glm::mat4 viewMatrix = camera->GetViewMatrix();
            glm::mat4 projMatrix = camera->GetProjectionMatrix();
            glm::mat4 worldMatrix = scene->selectedObject->GetTransform()->GetWorldTransform();

            ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
            switch (m_Gizmotype)
            {
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

            static bool wasUsingGizmo = false;

            ImGuizmo::Manipulate(glm::value_ptr(viewMatrix), glm::value_ptr(projMatrix), operation, ImGuizmo::LOCAL,
                                 glm::value_ptr(worldMatrix));

            if (ImGuizmo::IsUsing())
            {
                auto transform = scene->selectedObject->GetTransform();
                if (!wasUsingGizmo)
                {
                    CommandManager::startBatch(std::make_unique<PropertyChangeCommand<glm::mat4>>(
                        [transform](const glm::mat4& mat)
                        {
                            transform->SetWorldTransform(mat);
                            transform->eulerDirty = true;
                        },
                        [transform]() { return transform->GetWorldTransform(); }, worldMatrix, &worldMatrix));
                }
                else
                {
                    CommandManager::updateBatch(
                        [&](ICommand* cmd)
                        { static_cast<PropertyChangeCommand<glm::mat4>*>(cmd)->UpdateValue(worldMatrix); });
                }

                scene->selectedObject->GetTransform()->SetWorldTransform(worldMatrix);
                scene->selectedObject->GetTransform()->eulerDirty = true;
            }

            if (wasUsingGizmo && !ImGuizmo::IsUsing())
            {
                CommandManager::endBatch();
            }

            wasUsingGizmo = ImGuizmo::IsUsing();
        }
    }
    ImGui::End();

    if (scene->cameras.size() > 1)
    {
        if (ImGui::Begin("Game", nullptr,
                         ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse |
                             ImGuiWindowFlags_NoCollapse))
        {
            static ImVec2 lastSize = ImVec2(0, 0);
            ImVec2 currentSize = ImGui::GetWindowSize();

            bool wasResized = (currentSize.x != lastSize.x || currentSize.y != lastSize.y);
            lastSize = currentSize;

            auto size = ImGui::GetContentRegionAvail();

            if (wasResized)
                scene->renderManager->SetRenderDimensions(scene->cameras[1], size.x, size.y);

            auto texId = scene->renderManager->GetEndBuffer(scene->cameras[1]);

            if (texId != nullptr)
                ImGui::Image((ImTextureID)texId, size, ImVec2(0, 1), ImVec2(1, 0));
        }
        ImGui::End();
    }

    if (!m_AssetBrowser.getSelectedFile().empty())
    {
        Inspector::InspectObject(m_AssetBrowser.getSelectedFile());
    }
    else
    {
        Inspector::InspectObject(scene->selectedObject);
    }

    ShaderGraph::GetInstance().render();

    SceneHierarchy::RenderSceneHierarchy(REON::SceneManager::Get()->GetCurrentScene()->GetRootObjects(),
                                         scene->selectedObject);
    m_AssetBrowser.RenderAssetBrowser();
}

void EditorLayer::ProcessKeyPress(const REON::KeyPressedEvent& event)
{
    if (event.GetKeyCode() == REON_KEY_F2 && event.GetRepeatCount() == 0)
    {
        REON::SceneManager::Get()->GetCurrentScene()->renderManager->HotReloadShaders();
    }

    if (event.GetKeyCode() == REON_KEY_1)
    {
        m_Gizmotype = GizmoType::Translate;
    }
    if (event.GetKeyCode() == REON_KEY_2)
    {
        m_Gizmotype = GizmoType::Rotate;
    }
    if (event.GetKeyCode() == REON_KEY_3)
    {
        m_Gizmotype = GizmoType::Scale;
    }

    if (event.GetKeyCode() == REON_KEY_Z && event.GetRepeatCount() == 0)
    {
        if (Input::IsKeyPressed(REON_KEY_LEFT_CONTROL) && CommandManager::canUndo)
        {
            CommandManager::undo();
        }
    }

    if (event.GetKeyCode() == REON_KEY_Y && event.GetRepeatCount() == 0)
    {
        if (Input::IsKeyPressed(REON_KEY_LEFT_CONTROL) && CommandManager::canRedo)
        {
            CommandManager::redo();
        }
    }
}

void EditorLayer::ProcessMouseMove()
{
    if (m_SceneHovered)
    {
        if (REON::Input::IsMouseButtonPressed(REON_MOUSE_BUTTON_2))
        {
            if (!m_CursorLocked)
            {
                glfwGetCursorPos((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), &m_SavedX,
                                 &m_SavedY);
                glfwSetInputMode((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), GLFW_CURSOR,
                                 GLFW_CURSOR_DISABLED);
            }

            float Xoffset = REON::Input::GetMouseX() - (float)m_SavedX;
            float Yoffset = m_SavedY - REON::Input::GetMouseY();
            REON::SceneManager::Get()->GetCurrentScene()->GetEditorCamera()->ProcessMouseMovement(Xoffset, Yoffset);
            glfwSetCursorPos((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), m_SavedX, m_SavedY);
            m_CursorLocked = true;
        }
        else
        {
            glfwSetInputMode((GLFWwindow*)REON::Application::Get().GetWindow().GetNativeWindow(), GLFW_CURSOR,
                             GLFW_CURSOR_NORMAL);
            m_CursorLocked = false;
        }
    }
}

void EditorLayer::OnProjectLoaded(const ProjectOpenedEvent& event)
{
    auto assets = AssetScanner::scanAssets(event.GetProjectDirectory() / "Assets");

    for (const auto& asset : assets)
    {
        futures.push_back(
            std::async(std::launch::async, [&, asset, event]() { RegisterAsset(asset, event.GetProjectDirectory()); }));
    }

    while (!futures.empty())
    {
        for (auto it = futures.begin(); it != futures.end();)
        {
            auto& f = *it;
            if (f.valid() && f.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
            {
                it = futures.erase(it); // remove finished future
            }
            else
            {
                ++it;
            }
        }
    }

    cookPipeline.CookAll({event.GetProjectDirectory().string(), "EngineCache"}, m_BuildQueue);

    Application::Get().Init(event.GetProjectDirectory().string() + "\\EngineCache\\manifest.bin");

    Inspector::Initialize();
    m_AssetBrowser.SetRootDirectory(event.GetProjectDirectory().string() + "/Assets");
    m_ProjectLoaded = true;
}

void EditorLayer::RegisterAsset(const std::filesystem::path& assetPath, const std::filesystem::path& projectPath)
{
    nlohmann::json jsonData;

    if (AssetScanner::primaryAssetExtensions.find(assetPath.extension().string()) !=
        AssetScanner::primaryAssetExtensions.end())
    {
        std::ifstream primaryFile(assetPath);
        if (primaryFile.is_open())
        {
            primaryFile >> jsonData;
            AssetId assetId = jsonData["id"].get<AssetId>();
            AssetTypeId assetType = jsonData["assetType"].get<uint32_t>();

            AssetRegistry::Instance().Upsert(AssetRecord{assetId, assetType, assetPath, assetPath.filename().string()});
        }
        else
        {
            REON_WARN("Could not open primary file on project load: {}", assetPath.string());
            return;
        }
    }
    else
    {
        MetadataGenerator::EnsureMetadataExists(assetPath, projectPath);

        auto metaPath = assetPath.string() + ".meta";
        if (fs::exists(metaPath))
        {
            std::ifstream metaFile(metaPath);
            if (metaFile.is_open())
            {
                metaFile >> jsonData;

                AssetId assetId = jsonData["id"].get<AssetId>();
                AssetTypeId assetType = jsonData["assetType"].get<uint32_t>();
                AssetRegistry::Instance().Upsert(
                    AssetRecord{assetId, assetType, assetPath, assetPath.filename().string()});
            }
            else
            {
                REON_WARN("Could not open meta file on project load: {}", metaPath);
                return;
            }
        }
        else
        {
            REON_WARN("Could not find meta path for file: {}", assetPath.string());
            return;
        }
    }

    //compute bulid key from file timestamp
    auto lastWriteTime = fs::last_write_time(assetPath);
    auto buildKey = std::chrono::duration_cast<std::chrono::milliseconds>(lastWriteTime.time_since_epoch()).count();
    if (buildKey != jsonData["build"]["lastBuildKey"].get<long long>())
    {
        REON_INFO("Asset {} has changed since last build, marking for reimport", assetPath.string());
        BuildJob job;
        job.reason = BuildReason::SourceChanged;
        job.sourceId = jsonData["id"].get<AssetId>();
        job.type = jsonData["assetType"].get<uint32_t>();
        m_BuildQueue.Enqueue(job);
    }
}
void EditorLayer::CheckAssetsRegistered() {}
} // namespace REON::EDITOR
