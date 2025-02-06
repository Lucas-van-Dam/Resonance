#include "reonpch.h"
#include "GLTFProcessor.h"
#include <ProjectManagement/ProjectManager.h>
#include <REON/ResourceManagement/ResourceInfo.h>

#define TINYGLTF_IMPLEMENTATION
//#define TINYGLTF_USE_CPP14
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "tiny_gltf.h"

using json = nlohmann::json;

namespace REON::EDITOR {

	void GLTFProcessor::Process(const AssetInfo& assetInfo)
	{
		namespace tg = tinygltf;

		tg::Model model;
		tg::TinyGLTF loader;
		std::string err;
		std::string warn;

		auto extension = assetInfo.path.extension().string();

		if (extension == ".gltf") {
			if (!loader.LoadASCIIFromFile(&model, &err, &warn, assetInfo.path.string())) {
				REON_ERROR("Failed to parse GLTF: {}", err);
			}
		}
		else if (extension == ".glb") {
			if (!loader.LoadBinaryFromFile(&model, &err, &warn, assetInfo.path.string())) {
				REON_ERROR("Failed to parse GLTF: {}", err);
			}
		}
		else
			REON_ERROR("File was not recognised as a gltf file.");

		uid = assetInfo.id;
		localIdentifier = 0;
	}

	//void GLTFProcessor::ProcessNode(aiNode* node, const aiScene* scene, std::string path) {
	//	if (node->mNumMeshes > 0) {
	//		for (int i = 0; i < node->mNumMeshes; i++) {
	//			ProcessMesh(scene->mMeshes[node->mMeshes[i]], scene, path);
	//		}
	//	}

	//	for (int i = 0; i < node->mNumChildren; i++) {
	//		ProcessNode(node->mChildren[i], scene, path);
	//	}
	//}

	//void GLTFProcessor::ProcessMesh(aiMesh* mesh, const aiScene* scene, std::string path) {
	//	std::vector<Vertex> vertices;
	//	std::vector<unsigned int> indices;
	//	std::string meshIdentifier = path.substr(0, path.find_last_of('/')) + "/" + mesh->mName.C_Str();



	//	std::shared_ptr<Mesh> meshObj = ResourceManager::GetInstance().LoadResource<Mesh>(meshIdentifier, std::make_tuple(vertices, indices));

	//	meshObj->Serialize(ProjectManager::GetInstance().GetCurrentProjectPath() + "\\EngineCache\\Meshes");

	//	ResourceInfo info;
	//	info.path = path;
	//	info.UID = uid;
	//	info.localIdentifier = localIdentifier++;

	//	json infoData;
	//	infoData["UID"] = info.UID;
	//	infoData["Path"] = info.path;
	//	infoData["localIdentifier"] = info.localIdentifier;

	//	std::ofstream infoFile((ProjectManager::GetInstance().GetCurrentProjectPath() + "\\EngineCache\\Meshes\\" + meshObj->GetID() + ".mesh.info"));
	//	if (infoFile.is_open()) {
	//		infoFile << infoData.dump(4);
	//		infoFile.close();
	//	}

	//	AssetRegistry::Instance().RegisterAsset({ meshObj->GetID(), "Mesh", "EngineCache/Meshes/" + meshObj->GetID() + ".mesh" });
	//}

}
