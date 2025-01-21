#pragma once

#include "Reon.h"

class GameObjectAddedEvent : public REON::Event {
public:
    GameObjectAddedEvent(const std::string& gameObjectName)
        : m_GameObjectName(gameObjectName) {}

    inline const std::string& GetGameObjectName() const { return m_GameObjectName; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "GameObjectAddedEvent: " << m_GameObjectName;
        return ss.str();
    }

    EVENT_CLASS_TYPE(GameObjectAdded)
    EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorObjectManagement)

private:
    std::string m_GameObjectName;
};

class GameObjectDeletedEvent : public REON::Event {
public:
    GameObjectDeletedEvent(const std::string& gameObjectName)
        : m_GameObjectName(gameObjectName) {}

    inline const std::string& GetGameObjectName() const { return m_GameObjectName; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "GameObjectDeletedEvent: " << m_GameObjectName;
        return ss.str();
    }

    EVENT_CLASS_TYPE(GameObjectDeleted)
    EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorObjectManagement)

private:
    std::string m_GameObjectName;
};

class GameObjectRenamedEvent : public REON::Event {
public:
    GameObjectRenamedEvent(const std::string& oldName, const std::string& newName)
        : m_OldName(oldName), m_NewName(newName) {}

    inline const std::string& GetOldName() const { return m_OldName; }
    inline const std::string& GetNewName() const { return m_NewName; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "GameObjectRenamedEvent: " << m_OldName << " -> " << m_NewName;
        return ss.str();
    }

    EVENT_CLASS_TYPE(GameObjectRenamed)
    EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorObjectManagement)

private:
    std::string m_OldName, m_NewName;
};

class GameObjectSelectedEvent : public REON::Event {
public:
    GameObjectSelectedEvent(const std::string& gameObjectName)
        : m_GameObjectName(gameObjectName) {}

    inline const std::string& GetGameObjectName() const { return m_GameObjectName; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "GameObjectSelectedEvent: " << m_GameObjectName;
        return ss.str();
    }

    EVENT_CLASS_TYPE(GameObjectSelected)
    EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorObjectManagement)

private:
    std::string m_GameObjectName;
};

class GameObjectDeselectedEvent : public REON::Event {
public:
    GameObjectDeselectedEvent(const std::string& gameObjectName)
        : m_GameObjectName(gameObjectName) {}

    inline const std::string& GetGameObjectName() const { return m_GameObjectName; }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "GameObjectDeselectedEvent: " << m_GameObjectName;
        return ss.str();
    }

    EVENT_CLASS_TYPE(GameObjectDeselected)
    EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorObjectManagement)

private:
    std::string m_GameObjectName;
};