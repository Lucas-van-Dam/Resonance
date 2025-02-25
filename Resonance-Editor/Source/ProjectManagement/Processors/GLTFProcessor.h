#pragma once
#include "tiny_gltf.h"
#include "glm/glm.hpp"
#include "REON/AssetManagement/IAssetProcessor.h"

namespace tg = tinygltf;

namespace REON::EDITOR {
	class GLTFProcessor : public IAssetProcessor {
	public:
		// Inherited via IAssetProcessor
		void Process(AssetInfo& assetInfo) override;

	private:
		std::string uid;
		int localIdentifier = 0;

		void HandleGLTFNode(const tg::Model& model, const tg::Node& node, const glm::mat4& parentTransform);

		void HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, const glm::mat4& transform);

		glm::mat4 GetTransformFromGLTFNode(const tg::Node& node);

		void HandleGLTFBuffer(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec3>& data, const glm::mat4& transform, bool normal = false);

		template<typename T>
		void HandleGLTFBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<T>& data);

		void HandleGLTFIndices(const tg::Model& model, const tg::Accessor& accessor, int offset, std::vector<uint>& data);

		//void ProcessNode(aiNode* node, const aiScene* scene, std::string path);
		//void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::string path);
	};
}