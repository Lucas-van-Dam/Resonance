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
    ImportResult Import(std::filesystem::path src, ImportContext& ctx) override;

  private:
    AssetId HandleGLTFTexture(const tg::Model& model, const tg::Texture& texture, ImportedModel& impModel,
                              ImportContext& ctx);
    NodeIndex HandleGLTFNode(const tg::Model& model, int nodeId, ImportedModel& impModel, AssetRecord& modelRecord, float scale = 1.0f,
                             uint32_t parentId = UINT32_MAX);
    AssetId HandleGLTFMesh(const tg::Model& model, const tg::Mesh& mesh, ImportedModel& impModel, ImportedNode& impNode);

    void HandleGLTFBuffer(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec3>& data,
                          bool normal = false);

    template <typename T>
    void HandleGLTFBuffer(const tinygltf::Model& model, const tinygltf::Accessor& accessor, std::vector<T>& data);

    void HandleGLTFIndices(const tg::Model& model, const tg::Accessor& accessor, int offset, std::vector<uint>& data);

    float ReadNormalizedComponent(const uint8_t* p, int componentType, bool normalized);

    void HandleGLTFColor(const tg::Model& model, const tg::Accessor& accessor, std::vector<glm::vec4>& data);

    std::tuple<glm::vec3, Quaternion, glm::vec3> GetTRSFromGLTFNode(const tg::Node& node);

    std::vector<AssetId> materialIDs;

    ModelSourceAsset currentModelAsset;
    std::vector<AssetRecord> producedAssets;
};
} // namespace REON::EDITOR