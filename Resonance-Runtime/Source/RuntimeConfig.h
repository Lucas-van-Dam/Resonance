#pragma once

#include <filesystem>
#include <REON/Window.h>

namespace REON::RUNTIME {
	struct RuntimeConfig {
		std::string projectName;
		std::filesystem::path startScene;
		WindowProperties windowProperties;
		std::filesystem::path assetDirectory = "Assets/";
		std::filesystem::path shaderDirectory = "Assets/Shaders/";
	};
}