#pragma once

#include "REON/AssetManagement/AssetRegistry.h"

namespace REON {
	class IAssetProcessor {
	public:
		virtual void Process(const AssetInfo& assetInfo) = 0;
		virtual ~IAssetProcessor() = default;
	};
}