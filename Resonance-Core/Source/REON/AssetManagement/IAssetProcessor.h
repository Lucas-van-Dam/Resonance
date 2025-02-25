#pragma once

#include "REON/AssetManagement/AssetRegistry.h"

namespace REON {
	class IAssetProcessor {
	public:
		virtual void Process(AssetInfo& assetInfo) = 0;
		virtual ~IAssetProcessor() = default;
	};
}