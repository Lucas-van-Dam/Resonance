#pragma once

#include "Reon.h"

class EditorInitializedEvent : public REON::Event {
public:
    EditorInitializedEvent() {}

    std::string ToString() const override {
        return "EditorInitializedEvent";
    }

    EVENT_CLASS_TYPE(EditorInitialized)
        EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorLifecycle)
};

class EditorShutdownEvent : public REON::Event {
public:
    EditorShutdownEvent() {}

    std::string ToString() const override {
        return "EditorShutdownEvent";
    }

    EVENT_CLASS_TYPE(EditorShutdown)
        EVENT_CLASS_CATEGORY(REON::EventCategoryEditor | REON::EventCategoryEditorLifecycle)
};