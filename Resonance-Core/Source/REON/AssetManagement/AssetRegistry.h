#pragma once

#include "nlohmann/json.hpp"
#include <filesystem>
#include <mutex>

namespace fs = std::filesystem;
namespace REON
{
class IAssetProcessor;

struct AssetInfo
{
    std::string id;
    std::string type;
    fs::path path;
    nlohmann::json extraInformation;
    std::chrono::system_clock::time_point lastModified;
};

class AssetRegistry
{
  public:
    static AssetRegistry& Instance();

    void RegisterAsset(const AssetInfo& assetInfo);

    void RemoveAsset(const std::string& id);

    AssetInfo* GetAssetById(const std::string& id);

    AssetInfo* GetAssetByPath(const fs::path& path);

    const auto& GetAllAssets() const
    {
        return assetMap;
    }

    void Clear();

    static void RegisterProcessor(const std::string& extension, std::unique_ptr<IAssetProcessor> processor);

    static void ProcessAsset(AssetInfo& metaData);

    void SaveRegistryToFile(const fs::path& filePath);
    void LoadRegistryFromFile(const fs::path& filePath);

  private:
    std::unordered_map<std::string, AssetInfo> assetMap;   // Maps asset IDs to AssetInfo
    std::unordered_map<fs::path, std::string> pathToIdMap; // Maps paths to asset IDs

    std::mutex assetRegistrationMutex;

    static std::unordered_map<std::string, std::unique_ptr<IAssetProcessor>> processors;

    AssetRegistry() = default;
};
} // namespace REON
