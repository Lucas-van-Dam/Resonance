#pragma once

#include "Reon.h"

#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <rpc.h>

#include "nlohmann/json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::ordered_json;

namespace REON::EDITOR
{

using MetaInitFn = void (*)(nlohmann::json&);

struct MetaTypeInfo
{
    AssetTypeId assetType;
    MetaInitFn init;
};

class MetadataGenerator
{
  public:
    static void EnsureMetadataExists(const fs::path& assetPath, const fs::path& projectRoot)
    {
        const auto ext = ToLower(assetPath.extension().string());

        auto it = MetaByExtension().find(ext);
        if (it == MetaByExtension().end())
        {
            REON_WARN("No metadata generator registered for asset type: {}, skipping metadata generation", ext);
            return;
        }

        auto metaPath = assetPath.string() + ".meta";
        if (fs::exists(metaPath))
        {
            if (IsValidMetadata(metaPath))
                return;
            else
            {
                REON_CORE_WARN("Invalid meta file found in path: {}, regenerating metadata", metaPath);
                fs::remove(metaPath);
            }
        }

        const MetaTypeInfo& typeInfo = it->second;

        nlohmann::json metaData;
        metaData["metaVersion"] = 1;
        metaData["assetType"] = typeInfo.assetType;
        metaData["id"] = MakeRandomAssetId();
        metaData["sourcePath"] = fs::relative(assetPath, projectRoot).generic_string();

        typeInfo.init(metaData);

        std::ofstream metaFile(metaPath);
        if (metaFile.is_open())
        {
            metaFile << metaData.dump(4);
            metaFile.close();
        }
    }

    static void Register(const std::string& extension, MetaTypeInfo info)
    {
        MetaByExtension()[ToLower(extension)] = std::move(info);
    }

  private:
    static bool IsValidMetadata(const fs::path& metaPath)
    {
        std::ifstream metaFile(metaPath);
        if (!metaFile.is_open())
            return false;

        json metaData;
        try
        {
            metaFile >> metaData;
        }
        catch (...)
        {
            REON_CORE_WARN("Not valid Json structure in asset metafile: {}", metaPath.string());
            return false;
        }

        return metaData.contains("metaVersion") && metaData.contains("assetType") && metaData.contains("id") &&
               metaData.contains("source");
    }

    static std::string ToLower(std::string s)
    {
        for (char& c : s)
            c = (char)std::tolower((unsigned char)c);
        return s;
    }

    static std::unordered_map<std::string, MetaTypeInfo>& MetaByExtension()
    {
        static std::unordered_map<std::string, MetaTypeInfo> m;
        return m;
    }
};
} // namespace REON::EDITOR
