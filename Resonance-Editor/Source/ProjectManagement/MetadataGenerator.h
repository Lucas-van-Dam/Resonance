#pragma once

#include "Reon.h"
#include <filesystem>
#include <fstream>
#include <Windows.h>
#include "nlohmann/json.hpp"
#include <rpc.h>
#include <REON/AssetManagement/AssetRegistry.h>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace REON::EDITOR {

	class MetadataGenerator
	{
	public:
		static std::string GenerateUUID() {
			UUID uuid;
			RPC_STATUS status = UuidCreate(&uuid);

			if (status == RPC_S_OK) {
				std::string uuidStringOut;
				RPC_CSTR uuidString;
				UuidToStringA(&uuid, &uuidString);
				uuidStringOut = (char*)uuidString;
				RpcStringFreeA(&uuidString);
				return uuidStringOut;
			}
			else {
				REON_CORE_ERROR("Failed to create UUID. Error code: {}", status);
			}

			return 0;

		}

		static void EnsureMetadataExists(const fs::path& assetPath, const fs::path& projectRoot) {
			auto metaPath = assetPath.string() + ".meta";

			if (fs::exists(metaPath)) {
				if (IsValidMetadata(metaPath))
					return;
				else {
					REON_CORE_WARN("Invalid meta file found in path: {}, regenerating metadata", metaPath);
					fs::remove(metaPath);
				}
			}

			CreateMetadataFile(assetPath, projectRoot);
		}

	private:
		static bool IsValidMetadata(const fs::path& metaPath) {
			std::ifstream metaFile(metaPath);
			if (!metaFile.is_open()) return false;

			json metaData;
			try {
				metaFile >> metaData;
			}
			catch (...) {
				REON_CORE_WARN("Not valid Json structure in asset metafile: {}", metaPath.string());
				return false;
			}

			return metaData.contains("Id") && metaData.contains("Type") && metaData.contains("Path");
		}

		static void CreateMetadataFile(const fs::path& assetPath, const fs::path& projectRoot) {
			auto metaPath = assetPath.string() + ".meta";

			if (fs::exists(metaPath)) {
				return;
			}

			REON::AssetInfo assetInfo;

			assetInfo.id = GenerateUUID();
			assetInfo.path = fs::relative(assetPath, projectRoot).string();
			assetInfo.type = assetPath.extension().string();

			json metaData;
			metaData["Id"] = assetInfo.id;
			metaData["Type"] = assetInfo.type;
			metaData["Path"] = assetInfo.path.string();

			std::ofstream metaFile(metaPath);
			if (metaFile.is_open()) {
				metaFile << metaData.dump(4);
				metaFile.close();
			}

			REON::AssetRegistry::ProcessAsset(assetInfo);
		}
	};
}

