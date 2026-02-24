#pragma once

#include <string>
#include <filesystem>
#include <vector>
#include <unordered_map>
#include "REON/AssetManagement/Asset.h"

namespace REON::EDITOR
{
struct ModelSourceAsset
{
    AssetId id; // stable id for the source model asset
    std::string name;
    std::filesystem::path sourcePath; // relative to project

    // Import settings
    float scale = 1.0f;
    bool generateTangents = true;
    bool mergeMeshes = false;
    bool importMaterials = true;
    bool importTextures = true;

    // Outputs (sub-assets created by this import)
    std::vector<AssetId> meshIds;
    std::vector<AssetId> materialIds;
    std::vector<AssetId> textureIds;

    std::filesystem::path cookedArtifact; // path to engine cache file

    // For stable IDs across reimport
    // key: "mesh:0:prim:0" -> uuid
    std::unordered_map<std::string, AssetId> stableKeyToId;

    uint64_t lastBuildKey = 0; // hash(source + settings + tool version)
};

inline ModelSourceAsset LoadModelSourceAssetFromFile(const std::filesystem::path& metaPath)
{
    ModelSourceAsset out{};

    std::ifstream f(metaPath);
    if (!f.is_open())
        throw std::runtime_error("LoadModelSourceAssetFromFile: Failed to open " + metaPath.string());

    nlohmann::json j;
    f >> j;

    // Basic header
    if (j.contains("id"))
        out.id = AssetId::from_string(j.at("id").get<std::string>());

    out.name = j.value("name", std::string{});

    // Source path
    if (j.contains("sourcePath"))
        out.sourcePath = std::filesystem::path(j.at("sourcePath").get<std::string>());
    else if (j.contains("source"))
        out.sourcePath = std::filesystem::path(j.at("source").get<std::string>()); // tolerate older key

    // Import settings (support both flat and nested schemas)
    if (j.contains("importSettings") && j["importSettings"].is_object())
    {
        const auto& s = j["importSettings"];
        out.scale = s.value("scale", out.scale);
        out.generateTangents = s.value("generateTangents", out.generateTangents);
        out.mergeMeshes = s.value("mergeMeshes", out.mergeMeshes);
        out.importMaterials = s.value("importMaterials", out.importMaterials);
        out.importTextures = s.value("importTextures", out.importTextures);
    }
    else
    {
        out.scale = j.value("scale", out.scale);
        out.generateTangents = j.value("generateTangents", out.generateTangents);
        out.mergeMeshes = j.value("mergeMeshes", out.mergeMeshes);
        out.importMaterials = j.value("importMaterials", out.importMaterials);
        out.importTextures = j.value("importTextures", out.importTextures);
    }

    // Outputs
    auto readIdArray = [](const nlohmann::json& arr, std::vector<AssetId>& dst)
    {
        dst.clear();
        if (!arr.is_array())
            return;
        dst.reserve(arr.size());
        for (const auto& v : arr)
        {
            if (v.is_string())
                dst.push_back(AssetId::from_string(v.get<std::string>()));
        }
    };

    if (j.contains("outputs") && j["outputs"].is_object())
    {
        const auto& o = j["outputs"];
        if (o.contains("meshes"))
            readIdArray(o["meshes"], out.meshIds);
        if (o.contains("materials"))
            readIdArray(o["materials"], out.materialIds);
        if (o.contains("textures"))
            readIdArray(o["textures"], out.textureIds);
    }
    else
    {
        if (j.contains("meshIds"))
            readIdArray(j["meshIds"], out.meshIds);
        if (j.contains("materialIds"))
            readIdArray(j["materialIds"], out.materialIds);
        if (j.contains("textureIds"))
            readIdArray(j["textureIds"], out.textureIds);
    }

    // Cooked artifact path
    if (j.contains("build") && j["build"].is_object())
    {
        const auto& b = j["build"];
        if (b.contains("cookedArtifact"))
            out.cookedArtifact = std::filesystem::path(b.at("cookedArtifact").get<std::string>());
        out.lastBuildKey = b.value("lastBuildKey", out.lastBuildKey);
    }
    else
    {
        if (j.contains("cookedArtifact"))
            out.cookedArtifact = std::filesystem::path(j.at("cookedArtifact").get<std::string>());
        out.lastBuildKey = j.value("lastBuildKey", out.lastBuildKey);
    }

    // Stable IDs map
    out.stableKeyToId.clear();
    if (j.contains("stableIds") && j["stableIds"].is_object())
    {
        for (auto it = j["stableIds"].begin(); it != j["stableIds"].end(); ++it)
        {
            if (!it.value().is_string())
                continue;
            out.stableKeyToId.emplace(it.key(), AssetId::from_string(it.value().get<std::string>()));
        }
    }
    else if (j.contains("stableKeyToId") && j["stableKeyToId"].is_object())
    {
        for (auto it = j["stableKeyToId"].begin(); it != j["stableKeyToId"].end(); ++it)
        {
            if (!it.value().is_string())
                continue;
            out.stableKeyToId.emplace(it.key(), AssetId::from_string(it.value().get<std::string>()));
        }
    }

    return out;
}

inline void SaveModelSourceAssetToFile(const std::filesystem::path& metaPath, const ModelSourceAsset& a)
{
    nlohmann::json j;

    // Generic header
    j["metaVersion"] = 1;
    j["assetType"] = "ModelSource";
    j["id"] = a.id.to_string();
    j["name"] = a.name;
    j["sourcePath"] = a.sourcePath.generic_string();

    // Settings (nested, preferred)
    j["importSettings"] = {
        {"scale", a.scale},
        {"generateTangents", a.generateTangents},
        {"mergeMeshes", a.mergeMeshes},
        {"importMaterials", a.importMaterials},
        {"importTextures", a.importTextures},
    };

    // Outputs
    auto writeIdArray = [](const std::vector<AssetId>& src)
    {
        nlohmann::json arr = nlohmann::json::array();
        for (const auto& id : src)
            arr.push_back(id.to_string());
        return arr;
    };

    j["outputs"] = {
        {"meshes", writeIdArray(a.meshIds)},
        {"materials", writeIdArray(a.materialIds)},
        {"textures", writeIdArray(a.textureIds)},
    };

    // Build info
    j["build"] = {
        {"lastBuildKey", a.lastBuildKey},
        {"cookedArtifact", a.cookedArtifact.generic_string()},
    };

    // Stable ids mapping
    nlohmann::json stable = nlohmann::json::object();
    for (const auto& [k, v] : a.stableKeyToId)
        stable[k] = v.to_string();
    j["stableIds"] = std::move(stable);

    std::filesystem::create_directories(metaPath.parent_path());

    // Atomic-ish write: write temp then rename
    const auto tmpPath = metaPath.string() + ".tmp";
    {
        std::ofstream f(tmpPath, std::ios::trunc);
        if (!f.is_open())
            REON_ERROR("SaveModelSourceAssetToFile: Failed to open " + tmpPath);
        f << j.dump(2);
        f.flush();
        if (!f.good())
            REON_ERROR("SaveModelSourceAssetToFile: Failed to write " + tmpPath);
    }

    std::error_code ec;
    std::filesystem::rename(tmpPath, metaPath, ec);
    if (ec)
    {
        // Windows rename over existing can fail; fallback: remove then rename
        std::filesystem::remove(metaPath, ec);
        ec.clear();
        std::filesystem::rename(tmpPath, metaPath, ec);
        if (ec)
            REON_ERROR("SaveModelSourceAssetToFile: Failed to rename tmp to " + metaPath.string());
    }
}

}