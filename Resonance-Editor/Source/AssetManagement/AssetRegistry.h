#pragma once

#include "REON/AssetManagement/Asset.h"

#include <filesystem>

namespace REON_EDITOR
{
enum AssetOrigin
{
    Native,            // Created within the editor
    ImportedSubAsset,  // Created as part of importing another asset, source file is the original imported asset
    ImportedRootAsset, // Root non native asset
};

struct AssetRecord
{
    REON::AssetId id;
    REON::AssetTypeId type;

    AssetOrigin origin;

    std::filesystem::path sourcePath;

    REON::AssetId parentSourceId;           // only for importedsubasset
    std::filesystem::path importedBlobPath; // only for importedsubasset, used for cooking

    std::string logicalName;

    std::vector<REON::AssetId> assetDeps;
    std::vector<std::filesystem::path> fileDeps;
};

class AssetRegistry
{
  public:
    static AssetRegistry& Instance();
    const AssetRecord* FindById(REON::AssetId id) const;
    std::vector<AssetRecord> FindBySource(std::filesystem::path p) const;
    void Upsert(const AssetRecord&);
    void Remove(REON::AssetId);

  private:
    std::unordered_map<REON::AssetId, AssetRecord> registry_;
    std::unordered_map<std::filesystem::path, std::vector<REON::AssetId>> pathIndex_;
};

} // namespace REON_EDITOR