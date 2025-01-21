#pragma once

#include <string>
namespace REON::EDITOR {
    struct ProjectSettings {
        std::string projectName = "Untitled";
        std::string authorName = "Unknown";
        std::string projectVersion = "1.0.0";
    };

    struct AudioSettings {
        int sampleRate = 44100;
        int bitDepth = 16;
        bool useLowLatency = true;
    };

    struct VisualSettings {
        int resolutionWidth = 1920;
        int resolutionHeight = 1080;
        bool fullscreen = false;
    };
}