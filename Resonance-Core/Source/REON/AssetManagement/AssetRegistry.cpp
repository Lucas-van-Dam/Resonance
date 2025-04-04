#include "reonpch.h"
#include "AssetRegistry.h"

#include "REON/AssetManagement/IAssetProcessor.h"

namespace REON {

    std::unordered_map<std::string, std::unique_ptr<IAssetProcessor>> AssetRegistry::processors;

    AssetRegistry& AssetRegistry::Instance() {
        static AssetRegistry instance;
        return instance;
    }

    void AssetRegistry::RegisterAsset(const AssetInfo& assetInfo) {
        assetMap[assetInfo.id] = assetInfo;
        pathToIdMap[assetInfo.path] = assetInfo.id;
    }

    void AssetRegistry::RemoveAsset(const std::string& id) {
        auto it = assetMap.find(id);
        if (it != assetMap.end()) {
            pathToIdMap.erase(it->second.path);
            assetMap.erase(it);
        }
    }

    AssetInfo* AssetRegistry::GetAssetById(const std::string& id) {
        auto it = assetMap.find(id);
        return it != assetMap.end() ? &it->second : nullptr;
    }

    AssetInfo* AssetRegistry::GetAssetByPath(const fs::path& path) {
        auto it = pathToIdMap.find(path);
        return it != pathToIdMap.end() ? GetAssetById(it->second) : nullptr;
    }

    void AssetRegistry::Clear() {
        assetMap.clear();
        pathToIdMap.clear();
    }

    void AssetRegistry::RegisterProcessor(const std::string& extension, std::unique_ptr<IAssetProcessor> processor) {
        processors[extension] = std::move(processor);
    }

    void AssetRegistry::ProcessAsset(AssetInfo& metaData) {
        auto it = processors.find(metaData.type);
        if (it != processors.end()) {
            it->second->Process(metaData);
        }
    }

}