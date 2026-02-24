#pragma once

#include "REON/AssetManagement/Artifact.h"
#include "REON/AssetManagement/Asset.h"
#include "ManifestFormat.h"

namespace REON
{

struct IAssetResolver
{
    virtual bool Resolve(const AssetKey& key, ArtifactRef& out) const = 0;
    virtual ~IAssetResolver() = default;
};

class ManifestAssetResolver final : public IAssetResolver
{
  public:
    bool LoadFromFile(std::filesystem::path p);
    bool Resolve(const AssetKey& key, ArtifactRef& out) const override;

  private:
    std::vector<ManifestEntry> entries_;
    std::vector<uint8_t> strings_;
};
} // namespace REON