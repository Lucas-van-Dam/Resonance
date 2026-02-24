#pragma once

#include "AssetRegistry.h"
#include "Assets/Model/ModelImport.h"

#include <filesystem>
#include <vector>
#include <unordered_map>

namespace REON::EDITOR
{
struct ImportResult
{
    std::vector<AssetRecord> producedAssets; // includes sub-assets
};

struct ImportCache
{
    std::unordered_map<AssetId, ImportedModel> modelCache;
    std::unordered_map<AssetId, ImportedTexture> textureCache;
    //std::unordered_map<AssetId, 
};

struct ImportContext
{
    ImportCache& cache;
    AssetRegistry& registry;
};

struct IImporter
{
    virtual ~IImporter() = default;
    virtual bool CanImport(std::filesystem::path ext) const = 0;
    virtual ImportResult Import(std::filesystem::path src, ImportContext& ctx) = 0;
};

} // namespace REON::EDITOR