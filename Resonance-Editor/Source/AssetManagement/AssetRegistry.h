#pragma once

#include "REON/AssetManagement/Asset.h"

#include <filesystem>

namespace REON::EDITOR
{
struct AssetRecord
{
    AssetId id;
    AssetTypeId type;
    std::filesystem::path sourcePath;
    std::string logicalName;
    std::vector<AssetId> assetDeps;
    std::vector<std::filesystem::path> fileDeps;
};

class AssetRegistry
{
  public:
    static AssetRegistry& Instance();
    const AssetRecord* FindById(AssetId id) const;
    std::vector<AssetRecord> FindBySource(std::filesystem::path p) const;
    void Upsert(const AssetRecord&);
    void Remove(AssetId);

  private:
    std::unordered_map<AssetId, AssetRecord> registry_;
    std::unordered_map<std::filesystem::path, std::vector<AssetId>> pathIndex_;
};

} // namespace REON