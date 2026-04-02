#pragma once
#include <REON/AssetManagement/Asset.h>
#include <glm/glm.hpp>
#include <REON/Rendering/Material.h>

namespace REON
{

struct MaterialSourceData
{
    AssetId id;
    std::string debugName;

    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float metallic = 0.0f;
    float roughness = 1.0f;
    float precompF0 = 0.04f;
    float normalScalar = 1.0f;
    glm::vec4 emissiveFactor = glm::vec4(0.0f);
    bool doubleSided = false;
    bool flipNormals = false;

    AssetId baseColorTex = NullAssetId;
    AssetId normalTex = NullAssetId;
    AssetId mrTex = NullAssetId;
    AssetId emissiveTex = NullAssetId;
    AssetId specularTex = NullAssetId;
    AssetId specularColorTex = NullAssetId;

    uint32_t flags = 0;
    BlendingModes blendingMode = BlendingModes::Mask;
    RenderingModes renderingMode = RenderingModes::Opaque;
};
} // namespace REON