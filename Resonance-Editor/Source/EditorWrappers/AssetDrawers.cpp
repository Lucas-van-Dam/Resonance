#include "AssetDrawers.h"
#include "imgui.h"
#include <fstream>
#include "nlohmann/json.hpp"
#include "ProjectManagement/ProjectManager.h"
#include <iostream>
#include "ProjectManagement/MetadataGenerator.h"
#include <glm/gtc/type_ptr.hpp>
#include <AssetManagement/Assets/Model/ModelSourceAsset.h>
#include <AssetManagement/BuildQueue.h>

namespace REON::EDITOR {



	void AssetDrawers::DrawInspector_Material(std::filesystem::path path)
	{
		//std::shared_ptr<Material> mat;
		//nlohmann::json j;
		//std::ifstream inFile(path.string());
		//if (inFile.is_open()) {
		//	inFile >> j;
		//	inFile.close();
		//}

		//mat = ResourceManager::GetInstance().GetResource<Material>(j["GUID"]);

		//if (!mat) {
		//	mat = std::make_shared<Material>();
		//	//std::filesystem::path relativePath = std::filesystem::relative(path, ProjectManager::GetInstance().GetCurrentProjectPath());
		//	mat->Deserialize(path, ProjectManager::GetInstance().GetCurrentProjectPath());
		//	ResourceManager::GetInstance().AddResource(mat);
		//}

		//const char* renderingModes[2] = { "Opaque", "Transparent" };
		//ImGui::Combo("Rendering Mode", reinterpret_cast<int*>(&mat->renderingMode), renderingModes, IM_ARRAYSIZE(renderingModes));

		//ImGui::ColorEdit4("Albedo", glm::value_ptr(mat->flatData.albedo));
		//ImGui::DragFloat("Roughness", &mat->flatData.roughness, 0.005, 0.0, 1.0);
		//ImGui::DragFloat("Metallic", &mat->flatData.metallic, 0.005, 0.0, 1.0);
		//bool flip = mat->flatData.normalYScale;
		//if (ImGui::Checkbox("Flip Normal Y", &flip)) {
		//	mat->flatData.normalYScale = flip;
		//}

		//bool doubleSided = mat->getDoubleSided();
		//if (ImGui::Checkbox("Double Sided", &doubleSided)) {
		//	mat->setDoubleSided(doubleSided);
		//}
		//
		//if (ImGui::Button("Apply")) {
		//	mat->Serialize(path.parent_path());
		//}
	}

	void AssetDrawers::DrawInspector_Model(std::filesystem::path path, CookPipeline& pipeline)
	{

        static auto& currentPath = path;
        static auto metaPath = path.string() + ".meta";
        static ModelSourceAsset sourceAsset = LoadModelSourceAssetFromFile(metaPath);


		if (path != currentPath)
        {
            metaPath = path.string() + ".meta";
            sourceAsset = LoadModelSourceAssetFromFile(metaPath);
		}
         

		if (std::filesystem::exists(metaPath))
        {
			ImGui::DragFloat("Scale", &sourceAsset.scale, 0.001f);
            ImGui::Checkbox("Force Generate Tangents", &sourceAsset.forceGenerateTangents);
            ImGui::Checkbox("Import Materials", &sourceAsset.importMaterials);
            ImGui::Checkbox("Import Textures", &sourceAsset.importTextures);
            ImGui::Checkbox("Flip Normals", &sourceAsset.flipNormals);
        }

		if (ImGui::Button("Reimport")) {
            SaveModelSourceAssetToFile(metaPath, sourceAsset);

			BuildJob job;
            job.reason = BuildReason::ForceRebuild;
            job.sourceId = sourceAsset.id;
            job.type = ASSET_MODEL;
            job.doImport = true;

			BuildQueue queue;
            queue.Enqueue(job);
			
			pipeline.CookAll(queue);
		}
	}

}