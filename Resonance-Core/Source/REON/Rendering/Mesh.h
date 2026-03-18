#pragma once

#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/LightData.h"
#include "REON/Rendering/Structs/Vertex.h"

#include <filesystem>
#include <mikktspace.h>

#include <REON/Platform/Vulkan/VulkanBuffer.h>

#include "cppcodec/base64_rfc4648.hpp"
#include "nlohmann/json.hpp"

namespace REON
{

struct SubMesh
{
    int indexCount;
    int indexOffset;
    int materialIndex;
};

struct DecodedMeshData
{
    std::vector<glm::vec3> positions;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec4> tangents;
    std::vector<uint> indices;
    std::vector<glm::vec4> joints_0;
    std::vector<glm::vec4> joints_1;
    std::vector<glm::vec4> weights_0;
    std::vector<glm::vec4> weights_1;
};

class [[clang::annotate("serialize")]] Mesh : public ResourceBase
{
  public:
    static constexpr AssetTypeId kType = ASSET_MESH;

    Mesh(const DecodedMeshData& data);
    ~Mesh();

    std::vector<glm::vec3> positions;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec4> tangents;
    std::vector<uint> indices;
    std::vector<glm::vec4> joints_0;
    std::vector<glm::vec4> joints_1;
    std::vector<glm::vec4> weights_0;
    std::vector<glm::vec4> weights_1;

    std::vector<SubMesh> subMeshes;

    VulkanBuffer m_VertexBuffer;
    VulkanBuffer m_IndexBuffer;

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

  private:
    // initializes all the buffer objects/arrays
    void setupMesh();

    void setupBuffers();

  private:
    //  render data
    unsigned int m_VBO, m_EBO;
    unsigned int m_DepthMap;
    std::vector<Vertex> m_Vertices;
    // mesh data
    unsigned int m_VAO, m_SSBO;

  public:
};
} // namespace REON