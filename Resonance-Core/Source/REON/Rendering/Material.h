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
        int useRoughnessTexture = false; // bool → 4 bytes
        int useMetallicTexture = false;  // bool → 4 bytes
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
        void Deserialize(std::filesystem::path path);

    public:
        ResourceHandle albedoTexture;
        ResourceHandle metallicTexture;
        ResourceHandle roughnessTexture;
        ResourceHandle normalTexture;
        ResourceHandle shader;
        FlatData flatData;

        std::vector<VkBuffer> flatDataBuffers;
        std::vector<VkDescriptorSet> descriptorSets;
        std::vector<void*> flatDataBuffersMapped;

    private:

        std::vector<VmaAllocation> m_FlatDataBufferAllocations;
        //void createDescriptorSets();

    };
}

