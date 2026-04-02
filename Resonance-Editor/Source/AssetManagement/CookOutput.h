#pragma once

#include "REON/AssetManagement/Artifact.h"
#include "REON/AssetManagement/Asset.h"

namespace REON_EDITOR
{
struct CookOutput
{
    uint64_t buildKey;
    std::vector<REON::AssetId> assetDeps;
    std::vector<std::filesystem::path> fileDeps;
    std::unordered_map<REON::AssetKey, REON::ArtifactRef, REON::AssetKeyHash> artifacts;
};
} // namespace REON_EDITOR