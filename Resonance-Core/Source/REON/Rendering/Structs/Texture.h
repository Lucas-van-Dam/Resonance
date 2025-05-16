#pragma once

#include "REON/ResourceManagement/Resource.h"
#include "stb_image_wrapper.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"
#include "GLFW/glfw3.h"

namespace REON {


    class [[clang::annotate("serialize")]] Texture : public ResourceBase {
    public:
        Texture() {}
        ~Texture() override { Unload(); }

        // Inherited via Resource
        virtual void Load(const std::string& filePath, std::any metadata = {}) override;

        virtual void Load() override {};

        virtual void Unload() override;

        VkImageView getTextureView() const;
        VkSampler getSampler() const;

    private:
        VkImage m_Texture;
        VkImageView m_TextureView;
        VmaAllocation m_TextureAllocation;
        VkSampler m_TextureSampler;
    };
}