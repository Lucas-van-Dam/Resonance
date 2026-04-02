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
#include <AssetManagement/Assets/Material/MaterialSourceData.h>
#include <AssetManagement/Assets/Material/MaterialSerializer.h>

namespace REON::EDITOR {



	void AssetDrawers::DrawInspector_Material(std::filesystem::path path, CookPipeline& pipeline)
	{
        static auto& currentPath = path;
        static MaterialSourceData data = MaterialSerializer::Load(path).value();

		if (path != currentPath)
        {
            currentPath = path;
            data = MaterialSerializer::Load(path).value();
        }

		const char* renderingModes[2] = { "Opaque", "Transparent" };
		ImGui::Combo("Rendering Mode", reinterpret_cast<int*>(&data.renderingMode), renderingModes, IM_ARRAYSIZE(renderingModes));

		ImGui::ColorEdit4("Albedo", glm::value_ptr(data.baseColorFactor));
		ImGui::DragFloat("Roughness", &data.roughness, 0.005, 0.0, 1.0);
		ImGui::DragFloat("Metallic", &data.metallic, 0.005, 0.0, 1.0);
        ImGui::Checkbox("Flip Normal Y", &data.flipNormals);
        ImGui::Checkbox("Double Sided", &data.doubleSided);

		
		if (ImGui::Button("Apply"))
        {
            MaterialSerializer::Save(currentPath, data);

            BuildJob job;
            job.reason = BuildReason::ForceRebuild;
            job.sourceId = data.id;
            job.type = ASSET_MATERIAL;
            job.doImport = false;

            BuildQueue queue;
            queue.Enqueue(job);

            pipeline.CookAll(queue);
        }
	}

	void AssetDrawers::DrawInspector_Model(std::filesystem::path path, CookPipeline& pipeline)
	{

        static auto& currentPath = path;
        static auto metaPath = path.string() + ".meta";
        static ModelSourceAsset sourceAsset = LoadModelSourceAssetFromFile(metaPath);


		if (path != currentPath)
        {
            currentPath = path;
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