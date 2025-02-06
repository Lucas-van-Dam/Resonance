#pragma once

#include "Reon.h"
#include <filesystem>

class ProjectOpenedEvent : public REON::Event {
public:
    ProjectOpenedEvent(const std::filesystem::path& projectDirectory)
        : m_ProjectDirectory(projectDirectory) {}

    inline const std::filesystem::path& GetProjectDirectory() const { return m_ProjectDirectory; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "ProjectOpenedEvent: " << m_ProjectDirectory.generic_string();
        return ss.str();
    }

    EVENT_CLASS_NAME(ProjectOpened)

private:
    std::filesystem::path m_ProjectDirectory;
};

class ProjectSavedEvent : public REON::Event {
public:
    ProjectSavedEvent(const std::string& projectName)
        : m_ProjectName(projectName) {}

    inline const std::string& GetProjectName() const { return m_ProjectName; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "ProjectSavedEvent: " << m_ProjectName;
        return ss.str();
    }

    EVENT_CLASS_NAME(ProjectSaved)

private:
    std::string m_ProjectName;
};