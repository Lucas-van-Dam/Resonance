#pragma once

#include "Reon.h"

class EditorInitializedEvent : public REON::Event {
public:
    EditorInitializedEvent() {}

    std::string ToString() const override {
        return "EditorInitializedEvent";
    }

    EVENT_CLASS_NAME(EditorInitialized)
};

class EditorShutdownEvent : public REON::Event {
public:
    EditorShutdownEvent() {}

    std::string ToString() const override {
        return "EditorShutdownEvent";
    }

    EVENT_CLASS_NAME(EditorShutdown)
};