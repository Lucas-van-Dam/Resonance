#pragma once

#include "Reon.h"

class GizmoManipulatedEvent : public REON::Event {
public:
    GizmoManipulatedEvent(const std::string& action)
        : m_Action(action) {}

    inline const std::string& GetAction() const { return m_Action; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "GizmoManipulatedEvent: " << m_Action;
        return ss.str();
    }

    EVENT_CLASS_NAME(GizmoManipulated)

private:
    std::string m_Action;
};

class ToolActivatedEvent : public REON::Event {
public:
    ToolActivatedEvent(const std::string& toolName)
        : m_ToolName(toolName) {}

    inline const std::string& GetToolName() const { return m_ToolName; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "ToolActivatedEvent: " << m_ToolName;
        return ss.str();
    }

    EVENT_CLASS_NAME(ToolActivated)

private:
    std::string m_ToolName;
};

class ToolDeactivatedEvent : public REON::Event {
public:
    ToolDeactivatedEvent(const std::string& toolName)
        : m_ToolName(toolName) {}

    inline const std::string& GetToolName() const { return m_ToolName; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "ToolDeactivatedEvent: " << m_ToolName;
        return ss.str();
    }

    EVENT_CLASS_NAME(ToolDeactivated)

private:
    std::string m_ToolName;
};