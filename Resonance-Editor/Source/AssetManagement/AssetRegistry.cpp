#include "AssetRegistry.h"

namespace REON::EDITOR
{
AssetRegistry& AssetRegistry::Instance()
{
    static AssetRegistry instance;
    return instance;
}

const AssetRecord* AssetRegistry::FindById(AssetId id) const
{
    auto it = registry_.find(id);
    if (it != registry_.end())
        return &it->second;
    return nullptr;
}

std::vector<AssetRecord> AssetRegistry::FindBySource(std::filesystem::path p) const
{
    std::vector<AssetRecord> results;
    auto it = pathIndex_.find(p);
    if (it != pathIndex_.end())
    {
        for (auto& assetId : it->second)
        {
            auto recordIt = registry_.find(assetId);
            if (recordIt != registry_.end())
                results.push_back(recordIt->second);
        }
    }
    return results;
}

void AssetRegistry::Upsert(const AssetRecord& record)
{
    registry_[record.id] = record;
    pathIndex_[record.sourcePath].push_back(
        record.id); // TODO: this allows duplicates, we should probably check for existing entries and remove them
                    // first, plus this could have assets that dont exist anymore, we should probably clean those up at
                    // some point as well
}

void AssetRegistry::Remove(AssetId id)
{
    auto it = registry_.find(id);
    if (it != registry_.end())
    {
        pathIndex_.erase(it->second.sourcePath);
        registry_.erase(it);
    }
}

} // namespace REON::EDITOR
