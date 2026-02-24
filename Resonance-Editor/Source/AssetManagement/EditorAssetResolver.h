#pragma once

#include "AssetRegistry.h"
#include "REON/AssetManagement/Asset.h"
#include "REON/AssetManagement/AssetResolver.h"
#include "REON/AssetManagement/Artifact.h"

namespace REON::EDITOR {
class EditorAssetResolver final : public IAssetResolver
{
  public:
    EditorAssetResolver(AssetRegistry& reg, std::filesystem::path cookedRoot);
    bool Resolve(const AssetKey& key, ArtifactRef& out) const override;

  private:
    AssetRegistry& reg_;
    std::filesystem::path cookedRoot_;
};
}