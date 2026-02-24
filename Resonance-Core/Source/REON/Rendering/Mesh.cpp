#include "reonpch.h"
#include "Mesh.h"
#include <REON/ResourceManagement/ResourceInfo.h>
#include <REON/Platform/Vulkan/VulkanContext.h>
#include <REON/Application.h>

namespace REON {

    Mesh::~Mesh()
    {
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());
        vkDestroyBuffer(context->getDevice(), m_VertexBuffer, nullptr);
        vmaFreeMemory(context->getAllocator(), m_VertexBufferAllocation);
        vkDestroyBuffer(context->getDevice(), m_IndexBuffer, nullptr);
        vmaFreeMemory(context->getAllocator(), m_IndexBufferAllocation);
    }

    void Mesh::setupMesh()
    {
        setupBuffers();
    }

    void Mesh::setupBuffers()
    {
        m_Vertices.resize(positions.size());

        for (size_t i = 0; i < positions.size(); ++i) {
            Vertex vertex{};
            vertex.Position = positions[i];
            vertex.Color = colors[i];
            vertex.Normal = normals[i];
            vertex.TexCoords = texCoords[i];
            vertex.Tangent = tangents.size() > i ? tangents[i] : glm::vec4(0.0f,0.0f,0.0f,0.0f); //TODO: Calculate tangents when loading

            m_Vertices[i] = vertex;
        }

        VkDeviceSize bufferSize = sizeof(m_Vertices[0]) * m_Vertices.size();

        VkBuffer stagingBuffer;
        VmaAllocation stagingBufferAllocation;
        const VulkanContext* context = static_cast<const VulkanContext*>(Application::Get().GetRenderContext());
        context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

        void* data;
        vmaMapMemory(context->getAllocator(), stagingBufferAllocation, &data);
        memcpy(data, m_Vertices.data(), (size_t)bufferSize);
        vmaUnmapMemory(context->getAllocator(), stagingBufferAllocation);

        context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 0, m_VertexBuffer, m_VertexBufferAllocation);

        context->copyBuffer(stagingBuffer, m_VertexBuffer, bufferSize);

        vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
        vmaFreeMemory(context->getAllocator(), stagingBufferAllocation);

        bufferSize = sizeof(indices[0]) * indices.size();

        context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT, stagingBuffer, stagingBufferAllocation);

        vmaMapMemory(context->getAllocator(), stagingBufferAllocation, &data);
        memcpy(data, indices.data(), (size_t)bufferSize);
        vmaUnmapMemory(context->getAllocator(), stagingBufferAllocation);

        context->createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 0, m_IndexBuffer, m_IndexBufferAllocation);

        context->copyBuffer(stagingBuffer, m_IndexBuffer, bufferSize);

        vkDestroyBuffer(context->getDevice(), stagingBuffer, nullptr);
        vmaFreeMemory(context->getAllocator(), stagingBufferAllocation);
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
}