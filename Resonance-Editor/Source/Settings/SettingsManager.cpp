#include "SettingsManager.h"
namespace REON::EDITOR {
    void SettingsManager::LoadFromJson(const std::string& filepath, ProjectSettings& settings) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            REON_ERROR("Could not open settings file: {0}", filepath);
            return;
        }

        nlohmann::json json;
        file >> json;

        settings.projectName = json.value("ProjectName", settings.projectName);
        settings.authorName = json.value("AuthorName", settings.authorName);
        settings.projectVersion = json.value("ProjectVersion", settings.projectVersion);
    }

    void SettingsManager::SaveToJson(const std::string& filepath, const ProjectSettings& settings) const {
        nlohmann::json json = {
            {"ProjectName", settings.projectName},
            {"AuthorName", settings.authorName},
            {"ProjectVersion", settings.projectVersion}
        };

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << json.dump(4); // Pretty print
        }
        else {
            REON_ERROR("Could not open file for writing: {0}", filepath);
        }
    }

    void SettingsManager::LoadFromJson(const std::string& filepath, VisualSettings& settings) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            REON_ERROR("Could not open settings file: {0}", filepath);
            return;
        }

        nlohmann::json json;
        file >> json;

        settings.fullscreen = json.value("FullScreen", settings.fullscreen);
        settings.resolutionHeight = json.value("ResolutionHeight", settings.resolutionHeight);
        settings.resolutionWidth = json.value("ResolutionWidth", settings.resolutionWidth);
    }

    void SettingsManager::SaveToJson(const std::string& filepath, const VisualSettings& settings) const {
        nlohmann::json json = {
            {"FullScreen", settings.fullscreen},
            {"ResolutionHeight", settings.resolutionHeight},
            {"ResolutionWidth", settings.resolutionWidth}
        };

        std::ofstream file(filepath);
        if (file.is_open()) {
            file << json.dump(4); // Pretty print
        }
        else {
            REON_ERROR("Could not open file for writing: {0}", filepath);
        }
    }

    void SettingsManager::LoadSettings(const std::string& directory) {
        LoadFromJson(directory + "/Settings/ProjectSettings.json", m_ProjectSettings);
        //LoadFromJson(directory + "/Settings/AudioInputSettings.json", m_AudioSettings);
        LoadFromJson(directory + "/Settings/VisualSettings.json", m_VisualSettings);
    }

    void SettingsManager::SaveSettings(const std::string& directory) const {
        SaveToJson(directory + "/Settings/ProjectSettings.json", m_ProjectSettings);
        //SaveToJson(directory + "/Settings/AudioInputSettings.json", m_AudioSettings);
        SaveToJson(directory + "/Settings/VisualSettings.json", m_VisualSettings);
    }
}