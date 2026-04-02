#pragma once
#include <AssetManagement/Assets/Model/ModelImport.h>
#include <REON/AssetManagement/Asset.h>
#include <REON/Rendering/Material.h>
#include <glm/glm.hpp>

namespace REON_EDITOR
{
struct MaterialSourceData
{
    REON::AssetId id;
    std::string debugName;

    glm::vec4 baseColorFactor = glm::vec4(1.0f);
    float metallic = 0.0f;
    float roughness = 1.0f;
    float precompF0 = 0.04f;
    float normalScalar = 1.0f;
    glm::vec4 emissiveFactor = glm::vec4(0.0f);
    bool doubleSided = false;
    bool flipNormals = false;

    REON::AssetId baseColorTex = REON::NullAssetId;
    REON::AssetId normalTex = REON::NullAssetId;
    REON::AssetId mrTex = REON::NullAssetId;
    REON::AssetId emissiveTex = REON::NullAssetId;
    REON::AssetId specularTex = REON::NullAssetId;
    REON::AssetId specularColorTex = REON::NullAssetId;

    uint32_t flags = 0;
    REON::BlendingModes blendingMode = REON::BlendingModes::Mask;
    REON::RenderingModes renderingMode = REON::RenderingModes::Opaque;
};

inline MaterialSourceData FromImportedMaterial(const ImportedMaterial& impMat)
{
    MaterialSourceData mat{};
    mat.id = impMat.id;
    mat.debugName = impMat.debugName;
    mat.baseColorFactor = impMat.baseColorFactor;
    mat.metallic = impMat.metallic;
    mat.roughness = impMat.roughness;
    mat.precompF0 = impMat.precompF0;
    mat.normalScalar = impMat.normalScalar;
    mat.emissiveFactor = impMat.emissiveFactor;
    mat.doubleSided = impMat.doubleSided;
    mat.flipNormals = impMat.flipNormals;
    mat.baseColorTex = impMat.baseColorTex;
    mat.normalTex = impMat.normalTex;
    mat.mrTex = impMat.mrTex;
    mat.emissiveTex = impMat.emissiveTex;
    mat.specularTex = impMat.specularTex;
    mat.specularColorTex = impMat.specularColorTex;
    mat.flags = impMat.flags;
    mat.blendingMode = impMat.blendingMode;
    mat.renderingMode = impMat.renderingMode;
    return mat;
}
} // namespace REON_EDITOR