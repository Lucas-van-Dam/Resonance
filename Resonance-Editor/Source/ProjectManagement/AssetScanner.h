#pragma once

#include "REON/Core.h"
#include <filesystem>
#include <iostream>
#include <vector>
#include <string>
#include <unordered_set>
#include <Reon.h>

namespace fs = std::filesystem;

namespace REON::EDITOR {
	class AssetScanner
	{
	public:
		static const std::unordered_set<std::string> supportedExtensions;

		static std::vector<fs::path> scanAssets(const fs::path& projectDir) {
			std::vector<fs::path> assets;

			if (!fs::exists(projectDir) || !fs::is_directory(projectDir)) {
				//REON_CORE_ERROR("Invalid project directory: {0}", projectDir.generic_string());
				return assets;
			}

			for (const auto& entry : fs::recursive_directory_iterator(projectDir)) {
				if (entry.is_regular_file()) {
					auto extension = entry.path().extension().string();
					if (supportedExtensions.find(extension) != supportedExtensions.end()) {
						assets.push_back(entry.path());
					}
				}
			}

			return assets;
		}

	};
}



