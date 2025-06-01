#pragma once
#include "nlohmann/json.hpp"
#include "tiny_gltf.h"
#include "glm/glm.hpp"
#include "REON/AssetManagement/IAssetProcessor.h"
#include "REON/Rendering/Mesh.h"

namespace tg = tinygltf;

namespace REON::EDITOR {
	struct SceneNodeData {
		std::string name;
		glm::mat4 transform;
		std::vector<std::string> meshIDs;
		std::vector<std::string> materials;
		std::vector<SceneNodeData> children;
	};

	struct MetaFileData {
		std::string originPath;
		std::string modelUID; // e.g., assetInfo.id or similar unique identifier.
		std::string sceneName;
		std::vector<SceneNodeData> rootNodes;
		// Optionally, you can include a map of processed mesh data if needed.
		std::vector<std::shared_ptr<Mesh>> meshes;
	};

	class GLTFProcessor : public IAssetProcessor {
	public:
		// Inherited via IAssetProcessor
		void Process(AssetInfo& assetInfo) override;

	private:
		std::string uid;
		std::filesystem::path basePath;
		std::vector<std::string> materialIDs;
		std::filesystem::path fullPath;

		SceneNodeData HandleGLTFNode(const tg::Model& model, const tg::Node& node, const glm::mat4& parentTransform, MetaFileData& modelFileData);

		std::string HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, const glm::mat4& transform, SceneNodeData& sceneNode);

		glm::mat4 GetTransformFromGLTFNode(const tg::Node& node);

		void HandleGLTFBuffer(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec3>& data, const glm::mat4& transform, bool normal = false);

		template<typename T>
		void HandleGLTFBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<T>& data);

		void HandleGLTFIndices(const tg::Model& model, const tg::Accessor& accessor, int offset, std::vector<uint>& data);

		nlohmann::ordered_json SerializeSceneNode(const SceneNodeData& node);

		void SerializeCompanionFile(const MetaFileData& data, const std::string& outPath);

		std::shared_ptr<Texture> HandleGLTFTexture(const tg::Model& model, const tg::Texture& texture, VkFormat format, int textureIndex);

		//void ProcessNode(aiNode* node, const aiScene* scene, std::string path);
		//void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::string path);
	};
}