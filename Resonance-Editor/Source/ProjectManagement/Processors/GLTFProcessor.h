#pragma once

#include "REON/AssetManagement/IAssetProcessor.h"

namespace REON::EDITOR {
	class GLTFProcessor : public IAssetProcessor {
	public:
		// Inherited via IAssetProcessor
		void Process(const AssetInfo& assetInfo) override;

	private:
		std::string uid;
		int localIdentifier = 0;

		//void ProcessNode(aiNode* node, const aiScene* scene, std::string path);
		//void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::string path);
	};
}