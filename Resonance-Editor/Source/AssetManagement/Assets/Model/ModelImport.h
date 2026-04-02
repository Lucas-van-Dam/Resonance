#pragma once

#include "REON/AssetManagement/Asset.h"
#include "REON/Math/Quaternion.h"
#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/Texture.h"

#include <set>
#include <string>

namespace REON_EDITOR
{

using NodeIndex = uint32_t;

struct ImportedImage
{
    REON::AssetId id;
    std::string debugName;
    uint32_t width, height;
    std::vector<std::uint8_t> rgba8;
    uint8_t channels;
    bool srgb;
};

struct ImportedTexture
{
    REON::AssetId id;
    std::string debugName;
    REON::AssetId imageId;
    REON::SamplerWrapMode samplerStateWrapU;
    REON::SamplerWrapMode samplerStateWrapV;
    REON::SamplerFilterMode samplerStateMinFilter;
    REON::SamplerFilterMode samplerStateMagFilter;
};

struct ImportedMaterial
{
    REON::AssetId id;
    std::string debugName;

    glm::vec4 baseColorFactor;
    float metallic;
    float roughness;
    float precompF0;
    float normalScalar;
    glm::vec4 emissiveFactor;
    bool doubleSided;
    bool flipNormals;

    REON::AssetId baseColorTex;
    REON::AssetId normalTex;
    REON::AssetId mrTex;
    REON::AssetId emissiveTex;
    REON::AssetId specularTex;
    REON::AssetId specularColorTex;

    uint32_t flags;
    REON::BlendingModes blendingMode;
    REON::RenderingModes renderingMode;
};

struct ImportedMesh
{
    struct SubMesh
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        int materialId; // nil if none
    };

    REON::AssetId id;
    std::string debugName;

    std::vector<glm::vec3> positions;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> tangents;
    std::vector<glm::vec2> uv0;
    std::vector<glm::vec4> colors;
    std::vector<uint32_t> indices;
    std::vector<glm::u16vec4> joints_0;
    std::vector<glm::u16vec4> joints_1;
    std::vector<glm::vec4> weights_0;
    std::vector<glm::vec4> weights_1;

    std::vector<SubMesh> subMeshes;

    // if skinned later: joints/weights
};

struct ImportedSkin
{
    std::vector<uint32_t> jointIndices;
    std::vector<glm::mat4> inverseBindMatrices;
    uint32_t skeleton;
};

struct ImportedRig
{
    REON::AssetId rigId;
    std::vector<REON::AssetId> joints;
    std::vector<uint32_t> skinIndices;
};

struct ImportedNode
{
    REON::AssetId NodeId;
    NodeIndex parent = UINT32_MAX;
    std::vector<NodeIndex> children;
    std::string debugName;

    glm::vec3 t;
    REON::Quaternion r; // quat
    glm::vec3 s;

    // render binding
    REON::AssetId meshId; // nil if none
    uint32_t skinIndex;
    std::vector<REON::AssetId> materialId; // max 10, nil if none
};

struct ImportedModel
{
    REON::AssetId modelId;  // import root id (container)
    std::string sourcePath; // for editor/debug only
    std::string debugName;

    std::vector<ImportedImage> images;
    std::vector<ImportedTexture> textures;
    std::vector<ImportedMaterial> materials;
    std::vector<ImportedMesh> meshes;
    std::vector<ImportedNode> nodes;
    std::optional<ImportedRig> rig;
    std::vector<ImportedSkin> skins;
    std::vector<NodeIndex> rootNodes = {0};
};

} // namespace REON_EDITOR