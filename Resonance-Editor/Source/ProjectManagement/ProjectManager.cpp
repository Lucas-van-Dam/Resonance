#include "reonpch.h"
#include "ProjectManager.h"


namespace REON::EDITOR {

    ProjectManager::ProjectManager() : isProjectOpen(false) {
        serializers["int"] = [](const void* data) {
            return std::to_string(*static_cast<const int*>(data));
            };

        serializers["float"] = [](const void* data) {
            return std::to_string(*static_cast<const float*>(data));
            };

        serializers["glm::vec3"] = [](const void* data) {
            const auto& vec = *static_cast<const glm::vec3*>(data);
            return "(" + std::to_string(vec.x) + ", " + std::to_string(vec.y) + ", " + std::to_string(vec.z) + ")";
            };

        serializers["Quaternion"] = [](const void* data) {
            const auto& quat = *static_cast<const REON::Quaternion*>(data);
            return "(" + std::to_string(quat.x) + ", " + std::to_string(quat.y) + ", " + std::to_string(quat.z) + ", " + std::to_string(quat.w) + ")";
            };

        //serializers["std::string"] = [](const void* data) {
        //    return *static_cast<const std::string*>(data);
        //    };
    }

    bool ProjectManager::CreateNewProject(const std::string& projectName, const std::string& targetDirectory) {
        m_CurrentProjectPath = targetDirectory + "/" + projectName;

        if (!std::filesystem::exists(m_CurrentProjectPath)) {
            std::filesystem::create_directories(m_CurrentProjectPath);
            InitializeFolders();
            InitializeDefaultFiles(projectName);
            isProjectOpen = true;
            ProjectOpenedEvent event(m_CurrentProjectPath);
            REON::Application::Get().OnEvent(event);
            return true;
        }
        else {
            return false;
        }
    }

    bool ProjectManager::OpenProject(const std::string& projectPath) {
        std::error_code ec;
        if (!std::filesystem::exists(projectPath, ec)) {
            REON_ERROR("Project path does not exist: {0}.\n", ec.message());
            return false;
        }

        m_CurrentProjectPath = projectPath;

        if (!std::filesystem::exists(m_CurrentProjectPath + "/Settings/ProjectSettings.json", ec)) {
            REON_ERROR("Error: Missing ProjectSettings.json in the project.\n");
            return false;
        }

        SettingsManager::GetInstance().LoadSettings(m_CurrentProjectPath);

        ProjectOpenedEvent event(projectPath);
        REON::Application::Get().OnEvent(event);

        return true;
    }

    bool ProjectManager::SaveProject()
    {
        if (m_CurrentProjectPath.empty()) {
            REON_ERROR("No project loaded");
            return false;
        }

        if (!std::filesystem::exists(m_CurrentProjectPath)) {
            REON_ERROR("Project does not exist at: {0}", m_CurrentProjectPath);
            return false;
        }

        SettingsManager::GetInstance().SaveSettings(m_CurrentProjectPath);

        if (!SaveScenes()) {
            REON_ERROR("Failed to save scenes");
            return false;
        }

        return true;
    }

    void ProjectManager::InitializeFolders() {
        std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Materials");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Models");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Shaders");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Textures");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Assets/Videos");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Scenes");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Settings");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Outputs/Renders");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Outputs/Exports");
        std::filesystem::create_directories(m_CurrentProjectPath + "/Logs");
        std::filesystem::create_directories(m_CurrentProjectPath + "/EngineCache");
    }

    void ProjectManager::InitializeDefaultFiles(const std::string& projectName) {
        nlohmann::json projectSettings = {
            {"projectName", projectName},
            {"version", 1}
        };
        std::ofstream settingsFile(m_CurrentProjectPath + "/Settings/ProjectSettings.json");
        settingsFile << projectSettings.dump(4);

        nlohmann::json audioSettings = {
            {"device", "Default Microphone"},
            {"sampleRate", 48000},
            {"bufferSize", 512}
        };
        std::ofstream audioSettingsFile(m_CurrentProjectPath + "/Settings/AudioInputSettings.json");
        audioSettingsFile << audioSettings.dump(4);

        nlohmann::json visualSettings = {
            {"resolution", {1920, 1080}},
            {"frameRate", 60}
        };
        std::ofstream visualSettingsFile(m_CurrentProjectPath + "/Settings/VisualSettings.json");
        visualSettingsFile << visualSettings.dump(4);
    }

    bool ProjectManager::SaveSettings()
    {
        return false;
    }

    bool ProjectManager::SaveScene(const REON::Scene& scene)
    {
        return false;
    }

    bool ProjectManager::SaveScenes()
    {
        auto scene = REON::SceneManager::Get()->GetCurrentScene();

        if (!scene) {
            REON_ERROR("No current scene");
            return false;
        }

        nlohmann::json jsonScene;
        jsonScene["Id"] = scene->GetID();
        jsonScene["SceneName"] = scene->GetName();

        for (const auto& gameobject : scene->GetRootObjects()) {
            jsonScene["RootObjects"].push_back(gameobject->GetID());
            SerializeGameObjectForScene(jsonScene, gameobject);
        }



        std::ofstream file(m_CurrentProjectPath + "/Scenes/Scene1.scene");
        if (file.is_open()) {
            file << jsonScene.dump(4);
            file.close();
        }
        else {
            REON_ERROR("Failed to open file for writing: {0}", m_CurrentProjectPath + "/Scenes/Scene1.scene");
            return false;
        }

        return true;
    }

    void ProjectManager::SerializeGameObjectForScene(nlohmann::json& sceneJson, std::shared_ptr<REON::GameObject> object) {
        sceneJson["GameObjects"].push_back(SerializeGameObject(object));

        for (auto& child : object->GetChildren()) {
            SerializeGameObjectForScene(sceneJson, child);
        }
    }

    nlohmann::json ProjectManager::SerializeGameObject(std::shared_ptr<REON::GameObject> object) {
        nlohmann::json jsonObject;
        for (auto& child : object->GetChildren()) {
            jsonObject["Children"].push_back(child->GetID());
        }

        for (auto& component : object->GetComponents()) {
            nlohmann::json jsonComponent;
            jsonComponent["Name"] = component->GetTypeName();
            auto reflectionClass = ReflectionRegistry::Instance().GetClass(component->GetTypeName());
            for (int i = 0; i < reflectionClass->field_count; i++) {
                auto& field = reflectionClass->fields[i];
                // Print base address of component to verify its value
                std::uintptr_t baseAddress = reinterpret_cast<std::uintptr_t>(component.get());

                void* fieldPtr = reinterpret_cast<void*>(baseAddress + field.offset);

                // Now pass the field pointer to the serializer
                jsonComponent[field.name] = SerializeField(field.type, fieldPtr);
            }
            jsonObject["Component"].push_back(jsonComponent);
        }

        jsonObject["Name"] = object->GetName();

        jsonObject["Id"] = object->GetID();

        return jsonObject;
    }

    std::string ProjectManager::SerializeField(const char* fieldType, const void* data) {
        static auto dummy = std::make_shared<REON::Object>();
        auto it = serializers.find(fieldType);
        if (it != serializers.end()) {
            return it->second(data); // Call the corresponding serializer
        }
        else {
            try {
                std::string stringType = std::string(fieldType);
                if (stringType.find("ptr") != std::string::npos) {
                    auto sharedPtr = ReflectionRegistry::Instance().Create(ExtractInnerType(fieldType), const_cast<void*>(data));
                    return sharedPtr->GetID();
                }
                REON_WARN("No serializer found for type: {0}", fieldType);
                return "";
            }
            catch (std::exception ex) {
                REON_CORE_ERROR("Exception while trying to serialize field type {0}: {1}.", fieldType, ex.what());
            }
            //auto object = static_cast<const Object*>(data);
            //return std::to_string(object->GetID());
        }
    }

    std::string ProjectManager::ExtractInnerType(const std::string& typeString) {
        // Find the position of '<' and '>'
        size_t start = typeString.find('<');
        size_t end = typeString.find('>');

        // Ensure both characters are found
        if (start == std::string::npos || end == std::string::npos || end <= start) {
            return ""; // Return an empty string if the format is invalid
        }

        // Extract the substring inside the angle brackets
        return typeString.substr(start + 1, end - start - 1);
    }

}
