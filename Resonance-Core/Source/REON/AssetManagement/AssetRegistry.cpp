#include "reonpch.h"

#include "AssetRegistry.h"

#include "REON/AssetManagement/IAssetProcessor.h"

#include <chrono>

namespace REON
{

std::unordered_map<std::string, std::unique_ptr<IAssetProcessor>> AssetRegistry::processors;

AssetRegistry& AssetRegistry::Instance()
{
    static AssetRegistry instance;
    return instance;
}

void AssetRegistry::RegisterAsset(const AssetInfo& assetInfo)
{
    std::unique_lock<std::mutex> lock(assetRegistrationMutex);
    assetMap[assetInfo.id] = assetInfo;
    pathToIdMap[assetInfo.path] = assetInfo.id;
}

void AssetRegistry::RemoveAsset(const std::string& id)
{
    auto it = assetMap.find(id);
    if (it != assetMap.end())
    {
        pathToIdMap.erase(it->second.path);
        assetMap.erase(it);
    }
}

AssetInfo* AssetRegistry::GetAssetById(const std::string& id)
{
    auto it = assetMap.find(id);
    return it != assetMap.end() ? &it->second : nullptr;
}

AssetInfo* AssetRegistry::GetAssetByPath(const fs::path& path)
{
    auto it = pathToIdMap.find(path);
    return it != pathToIdMap.end() ? GetAssetById(it->second) : nullptr;
}

void AssetRegistry::Clear()
{
    assetMap.clear();
    pathToIdMap.clear();
}

void AssetRegistry::RegisterProcessor(const std::string& extension, std::unique_ptr<IAssetProcessor> processor)
{
    processors[extension] = std::move(processor);
}

void AssetRegistry::ProcessAsset(AssetInfo& metaData)
{
    auto it = processors.find(metaData.type);
    if (it != processors.end())
    {
        it->second->Process(metaData);
    }
}

void AssetRegistry::SaveRegistryToFile(const fs::path& filePath)
{
    nlohmann::json json;
    for (const auto& [id, assetInfo] : assetMap)
    {
        nlohmann::json assetJson;
        assetJson["id"] = assetInfo.id;
        assetJson["type"] = assetInfo.type;
        assetJson["path"] = assetInfo.path.string();
        assetJson["lastModified"] =
            std::chrono::duration_cast<std::chrono::milliseconds>(assetInfo.lastModified.time_since_epoch()).count();
        assetJson["extraInformation"] = assetInfo.extraInformation;
        json.push_back(assetJson);
    }
    std::ofstream file(filePath);
    if (file.is_open())
    {
        file << json.dump(4);
        file.close();
    }
}

void AssetRegistry::LoadRegistryFromFile(const fs::path& filePath)
{
    std::ifstream file(filePath);
    if (file.is_open())
    {
        nlohmann::json json;
        file >> json;
        file.close();
        for (const auto& assetJson : json)
        {
            AssetInfo assetInfo;
            assetInfo.id = assetJson["id"];
            assetInfo.type = assetJson["type"];
            assetInfo.path = assetJson["path"].get<std::string>();
            if (!assetJson.contains("lastModified") || !assetJson["lastModified"].is_number_integer())
            {
                REON_ERROR("Unvalid value in lastModified");
            }
            else
            {
                auto ms = assetJson.at("lastModified").get<std::uint64_t>();

                assetInfo.lastModified = std::chrono::system_clock::time_point{std::chrono::milliseconds{ms}};
            }
            assetInfo.extraInformation = assetJson["extraInformation"];
            RegisterAsset(assetInfo);
        }
    }
}

} // namespace REON