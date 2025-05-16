#pragma once

#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/LightData.h"
#include "REON/Rendering/Structs/Vertex.h"
#include <filesystem>
#include "nlohmann/json.hpp"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>

namespace REON {

    struct SubMesh {
        int indexCount;
        int indexOffset;
        int materialIndex;
    };
    
    class [[clang::annotate("serialize")]] Mesh : public ResourceBase {
    public:
        Mesh(std::string ID) { m_ID = ID; }
        Mesh(const Mesh& mesh);
        Mesh() {}
        ~Mesh() override { Unload(); }

        void Draw(Material& material, std::vector<LightData> lightData) const;

        virtual void Load(const std::string& name, std::any metadata = {}) override;

        virtual void Load() override;

        virtual void Unload() override;

        nlohmann::ordered_json Serialize() const;
        void DeSerialize(const nlohmann::ordered_json& json);

        nlohmann::ordered_json ConvertVec2Array(const std::vector<glm::vec2>& vecs) const;
        nlohmann::ordered_json ConvertVec3Array(const std::vector<glm::vec3>& vecs) const;
        nlohmann::ordered_json ConvertVec4Array(const std::vector<glm::vec4>& vecs) const;

        std::vector<glm::vec2> ConvertJsonToVec2Array(const nlohmann::json& jsonArray);
        std::vector<glm::vec3> ConvertJsonToVec3Array(const nlohmann::json& jsonArray);
        std::vector<glm::vec4> ConvertJsonToVec4Array(const nlohmann::json& jsonArray);

        void calculateTangents();

        std::vector<glm::vec3> positions;
        std::vector<glm::vec4> colors;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec3> tangents;
        std::vector<uint> indices;

        std::vector<SubMesh> subMeshes;

        VkBuffer m_VertexBuffer = VK_NULL_HANDLE;
        VkBuffer m_IndexBuffer = VK_NULL_HANDLE;
    private:
        // initializes all the buffer objects/arrays
        void setupMesh();

        void setupMesh2();

        void setupBuffers();

    private:
        
        VmaAllocation m_VertexBufferAllocation = VK_NULL_HANDLE;
        VmaAllocation m_IndexBufferAllocation = VK_NULL_HANDLE;

        //  render data
        unsigned int m_VBO, m_EBO;
        unsigned int m_DepthMap;
        std::vector<Vertex> m_Vertices;
        // mesh data
        unsigned int m_VAO, m_SSBO;
    };
}