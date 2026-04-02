#pragma once

#include <vulkan/vulkan.h>
#include <vma/vk_mem_alloc.h>
#include <memory>

namespace REON
{
struct ImageCreateInfo
{
    unsigned width = 0;
    unsigned height = 0;
    unsigned depth = 1;
    unsigned levels = 1;
    VkFormat format = VK_FORMAT_UNDEFINED;
    VkImageType type = VK_IMAGE_TYPE_2D;
    unsigned layers = 1;
    VkImageUsageFlags usage = 0;
    VkSampleCountFlagBits samples = VK_SAMPLE_COUNT_1_BIT;
    VkImageCreateFlags flags = 0;
    VkImageLayout initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    VkComponentMapping swizzle = {
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
        VK_COMPONENT_SWIZZLE_IDENTITY,
    };
    VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
};

class VulkanContext;

class VulkanImage
{
  public:
    VulkanImage() = delete;
    VulkanImage(VulkanImage&&) noexcept;
    VulkanImage& operator=(VulkanImage&&) noexcept;
    VulkanImage(const VulkanImage&) = delete;
    VulkanImage& operator=(const VulkanImage&) = delete;
    ~VulkanImage();

    VkImage getVkImage() const {
        return m_texture;
    }

    VkImageView getVkImageView() const {
        return m_textureView;
    }

    void setImageLayout(VkImageLayout layout) {
        m_currentLayout = layout;
    }

    VkImageLayout getImageLayout() const {
        return m_currentLayout;
    }

  private:
    friend class VulkanContext;
    VulkanImage(const VulkanContext* context, VkImage image, VkImageView view, VmaAllocation allocation, ImageCreateInfo createInfo);

    void reset();

    void moveFrom(VulkanImage&& other);

    const VulkanContext* m_context = nullptr;
    VkImage m_texture = VK_NULL_HANDLE;
    VkImageView m_textureView = VK_NULL_HANDLE;
    VmaAllocation m_textureAllocation = nullptr;
    ImageCreateInfo m_imageCreateInfo{};

    VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
};

using ImageHandle = std::shared_ptr<VulkanImage>;

} // namespace REON