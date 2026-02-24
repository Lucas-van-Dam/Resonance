#pragma once

#include "REON/AssetManagement/Asset.h"
#include "REON/AssetManagement/Artifact.h"

namespace REON::EDITOR
{
struct CookOutput
{
    uint64_t buildKey;
    std::vector<AssetId> assetDeps;
    std::vector<std::filesystem::path> fileDeps;
    std::unordered_map<AssetKey, ArtifactRef, AssetKeyHash> artifacts;
};
}