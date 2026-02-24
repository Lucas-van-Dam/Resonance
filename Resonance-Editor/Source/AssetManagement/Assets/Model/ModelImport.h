#pragma once

#include "REON/AssetManagement/Asset.h"
#include "REON/Rendering/Material.h"
#include "REON/Math/Quaternion.h"
#include "REON/Rendering/Structs/Texture.h"

#include <string>

namespace REON::EDITOR
{

using NodeIndex = uint32_t;

struct ImportedImage
{
    AssetId id;
    std::string debugName;
    uint32_t width, height;
    std::vector<std::uint8_t> rgba8;
    uint8_t channels;
    bool srgb;
};

struct ImportedTexture
{
    AssetId id;
    std::string debugName;
    AssetId imageId;
    SamplerWrapMode samplerStateWrapU;
    SamplerWrapMode samplerStateWrapV;
    SamplerFilterMode samplerStateMinFilter;
    SamplerFilterMode samplerStateMagFilter;
};

struct ImportedMaterial
{
    AssetId id;
    std::string debugName;

    glm::vec4 baseColorFactor;
    float metallic;
    float roughness;
    float precompF0;
    float normalScalar;
    glm::vec4 emissiveFactor;
    bool doubleSided;

    AssetId baseColorTex;
    AssetId normalTex;
    AssetId mrTex;
    AssetId emissiveTex;
    AssetId specularTex;
    AssetId specularColorTex;

    uint32_t flags;
    BlendingModes blendingMode;
    RenderingModes renderingMode;
};

struct ImportedMesh
{
    struct SubMesh
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        int materialId; // nil if none
    };

    AssetId id;
    std::string debugName;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;  
    std::vector<glm::vec4> tangents; 
    std::vector<glm::vec2> uv0;      
    std::vector<glm::vec4> colors;   
    std::vector<uint32_t> indices;

    std::vector<SubMesh> subMeshes;

    // if skinned later: joints/weights
};

struct ImportedNode
{
    NodeIndex parent = UINT32_MAX;
    std::vector<NodeIndex> children;
    std::string debugName;

    glm::vec3 t;
    Quaternion r; // quat
    glm::vec3 s;

    // render binding
    AssetId meshId;     // nil if none
    std::vector<AssetId> materialId; // max 10, nil if none
};

struct ImportedModel
{
    AssetId modelId;        // import root id (container)
    std::string sourcePath; // for editor/debug only
    std::string debugName;

    std::vector<ImportedImage> images;
    std::vector<ImportedTexture> textures;
    std::vector<ImportedMaterial> materials;
    std::vector<ImportedMesh> meshes;
    std::vector<ImportedNode> nodes;
    std::vector<NodeIndex> rootNodes = {0};
};

} // namespace REON::EDITOR