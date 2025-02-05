#pragma once

#include "REON/AssetManagement/IAssetProcessor.h"

#include "assimp/DefaultLogger.hpp"
#include "assimp/Logger.hpp"
#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"
#include "AssetProcessorLogStream.h"

namespace REON::EDITOR {
	class GLTFProcessor : public IAssetProcessor {
	public:
		// Inherited via IAssetProcessor
		void Process(const AssetInfo& assetInfo) override;

	private:
		std::string uid;
		int localIdentifier = 0;

		void ProcessNode(aiNode* node, const aiScene* scene, std::string path);
		void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::string path);
	};
}