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

    if (std::find_if(pathIndex_[record.sourcePath].begin(), pathIndex_[record.sourcePath].end(),
                     [record](const AssetId& id) { return id == record.id; }) == pathIndex_[record.sourcePath].end())
    {
        pathIndex_[record.sourcePath].push_back(record.id);
    }
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
