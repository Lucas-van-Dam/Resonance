#pragma once

#include <string>
#include <memory>
#include <fstream>
#include <nlohmann/json.hpp>

#include "Reon.h"
#include "Settings.h"

namespace REON::EDITOR {
    class SettingsManager {
    public:
        static SettingsManager& GetInstance() {
            static SettingsManager instance;
            return instance;
        }

        // Accessors for specific settings
        ProjectSettings& GetProjectSettings() { return m_ProjectSettings; }
        //AudioSettings& GetAudioSettings() { return m_AudioSettings; }
        VisualSettings& GetVisualSettings() { return m_VisualSettings; }

        // Load settings from disk
        void LoadSettings(const std::string& directory);

        // Save settings to disk
        void SaveSettings(const std::string& directory) const;

    private:
        SettingsManager() = default;
        ~SettingsManager() = default;

        SettingsManager(const SettingsManager&) = delete;
        SettingsManager& operator=(const SettingsManager&) = delete;

        ProjectSettings m_ProjectSettings;
        //AudioSettings m_AudioSettings;
        VisualSettings m_VisualSettings;

        // Serialization helpers
        void LoadFromJson(const std::string& filepath, ProjectSettings& settings);
        //void LoadFromJson(const std::string& filepath, AudioSettings& settings);
        void LoadFromJson(const std::string& filepath, VisualSettings& settings);

        void SaveToJson(const std::string& filepath, const ProjectSettings& settings) const;
        //void SaveToJson(const std::string& filepath, const AudioSettings& settings) const;
        void SaveToJson(const std::string& filepath, const VisualSettings& settings) const;
    };
}

