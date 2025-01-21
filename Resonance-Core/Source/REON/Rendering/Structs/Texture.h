#pragma once

#include "REON/ResourceManagement/Resource.h"

#include "stb_image_wrapper.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "glad/glad.h"
#include "GLFW/glfw3.h"

namespace REON {
    class Texture : public Resource {
    public:
        Texture() : m_TextureId(0) {}
        ~Texture() override { Unload(); }

        // Inherited via Resource
        virtual void Load(const std::string& filePath, std::any metadata = {}) override;

        virtual void Unload() override;

        unsigned int GetTextureId();

    private:
        unsigned int m_TextureId;
    };
}