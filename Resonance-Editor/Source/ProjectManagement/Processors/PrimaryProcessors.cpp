#include "reonpch.h"
#include "PrimaryProcessors.h"

namespace REON::EDITOR {

	void ModelProcessor::Process(AssetInfo& assetInfo) {
		nlohmann::json j;
		auto path = ProjectManager::GetInstance().GetCurrentProjectPath() + "\\" + assetInfo.path.string();
		std::ifstream file(path);
		if (file.is_open()) {
			file >> j;
			file.close();
		}

		if (j.contains("meshes")) {
			for (auto mesh : j["meshes"]) {
				AssetInfo info;
				info.id = mesh["GUID"];
				info.type = "mesh";
				info.path = assetInfo.path;
				AssetRegistry::Instance().RegisterAsset(info);
			}
		}
	}
}