#pragma once
#include "REON/AssetManagement/IAssetProcessor.h"
#include "ProjectManagement/ProjectManager.h"
#include "nlohmann/json.hpp"

namespace REON::EDITOR {

	class ModelProcessor : public IAssetProcessor {
		void Process(AssetInfo& assetInfo) override;
	};
}