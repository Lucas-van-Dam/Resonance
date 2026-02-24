#pragma once

#include "GLFW/glfw3.h"
#include "REON/ResourceManagement/Resource.h"
#include "assimp/scene.h"
#include "stb_image_wrapper.h"
#include "vma/vk_mem_alloc.h"
#include "vulkan/vulkan.h"

#include "assimp/Importer.hpp"

namespace REON
{

    enum SamplerWrapMode
{
    Repeat = 0,
    MirroredRepeat = 1,
    ClampToEdge = 2,
    ClampToBorder = 3
};

enum SamplerFilterMode
{
    Nearest = 0,
    Linear = 1
};

#pragma pack(push, 1)
struct SamplerData
{
    VkFilter magFilter = VK_FILTER_LINEAR;
    VkFilter minFilter = VK_FILTER_LINEAR;
    VkSamplerAddressMode addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    VkSamplerAddressMode addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
};

struct TextureHeader
{
    uint32_t width;
    uint32_t height;
    uint32_t channels;

    // std::vector<uint8_t> pixels;
    VkFormat format = VK_FORMAT_R8G8B8A8_SRGB;
    SamplerData samplerData;
};

struct TextureData
{
    uint32_t width;
    uint32_t height;
    std::vector<uint8_t> pixels;
    SamplerData samplerData;
};
#pragma pack(pop)

class [[clang::annotate("serialize")]] Texture : public ResourceBase
{
  public:
    static constexpr AssetTypeId kType = ASSET_TEXTURE;
    Texture(const TextureData& data);
    ~Texture();

    VkImageView getTextureView() const;
    VkSampler getSampler() const;

    static std::shared_ptr<Texture> getTextureFromId(const std::string& Id, const std::string& basePath);

  private:
    VkImage m_Texture;
    VkImageView m_TextureView;
    VmaAllocation m_TextureAllocation;
    VkSampler m_TextureSampler;
};
} // namespace REON