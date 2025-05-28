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
        int useAlbedoTexture = false;    // bool → 4 bytes (no native bools in std140)
        int useNormalTexture = false;    // bool → 4 bytes
        int useMetallicRoughnessTexture = false;  // bool → 4 bytes
        float normalScalar; // float -> 4 bytes
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
        ResourceHandle roughnessMetallicTexture;
        ResourceHandle normalTexture;
        ResourceHandle shader;
        FlatData flatData;

        std::vector<VkBuffer> flatDataBuffers;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<void*> flatDataBuffersMapped;

    private:
        bool m_DoubleSided;
        std::vector<VmaAllocation> m_FlatDataBufferAllocations;
        //void createDescriptorSets();

    };
}

