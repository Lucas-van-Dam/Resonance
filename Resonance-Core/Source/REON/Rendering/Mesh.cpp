#include "reonpch.h"

#include "Mesh.h"

#include <REON/Application.h>
#include <REON/Platform/Vulkan/VulkanContext.h>
#include <REON/ResourceManagement/ResourceInfo.h>

namespace REON
{

Mesh::~Mesh()
{
}

void Mesh::setupMesh()
{
    setupBuffers();
}

void Mesh::setupBuffers()
{
    m_Vertices.resize(positions.size());

    for (size_t i = 0; i < positions.size(); ++i)
    {
        Vertex vertex{};
        vertex.Position = positions[i];
        vertex.Color = colors[i];
        vertex.Normal = normals[i];
        vertex.TexCoords = texCoords[i];
        vertex.Tangent = tangents.size() > i ? tangents[i] : glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);

        m_Vertices[i] = vertex;
    }

    VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

    const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());

    BufferCreateInfo bufCreateInfo;
    bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufCreateInfo.memoryHint = BufferMemoryHint::CpuToGpu;
    bufCreateInfo.cpuAccess = CpuAccessPattern::SequentialWrite;
    bufCreateInfo.persistentlyMapped = false;
    bufCreateInfo.size = bufferSize;

    BufferHandle stagingBuffer = context->createBuffer(bufCreateInfo, m_Vertices.data());

    bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufCreateInfo.cpuAccess = CpuAccessPattern::None;
    bufCreateInfo.memoryHint = BufferMemoryHint::GpuOnly;
    bufCreateInfo.persistentlyMapped = false;
    bufCreateInfo.size = bufferSize;

    m_VertexBuffer = context->createBuffer(bufCreateInfo);

    context->copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

    bufferSize = sizeof(indices[0]) * indices.size();

    bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    bufCreateInfo.memoryHint = BufferMemoryHint::CpuToGpu;
    bufCreateInfo.cpuAccess = CpuAccessPattern::SequentialWrite;
    bufCreateInfo.persistentlyMapped = false;
    bufCreateInfo.size = bufferSize;

    stagingBuffer = context->createBuffer(bufCreateInfo, indices.data());

    bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    bufCreateInfo.cpuAccess = CpuAccessPattern::None;
    bufCreateInfo.memoryHint = BufferMemoryHint::GpuOnly;
    bufCreateInfo.persistentlyMapped = false;
    bufCreateInfo.size = bufferSize;

    m_IndexBuffer = context->createBuffer(bufCreateInfo);

    context->copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);
}

Mesh::Mesh(const DecodedMeshData& data)
{
    positions = data.positions;
    colors = data.colors;
    normals = data.normals;
    texCoords = data.texCoords;
    tangents = data.tangents;
    indices = data.indices;
    setupMesh();
}
} // namespace REON