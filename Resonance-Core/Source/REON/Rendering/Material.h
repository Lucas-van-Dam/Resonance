#pragma once

#include "REON/Rendering/Shader.h"
#include "REON/Rendering/Structs/Texture.h"
#include "REON/ResourceManagement/Resource.h"
#include "nlohmann/json.hpp"

namespace REON {
    struct alignas(16) FlatData {
        glm::vec4 albedo;       // float4 → 16 bytes
        float roughness;        // float → 4 bytes
        float metallic;         // float → 4 bytes
        float normalScalar; // float -> 4 bytes
        int normalYScale = 1; // int -> 4 bytes
        glm::vec4 emissiveFactor = glm::vec4(0,0,0,0.5f); // w = alphaCutoff
        glm::vec4 specularFactor = glm::vec4(1, 1, 1, 1);
        float preCompF0 = 0.04f; // float -> 4 bytes
    };

    enum MaterialShaderFlags {
        AlbedoTexture = 1 << 0,
        MetallicRoughnessTexture = 1 << 1,
        NormalTexture = 1 << 2,
        OcclusionTexture = 1 << 3,
        EmissiveTexture = 1 << 4,
        AlphaCutoff = 1 << 5,
        Specular = 1 << 6
    };

    enum RenderingModes {
        Opaque = 0,
        Transparent = 1
    };

    enum BlendingModes {
        Mask, 
        Blend
    };
    
    class [[clang::annotate("serialize")]] Material : public ResourceBase {
    public:
        Material(std::shared_ptr<Shader> shader);
        Material();
        ~Material() override { Unload(); }

        virtual void Load(const std::string& filePath, std::any metadata = {}) override;

        virtual void Load() override;

        virtual void Unload() override;

        std::filesystem::path Serialize(std::filesystem::path path);
        void Deserialize(std::filesystem::path path, std::filesystem::path basePath);

        void setDoubleSided(bool doubleSided) {
            m_DoubleSided = doubleSided;
        }

        bool getDoubleSided() const { return m_DoubleSided; }

    public:
        ResourceHandle albedoTexture;
        ResourceHandle metallicRoughnessTexture;
        ResourceHandle normalTexture;
        ResourceHandle emissiveTexture;
        ResourceHandle specularTexture;
        ResourceHandle specularColorTexture;
        ResourceHandle shader;
        FlatData flatData;
        uint32_t materialFlags;
        RenderingModes renderingMode = Opaque;
        BlendingModes blendingMode = Blend;

        std::vector<VkBuffer> flatDataBuffers;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<void*> flatDataBuffersMapped;

    private:
        bool m_DoubleSided;
        std::vector<VmaAllocation> m_FlatDataBufferAllocations;
        //void createDescriptorSets();

    };
}

