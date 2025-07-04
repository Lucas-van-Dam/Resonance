#pragma once
#include <string>
#include <memory>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <unordered_set>

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

        void LoadScene(const std::filesystem::path& path);

        bool BuildProject(const std::filesystem::path& buildDirectory);

        std::unordered_set<std::string> getUsedAssetsFromScene(const std::shared_ptr<Scene>& scene);

        const std::string& GetCurrentProjectPath() const { return m_CurrentProjectPath; }
        bool IsProjectOpen() const { return isProjectOpen; }

    private:
        ProjectManager();

        void InitializeFolders();

        void InitializeDefaultFiles(const std::string& projectName);

        bool SaveSettings();

        bool SaveScene(const REON::Scene& scene);
        bool SaveScenes();

        void DeSerializeGameObjectForScene(const nlohmann::json& objectJson, std::shared_ptr<GameObject> object, const nlohmann::json& sceneJson);

        void SerializeGameObjectForScene(nlohmann::json& sceneJson, std::shared_ptr<REON::GameObject> object);

        nlohmann::json SerializeGameObject(std::shared_ptr<REON::GameObject> object);

        std::string SerializeField(const char* fieldType, const void* data);

        std::string ExtractInnerType(const std::string& typeString);

        template <typename T>
        void RegisterDeserializer(const std::string& name, std::function<T(const std::string&)> func) {
            deserializers[name] = [func](const std::string& str) -> void* {
                T result = func(str);  // Deserialize to the desired type
                return new T(result);  // Allocate and return a pointer to the deserialized object
                };
        }

        template <typename T>
        T Deserialize(const std::string& name, const std::string& str) {
            auto it = deserializers.find(name);
            if (it != deserializers.end()) {
                // Cast the void* to the correct type (T)
                T* ptr = static_cast<T*>(it->second(str));
                T result = *ptr;  // Dereference to get the object
                delete ptr;  // Remember to delete the allocated memory
                return result;
            }
            REON_ERROR("Deserializer not found for type: {}", name);
            T obj;
            return obj;
        }

        void* DeserializeToVoid(const std::string& name, const std::string& str) {
            auto it = deserializers.find(name);
            if (it != deserializers.end()) {
                return it->second(str);
            }
            REON_ERROR("Deserializer not found for type: {}", name);
            return nullptr;
        }

        std::string m_CurrentProjectPath;
        bool isProjectOpen;

		ProjectSettings m_ProjectSettings;

        std::unordered_map<std::string, std::function<std::string(const void*)>> serializers;
        std::unordered_map<std::string, std::function<void*(const std::string&)>> deserializers; 
    };

}

