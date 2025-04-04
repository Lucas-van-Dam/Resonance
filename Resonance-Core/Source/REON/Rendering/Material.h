#pragma once

#include "REON/Rendering/Shader.h"
#include "REON/Rendering/Structs/Texture.h"
#include "REON/ResourceManagement/Resource.h"
#include "nlohmann/json.hpp"

namespace REON {

    
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
        std::shared_ptr<Texture> albedoTexture;
        glm::vec4 albedoColor;
        std::shared_ptr<Texture> metallicTexture;
        float metallic;
        std::shared_ptr<Texture> roughnessTexture;
        float roughness;
        std::shared_ptr<Texture> normalTexture;
        std::shared_ptr<Shader> shader;
    };
}

