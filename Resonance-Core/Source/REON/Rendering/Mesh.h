#pragma once

#include "REON/Rendering/Material.h"
#include "REON/Rendering/Structs/LightData.h"
#include "REON/Rendering/Structs/Vertex.h"
#include <filesystem>
#include "nlohmann/json.hpp"
#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <mikktspace.h>
#include "cppcodec/base64_rfc4648.hpp"

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

        std::string encodeBase64(const uint8_t* data, size_t size) const;
        template<typename T>
        void decodeBase64(const std::string& base64, std::vector<T>& out) const;

        nlohmann::ordered_json ConvertVec2Array(const std::vector<glm::vec2>& vecs) const;
        nlohmann::ordered_json ConvertVec3Array(const std::vector<glm::vec3>& vecs) const;
        nlohmann::ordered_json ConvertVec4Array(const std::vector<glm::vec4>& vecs) const;

        std::vector<glm::vec2> ConvertJsonToVec2Array(const nlohmann::json& jsonArray);
        std::vector<glm::vec3> ConvertJsonToVec3Array(const nlohmann::json& jsonArray);
        std::vector<glm::vec4> ConvertJsonToVec4Array(const nlohmann::json& jsonArray);

        static void calculateTangents(std::shared_ptr<Mesh> mesh);

        int getNumberOfFaces() const { return indices.size() / 3; }
        int getIndexFromFace(int face, int vert) const { return indices[face * 3 + vert]; }

        std::vector<glm::vec3> positions;
        std::vector<glm::vec4> colors;
        std::vector<glm::vec3> normals;
        std::vector<glm::vec2> texCoords;
        std::vector<glm::vec4> tangents;
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

    public:
        struct SMikkTSpaceMeshContext {
            std::shared_ptr<Mesh> mesh;
        };

        static int getNumFaces(const SMikkTSpaceContext* pContext) {
            auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
            return ctx->mesh->getNumberOfFaces();
        }

        static int getNumVerticesOfFace(const SMikkTSpaceContext* pContext, int face) {
            return 3;
        }

        static void getPosition(const SMikkTSpaceContext* pContext, float fvPosOut[], int face, int vert) {
            auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
            int index = ctx->mesh->getIndexFromFace(face, vert);
            fvPosOut[0] = ctx->mesh->positions[index].x;
            fvPosOut[1] = ctx->mesh->positions[index].y;
            fvPosOut[2] = ctx->mesh->positions[index].z;
        }

        static void getNormal(const SMikkTSpaceContext* pContext, float fvNormalOut[], int face, int vert) {
            auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
            int index = ctx->mesh->getIndexFromFace(face, vert);
            fvNormalOut[0] = ctx->mesh->normals[index].x;
            fvNormalOut[1] = ctx->mesh->normals[index].y;
            fvNormalOut[2] = ctx->mesh->normals[index].z;
        }

        static void getTexCoord(const SMikkTSpaceContext* pContext, float fvTexOut[], int face, int vert) {
            auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
            int index = ctx->mesh->getIndexFromFace(face, vert);
            fvTexOut[0] = ctx->mesh->texCoords[index].x;
            fvTexOut[1] = ctx->mesh->texCoords[index].y;
        }

        static void setTSpaceBasic(const SMikkTSpaceContext* pContext, const float fvTangent[], const float fSign, int face, int vert) {
            auto* ctx = static_cast<SMikkTSpaceMeshContext*>(pContext->m_pUserData);
            int index = ctx->mesh->getIndexFromFace(face, vert);
            ctx->mesh->tangents[index].x = fvTangent[0];
            ctx->mesh->tangents[index].y = fvTangent[1];
            ctx->mesh->tangents[index].z = fvTangent[2];
            ctx->mesh->tangents[index].w = fSign;
        }
    };




    template<typename T>
    inline void Mesh::decodeBase64(const std::string& base64, std::vector<T>& out) const
    {
        std::vector<uint8_t> binary = cppcodec::base64_rfc4648::decode(base64);

        if (binary.size() % sizeof(T) != 0) {
            REON_CORE_ERROR("Invalid base64 data: size mismatch for type");
        }
        size_t count = binary.size() / sizeof(T);

        std::cout << "binary.size(): " << binary.size() << std::endl;
        std::cout << "sizeof(T): " << sizeof(T) << std::endl;
        std::cout << "Count: " << count << std::endl;
        std::cout << "Assigning vector of size " << count << std::endl;

        std::vector<T> temp(count);
        std::memcpy(temp.data(), binary.data(), count * sizeof(T));
        out = std::move(temp);
    }

}