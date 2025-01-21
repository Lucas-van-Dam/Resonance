#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include <nlohmann/json.hpp>

#include "Reon.h"

#include "Events/ProjectEvent.h"
#include "Settings/SettingsManager.h"
#include "ReflectionSystem.h"

namespace REON::EDITOR {

    class ProjectManager
    {
    public:
        static ProjectManager& GetInstance() {
            static ProjectManager instance;
            return instance;
        }

        ProjectManager(const ProjectManager&) = delete;
        ProjectManager& operator=(const ProjectManager&) = delete;

        bool CreateNewProject(const std::string& projectName, const std::string& targetDirectory);

        bool OpenProject(const std::string& projectPath);

        bool SaveProject();

        const std::string& GetCurrentProjectPath() const { return m_CurrentProjectPath; }
        bool IsProjectOpen() const { return isProjectOpen; }

    private:
        ProjectManager();

        void InitializeFolders();

        void InitializeDefaultFiles(const std::string& projectName);

        bool SaveSettings();

        bool SaveScene(const REON::Scene& scene);
        bool SaveScenes();

        void SerializeGameObjectForScene(nlohmann::json& sceneJson, std::shared_ptr<REON::GameObject> object);

        nlohmann::json SerializeGameObject(std::shared_ptr<REON::GameObject> object);

        std::string SerializeField(const char* fieldType, const void* data);

        std::string ExtractInnerType(const std::string& typeString);

        std::string m_CurrentProjectPath;
        bool isProjectOpen;

        std::unordered_map<std::string, std::function<std::string(const void*)>> serializers;
    };

}

