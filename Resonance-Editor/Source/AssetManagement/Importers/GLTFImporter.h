#pragma once

#include "AssetManagement/AssetImporter.h"
#include "tiny_gltf.h"

#include <filesystem>
#include <glm/glm.hpp>
#include <tuple>
#include <AssetManagement/Assets/Model/ModelSourceAsset.h>

namespace tg = tinygltf;

namespace REON::EDITOR
{
class GltfImporter final : public IImporter
{
  public:
    bool CanImport(std::filesystem::path ext) const override;
    ImportResult Import(std::filesystem::path src) override;

  private:
    AssetId HandleGLTFTexture(const tg::Model& model, const tg::Texture& texture, ImportedModel& impModel,
                              bool isSRGB, AssetId texId, AssetId imgId);
    NodeIndex HandleGLTFNode(const tg::Model& model, int nodeId, ImportedModel& impModel, AssetRecord& modelRecord, float scale = 1.0f,
                             uint32_t parentId = UINT32_MAX);
    AssetId HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, ImportedModel& impModel, ImportedNode& impNode, AssetId id);

    std::tuple<glm::vec3, Quaternion, glm::vec3> GetTRSFromGLTFNode(const tg::Node& node);

    void GetAccessorBaseAndStride(const tg::Model& model, const tg::Accessor& accessor, const uint8_t*& base,
                                  size_t& strideBytes);

    float ReadComponentAsFloat(const uint8_t* p, int componentType, bool normalized);
    uint32_t ReadComponentAsU32(const uint8_t* p, int componentType);

    glm::vec2 ReadVec2(const tg::Accessor& accessor, const uint8_t* p);
    glm::vec3 ReadVec3(const tg::Accessor& accessor, const uint8_t* p);
    glm::vec4 ReadVec4(const tg::Accessor& accessor, const uint8_t* p);

    glm::mat4 ReadMat4(const tg::Accessor& accessor, const uint8_t* p);

    glm::u16vec4 ReadJointsU16x4(const tg::Accessor& accessor, const uint8_t* p);
    glm::vec4 ReadWeightsVec4(const tg::Accessor& accessor, const uint8_t* p);

    bool ReadAccessorVec2(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec2>& out);
    bool ReadAccessorVec3(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec3>& out);
    bool ReadAccessorVec4(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec4>& out);

    bool ReadAccessorMat4(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::mat4>& out);

    bool ReadAccessorColor(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec4>& out);
    bool ReadAccessorJoints(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::u16vec4>& out);
    bool ReadAccessorWeights(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec4>& out);
    bool ReadAccessorIndices(const tg::Model& model, const tg::Accessor& accessor, std::vector<uint32_t>& out,
                             uint32_t offset = 0);

    std::vector<AssetId> materialIDs;

    int currentMeshId = 0;

    ModelSourceAsset currentModelAsset;
    std::vector<AssetRecord> producedAssets;
};
} // namespace REON::EDITOR