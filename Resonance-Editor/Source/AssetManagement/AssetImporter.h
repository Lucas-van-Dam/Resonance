#pragma once

#include "AssetRegistry.h"
#include "Assets/Model/ModelImport.h"

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace REON_EDITOR
{
struct ImportResult
{
    std::vector<AssetRecord> producedAssets; // includes sub-assets
};

struct ImportCache
{
    std::unordered_map<REON::AssetId, ImportedModel> modelCache;
    std::unordered_map<REON::AssetId, ImportedTexture> textureCache;
    std::unordered_map<REON::AssetId, ImportedMaterial> materialCache;
    // std::unordered_map<AssetId,
};

struct IImporter
{
    virtual ~IImporter() = default;
    virtual bool CanImport(std::filesystem::path ext) const = 0;
    virtual ImportResult Import(std::filesystem::path src) = 0;
};

} // namespace REON_EDITOR