#include "AssetDrawers.h"
#include "imgui.h"
#include <fstream>
#include "nlohmann/json.hpp"
#include "REON/AssetManagement/AssetRegistry.h"
#include "ProjectManagement/ProjectManager.h"
#include <iostream>
#include "ProjectManagement/MetadataGenerator.h"
#include <glm/gtc/type_ptr.hpp>

namespace REON::EDITOR {



	void AssetDrawers::DrawInspector_Material(std::filesystem::path path)
	{
		std::shared_ptr<Material> mat;
		nlohmann::json j;
		std::ifstream inFile(path.string());
		if (inFile.is_open()) {
			inFile >> j;
			inFile.close();
		}

		mat = ResourceManager::GetInstance().GetResource<Material>(j["GUID"]);

		if (!mat) {
			mat = std::make_shared<Material>();
			//std::filesystem::path relativePath = std::filesystem::relative(path, ProjectManager::GetInstance().GetCurrentProjectPath());
			mat->Deserialize(path, ProjectManager::GetInstance().GetCurrentProjectPath());
			ResourceManager::GetInstance().AddResource(mat);
		}

		const char* renderingModes[2] = { "Opaque", "Transparent" };
		ImGui::Combo("Rendering Mode", reinterpret_cast<int*>(&mat->renderingMode), renderingModes, IM_ARRAYSIZE(renderingModes));

		ImGui::ColorEdit4("Albedo", glm::value_ptr(mat->flatData.albedo));
		ImGui::DragFloat("Roughness", &mat->flatData.roughness, 0.005, 0.0, 1.0);
		ImGui::DragFloat("Metallic", &mat->flatData.metallic, 0.005, 0.0, 1.0);
		bool flip = mat->flatData.normalYScale;
		if (ImGui::Checkbox("Flip Normal Y", &flip)) {
			mat->flatData.normalYScale = flip;
		}

		bool doubleSided = mat->getDoubleSided();
		if (ImGui::Checkbox("Double Sided", &doubleSided)) {
			mat->setDoubleSided(doubleSided);
		}
		
		if (ImGui::Button("Apply")) {
			mat->Serialize(path.parent_path());
		}
	}

	void AssetDrawers::DrawInspector_Model(std::filesystem::path path)
	{
		if (ImGui::Button("Reimport")) {
			if (std::filesystem::exists(path.string() + ".meta")) {
				AssetInfo* info;
				nlohmann::ordered_json j;
				std::ifstream inFile(path.string() + ".meta");
				if (inFile.is_open()) {
					inFile >> j;
					inFile.close();
				}
				//modelOriginPath = j["Origin"].get<std::string>();
				info = AssetRegistry::Instance().GetAssetById(j["Id"].get<std::string>());
				info->path = std::filesystem::relative(path, ProjectManager::GetInstance().GetCurrentProjectPath());
				info->type = path.extension().string();

				AssetRegistry::ProcessAsset(*info);

				nlohmann::json metaData;
				metaData["Id"] = info->id;
				metaData["Type"] = info->type;
				metaData["Path"] = info->path.string();
				metaData["Extra Data"] = info->extraInformation;

				std::ofstream metaFile(path.string() + ".meta");
				if (metaFile.is_open()) {
					metaFile << metaData.dump(4);
					metaFile.close();
				}
			}
			else {
				MetadataGenerator::CreateMetadataFile(path, ProjectManager::GetInstance().GetCurrentProjectPath());
			}
		}
	}

}