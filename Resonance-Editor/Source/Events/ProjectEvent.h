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

    EVENT_CLASS_TYPE(ProjectOpened)
        EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorProjectManagement)

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

    EVENT_CLASS_TYPE(ProjectSaved)
        EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorProjectManagement)

private:
    std::string m_ProjectName;
};