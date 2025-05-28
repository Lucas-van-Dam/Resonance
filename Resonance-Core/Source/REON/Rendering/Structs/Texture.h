#pragma once

#include "REON/ResourceManagement/Resource.h"
#include "stb_image_wrapper.h"
#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "vulkan/vulkan.h"
#include "vma/vk_mem_alloc.h"
#include "GLFW/glfw3.h"

namespace REON {
#pragma pack(push, 1)
    struct TextureHeader {
        uint width;
        uint height;
        uint channels;
        //std::vector<uint8_t> pixels;
        VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
        VkFilter magFilter = VK_FILTER_LINEAR;
        VkFilter minFilter = VK_FILTER_LINEAR;
        VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    };
#pragma pack(pop)

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

        static std::shared_ptr<Texture> getTextureFromId(const std::string& Id, const std::string& basePath);

    private:
        VkImage m_Texture;
        VkImageView m_TextureView;
        VmaAllocation m_TextureAllocation;
        VkSampler m_TextureSampler;
    };
}